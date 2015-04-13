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
addpoint(chunk_t *chunk, int *c, uint16_t *i, GLuint **ebos, int *v, uint16_t *o, GLfloat **vbos, GLfloat **color, GLuint *ebc, GLint x, GLint y, GLint z, vec3_t blockcolor, uint8_t blockid)
{
	GLint index = x + y*(CHUNKSIZE+1) + z*(CHUNKSIZE+1)*(CHUNKSIZE+1) + blockid * (CHUNKSIZE+1)*(CHUNKSIZE+1)*256;
	if(!ebc[index])
	{
		//add point to vbo
		//the max for o is a multiple for three, so we only have to check for the 'overflow' every three floats or one point
		vbos[v[0]][o[0]] = x + chunk->pos.x*CHUNKSIZE;
		color[v[0]][o[0]++] = blockcolor.x;
		vbos[v[0]][o[0]] = y + chunk->pos.y*CHUNKSIZE;
		color[v[0]][o[0]++] = blockcolor.y;
		vbos[v[0]][o[0]] = z + chunk->pos.z*CHUNKSIZE;
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

//north
1,1,0,
1,0,0,
0,1,0,

0,0,0,
0,1,0,
1,0,0,

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

GLfloat terminaltexcoords[] = {
	0,0,
	1,0,
	1,1,
	0,1
};

int terminaltexcoordsabstraction[] = {
	0,
	3,
	1,
	2,
	1,
	3
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

	//TODO: prevent overflow
	GLfloat *terminalscreens = (GLfloat *)malloc(sizeof(GLfloat) * 65536);
	uint16_t termi= 0;

	ebos[0] = (GLuint *)malloc(sizeof(GLuint) * 9999);
	vbos[0] = (GLfloat *)malloc(sizeof(GLfloat) * 9999);
	color[0] = (GLfloat *)malloc(sizeof(GLfloat) * 9999);

	GLuint *ebc = (GLuint *)calloc(sizeof(GLuint), (CHUNKSIZE+1) * (CHUNKSIZE+1) * (CHUNKSIZE+1) * 256);

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
						if(chunkabove)
						{
							if(chunk_getblock(chunkabove,x,0,z).id)
								top=0;
							else
								top=1;
						} else
							top=1;
					} else {
						if(chunk_getblock(chunk,x,y+1,z).id)
							top = 0;
						else
							top = 1;
					}
					if(y==0)
					{
						if(chunkbelow)
						{
							if(chunk_getblock(chunkbelow,x,CHUNKSIZE-1,z).id)
								bottom=0;
							else
								bottom=1;
						} else
							bottom=1;
					} else {
						if(chunk_getblock(chunk,x,y-1,z).id)
							bottom = 0;
						else
							bottom = 1;
					}
					if(z==0)
					{
						if(chunknorth)
						{
							if(chunk_getblock(chunknorth,x,y,CHUNKSIZE-1).id)
								north=0;
							else
								north=1;
						} else
							north=1;
					} else {
						if(chunk_getblock(chunk,x,y,z-1).id)
							north = 0;
						else
							north = 1;
					}
					if(z==CHUNKSIZE-1)
					{
						if(chunksouth)
						{
							if(chunk_getblock(chunksouth,x,y,0).id)
								south=0;
							else
								south=1;
						} else
							south=1;
					} else {
						if(chunk_getblock(chunk,x,y,z+1).id)
							south = 0;
						else
							south = 1;
					}
					if(x==CHUNKSIZE-1)
					{
						if(chunkeast)
						{
							if(chunk_getblock(chunkeast,0,y,z).id)
								east=0;
							else
								east=1;
						} else
							east=1;
					} else {
						if(chunk_getblock(chunk,x+1,y,z).id)
							east = 0;
						else
							east = 1;
					}
					if(x==0)
					{
						if(chunkwest)
						{
							if(chunk_getblock(chunkwest,CHUNKSIZE-1,y,z).id)
								west=0;
							else
								west=1;
						} else
							west=1;
					} else {
						if(chunk_getblock(chunk,x-1,y,z).id)
							west = 0;
						else
							west = 1;
					}

					block_t block = chunk_getblock(chunk,x,y,z);
					vec3_t blockcolor = block_getcolor(block.id);

					int U[6] = {
						top, bottom, north, south, east, west
					};
					int q=0;
					int t;

					switch(block.id)
					{
					case 2:
					{
						for(t=0;t<6;t++)
						{
							if(U[t])
							{
								if(t == ((struct block_term_t *)(block.metadata.pointer))->face)
								{
									int Q=q+18;
									int texcount = 0;
									while(q<Q)
									{
										//point data
										terminalscreens[termi++] = faces[q++] + x + chunk->pos.x*CHUNKSIZE;
										terminalscreens[termi++] = faces[q++] + y + chunk->pos.y*CHUNKSIZE;
										terminalscreens[termi++] = faces[q++] + z + chunk->pos.z*CHUNKSIZE;

										//texcoord data
										terminalscreens[termi++] = terminaltexcoords[terminaltexcoordsabstraction[texcount]*2];
										terminalscreens[termi++] = terminaltexcoords[terminaltexcoordsabstraction[texcount++]*2+1];
									}
								} else {
									int Q=q+18;
									while(q<Q)
									{
										GLfloat x_ = faces[q++] + x;
										GLfloat y_ = faces[q++] + y;
										GLfloat z_ = faces[q++] + z;
										addpoint(chunk, &c,&i,ebos,&v,&o,vbos,color,ebc,x_,y_,z_,blockcolor,block.id);
									}
								}
							} else {
								q+=18;
							}
						}
						break;
					}

					default:
					{
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
									addpoint(chunk, &c,&i,ebos,&v,&o,vbos,color,ebc,x_,y_,z_,blockcolor,block.id);
								}
							} else {
								q+=18;
							}
						}
						break;
					}
					}
				}
			}
		}
	}

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
	GLfloat *finalcolordata = (GLfloat *)malloc(sizeof(GLfloat) * ((long)9999*v + o));

	for(w=0; w<v; w++)
	{
		memcpy(&finalvbodata[(long)w*9999], vbos[w], (long)9999 * sizeof(GLfloat));
		free(vbos[w]);
		memcpy(&finalcolordata[(long)w*9999], ebos[w], (long)9999 * sizeof(GLfloat));
		free(color[w]);
	}
	memcpy(&finalvbodata[(long)v*9999], vbos[v], o*sizeof(GLfloat));
	free(vbos[v]);
	memcpy(&finalcolordata[(long)v*9999], color[v], o*sizeof(GLfloat));
	free(color[v]);

	GLfloat *finaltermscreendata = (GLfloat *)malloc(sizeof(GLfloat) * termi);
	memcpy(finaltermscreendata, terminalscreens, sizeof(GLfloat) * termi);
	free(terminalscreens);

	mesh_t ret;
	ret.ebodata = finalebodata;
	ret.vbodata = finalvbodata;
	ret.colordata = finalcolordata;
	ret.termscreendata = finaltermscreendata;

	ret.ebosize = (long)9999*c + i;
	ret.vbosize = (long)9999*v + o;
	ret.colorsize = (long)9999*v + o;
	ret.termscreensize = termi;

	return ret;
}

void
worldgen_genchunk(chunk_t *chunk)
{

	//chunk->data[0].id=1;

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
				else
					chunk->data[x + y*CHUNKSIZE + z*CHUNKSIZE*CHUNKSIZE].id=0;
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
	(*chunk)->data = (block_t *)calloc(CHUNKSIZE*CHUNKSIZE*CHUNKSIZE, sizeof(block_t));

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
