#include "chunk.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <GL/glew.h>

#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX(a,b) ((a)>(b)?(a):(b))

struct chunk_s {
	long3_t pos;
	block_t *data;
};

static inline void
addpoint(chunk_t *chunk, int *c, uint16_t *i, GLuint **ebos, int *v, uint16_t *o, GLfloat **vbos, int *k, uint16_t *j, GLfloat **color, GLuint *ebc, GLint x, GLint y, GLint z, vec3_t blockcolor)
{
	GLint index = x + y*(CHUNKSIZE+1) + z*(CHUNKSIZE+1)*(CHUNKSIZE+1);
	if(!ebc[index])
	{
		//add point to vbo
		//the max for o is a multiple for three, so we only have to check for the 'overflow' every three floats or one point
		vbos[v[0]][o[0]++] = x + chunk->pos.x*CHUNKSIZE;
		vbos[v[0]][o[0]++] = y + chunk->pos.y*CHUNKSIZE;
		vbos[v[0]][o[0]++] = z + chunk->pos.z*CHUNKSIZE;
		if(o[0] == 9999)
		{
			o[0]=0;
			v[0]++;
			vbos[v[0]] = (GLfloat *)malloc(sizeof(GLfloat) * 9999);
		}

		ebc[index] = v[0]*3333 + o[0]/3 +1;//add one because 0 is null. subtract one everywhere else.
	}
	ebos[c[0]][i[0]++] = ebc[index] -1;
	if(i[0] == 9999)
	{
		i[0]=0;
		c[0]++;
		ebos[c[0]] = (GLuint *)malloc(sizeof(GLuint) * 9999);
	}

	color[k[0]][j[0]++] = blockcolor.x;
	color[k[0]][j[0]++] = blockcolor.y;
	color[k[0]][j[0]++] = blockcolor.z;
	if(j[0] == 9999)
	{
		j[0]=0;
		k[0]++;
		color[k[0]] = (GLfloat *)malloc(sizeof(GLfloat) * 9999);
	}
}

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
1,0,1,
0,0,1,

1,0,1,
0,0,1,
1,0,0,

//north
0,1,0,
1,1,0,
0,0,0,

1,0,0,
0,0,0,
1,1,0,

//south
0,1,1,
0,0,1,
1,1,1,

1,0,1,
1,1,1,
0,0,1,

//east
1,1,0,
1,1,1,
1,0,0,

1,0,1,
1,0,0,
1,1,1,

//west
0,1,0,
0,0,0,
0,1,1,

0,0,1,
0,1,1,
0,0,0
};

mesh_t
chunk_getmesh(chunk_t *chunk, chunk_t *chunkabove, chunk_t *chunkbelow, chunk_t *chunknorth, chunk_t *chunksouth, chunk_t *chunkeast, chunk_t *chunkwest)
{
	GLuint *ebos[256];
	int c = 0;
	uint16_t i = 0;

	GLfloat *vbos[256];
	int v = 0;
	uint16_t o = 0;

	GLfloat *color[256];
	int k = 0;
	uint16_t j = 0;

	ebos[0] = (GLuint *)malloc(sizeof(GLuint) * 9999);
	vbos[0] = (GLfloat *)malloc(sizeof(GLfloat) * 9999);
	color[0] = (GLfloat *)malloc(sizeof(GLfloat) * 9999);

	GLuint *ebc = (GLuint *)calloc(sizeof(GLuint), (CHUNKSIZE+1) * (CHUNKSIZE+1) * (CHUNKSIZE+1));

	int x, y, z;
	for(x=0; x<CHUNKSIZE; x++)
	{
		for(y=0; y<CHUNKSIZE; y++)
		{
			for(z=0; z<CHUNKSIZE; z++)
			{
				long index = x+CHUNKSIZE*y+CHUNKSIZE*CHUNKSIZE*z;
				if(chunk->data[index].id)
				{
					int top, bottom, north, south, east, west;

					if(y==CHUNKSIZE-1)
					{
						top=1;
					} else {
						if(chunk->data[index+CHUNKSIZE].id)
							top = 0;
						else
							top = 1;
					}
					if(y==0)
					{
						bottom=1;
					} else {
						if(chunk->data[index-CHUNKSIZE].id)
							bottom = 0;
						else
							bottom = 1;
					}
					if(z==0)
					{
						north=1;
					} else {
						if(chunk->data[index-CHUNKSIZE*CHUNKSIZE].id)
							north = 0;
						else
							north = 1;
					}
					if(z==CHUNKSIZE-1)
					{
						south=1;
					} else {
						if(chunk->data[index+CHUNKSIZE*CHUNKSIZE].id)
							south = 0;
						else
							south = 1;
					}
					if(x==CHUNKSIZE-1)
					{
						east=1;
					} else {
						if(chunk->data[index+1].id)
							east = 0;
						else
							east = 1;
					}
					if(x==0)
					{
						west=1;
					} else {
						if(chunk->data[index-1].id)
							west = 0;
						else
							west = 1;
					}

					vec3_t blockcolor = block_getcolor(chunk->data[index].id);

					int U[6];
					U[0]=top;
					U[1]=bottom;
					U[2]=north;
					U[3]=south;
					U[4]=east;
					U[5]=west;
					int q=0;
					int t;
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
								addpoint(chunk, &c,&i,ebos,&v,&o,vbos,&k,&j,color,ebc,x_,y_,z_,blockcolor);
							}
						} else {
							q+=18;
						}
					}
				}
			}
		}
	}

	vec3_t blockcolor;
	blockcolor.x = 1;
	blockcolor.y = 1;
	blockcolor.z = 0;

	free(ebc);

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

	for(w=0; w<v; w++)
	{
		memcpy(&finalvbodata[(long)w*9999], vbos[w], (long)9999 * sizeof(GLfloat));
		free(vbos[w]);
	}
	memcpy(&finalvbodata[(long)c*9999], vbos[v], o*sizeof(GLfloat));
	free(vbos[v]);

	GLfloat *finalcolordata = (GLfloat *)malloc(sizeof(GLfloat) * ((long)9999*k + j));

	for(w=0; w<k; w++)
	{
		memcpy(&finalcolordata[(long)w*9999], ebos[w], (long)9999 * sizeof(GLfloat));
		free(color[w]);
	}
	memcpy(&finalcolordata[(long)k*9999], color[k], j*sizeof(GLfloat));
	free(color[c]);

	mesh_t ret;
	ret.ebodata = finalebodata;
	ret.vbodata = finalvbodata;
	ret.colordata = finalcolordata;

	ret.ebosize = (long)9999*c + i;
	ret.vbosize = (long)9999*v + o;
	ret.colorsize = (long)9999*k + j;

	return ret;
}

void
worldgen_genchunk(chunk_t *chunk)
{
	if(!chunk->data)
		free(chunk->data);
	chunk->data = chunk_callocdata();

	int x, y, z;
	for(x=0; x<CHUNKSIZE; x++)
	{
		for(y=0; y<CHUNKSIZE; y++)
		{
			for(z=0; z<CHUNKSIZE; z++)
			{
				int i = 0;
				if(x==0 || x==CHUNKSIZE-1)
					i++;
				if(y==0 || y==CHUNKSIZE-1)
					i++;
				if(z==0 || z==CHUNKSIZE-1)
					i++;
				if(i>1)
					chunk->data[x + y*CHUNKSIZE + z*CHUNKSIZE*CHUNKSIZE].id=1;
			}
		}
	}
}

void
chunk_freechunk(chunk_t *chunk)
{
	free(chunk->data);
	free(chunk);
}

int
chunk_loadchunk(long3_t pos, chunk_t **chunk)
{
	*chunk = (chunk_t *)malloc(sizeof(chunk_t));

	(*chunk)->pos = pos;
	(*chunk)->data=0;

	worldgen_genchunk(*chunk);

	return 0;//never laods from disk.
}

void
chunk_loademptychunk(long3_t pos, chunk_t **chunk)
{
	*chunk = (chunk_t *)malloc(sizeof(chunk_t));

	(*chunk)->pos = pos;
	(*chunk)->data = calloc(CHUNKSIZE*CHUNKSIZE*CHUNKSIZE, sizeof(block_t));
}

int
chunk_reloadchunk(long3_t pos, chunk_t *chunk)
{
	chunk->pos = pos;
	worldgen_genchunk(chunk);
	return 0;//never loads from disk
}

void
chunk_emptychunk(chunk_t *chunk)
{
	if(chunk->data)
		free(chunk->data);
	chunk->data = calloc(CHUNKSIZE*CHUNKSIZE*CHUNKSIZE, sizeof(block_t));
}

block_t
chunk_getblock(chunk_t *c, int x, int y, int z)
{
	if(MIN(MIN(x,y),z) < 0 || MAX(MAX(x,y),z) >= CHUNKSIZE)
	{
		block_t ret;
		ret.id = 255;
		return ret;
	}

	return c->data[x + y*CHUNKSIZE + z*CHUNKSIZE*CHUNKSIZE];
}

void
chunk_setblock(chunk_t *c, int x, int y, int z, block_t b)
{
	if(MIN(MIN(x,y),z) < 0 || MAX(MAX(x,y),z) >= CHUNKSIZE)
		return;

	c->data[x + y*CHUNKSIZE + z*CHUNKSIZE*CHUNKSIZE] = b;
}

long3_t
chunk_getpos(chunk_t *chunk)
{
	return chunk->pos;
}
