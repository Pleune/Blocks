#include "chunk.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <pthread.h>

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
	uint8_t time;
};

struct chunk_s {
	long3_t pos;
	octree_t *data;

	struct updatequeue_s *updates;

	struct mesh_s mesh;
	int iscurrent;

	pthread_rwlock_t  rwlock;
};

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
lockWrite(chunk_t *chunk)
{
	pthread_rwlock_wrlock(&chunk->rwlock);
}

void
unlockWrite(chunk_t *chunk)
{
	pthread_rwlock_unlock(&chunk->rwlock);
}


void
lockRead(chunk_t *chunk)
{
	pthread_rwlock_rdlock(&chunk->rwlock);
}

void
unlockRead(chunk_t *chunk)
{
	pthread_rwlock_unlock(&chunk->rwlock);
}
void
init(chunk_t *chunk)
{
	glGenBuffers(BUFFERS_MAX, chunk->mesh.bufferobjs);
	chunk->updates = 0;
	chunk->mesh.uploadnext = 0;
	chunk->iscurrent = 0;
	pthread_rwlock_init(&chunk->rwlock, 0);
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
addpoint(chunk_t *chunk, int *c, uint16_t *i, GLuint **ebos, int *v, uint16_t *o, GLfloat **vbos, GLfloat **color, GLuint *ebc, GLint x, GLint y, GLint z, vec3_t blockcolor, uint8_t blockid)
{
	GLint index = x + y*(CHUNKSIZE+1) + z*(CHUNKSIZE+1)*(CHUNKSIZE+1) + blockid * (CHUNKSIZE+1)*(CHUNKSIZE+1)*256;
	if(!ebc[index])
	{
		//add point to vbo
		//the max for o is a multiple for three, so we only have to check for the 'overflow' every three floats or one point
		vec3_t pos;
		pos.x = x + chunk->pos.x*CHUNKSIZE;
		pos.y = y + chunk->pos.y*CHUNKSIZE;
		pos.z = z + chunk->pos.z*CHUNKSIZE;

		vec3_t n;
		n.x = (float)(noise(pos.x, pos.y, pos.z)%1000) * ((float)RENDER_WOBBLE / 1000.0f);
		n.y = (float)(noise(pos.y, pos.z, pos.x)%1000) * ((float)RENDER_WOBBLE / 1000.0f);
		n.z = (float)(noise(pos.z, pos.x, pos.y)%1000) * ((float)RENDER_WOBBLE / 1000.0f);

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
	static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
	pthread_mutex_lock(&lock);
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
	pthread_mutex_unlock(&lock);

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

	GLuint *ebc = (GLuint *)calloc(sizeof(GLuint), (CHUNKSIZE+1) * (CHUNKSIZE+1) * (CHUNKSIZE+1) * 256);

	int x, y, z;
	for(x=0; x<CHUNKSIZE; x++)
	{
		for(y=0; y<CHUNKSIZE; y++)
		{
			for(z=0; z<CHUNKSIZE; z++)
			{
		//		long index = x+CHUNKSIZE*y+CHUNKSIZE*CHUNKSIZE*z;
				if(octree_get(x, y, z, chunk->data).id)
				{
					int top, bottom, south, north, east, west;

					if(y==CHUNKSIZE-1)
					{
						if(chunkabove)
						{
							if(octree_get(x,0,z,chunkabove->data).id)
								top=0;
							else
								top=1;
						} else
							top=1;
					} else {
						if(octree_get(x,y+1,z,chunk->data).id)
							top = 0;
						else
							top = 1;
					}
					if(y==0)
					{
						if(chunkbelow)
						{
							if(octree_get(x,CHUNKSIZE-1,z,chunkbelow->data).id)
								bottom=0;
							else
								bottom=1;
						} else
							bottom=1;
					} else {
						if(octree_get(x,y-1,z,chunk->data).id)
							bottom = 0;
						else
							bottom = 1;
					}
					if(z==CHUNKSIZE-1)
					{
						if(chunksouth)
						{
							if(octree_get(x,y,0,chunksouth->data).id)
								south=0;
							else
								south=1;
						} else
							south=1;
					} else {
						if(octree_get(x,y,z+1,chunk->data).id)
							south = 0;
						else
							south = 1;
					}
					if(z==0)
					{
						if(chunknorth)
						{
							if(octree_get(x,y,CHUNKSIZE-1, chunknorth->data).id)
								north=0;
							else
								north=1;
						} else
							north=1;
					} else {
						if(octree_get(x,y,z-1,chunk->data).id)
							north = 0;
						else
							north = 1;
					}

					if(x==CHUNKSIZE-1)
					{
						if(chunkeast)
						{
							if(octree_get(0,y,z,chunkeast->data).id)
								east=0;
							else
								east=1;
						} else
							east=1;
					} else {
						if(octree_get(x+1,y,z,chunk->data).id)
							east = 0;
						else
							east = 1;
					}
					if(x==0)
					{
						if(chunkwest)
						{
							if(octree_get(CHUNKSIZE-1,y,z, chunkwest->data).id)
								west=0;
							else
								west=1;
						} else
							west=1;
					} else {
						if(octree_get(x-1,y,z,chunk->data).id)
							west = 0;
						else
							west = 1;
					}

					block_t block = octree_get(x,y,z,chunk->data);
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

					/*
					//point data
					termbodata[termi++] = faces[q++] + x + chunk->pos.x*CHUNKSIZE;
					termbodata[termi++] = faces[q++] + y + chunk->pos.y*CHUNKSIZE;
					termbodata[termi++] = faces[q++] + z + chunk->pos.z*CHUNKSIZE;
					//texcoord data
					termbodata[termi++] = uvcoords[uvcoordsabstraction[texcount]*2];
					termbodata[termi++] = uvcoords[uvcoordsabstraction[texcount++]*2+1];
					*/
					for(t=0;t<6;t++)
					{
						if(U[t])
						{
							int Q=q+18;
							while(q<Q)
							{
								GLfloat x_ = faces[q++] + x;
								GLfloat y_ = faces[q++] + y;
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
	lockWrite(chunk);

	if(chunk->mesh.uploadnext)
	{
		free(chunk->mesh.vbodata);
		free(chunk->mesh.ebodata);
		free(chunk->mesh.cbodata);
	}

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
	return chunk->iscurrent;
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

	lockRead(c);
	block_t ret = octree_get(x, y, z, c->data);
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
	octree_set(x, y, z, c->data, &b);
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
chunk_updatequeue(chunk_t *chunk, int x, int y, int z, uint8_t time, update_flags_t flags)
{
	struct updatequeue_s *new = malloc(sizeof(struct updatequeue_s));
	if(chunk->updates)
	{
		struct updatequeue_s *top = chunk->updates;
		while(top->next)
			top = top->next;
		top->next = new;
		new->prev = top;
		new->next = 0;
	} else {
		chunk->updates = new;
		new->next = 0;
		new->prev = 0;
	}

	new->pos = world_getinternalposofworldpos(x,y,z);
	new->time = time;
	new->flags = flags;
}

long
chunk_updaterun(chunk_t *chunk)
{
	long num = 0;

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

				block_updaterun(chunk_getblockid(chunk, node->pos.x, node->pos.y, node->pos.z), pos, node->flags);
				num++;

				free(node);
			} else {
				node->time--;
			}
			node = next;
		}
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
	pthread_rwlock_destroy(&chunk->rwlock);
	free(chunk);
}

int
chunk_reloadchunk(long3_t pos, chunk_t *chunk)
{
	lockWrite(chunk);
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
