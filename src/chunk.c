#include "chunk.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <GL/glew.h>

#include "world.h"
#include "minmax.h"
#include "octree.h"
#include "stack.h"
#include "noise.h"
#include "save.h"
#include "debug.h"

struct mesh_s {
	GLuint element_buffer;

	struct stack elements;

	long points;

	int uploadnext;
};

struct update_queue {
	struct update_queue *next;

	update_flags_t flags;

	int3_t pos;
	int time;
};

struct chunk {
	long3_t pos;

	octree_t *data;
	block_t *rawblocks;

	struct update_queue *updates;
	struct update_queue *rawupdates;

	int iscompressed;

	struct mesh_s mesh;
	int iscurrent;

	SDL_mutex *externallock;

	SDL_mutex *mutex_read;
	SDL_sem *sem_write;
	int readers;
};

static int numuncompressed = 0;

const static int faces[] = {
//top
0,1,0,
0,1,1,
1,1,0,

1,1,1,
1,1,0,
0,1,1,

//bottom
0,0,0,
1,0,0,
0,0,1,

1,0,1,
0,0,1,
1,0,0,

//south
0,1,1,
0,0,1,
1,1,1,

1,0,1,
1,1,1,
0,0,1,

//north
1,1,0,
1,0,0,
0,1,0,

0,0,0,
0,1,0,
1,0,0,

//east
1,1,0,
1,1,1,
1,0,0,

1,0,0,
1,1,1,
1,0,1,

//west
0,1,0,
0,0,0,
0,1,1,

0,0,1,
0,1,1,
0,0,0
};

typedef GLuint chunk_mesh_normal_index_t;
#define CHUNK_MESH_NORMAL_INDEX_MAX ((CHUNKSIZE+1)*(CHUNKSIZE+1)*(CHUNKSIZE+1)*BLOCK_NUM_TYPES)

static GLuint index_buffer_vertices = 0;
static GLuint index_buffer_colors = 0;

static void
lock_read(chunk_t *chunk)
{
	SDL_LockMutex(chunk->mutex_read);
	chunk->readers++;
	if(chunk->readers == 1)
		SDL_SemWait(chunk->sem_write);

	SDL_UnlockMutex(chunk->mutex_read);
}

static void
unlock_read(chunk_t *chunk)
{
	SDL_LockMutex(chunk->mutex_read);
	chunk->readers--;
	if(chunk->readers == 0)
		SDL_SemPost(chunk->sem_write);

	if(chunk->readers < 0)
	{
		//TODO: error
		chunk->readers = 0;
		printf("Semaphore: too many unlocks\n");
	}
	SDL_UnlockMutex(chunk->mutex_read);
}

static void
lock_write(chunk_t *chunk)
{
	SDL_SemWait(chunk->sem_write);
}

static void
unlock_write(chunk_t *chunk)
{
	SDL_SemPost(chunk->sem_write);
}

inline static long3_t
get_worldpos_from_internalpos(long3_t *cpos, int x, int y, int z)
{
	long3_t ret;
	ret.x = cpos->x * CHUNKSIZE + x;
	ret.y = cpos->y * CHUNKSIZE + y;
	ret.z = cpos->z * CHUNKSIZE + z;
	return ret;
}

static void
init_chunk(chunk_t *chunk)
{
	glGenBuffers(1, &(chunk->mesh.element_buffer));
	chunk->updates = 0;
	chunk->mesh.uploadnext = 0;
	chunk->mesh.points = 0;
	chunk->iscurrent = 0;
	chunk->iscompressed = 1;
	chunk->externallock = SDL_CreateMutex();
	chunk->mutex_read = SDL_CreateMutex();
	chunk->sem_write = SDL_CreateSemaphore(1);
	chunk->readers = 0;
}

static void
update_queue_compressed(chunk_t *chunk, int x, int y, int z, int time, update_flags_t flags)
{
	struct update_queue *new = malloc(sizeof(struct update_queue));

	new->pos.x = x;
	new->pos.y = y;
	new->pos.z = z;

	new->time = time;
	new->flags = flags;

	if(chunk->updates)
	{
		struct update_queue *top = chunk->updates;
		while(top->next)
		{
			if(new->pos.x == top->pos.x && new->pos.y == top->pos.y && new->pos.z == top->pos.z)
			{
				if(top->time > time)
					top->time = time;
				top->flags |= flags;
				free(new);
				return;
			}
			top = top->next;
		}
		top->next = new;
		new->next = 0;
	} else {
		chunk->updates = new;
		new->next = 0;
	}
}

static void
compress(chunk_t *chunk)
{
	if(chunk->iscompressed)
		return;

	lock_write(chunk);
	int x, y, z;
	for(x=0; x<CHUNKSIZE; ++x)
	for(y=0; y<CHUNKSIZE; ++y)
	for(z=0; z<CHUNKSIZE; ++z)
	{
		octree_set(x, y, z, chunk->data, &chunk->rawblocks[x + y*CHUNKSIZE + z*CHUNKSIZE*CHUNKSIZE]);
		struct update_queue *update = &chunk->rawupdates[x + y*CHUNKSIZE + z*CHUNKSIZE*CHUNKSIZE];
		if(update->time >= 0)
		{
			update_queue_compressed(chunk, x, y, z, update->time, update->flags);
		}
	}

	chunk->iscompressed = 1;

	unlock_write(chunk);

	free(chunk->rawblocks);
	free(chunk->rawupdates);
	numuncompressed--;
}

static void
uncompress(chunk_t *chunk)
{
	if(!chunk->iscompressed)
		return;

	chunk->rawblocks = malloc(CHUNKSIZE*CHUNKSIZE*CHUNKSIZE* sizeof(block_t));
	chunk->rawupdates = malloc(CHUNKSIZE*CHUNKSIZE*CHUNKSIZE* sizeof(struct update_queue));

	lock_write(chunk);

	int x, y, z;
	for(x=0; x<CHUNKSIZE; ++x)
	for(y=0; y<CHUNKSIZE; ++y)
	for(z=0; z<CHUNKSIZE; ++z)
	{
		chunk->rawblocks[x + y*CHUNKSIZE + z*CHUNKSIZE*CHUNKSIZE] = octree_get(x, y, z, chunk->data);
		chunk->rawupdates[x + y*CHUNKSIZE + z*CHUNKSIZE*CHUNKSIZE].time = -1;
	}

	if(chunk->updates)
	{
		struct update_queue *node = chunk->updates;

		while(node)
		{
			struct update_queue *next = node->next;

			struct update_queue *raw = &chunk->rawupdates[node->pos.x + node->pos.y*CHUNKSIZE + node->pos.z*CHUNKSIZE*CHUNKSIZE];

			raw->time = node->time;
			raw->pos = node->pos;

			free(node);

			node = next;
		}

		chunk->updates = 0;
	}

	chunk->iscompressed = 0;

	unlock_write(chunk);

	numuncompressed++;
}

inline static block_t
get_block(chunk_t *c, int x, int y, int z)
{
	block_t ret;
	if(c->iscompressed)
		ret = octree_get(x, y, z, c->data);
	else
		ret = c->rawblocks[x + y*CHUNKSIZE + z*CHUNKSIZE*CHUNKSIZE];
	return ret;
}

inline static void
set_block(chunk_t *c, int x, int y, int z, block_t b)
{
	if(c->iscompressed)
		octree_set(x, y, z, c->data, &b);
	else
		c->rawblocks[x + y*CHUNKSIZE + z*CHUNKSIZE*CHUNKSIZE] = b;
}

void
chunk_static_init()
{
	glGenBuffers(1, &index_buffer_vertices);
	glGenBuffers(1, &index_buffer_colors);

	GLfloat *vertices = malloc(CHUNK_MESH_NORMAL_INDEX_MAX * 3 * sizeof(GLfloat));
	GLfloat *colors = malloc(CHUNK_MESH_NORMAL_INDEX_MAX * 3 * sizeof(GLfloat));

	int i = 0;

	int x, y, z, id;
	for(id = 0; id<BLOCK_NUM_TYPES; ++id)
	for(z = 0; z<CHUNKSIZE+1; ++z)
	for(y = 0; y<CHUNKSIZE+1; ++y)
	for(x = 0; x<CHUNKSIZE+1; ++x)
	{
		vertices[i] = x + ((int)(noise3D(x%(CHUNKSIZE), y%(CHUNKSIZE), z%(CHUNKSIZE), 1) % 100) - 50) * (RENDER_WOBBLE / 100.0f);
		colors[i] = block_properties[id].color.x;
		++i;

		vertices[i] = y + ((int)(noise3D(y%(CHUNKSIZE), z%(CHUNKSIZE), x%(CHUNKSIZE), 1) % 100) - 50) * (RENDER_WOBBLE / 100.0f);
		colors[i] = block_properties[id].color.y;
		++i;

		vertices[i] = z + ((int)(noise3D(z%(CHUNKSIZE), x%(CHUNKSIZE), y%(CHUNKSIZE), 1) % 100) - 50) * (RENDER_WOBBLE / 100.0f);
		colors[i] = block_properties[id].color.z;
		++i;
	}

	glBindBuffer(GL_ARRAY_BUFFER, index_buffer_vertices);
	glBufferData(GL_ARRAY_BUFFER, CHUNK_MESH_NORMAL_INDEX_MAX * 3 * sizeof(GLfloat), vertices, GL_STATIC_DRAW);
	free(vertices);

	glBindBuffer(GL_ARRAY_BUFFER, index_buffer_colors);
	glBufferData(GL_ARRAY_BUFFER, CHUNK_MESH_NORMAL_INDEX_MAX * 3 * sizeof(GLfloat), colors, GL_STATIC_DRAW);
	free(colors);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void
chunk_static_cleanup()
{
	glDeleteBuffers(1, &index_buffer_vertices);
	glDeleteBuffers(1, &index_buffer_colors);
}

long
chunk_render(chunk_t *chunk)
{
	if(chunk->mesh.uploadnext)
	{
		lock_read(chunk);
		if(chunk->mesh.uploadnext)
		{
			glBindBuffer(GL_ARRAY_BUFFER, chunk->mesh.element_buffer);
			glBufferData(GL_ARRAY_BUFFER, chunk->mesh.elements.size, chunk->mesh.elements.data, GL_STATIC_DRAW);
			stack_destroy(&(chunk->mesh.elements));

			chunk->mesh.uploadnext = 0;
		}
		unlock_read(chunk);
	}

	if(chunk->mesh.points > 0)
	{
		glBindBuffer(GL_ARRAY_BUFFER, index_buffer_vertices);
		glVertexAttribPointer(
				0,
				3,
				GL_FLOAT,
				GL_FALSE,
				0,
				0);

		glBindBuffer(GL_ARRAY_BUFFER, index_buffer_colors);
		glVertexAttribPointer(
				1,
				3,
				GL_FLOAT,
				GL_FALSE,
				0,
				0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, chunk->mesh.element_buffer);
		glDrawElements(GL_TRIANGLES, chunk->mesh.points, GL_UNSIGNED_INT, 0);
	}

	return chunk->mesh.points;
}

void
chunk_remesh(chunk_t *chunk, chunk_t *chunkabove, chunk_t *chunkbelow, chunk_t *chunknorth, chunk_t *chunksouth, chunk_t *chunkeast, chunk_t *chunkwest)
{
	chunk_lock(chunk);
	lock_read(chunk);

	if(chunkabove)
		lock_read(chunkabove);
	if(chunkbelow)
		lock_read(chunkbelow);
	if(chunknorth)
		lock_read(chunknorth);
	if(chunksouth)
		lock_read(chunksouth);
	if(chunkeast)
		lock_read(chunkeast);
	if(chunkwest)
		lock_read(chunkwest);

	struct stack elements;
	stack_init(&elements, sizeof(chunk_mesh_normal_index_t), 1000, 2.0); //TODO: better constants

	int x, y, z;
	for(x=0; x<CHUNKSIZE; ++x)
	{
		for(y=0; y<CHUNKSIZE; ++y)
		{
			for(z=0; z<CHUNKSIZE; ++z)
			{
				block_t block = get_block(chunk, x, y, z);
				if(block.id != AIR)
				{
					int top, bottom, south, north, east, west;

					if(y==CHUNKSIZE-1)
					{
						if(chunkabove)
						{
							block_t b = get_block(chunkabove, x,0,z);
							if(b.id != AIR && (block.id != WATER || block.metadata.number == SIM_WATER_LEVELS))
								top=0;
							else
								top=1;
						} else
							top=1;
					} else {
						if(get_block(chunk, x,y+1,z).id && (block.id != WATER || block.metadata.number == SIM_WATER_LEVELS))
							top = 0;
						else
							top = 1;
					}
					if(y==0)
					{
						if(chunkbelow)
						{
							if(get_block(chunkbelow, x,CHUNKSIZE-1,z).id)
								bottom=0;
							else
								bottom=1;
						} else
							bottom=1;
					} else {
						if(get_block(chunk, x,y-1,z).id)
							bottom = 0;
						else
							bottom = 1;
					}
					if(z==CHUNKSIZE-1)
					{
						if(chunksouth)
						{
							block_t b = get_block(chunksouth, x,y,0);
							if(b.id != AIR && (b.id != WATER || (block.id == WATER && b.metadata.number == block.metadata.number)))
								south=0;
							else
								south=1;
						} else
							south=1;
					} else {
						block_t b = get_block(chunk, x,y,z+1);
						if(b.id != AIR && (b.id != WATER || (block.id == WATER && b.metadata.number == block.metadata.number)))
							south = 0;
						else
							south = 1;
					}
					if(z==0)
					{
						if(chunknorth)
						{
							block_t b = get_block(chunknorth, x,y,CHUNKSIZE-1);
							if(b.id != AIR && (b.id != WATER || (block.id == WATER && b.metadata.number == block.metadata.number)))
								north=0;
							else
								north=1;
						} else
							north=1;
					} else {
						block_t b = get_block(chunk, x,y,z-1);
						if(b.id != AIR && (b.id != WATER || (block.id == WATER && b.metadata.number == block.metadata.number)))
							north = 0;
						else
							north = 1;
					}

					if(x==CHUNKSIZE-1)
					{
						if(chunkeast)
						{
							block_t b = get_block(chunkeast, 0,y,z);
							if(b.id != AIR && (b.id != WATER || (block.id == WATER && b.metadata.number == block.metadata.number)))
								east=0;
							else
								east=1;
						} else
							east=1;
					} else {
						block_t b = get_block(chunk, x+1,y,z);
						if(b.id != AIR && (b.id != WATER || (block.id == WATER && b.metadata.number == block.metadata.number)))
							east = 0;
						else
							east = 1;
					}
					if(x==0)
					{
						if(chunkwest)
						{
							block_t b = get_block(chunkwest, CHUNKSIZE-1,y,z);
							if(b.id != AIR && (b.id != WATER || (block.id == WATER && b.metadata.number == block.metadata.number)))
								west=0;
							else
								west=1;
						} else
							west=1;
					} else {
						block_t b = get_block(chunk, x-1,y,z);
						if(b.id != AIR && (b.id != WATER || (block.id == WATER && b.metadata.number == block.metadata.number)))
							west = 0;
						else
							west = 1;
					}

					int U[6] = {
						top,
						bottom,
						south,
						north,
						east,
						west
					};
					int q=0;
					int t;

					for(t=0; t<6; ++t)
					{
						if(U[t])
						{
							int Q=q+18;
							while(q<Q)
							{
								int x_ = faces[q++] + x;
								int y_ = faces[q++] + y;
								int z_ = faces[q++] + z;

								//Add point to vbo
								chunk_mesh_normal_index_t index = x_ + y_*(CHUNKSIZE+1) + z_*(CHUNKSIZE+1)*(CHUNKSIZE+1) + block.id * (CHUNKSIZE+1)*(CHUNKSIZE+1)*(CHUNKSIZE+1);
								stack_push(&elements, &index);
							}
						} else {
							q+=18;
						}
					}
				}
			}
		}
	}

	if(chunkabove)
		unlock_read(chunkabove);
	if(chunkbelow)
		unlock_read(chunkbelow);
	if(chunknorth)
		unlock_read(chunknorth);
	if(chunksouth)
		unlock_read(chunksouth);
	if(chunkeast)
		unlock_read(chunkeast);
	if(chunkwest)
		unlock_read(chunkwest);

	unlock_read(chunk);

	stack_trim(&elements);

	lock_write(chunk);

	if(chunk->mesh.uploadnext)
		stack_destroy(&(chunk->mesh.elements));

	chunk->mesh.elements = elements;
	chunk->mesh.points = stack_objects_get_num(&elements);

	chunk->iscurrent = 1;
	chunk->mesh.uploadnext = 1;

	unlock_write(chunk);
	chunk_unlock(chunk);
}


void
chunk_lock(chunk_t *chunk)
{
	SDL_LockMutex(chunk->externallock);
}


void
chunk_unlock(chunk_t *chunk)
{
	SDL_UnlockMutex(chunk->externallock);
}

int
chunk_mesh_is_current(chunk_t *chunk)
{
	lock_read(chunk);
	int ret = chunk->iscurrent;
	if(ret)
	{
		unlock_read(chunk);
		return 1;
	} else {
		ret = chunk->iscurrent;
		unlock_read(chunk);
		return ret;
	}
}

void
chunk_mesh_clear_current(chunk_t *chunk)
{
	chunk->iscurrent = 0;
}

void
chunk_mesh_clear(chunk_t *chunk)
{
	chunk->mesh.points = 0;
}

block_t
chunk_block_get(chunk_t *c, int x, int y, int z)
{
	if(MIN(MIN(x,y),z) < 0 || MAX(MAX(x,y),z) >= CHUNKSIZE)
	{
		block_t ret;
		ret.id = ERR;
		return ret;
	}

	block_t ret;

	lock_read(c);
	ret = get_block(c, x, y, z);
	unlock_read(c);

	return ret;
}

blockid_t
chunk_block_get_id(chunk_t *c, int x, int y, int z)
{
	return chunk_block_get(c, x, y, z).id;
}

void
chunk_block_set(chunk_t *c, int x, int y, int z, block_t b)
{
	if(MIN(MIN(x,y),z) < 0 || MAX(MAX(x,y),z) >= CHUNKSIZE)
		return;

	lock_write(c);
		set_block(c, x, y, z, b);
	unlock_write(c);
}

void
chunk_block_set_id(chunk_t *c, int x, int y, int z, blockid_t id)
{
	block_t b;
	b.id = id;
	chunk_block_set(c, x, y, z, b);
}

long3_t
chunk_pos_get(chunk_t *chunk)
{
	return chunk->pos;
}

void
chunk_update_queue(chunk_t *chunk, int x, int y, int z, int time, update_flags_t flags)
{
	lock_write(chunk);
	if(chunk->iscompressed)
	{
		update_queue_compressed(chunk, x, y, z, time, flags);
	} else {
		struct update_queue *update = &(chunk->rawupdates[x + y*CHUNKSIZE + z*CHUNKSIZE*CHUNKSIZE]);
		if(update->time >= 0)
		{
			update->flags |= flags;
			update->time = imin(update->time, time);
		} else {
			update->time = time;
			update->flags = flags;
		}
	}
	unlock_write(chunk);
}

long
chunk_update_run(chunk_t *chunk)
{
	long num = 0;

	if(chunk->iscompressed)
	{
		if(chunk->updates)
		{
			struct update_queue *node = chunk->updates;
			struct update_queue *prev = 0;

			while(node)
			{
				struct update_queue *next = node->next;
				if(node->time == 0)
				{
					if(chunk->updates == node)
						chunk->updates = node->next;

					if(prev != 0)
						prev->next = node->next;

					long3_t pos = get_worldpos_from_internalpos(
							&(chunk->pos),
							node->pos.x,
							node->pos.y,
							node->pos.z
						);

					update_run(chunk_block_get(chunk, node->pos.x, node->pos.y, node->pos.z), pos, node->flags);
					num++;

					free(node);
				} else {
					node->time--;
					prev = node;
				}
				node = next;
			}
		}
	} else {
		int x, y, z;
		for(x=0; x<CHUNKSIZE; ++x)
		for(y=0; y<CHUNKSIZE; ++y)
		for(z=0; z<CHUNKSIZE; ++z)
		{
			struct update_queue *node = &chunk->rawupdates[x + y*CHUNKSIZE + z*CHUNKSIZE*CHUNKSIZE];
			if(node->time >= 0)
			{
				node->time--;
				if(node->time == -1)
				{
					long3_t pos = get_worldpos_from_internalpos(
							&(chunk->pos),
							x, y, z
						);

					block_t block = chunk_block_get(chunk, x, y, z);
					update_run(block, pos, node->flags);
					num++;
				}
			}
		}
	}

	if(num > CHUNK_UNCOMPRESS && chunk->iscompressed)
	{
		uncompress(chunk);
	}
	else if(num < CHUNK_RECOMPRESS && !chunk->iscompressed)
	{
		compress(chunk);
	}

	return num;
}

chunk_t *
chunk_load_empty(long3_t pos)
{
	chunk_t *chunk = malloc(sizeof(chunk_t));

	chunk->pos = pos;
	chunk->data = octree_create();

	init_chunk(chunk);

	return chunk;
}

void
chunk_free(chunk_t *chunk)
{
	glDeleteBuffers(1, &(chunk->mesh.element_buffer));
	octree_destroy(chunk->data);
	SDL_DestroyMutex(chunk->externallock);
	SDL_DestroyMutex(chunk->mutex_read);
	SDL_DestroySemaphore(chunk->sem_write);
	free(chunk);
}

int
chunk_recenter(chunk_t *chunk, long3_t *pos)
{
	lock_write(chunk);

	if(!chunk->iscompressed)
	{
		chunk->iscompressed = 1;
		free(chunk->rawblocks);
		free(chunk->rawupdates);
		numuncompressed--;
	}

	if(chunk->mesh.uploadnext)
	{
		stack_destroy(&chunk->mesh.elements);
		chunk->mesh.uploadnext = 0;
	}

	//TODO: clear updates

	chunk->pos = *pos;
	octree_zero(chunk->data);
	chunk->iscurrent = 0;
	unlock_write(chunk);
	return 0;//never loads from disk
}

void
chunk_fill_air(chunk_t *chunk)
{
	lock_write(chunk);
	octree_zero(chunk->data);
	unlock_write(chunk);
}

size_t
update_dump(struct update_queue *queue, unsigned char **data)
{
	//TODO: constants
	struct stack stack;
	stack_init(&stack, 1, 100, 2.0);

	struct update_queue *this = queue;
	while(this)
	{
		save_write_uint16(&stack, this->flags);
		save_write_uint32(&stack, this->pos.x);
		save_write_uint32(&stack, this->pos.y);
		save_write_uint32(&stack, this->pos.z);
		save_write_uint32(&stack, this->time);

		this = this->next;
	}

	stack_trim(&stack);
	*data = stack.data;

	return stack.size;
}

struct update_queue *
update_read(unsigned char *data, size_t size)
{
	struct update_queue *begin = 0;
	struct update_queue *prev = 0;

	size_t count = 0;
	for(count=0; count<size; count+=18)
	{
		struct update_queue *update = malloc(sizeof(struct update_queue));
		data += save_read_uint16(data, &update->flags, sizeof(update->flags));
		data += save_read_uint32(data, &update->pos.x, sizeof(update->pos.x));
		data += save_read_uint32(data, &update->pos.y, sizeof(update->pos.y));
		data += save_read_uint32(data, &update->pos.z, sizeof(update->pos.z));
		data += save_read_uint32(data, &update->time, sizeof(update->time));

		if(begin == 0)
			begin = update;

		if(prev)
			prev->next = update;

		prev = update;
	}

	if(prev)
		prev->next = 0;

	return begin;
}

size_t
chunk_dump(chunk_t *chunk, unsigned char **data)
{
	unsigned char *octree_data;
	size_t octree_size;

	unsigned char *updates_data = 0;
	size_t updates_size = 0;

	if(!chunk->iscompressed)
		compress(chunk);

	lock_read(chunk);

	octree_size = octree_dump(chunk->data, &octree_data);
	updates_size = update_dump(chunk->updates, &updates_data);

	unlock_read(chunk);

	//TODO: constants
	struct stack stack;
	stack_init(&stack, 1, 10000, 2.0);
	stack_push_mult(&stack, "CHUNK.v000", 10);

	save_write_uint64(&stack, octree_size);
	save_write_uint64(&stack, updates_size);
	save_write_uint64(&stack, chunk->pos.x);
	save_write_uint64(&stack, chunk->pos.y);
	save_write_uint64(&stack, chunk->pos.z);
	stack_push_mult(&stack, octree_data, octree_size);
	free(octree_data);
	stack_push_mult(&stack, updates_data, updates_size);
	free(updates_data);

	stack_trim(&stack);

	*data = stack.data;
	return stack.size;
}

chunk_t *
chunk_read(unsigned char *data)
{
	chunk_t *ret = malloc(sizeof(chunk_t));
	init_chunk(ret);
	if(strncmp((char *)data, "CHUNK.v000", 10) != 0)
	{
		error("reading chunk wrong version");
		free(ret);
		return 0;
	}

	size_t octree_size;
	size_t updates_size;

	data += 10;
	data += save_read_uint64(data, &octree_size, sizeof(octree_size));
	data += save_read_uint64(data, &updates_size, sizeof(updates_size));
	data += save_read_uint64(data, &ret->pos.x, sizeof(ret->pos.x));
	data += save_read_uint64(data, &ret->pos.y, sizeof(ret->pos.y));
	data += save_read_uint64(data, &ret->pos.z, sizeof(ret->pos.z));

	ret->data = octree_read(data);
	data += octree_size;
	ret->updates = update_read(data, updates_size);

	return ret;
}
