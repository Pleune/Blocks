#include "chunk.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <SDL_thread.h>
#include <GL/glew.h>
#include <zlib.h>

#include "world.h"
#include "minmax.h"
#include "octree.h"
#include "stack.h"
#include "noise.h"
#include "save.h"
#include "debug.h"

typedef GLuint chunk_mesh_normal_index_t;
#define CHUNK_MESH_NORMAL_INDEX_MAX ((CHUNKSIZE+1)*(CHUNKSIZE+1)*(CHUNKSIZE+1)*BLOCK_NUM_TYPES)

struct mesh_s {
	GLuint element_buffer;

	chunk_mesh_normal_index_t *elements;

	long points;

	int uploadnext;
};

struct chunk {
	long3_t pos;

	octree_t *data;
	block_t *rawblocks;

	update_stack_t *updates;
	struct update_node *rawupdates;

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

static void
init_chunk(chunk_t *chunk)
{
	chunk->mesh.element_buffer = 0;
	glCreateBuffers(1, &chunk->mesh.element_buffer);
	chunk->updates = update_stack_create();
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
compress_chunk(chunk_t *chunk)
{
	if(chunk->iscompressed)
		return;

	int x, y, z;
	for(x=0; x<CHUNKSIZE; ++x)
	for(y=0; y<CHUNKSIZE; ++y)
	for(z=0; z<CHUNKSIZE; ++z)
	{
		octree_set(x, y, z, chunk->data, &chunk->rawblocks[x + y*CHUNKSIZE + z*CHUNKSIZE*CHUNKSIZE]);
		struct update_node *update = &chunk->rawupdates[x + y*CHUNKSIZE + z*CHUNKSIZE*CHUNKSIZE];
		if(update->time >= 0)
			update_queue(chunk->updates, update->pos.x, update->pos.y, update->pos.z, update->time, update->flags);
	}

	chunk->iscompressed = 1;

	free(chunk->rawblocks);
	free(chunk->rawupdates);
	numuncompressed--;
}

static void
uncompress_chunk(chunk_t *chunk)
{
	if(!chunk->iscompressed)
		return;

	chunk->rawblocks = malloc(CHUNKSIZE*CHUNKSIZE*CHUNKSIZE* sizeof(block_t));
	chunk->rawupdates = malloc(CHUNKSIZE*CHUNKSIZE*CHUNKSIZE* sizeof(struct update_node));

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
		struct update_node *node = chunk->updates->queue;

		while(node)
		{
			struct update_node *next = node->next;

			int3_t pos = world_get_internalpos_of_worldpos(node->pos.x, node->pos.y, node->pos.z);
			struct update_node *raw = &chunk->rawupdates[pos.x + pos.y*CHUNKSIZE + pos.z*CHUNKSIZE*CHUNKSIZE];

			*raw = *node;
			free(node);
			node = next;
		}

		chunk->updates->queue = 0;
	}

	chunk->iscompressed = 0;
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
			if(chunk->mesh.points > 0)
			{
				//if(chunk->mesh.element_buffer == 0)
					//glCreateBuffers(1, &chunk->mesh.element_buffer);

				glBindBuffer(GL_ARRAY_BUFFER, chunk->mesh.element_buffer);
				glBufferData(GL_ARRAY_BUFFER, chunk->mesh.points * sizeof(chunk_mesh_normal_index_t), chunk->mesh.elements, GL_STATIC_DRAW);
				free(chunk->mesh.elements);
			} else if(chunk->mesh.element_buffer)
			{
				//glDeleteBuffers(1, &chunk->mesh.element_buffer);
			}

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

	stack_t *elements = stack_create(sizeof(chunk_mesh_normal_index_t), 1000, 2.0); //TODO: better constants

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
								stack_push(elements, &index);
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


	lock_write(chunk);

	if(chunk->mesh.uploadnext)
		free(chunk->mesh.elements);

	chunk->iscurrent = 1;

	long points = stack_objects_get_num(elements);

	chunk->mesh.points = points;

	if(points > 0)
	{
		stack_trim(elements);
		chunk->mesh.elements = stack_transform_dataptr(elements);
	} else {
		chunk->mesh.elements = 0;
		stack_destroy(elements);
	}

	chunk->mesh.uploadnext = 1;

	unlock_write(chunk);
	chunk_unlock(chunk);
}

void
chunk_lock(chunk_t *chunk)
{
	SDL_LockMutex(chunk->externallock);
}

int
chunk_trylock(chunk_t *chunk)
{
	return SDL_TryLockMutex(chunk->externallock) == 0 ? BLOCKS_SUCCESS : BLOCKS_FAIL;
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
	chunk->iscurrent = 0;
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
	b.metadata.number = 0;
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
	long3_t pos = world_get_worldpos_of_internalpos(&chunk->pos, x, y, z);
	chunk_lock(chunk);
	lock_read(chunk);
	if(chunk->iscompressed)
	{
		unlock_read(chunk);
		update_queue(chunk->updates, pos.x, pos.y, pos.z, time, flags);
	} else {
		unlock_read(chunk);
		lock_write(chunk);
		struct update_node *update = &(chunk->rawupdates[x + y*CHUNKSIZE + z*CHUNKSIZE*CHUNKSIZE]);
		if(update->time >= 0)
		{
			update->flags |= flags;
			update->time = imin(update->time, time);
		} else {
			update->time = time;
			update->flags = flags;
			update->pos = pos;
		}
		unlock_write(chunk);
	}

	chunk_unlock(chunk);
}

long
chunk_update_run(chunk_t *chunk)
{
	long num = 0;

	if(chunk_trylock(chunk) == BLOCKS_SUCCESS)
	{
		if(chunk->iscompressed)
		{
			num += update_run(chunk->updates);
		} else {

			int x, y, z;
			for(x=0; x<CHUNKSIZE; ++x)
				for(y=0; y<CHUNKSIZE; ++y)
					for(z=0; z<CHUNKSIZE; ++z)
					{
						struct update_node *node = &chunk->rawupdates[x + y*CHUNKSIZE + z*CHUNKSIZE*CHUNKSIZE];
						if(node->time >= 0)
						{
							node->time--;
							if(node->time == -1)
							{
								update_run_single(node);
								num++;
							}
						}
					}
		}

		if(num > CHUNK_UNCOMPRESS && chunk->iscompressed)
		{
			lock_write(chunk);
			uncompress_chunk(chunk);
			unlock_write(chunk);
		}
		else if(num < CHUNK_RECOMPRESS && !chunk->iscompressed)
		{
			lock_write(chunk);
			compress_chunk(chunk);
			unlock_write(chunk);
		}

		chunk_unlock(chunk);
	} else {
		//TODO: make up for failed updates
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
	if(chunk->mesh.uploadnext)
		free(chunk->mesh.elements);

	if(!chunk->iscompressed)
		compress_chunk(chunk);
	update_stack_destroy(chunk->updates);

	if(chunk->mesh.element_buffer)
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
		free(chunk->mesh.elements);
		chunk->mesh.uploadnext = 0;
	}

	update_stack_clear(chunk->updates);

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
chunk_dump(chunk_t *chunk, unsigned char **data)
{
	unsigned char *octree_data;
	size_t octree_size;

	unsigned char *updates_data = 0;
	size_t updates_size = 0;

	chunk_lock(chunk);

	lock_write(chunk);
	if(!chunk->iscompressed)
		compress_chunk(chunk);
	unlock_write(chunk);

	lock_read(chunk);

	octree_size = octree_dump(chunk->data, &octree_data);
	updates_size = update_dump(chunk->updates, &updates_data);

	unlock_read(chunk);

	//TODO: constants
	stack_t *stack = stack_create(1, 10000, 2.0);
	stack_push_mult(stack, "CHUNK.v000", 10);

	unsigned char tmp[8];

	save_write_uint64(tmp, octree_size);
	stack_push_mult(stack, tmp, 8);
	save_write_uint64(tmp, updates_size);
	stack_push_mult(stack, tmp, 8);
	save_write_int64(tmp, chunk->pos.x);
	stack_push_mult(stack, tmp, 8);
	save_write_int64(tmp, chunk->pos.y);
	stack_push_mult(stack, tmp, 8);
	save_write_int64(tmp, chunk->pos.z);
	stack_push_mult(stack, tmp, 8);
	stack_push_mult(stack, octree_data, octree_size);
	free(octree_data);
	if(updates_size)
		stack_push_mult(stack, updates_data, updates_size);
	free(updates_data);

	stack_trim(stack);

	size_t size_uncompressed = stack_objects_get_num(stack);
	unsigned char *data_uncompressed = stack_transform_dataptr(stack);
	unsigned char *data_compressed = malloc(size_uncompressed + 8);



	int zret;
	z_stream zstrm;
	zstrm.zalloc = Z_NULL;
	zstrm.zfree = Z_NULL;
	zstrm.opaque = Z_NULL;
	zret = deflateInit(&zstrm, OCTREE_ZLIB_COMPRESSION_LEVEL);
	if(zret != Z_OK)
		fail("chunk_dump(): zlib deflateInit() failed");

	zstrm.next_out = data_compressed + 8;
	zstrm.avail_out = size_uncompressed;
	zstrm.next_in = data_uncompressed;
	zstrm.avail_in = size_uncompressed;
	zret = deflate(&zstrm, Z_FINISH);

	deflateEnd(&zstrm);

	free(data_uncompressed);

	size_t size_compressed = size_uncompressed - zstrm.avail_out;
	data_compressed = realloc(data_compressed, size_compressed + 8);
	if(!data_compressed)
		fail("chunk_dump(): realloc failed");

	save_write_uint64(data_compressed, size_uncompressed);
	*data = data_compressed;

	chunk_unlock(chunk);

	return size_compressed + 8;
}

int
chunk_read(chunk_t *chunk, const unsigned char *data)
{
	size_t uncompressed_size = save_read_uint64(data);
	unsigned char *uncompressed_data = malloc(uncompressed_size);

	int zret;
	z_stream zstrm;
	zstrm.zalloc = Z_NULL;
	zstrm.zfree = Z_NULL;
	zstrm.opaque = Z_NULL;
	zstrm.avail_in = 0;
	zstrm.next_in = Z_NULL;
	zret = inflateInit(&zstrm);
	if(zret != Z_OK)
		fail("chunk_read(): inflateInit() failed");

	zstrm.avail_in = 99999999;
	zstrm.next_in = (unsigned char *)data + 8;
	zstrm.avail_out = uncompressed_size;
	zstrm.next_out = uncompressed_data;
	zret = inflate(&zstrm, Z_NO_FLUSH);
	//	if(zret != Z_OK)
		//	fail("octree_read(): inflate() error");

	inflateEnd(&zstrm);

	data = uncompressed_data;

	if(strncmp((char *)data, "CHUNK.v000", 10) != 0)
	{
		error("reading chunk wrong version");
		free(uncompressed_data);
		return BLOCKS_ERROR;
	}

	chunk_lock(chunk);
	lock_write(chunk);

	if(!chunk->iscompressed)
		compress_chunk(chunk);

	octree_destroy(chunk->data);
	update_stack_clear(chunk->updates);

	size_t octree_size;
	size_t updates_size;

	data += 10;
	octree_size = save_read_uint64(data);
	data += 8;
	updates_size = save_read_uint64(data);
	data += 8;
	chunk->pos.x = save_read_int64(data);
	data += 8;
	chunk->pos.y = save_read_int64(data);
	data += 8;
	chunk->pos.z = save_read_int64(data);
	data += 8;

	chunk->data = octree_read(data);
	data += octree_size;
	update_read(chunk->updates, &chunk->pos, data, updates_size);

	chunk_mesh_clear(chunk);

	unlock_write(chunk);
	free(uncompressed_data);

	chunk_unlock(chunk);

	return BLOCKS_SUCCESS;
}
