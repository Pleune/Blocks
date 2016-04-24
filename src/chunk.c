#include "chunk.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <GL/glew.h>

#include "world.h"
#include "minmax.h"
#include "worldgen.h"
#include "octree.h"

enum buffer {vbo, cbo, ebo, BUFFERS_MAX};

struct mesh_s {
	GLuint bufferobjs[BUFFERS_MAX];

	GLfloat *vbodata;
	long vbodatasize;

	GLuint *ebodata;
	long ebodatasize;

	GLfloat *cbodata;
	long cbodatasize;

	long points;

	int uploadnext;
};

struct updatequeue_s {
	struct updatequeue_s *next;
	struct updatequeue_s *prev;

	update_flags_t flags;

	int3_t pos;
	int time;
};

struct chunk_s {
	long3_t pos;
	octree_t *data;
	block_t *rawblocks;

	struct updatequeue_s *updates;
	struct updatequeue_s *rawupdates;

	int iscompressed;

	struct mesh_s mesh;
	int iscurrent;
	
	SDL_mutex *mutex_read;
	SDL_sem *sem_write;
	int readers;
};

int numuncompressed = 0;

const static GLfloat faces[] = {
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

GLfloat uvcoords[] = {
	0,0,
	1,0,
	1,1,
	0,1
};

int uvcoordsabstraction[] = {
	0,
	3,
	1,
	2,
	1,
	3
};

static uint32_t
hash( uint32_t a)
{
	a = (a+0x7ed55d16) + (a<<12);
	a = (a^0xc761c23c) ^ (a>>19);
	a = (a+0x165667b1) + (a<<5);
	a = (a+0xd3a2646c) ^ (a<<9);
	a = (a+0xfd7046c5) + (a<<3);
	a = (a^0xb55a4f09) ^ (a>>16);
	return a;
}

static uint32_t
noise(uint32_t x, uint32_t y, uint32_t z)
{
	return hash((hash(x) ^ hash(y)) ^ hash(z));
}

void
lockRead(chunk_t *chunk)
{
	SDL_LockMutex(chunk->mutex_read);
	chunk->readers++;
	
	if(chunk->readers == 1)
	{
		SDL_SemWait(chunk->sem_write);
	}
	SDL_UnlockMutex(chunk->mutex_read);
}

void
unlockRead(chunk_t *chunk)
{
	SDL_LockMutex(chunk->mutex_read);
	chunk->readers--;
	if(chunk->readers == 0)
	{
		SDL_SemPost(chunk->sem_write);
	}
	if(chunk->readers < 0)
	{
		chunk->readers = 0;
		printf("Semaphore: too many unlocks\n");
	}
	SDL_UnlockMutex(chunk->mutex_read);
}

void
lockWrite(chunk_t *chunk)
{
	SDL_SemWait(chunk->sem_write);
}

void
unlockWrite(chunk_t *chunk)
{
	SDL_SemPost(chunk->sem_write);
}

static void
init(chunk_t *chunk)
{
	glGenBuffers(BUFFERS_MAX, chunk->mesh.bufferobjs);
	chunk->updates = 0;
	chunk->mesh.uploadnext = 0;
	chunk->iscurrent = 0;
	chunk->iscompressed = 1;
	chunk->mutex_read = SDL_CreateMutex();
	chunk->sem_write = SDL_CreateSemaphore(1);
	chunk->readers = 0;
}

long3_t
chunk_getworldposfromchunkpos(long3_t cpos, int x, int y, int z)
{
	long3_t ret;
	ret.x = cpos.x * CHUNKSIZE + x;
	ret.y = cpos.y * CHUNKSIZE + y;
	ret.z = cpos.z * CHUNKSIZE + z;
	return ret;
}

static void
updatequeue(chunk_t *chunk, int x, int y, int z, int time, update_flags_t flags)
{
	struct updatequeue_s *new = malloc(sizeof(struct updatequeue_s));

	new->pos.x = x;
	new->pos.y = y;
	new->pos.z = z;

	new->time = time;
	new->flags = flags;

	if(chunk->updates)
	{
		struct updatequeue_s *top = chunk->updates;
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
		new->prev = top;
		new->next = 0;
	} else {
		chunk->updates = new;
		new->next = 0;
		new->prev = 0;
	}
}

static void
compress(chunk_t *chunk)
{
	if(chunk->iscompressed)
		return;

	lockWrite(chunk);
	int x, y, z;
	for(x=0; x<CHUNKSIZE; x++)
	for(y=0; y<CHUNKSIZE; y++)
	for(z=0; z<CHUNKSIZE; z++)
	{
		octree_set(x, y, z, chunk->data, &chunk->rawblocks[x + y*CHUNKSIZE + z*CHUNKSIZE*CHUNKSIZE]);
		struct updatequeue_s *update = &chunk->rawupdates[x + y*CHUNKSIZE + z*CHUNKSIZE*CHUNKSIZE];
		if(update->time >= 0)
		{
			updatequeue(chunk, x, y, z, update->time, update->flags);
		}
	}

	chunk->iscompressed = 1;

	unlockWrite(chunk);

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
	chunk->rawupdates = malloc(CHUNKSIZE*CHUNKSIZE*CHUNKSIZE* sizeof(struct updatequeue_s));

	lockWrite(chunk);

	int x, y, z;
	for(x=0; x<CHUNKSIZE; x++)
	for(y=0; y<CHUNKSIZE; y++)
	for(z=0; z<CHUNKSIZE; z++)
	{
		chunk->rawblocks[x + y*CHUNKSIZE + z*CHUNKSIZE*CHUNKSIZE] = octree_get(x, y, z, chunk->data);
		chunk->rawupdates[x + y*CHUNKSIZE + z*CHUNKSIZE*CHUNKSIZE].time = -1;
	}

	if(chunk->updates)
	{
		struct updatequeue_s *node = chunk->updates;

		while(node)
		{
			struct updatequeue_s *next = node->next;

			struct updatequeue_s *raw = &chunk->rawupdates[node->pos.x + node->pos.y*CHUNKSIZE + node->pos.z*CHUNKSIZE*CHUNKSIZE];

			raw->time = node->time;
			raw->pos = node->pos;

			free(node);

			node = next;
		}

		chunk->updates = 0;
	}

	chunk->iscompressed = 0;

	unlockWrite(chunk);

	numuncompressed++;
}

static block_t
getblock(chunk_t *c, int x, int y, int z)
{
	block_t ret;
	if(c->iscompressed)
		ret = octree_get(x, y, z, c->data);
	else
		ret = c->rawblocks[x + y*CHUNKSIZE + z*CHUNKSIZE*CHUNKSIZE];
	return ret;
}

static void
setblock(chunk_t *c, int x, int y, int z, block_t b)
{
	if(c->iscompressed)
		octree_set(x, y, z, c->data, &b);
	else
		c->rawblocks[x + y*CHUNKSIZE + z*CHUNKSIZE*CHUNKSIZE] = b;
}

void
chunk_render(chunk_t *chunk)
{
	if(chunk->mesh.uploadnext)
	{
		lockRead(chunk);
		if(chunk->mesh.uploadnext)
		{
			glBindBuffer(GL_ARRAY_BUFFER, chunk->mesh.bufferobjs[vbo]);
			glBufferData(GL_ARRAY_BUFFER, chunk->mesh.vbodatasize * sizeof(GLfloat), chunk->mesh.vbodata, GL_STATIC_DRAW);
			free(chunk->mesh.vbodata);

			glBindBuffer(GL_ARRAY_BUFFER, chunk->mesh.bufferobjs[ebo]);
			glBufferData(GL_ARRAY_BUFFER, chunk->mesh.ebodatasize * sizeof(GLfloat), chunk->mesh.ebodata, GL_STATIC_DRAW);
			free(chunk->mesh.ebodata);

			glBindBuffer(GL_ARRAY_BUFFER, chunk->mesh.bufferobjs[cbo]);
			glBufferData(GL_ARRAY_BUFFER, chunk->mesh.cbodatasize * sizeof(GLfloat), chunk->mesh.cbodata, GL_STATIC_DRAW);
			free(chunk->mesh.cbodata);

			chunk->mesh.uploadnext = 0;
		}
		unlockRead(chunk);
	}

	glBindBuffer(GL_ARRAY_BUFFER, chunk->mesh.bufferobjs[vbo]);
	glVertexAttribPointer(
			0,
			3,
			GL_FLOAT,
			GL_FALSE,
			0,
			0);

	glBindBuffer(GL_ARRAY_BUFFER, chunk->mesh.bufferobjs[cbo]);
	glVertexAttribPointer(
			1,
			3,
			GL_FLOAT,
			GL_FALSE,
			0,
			0);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, chunk->mesh.bufferobjs[ebo]);
	glDrawElements(GL_TRIANGLES, chunk->mesh.points, GL_UNSIGNED_INT, 0);
}

static inline void
addpoint(chunk_t *chunk, int *c, uint16_t *i, GLuint **ebos, int *v, uint16_t *o, GLfloat **vbos, GLfloat **color, GLuint *ebc, GLfloat x, GLfloat y, GLfloat z, vec3_t blockcolor, uint8_t blockid)
{
	GLint index = x + y*(CHUNKSIZE+1) + z*(CHUNKSIZE+1)*(CHUNKSIZE+1) + blockid * (CHUNKSIZE+1)*(CHUNKSIZE+1)*(CHUNKSIZE+1);
	if(!ebc[index] || blockid == WATER)
	{
		//add point to vbo
		//the max for o is a multiple for three, so we only have to check for the 'overflow' every three floats or one point
		vec3_t pos;
		pos.x = x + chunk->pos.x*CHUNKSIZE;
		pos.y = y + chunk->pos.y*CHUNKSIZE;
		pos.z = z + chunk->pos.z*CHUNKSIZE;

		vec3_t n;
		n.x = ((float)(noise(pos.x, pos.y, pos.z)%1000)-.5f) * ((float)RENDER_WOBBLE / 1000.0f);
		n.y = ((float)(noise(pos.y, pos.z, pos.x)%1000)-.5f) * ((float)RENDER_WOBBLE / 1000.0f);
		n.z = ((float)(noise(pos.z, pos.x, pos.y)%1000)-.5f) * ((float)RENDER_WOBBLE / 1000.0f);

		pos.x += n.x;
		pos.y += n.y;
		pos.z += n.z;

		vbos[v[0]][o[0]] = pos.x;
		color[v[0]][o[0]++] = blockcolor.x;
		vbos[v[0]][o[0]] = pos.y;
		color[v[0]][o[0]++] = blockcolor.y;
		vbos[v[0]][o[0]] = pos.z;
		color[v[0]][o[0]++] = blockcolor.z;

		if(o[0] == 9999)
		{
			o[0]=0;
			v[0]++;
			vbos[v[0]] = (GLfloat *)malloc(sizeof(GLfloat) * 9999);
			color[v[0]] = (GLfloat *)malloc(sizeof(GLfloat) * 9999);
		}

		ebc[index] = v[0]*3333 + o[0]/3;
	}
	ebos[c[0]][i[0]++] = ebc[index] - 1;
	if(i[0] == 9999)
	{
		i[0]=0;
		c[0]++;
		ebos[c[0]] = (GLuint *)malloc(sizeof(GLuint) * 9999);
	}
}

void
chunk_remesh(chunk_t *chunk, chunk_t *chunkabove, chunk_t *chunkbelow, chunk_t *chunknorth, chunk_t *chunksouth, chunk_t *chunkeast, chunk_t *chunkwest)
{
	lockRead(chunk);

	if(chunkabove)
		lockRead(chunkabove);
	if(chunkbelow)
		lockRead(chunkbelow);
	if(chunknorth)
		lockRead(chunknorth);
	if(chunksouth)
		lockRead(chunksouth);
	if(chunkeast)
		lockRead(chunkeast);
	if(chunkwest)
		lockRead(chunkwest);

	GLuint *ebos[256];
	int c = 0;
	uint16_t i = 0;

	ebos[0] = (GLuint *)malloc(sizeof(GLuint) * 9999);

	GLfloat *vbos[256];
	GLfloat *cbos[256];

	int v = 0;
	uint16_t o = 0;

	vbos[0] = (GLfloat *)malloc(sizeof(GLfloat) * 9999);
	cbos[0] = (GLfloat *)malloc(sizeof(GLfloat) * 9999);

	GLuint *ebc = (GLuint *)calloc(sizeof(GLuint), (CHUNKSIZE+1) * (CHUNKSIZE+1) * (CHUNKSIZE+1) * BLOCK_MAXTYPES);

	int x, y, z;
	for(x=0; x<CHUNKSIZE; x++)
	{
		for(y=0; y<CHUNKSIZE; y++)
		{
			for(z=0; z<CHUNKSIZE; z++)
			{
				block_t block = getblock(chunk, x, y, z);
				if(block.id != AIR)
				{
					int top, bottom, south, north, east, west;

					if(y==CHUNKSIZE-1)
					{
						if(chunkabove)
						{
							block_t b = getblock(chunkabove, x,0,z);
							if(b.id != AIR && (block.id != WATER || block.metadata.number == SIM_WATER_LEVELS))
								top=0;
							else
								top=1;
						} else
							top=1;
					} else {
						if(getblock(chunk, x,y+1,z).id && (block.id != WATER || block.metadata.number == SIM_WATER_LEVELS))
							top = 0;
						else
							top = 1;
					}
					if(y==0)
					{
						if(chunkbelow)
						{
							if(getblock(chunkbelow, x,CHUNKSIZE-1,z).id)
								bottom=0;
							else
								bottom=1;
						} else
							bottom=1;
					} else {
						if(getblock(chunk, x,y-1,z).id)
							bottom = 0;
						else
							bottom = 1;
					}
					if(z==CHUNKSIZE-1)
					{
						if(chunksouth)
						{
							block_t b = getblock(chunksouth, x,y,0);
							if(b.id != AIR && (b.id != WATER || (block.id == WATER && b.metadata.number == block.metadata.number)))
								south=0;
							else
								south=1;
						} else
							south=1;
					} else {
						block_t b = getblock(chunk, x,y,z+1);
						if(b.id != AIR && (b.id != WATER || (block.id == WATER && b.metadata.number == block.metadata.number)))
							south = 0;
						else
							south = 1;
					}
					if(z==0)
					{
						if(chunknorth)
						{
							block_t b = getblock(chunknorth, x,y,CHUNKSIZE-1);
							if(b.id != AIR && (b.id != WATER || (block.id == WATER && b.metadata.number == block.metadata.number)))
								north=0;
							else
								north=1;
						} else
							north=1;
					} else {
						block_t b = getblock(chunk, x,y,z-1);
						if(b.id != AIR && (b.id != WATER || (block.id == WATER && b.metadata.number == block.metadata.number)))
							north = 0;
						else
							north = 1;
					}

					if(x==CHUNKSIZE-1)
					{
						if(chunkeast)
						{
							block_t b = getblock(chunkeast, 0,y,z);
							if(b.id != AIR && (b.id != WATER || (block.id == WATER && b.metadata.number == block.metadata.number)))
								east=0;
							else
								east=1;
						} else
							east=1;
					} else {
						block_t b = getblock(chunk, x+1,y,z);
						if(b.id != AIR && (b.id != WATER || (block.id == WATER && b.metadata.number == block.metadata.number)))
							east = 0;
						else
							east = 1;
					}
					if(x==0)
					{
						if(chunkwest)
						{
							block_t b = getblock(chunkwest, CHUNKSIZE-1,y,z);
							if(b.id != AIR && (b.id != WATER || (block.id == WATER && b.metadata.number == block.metadata.number)))
								west=0;
							else
								west=1;
						} else
							west=1;
					} else {
						block_t b = getblock(chunk, x-1,y,z);
						if(b.id != AIR && (b.id != WATER || (block.id == WATER && b.metadata.number == block.metadata.number)))
							west = 0;
						else
							west = 1;
					}

					vec3_t blockcolor = block_getcolor(block.id);

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

					for(t=0;t<6;t++)
					{
						if(U[t])
						{
							int Q=q+18;
							while(q<Q)
							{
								GLfloat hmux = 1.0f;
								if(block.id == WATER)
									hmux = (GLfloat)block.metadata.number / (GLfloat)SIM_WATER_LEVELS;

								GLfloat x_ = faces[q++] + x;
								GLfloat y_ = faces[q++]*hmux + y;
								GLfloat z_ = faces[q++] + z;
								addpoint(chunk, &c,&i,ebos,&v,&o,vbos,cbos,ebc,x_,y_,z_,blockcolor,block.id);
							}
						} else {
							q+=18;
						}
					}
				}
			}
		}
	}

	free(ebc);

	if(chunkabove)
		unlockRead(chunkabove);
	if(chunkbelow)
		unlockRead(chunkbelow);
	if(chunknorth)
		unlockRead(chunknorth);
	if(chunksouth)
		unlockRead(chunksouth);
	if(chunkeast)
		unlockRead(chunkeast);
	if(chunkwest)
		unlockRead(chunkwest);

	unlockRead(chunk);

	int w;

	GLuint *finalebodata = (GLuint *)malloc(sizeof(GLuint) * ((long)9999*c + i));

	for(w=0; w<c; w++)
	{
		memcpy(&finalebodata[(long)w*9999], ebos[w], (long)9999 * sizeof(GLuint));
		free(ebos[w]);
	}
	memcpy(&finalebodata[(long)c*9999], ebos[c], i*sizeof(GLuint));
	free(ebos[c]);

	GLfloat *finalvbodata = (GLfloat *)malloc(sizeof(GLfloat) * ((long)9999*v + o));
	GLfloat *finalcbodata = (GLfloat *)malloc(sizeof(GLfloat) * ((long)9999*v + o));

	for(w=0; w<v; w++)
	{
		memcpy(&finalvbodata[(long)w*9999], vbos[w], (long)9999 * sizeof(GLfloat));
		free(vbos[w]);
		memcpy(&finalcbodata[(long)w*9999], cbos[w], (long)9999 * sizeof(GLfloat));
		free(cbos[w]);
	}
	memcpy(&finalvbodata[(long)v*9999], vbos[v], o*sizeof(GLfloat));
	free(vbos[v]);
	memcpy(&finalcbodata[(long)v*9999], cbos[v], o*sizeof(GLfloat));
	free(cbos[v]);

	long ebosize = (long)9999*c + i;
	long vbosize = (long)9999*v + o;
	long cbosize = (long)9999*v + o;

	lockWrite(chunk);

	if(chunk->mesh.uploadnext)
	{
		free(chunk->mesh.vbodata);
		free(chunk->mesh.ebodata);
		free(chunk->mesh.cbodata);
	}

	chunk->mesh.points = ebosize;

	chunk->mesh.vbodata = finalvbodata;
	chunk->mesh.ebodata = finalebodata;
	chunk->mesh.cbodata = finalcbodata;

	chunk->mesh.vbodatasize = vbosize;
	chunk->mesh.ebodatasize = ebosize;
	chunk->mesh.cbodatasize = cbosize;

	chunk->iscurrent = 1;
	chunk->mesh.uploadnext = 1;

	unlockWrite(chunk);
}

int
chunk_iscurrent(chunk_t *chunk)
{
	int ret = chunk->iscurrent;
	if(ret)
	{
		return 1;
	} else {
		lockRead(chunk);
		ret = chunk->iscurrent;
		unlockRead(chunk);
		return ret;
	}
}

void
chunk_setnotcurrent(chunk_t *chunk)
{
	chunk->iscurrent = 0;
}

block_t
chunk_getblock(chunk_t *c, int x, int y, int z)
{
	if(MIN(MIN(x,y),z) < 0 || MAX(MAX(x,y),z) >= CHUNKSIZE)
	{
		block_t ret;
		ret.id = ERR;
		return ret;
	}

	block_t ret;

	lockRead(c);
	ret = getblock(c, x, y, z);
	unlockRead(c);

	return ret;
}

blockid_t
chunk_getblockid(chunk_t *c, int x, int y, int z)
{
	return chunk_getblock(c, x, y, z).id;
}

void
chunk_setblock(chunk_t *c, int x, int y, int z, block_t b)
{
	if(MIN(MIN(x,y),z) < 0 || MAX(MAX(x,y),z) >= CHUNKSIZE)
		return;

	lockWrite(c);
		setblock(c, x, y, z, b);
	unlockWrite(c);
}

void
chunk_setblockid(chunk_t *c, int x, int y, int z, blockid_t id)
{
	block_t b;
	b.id = id;
	chunk_setblock(c, x, y, z, b);
}

void
chunk_setair(chunk_t *c, int x, int y, int z)
{
	chunk_setblockid(c, x, y, z, AIR);
}

long3_t
chunk_getpos(chunk_t *chunk)
{
	return chunk->pos;
}

void
chunk_updatequeue(chunk_t *chunk, int x, int y, int z, int time, update_flags_t flags)
{
	lockWrite(chunk);
	if(chunk->iscompressed)
	{
		updatequeue(chunk, x, y, z, time, flags);
	} else {
		struct updatequeue_s *update = &(chunk->rawupdates[x + y*CHUNKSIZE + z*CHUNKSIZE*CHUNKSIZE]);
		if(update->time >= 0)
		{
			update->flags |= flags;
			update->time = imin(update->time, time);
		} else {
			update->time = time;
			update->flags = flags;
		}
	}
	unlockWrite(chunk);
}

long
chunk_updaterun(chunk_t *chunk)
{
	long num = 0;

	if(chunk->iscompressed)
	{
		if(chunk->updates)
		{
			struct updatequeue_s *node = chunk->updates;

			while(node)
			{
				struct updatequeue_s *next = node->next;
				if(node->time == 0)
				{
					if(chunk->updates == node)
						chunk->updates = node->next;

					if(node->next != 0)
						node->next->prev = node->prev;
					if(node->prev != 0)
						node->prev->next = node->next;

					long3_t pos = chunk_getworldposfromchunkpos(
							chunk->pos,
							node->pos.x,
							node->pos.y,
							node->pos.z
						);

					block_updaterun(chunk_getblock(chunk, node->pos.x, node->pos.y, node->pos.z), pos, node->flags);
					num++;

					free(node);
				} else {
					node->time--;
				}
				node = next;
			}
		}
	} else {
		int x, y, z;
		for(x=0; x<CHUNKSIZE; x++)
		for(y=0; y<CHUNKSIZE; y++)
		for(z=0; z<CHUNKSIZE; z++)
		{
			struct updatequeue_s *node = &chunk->rawupdates[x + y*CHUNKSIZE + z*CHUNKSIZE*CHUNKSIZE];
			if(node->time >= 0)
			{
				node->time--;
				if(node->time == -1)
				{
					long3_t pos = chunk_getworldposfromchunkpos(
							chunk->pos,
							x, y, z
						);

					block_t block = chunk_getblock(chunk, x, y, z); 
					block_updaterun(block, pos, node->flags);
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
chunk_loadchunk(long3_t pos)
{
	chunk_t *chunk = chunk_loademptychunk(pos);

	worldgen_genchunk(chunk);

	return chunk;//never laods from disk.
}

chunk_t *
chunk_loademptychunk(long3_t pos)
{
	chunk_t *chunk = malloc(sizeof(chunk_t));

	chunk->pos = pos;
	chunk->data = octree_create();

	init(chunk);

	return chunk;
}

void
chunk_freechunk(chunk_t *chunk)
{
	glDeleteBuffers(BUFFERS_MAX, chunk->mesh.bufferobjs);
	octree_destroy(chunk->data);
	SDL_DestroyMutex(chunk->mutex_read);
	SDL_DestroySemaphore(chunk->sem_write);
	free(chunk);
}

int
chunk_reloadchunk(long3_t pos, chunk_t *chunk)
{
	lockWrite(chunk);
	if(!chunk->iscompressed)
	{
		chunk->iscompressed = 1;
		free(chunk->rawblocks);
		free(chunk->rawupdates);
		numuncompressed--;
	}

	//TODO: clear updates

	chunk->pos = pos;
	octree_zero(chunk->data);
	unlockWrite(chunk);
	worldgen_genchunk(chunk);
	lockWrite(chunk);
	chunk->iscurrent = 0;
	unlockWrite(chunk);
	return 0;//never loads from disk
}

void
chunk_zerochunk(chunk_t *chunk)
{
	lockWrite(chunk);
	octree_zero(chunk->data);
	unlockWrite(chunk);
}
