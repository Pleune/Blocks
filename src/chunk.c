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

static inline uint32_t
smartinc(int *c, uint32_t *i, GLfloat **memchunks)
{
	if(i[0]==65535)
	{
		i[0]=0;
		c[0]++;
		memchunks[c[0]] = (GLfloat *)malloc(sizeof(GLfloat) * 65536);
		return 65535;
	} else {
		i[0] += 1;
		return i[0]-1;
	}
}

mesh_t
chunk_getmesh(chunk_t *chunk, chunk_t *chunkabove, chunk_t *chunkbelow, chunk_t *chunknorth, chunk_t *chunksouth, chunk_t *chunkeast, chunk_t *chunkwest)
{
	GLfloat *memchunks[256];
	int c = 0;
	uint32_t i = 0;

	GLfloat *colorchunks[256];
	int k = 0;
	uint32_t j = 0;

	memchunks[0] = (GLfloat *)malloc(sizeof(GLfloat) * 65536);
	colorchunks[0] = (GLfloat *)malloc(sizeof(GLfloat) * 65536);

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

					vec3_t color = block_getcolor(chunk->data[index].id);

					if(i<65400)
					{
						//dont worry about filling the memchunk
						//top
						if(top)
						{
							memchunks[c][i++] = x+chunk->pos.x*CHUNKSIZE; memchunks[c][i++] = y+chunk->pos.y*CHUNKSIZE+1; memchunks[c][i++] = z+chunk->pos.z*CHUNKSIZE;
							memchunks[c][i++] = x+chunk->pos.x*CHUNKSIZE; memchunks[c][i++] = y+chunk->pos.y*CHUNKSIZE+1; memchunks[c][i++] = z+chunk->pos.z*CHUNKSIZE+1;
							memchunks[c][i++] = x+chunk->pos.x*CHUNKSIZE+1; memchunks[c][i++] = y+chunk->pos.y*CHUNKSIZE+1; memchunks[c][i++] = z+chunk->pos.z*CHUNKSIZE;

							memchunks[c][i++] = x+chunk->pos.x*CHUNKSIZE+1; memchunks[c][i++] = y+chunk->pos.y*CHUNKSIZE+1; memchunks[c][i++] = z+chunk->pos.z*CHUNKSIZE+1;
							memchunks[c][i++] = x+chunk->pos.x*CHUNKSIZE+1; memchunks[c][i++] = y+chunk->pos.y*CHUNKSIZE+1; memchunks[c][i++] = z+chunk->pos.z*CHUNKSIZE;
							memchunks[c][i++] = x+chunk->pos.x*CHUNKSIZE; memchunks[c][i++] = y+chunk->pos.y*CHUNKSIZE+1; memchunks[c][i++] = z+chunk->pos.z*CHUNKSIZE+1;

							//color
							colorchunks[k][j++] = color.x; colorchunks[k][j++] = color.y; colorchunks[k][j++] = color.z;
							colorchunks[k][j++] = color.x; colorchunks[k][j++] = color.y; colorchunks[k][j++] = color.z;
							colorchunks[k][j++] = color.x; colorchunks[k][j++] = color.y; colorchunks[k][j++] = color.z;
							colorchunks[k][j++] = color.x; colorchunks[k][j++] = color.y; colorchunks[k][j++] = color.z;
							colorchunks[k][j++] = color.x; colorchunks[k][j++] = color.y; colorchunks[k][j++] = color.z;
							colorchunks[k][j++] = color.x; colorchunks[k][j++] = color.y; colorchunks[k][j++] = color.z;
						}
						//bottom
						if(bottom)
						{
							memchunks[c][i++] = x+chunk->pos.x*CHUNKSIZE; memchunks[c][i++] = y+chunk->pos.y*CHUNKSIZE; memchunks[c][i++] = z+chunk->pos.z*CHUNKSIZE;
							memchunks[c][i++] = x+chunk->pos.x*CHUNKSIZE+1; memchunks[c][i++] = y+chunk->pos.y*CHUNKSIZE; memchunks[c][i++] = z+chunk->pos.z*CHUNKSIZE;
							memchunks[c][i++] = x+chunk->pos.x*CHUNKSIZE; memchunks[c][i++] = y+chunk->pos.y*CHUNKSIZE; memchunks[c][i++] = z+chunk->pos.z*CHUNKSIZE+1;

							memchunks[c][i++] = x+chunk->pos.x*CHUNKSIZE+1; memchunks[c][i++] = y+chunk->pos.y*CHUNKSIZE; memchunks[c][i++] = z+chunk->pos.z*CHUNKSIZE+1;
							memchunks[c][i++] = x+chunk->pos.x*CHUNKSIZE; memchunks[c][i++] = y+chunk->pos.y*CHUNKSIZE; memchunks[c][i++] = z+chunk->pos.z*CHUNKSIZE+1;
							memchunks[c][i++] = x+chunk->pos.x*CHUNKSIZE+1; memchunks[c][i++] = y+chunk->pos.y*CHUNKSIZE; memchunks[c][i++] = z+chunk->pos.z*CHUNKSIZE;

							//color
							colorchunks[k][j++] = color.x; colorchunks[k][j++] = color.y; colorchunks[k][j++] = color.z;
							colorchunks[k][j++] = color.x; colorchunks[k][j++] = color.y; colorchunks[k][j++] = color.z;
							colorchunks[k][j++] = color.x; colorchunks[k][j++] = color.y; colorchunks[k][j++] = color.z;
							colorchunks[k][j++] = color.x; colorchunks[k][j++] = color.y; colorchunks[k][j++] = color.z;
							colorchunks[k][j++] = color.x; colorchunks[k][j++] = color.y; colorchunks[k][j++] = color.z;
							colorchunks[k][j++] = color.x; colorchunks[k][j++] = color.y; colorchunks[k][j++] = color.z;
						}
						//north
						if(north)
						{
							memchunks[c][i++] = x+chunk->pos.x*CHUNKSIZE; memchunks[c][i++] = y+chunk->pos.y*CHUNKSIZE+1; memchunks[c][i++] = z+chunk->pos.z*CHUNKSIZE;
							memchunks[c][i++] = x+chunk->pos.x*CHUNKSIZE+1; memchunks[c][i++] = y+chunk->pos.y*CHUNKSIZE+1; memchunks[c][i++] = z+chunk->pos.z*CHUNKSIZE;
							memchunks[c][i++] = x+chunk->pos.x*CHUNKSIZE; memchunks[c][i++] = y+chunk->pos.y*CHUNKSIZE; memchunks[c][i++] = z+chunk->pos.z*CHUNKSIZE;

							memchunks[c][i++] = x+chunk->pos.x*CHUNKSIZE+1; memchunks[c][i++] = y+chunk->pos.y*CHUNKSIZE; memchunks[c][i++] = z+chunk->pos.z*CHUNKSIZE;
							memchunks[c][i++] = x+chunk->pos.x*CHUNKSIZE; memchunks[c][i++] = y+chunk->pos.y*CHUNKSIZE; memchunks[c][i++] = z+chunk->pos.z*CHUNKSIZE;
							memchunks[c][i++] = x+chunk->pos.x*CHUNKSIZE+1; memchunks[c][i++] = y+chunk->pos.y*CHUNKSIZE+1; memchunks[c][i++] = z+chunk->pos.z*CHUNKSIZE;

							//color
							colorchunks[k][j++] = color.x; colorchunks[k][j++] = color.y; colorchunks[k][j++] = color.z;
							colorchunks[k][j++] = color.x; colorchunks[k][j++] = color.y; colorchunks[k][j++] = color.z;
							colorchunks[k][j++] = color.x; colorchunks[k][j++] = color.y; colorchunks[k][j++] = color.z;
							colorchunks[k][j++] = color.x; colorchunks[k][j++] = color.y; colorchunks[k][j++] = color.z;
							colorchunks[k][j++] = color.x; colorchunks[k][j++] = color.y; colorchunks[k][j++] = color.z;
							colorchunks[k][j++] = color.x; colorchunks[k][j++] = color.y; colorchunks[k][j++] = color.z;
						}
						//south
						if(south)
						{
							memchunks[c][i++] = x+chunk->pos.x*CHUNKSIZE; memchunks[c][i++] = y+chunk->pos.y*CHUNKSIZE+1; memchunks[c][i++] = z+chunk->pos.z*CHUNKSIZE+1;
							memchunks[c][i++] = x+chunk->pos.x*CHUNKSIZE; memchunks[c][i++] = y+chunk->pos.y*CHUNKSIZE; memchunks[c][i++] = z+chunk->pos.z*CHUNKSIZE+1;
							memchunks[c][i++] = x+chunk->pos.x*CHUNKSIZE+1; memchunks[c][i++] = y+chunk->pos.y*CHUNKSIZE+1; memchunks[c][i++] = z+chunk->pos.z*CHUNKSIZE+1;

							memchunks[c][i++] = x+chunk->pos.x*CHUNKSIZE+1; memchunks[c][i++] = y+chunk->pos.y*CHUNKSIZE; memchunks[c][i++] = z+chunk->pos.z*CHUNKSIZE+1;
							memchunks[c][i++] = x+chunk->pos.x*CHUNKSIZE+1; memchunks[c][i++] = y+chunk->pos.y*CHUNKSIZE+1; memchunks[c][i++] = z+chunk->pos.z*CHUNKSIZE+1;
							memchunks[c][i++] = x+chunk->pos.x*CHUNKSIZE; memchunks[c][i++] = y+chunk->pos.y*CHUNKSIZE; memchunks[c][i++] = z+chunk->pos.z*CHUNKSIZE+1;

							//color
							colorchunks[k][j++] = color.x; colorchunks[k][j++] = color.y; colorchunks[k][j++] = color.z;
							colorchunks[k][j++] = color.x; colorchunks[k][j++] = color.y; colorchunks[k][j++] = color.z;
							colorchunks[k][j++] = color.x; colorchunks[k][j++] = color.y; colorchunks[k][j++] = color.z;
							colorchunks[k][j++] = color.x; colorchunks[k][j++] = color.y; colorchunks[k][j++] = color.z;
							colorchunks[k][j++] = color.x; colorchunks[k][j++] = color.y; colorchunks[k][j++] = color.z;
							colorchunks[k][j++] = color.x; colorchunks[k][j++] = color.y; colorchunks[k][j++] = color.z;
						}
						//east
						if(east)
						{
							memchunks[c][i++] = x+chunk->pos.x*CHUNKSIZE+1; memchunks[c][i++] = y+chunk->pos.y*CHUNKSIZE+1; memchunks[c][i++] = z+chunk->pos.z*CHUNKSIZE;
							memchunks[c][i++] = x+chunk->pos.x*CHUNKSIZE+1; memchunks[c][i++] = y+chunk->pos.y*CHUNKSIZE+1; memchunks[c][i++] = z+chunk->pos.z*CHUNKSIZE+1;
							memchunks[c][i++] = x+chunk->pos.x*CHUNKSIZE+1; memchunks[c][i++] = y+chunk->pos.y*CHUNKSIZE; memchunks[c][i++] = z+chunk->pos.z*CHUNKSIZE;

							memchunks[c][i++] = x+chunk->pos.x*CHUNKSIZE+1; memchunks[c][i++] = y+chunk->pos.y*CHUNKSIZE; memchunks[c][i++] = z+chunk->pos.z*CHUNKSIZE+1;
							memchunks[c][i++] = x+chunk->pos.x*CHUNKSIZE+1; memchunks[c][i++] = y+chunk->pos.y*CHUNKSIZE; memchunks[c][i++] = z+chunk->pos.z*CHUNKSIZE;
							memchunks[c][i++] = x+chunk->pos.x*CHUNKSIZE+1; memchunks[c][i++] = y+chunk->pos.y*CHUNKSIZE+1; memchunks[c][i++] = z+chunk->pos.z*CHUNKSIZE+1;

							//color
							colorchunks[k][j++] = color.x; colorchunks[k][j++] = color.y; colorchunks[k][j++] = color.z;
							colorchunks[k][j++] = color.x; colorchunks[k][j++] = color.y; colorchunks[k][j++] = color.z;
							colorchunks[k][j++] = color.x; colorchunks[k][j++] = color.y; colorchunks[k][j++] = color.z;
							colorchunks[k][j++] = color.x; colorchunks[k][j++] = color.y; colorchunks[k][j++] = color.z;
							colorchunks[k][j++] = color.x; colorchunks[k][j++] = color.y; colorchunks[k][j++] = color.z;
							colorchunks[k][j++] = color.x; colorchunks[k][j++] = color.y; colorchunks[k][j++] = color.z;
						}
						//west
						if(west)
						{
							memchunks[c][i++] = x+chunk->pos.x*CHUNKSIZE; memchunks[c][i++] = y+chunk->pos.y*CHUNKSIZE+1; memchunks[c][i++] = z+chunk->pos.z*CHUNKSIZE;
							memchunks[c][i++] = x+chunk->pos.x*CHUNKSIZE; memchunks[c][i++] = y+chunk->pos.y*CHUNKSIZE; memchunks[c][i++] = z+chunk->pos.z*CHUNKSIZE;
							memchunks[c][i++] = x+chunk->pos.x*CHUNKSIZE; memchunks[c][i++] = y+chunk->pos.y*CHUNKSIZE+1; memchunks[c][i++] = z+chunk->pos.z*CHUNKSIZE+1;

							memchunks[c][i++] = x+chunk->pos.x*CHUNKSIZE; memchunks[c][i++] = y+chunk->pos.y*CHUNKSIZE; memchunks[c][i++] = z+chunk->pos.z*CHUNKSIZE+1;
							memchunks[c][i++] = x+chunk->pos.x*CHUNKSIZE; memchunks[c][i++] = y+chunk->pos.y*CHUNKSIZE+1; memchunks[c][i++] = z+chunk->pos.z*CHUNKSIZE+1;
							memchunks[c][i++] = x+chunk->pos.x*CHUNKSIZE; memchunks[c][i++] = y+chunk->pos.y*CHUNKSIZE; memchunks[c][i++] = z+chunk->pos.z*CHUNKSIZE;

							//color
							colorchunks[k][j++] = color.x; colorchunks[k][j++] = color.y; colorchunks[k][j++] = color.z;
							colorchunks[k][j++] = color.x; colorchunks[k][j++] = color.y; colorchunks[k][j++] = color.z;
							colorchunks[k][j++] = color.x; colorchunks[k][j++] = color.y; colorchunks[k][j++] = color.z;
							colorchunks[k][j++] = color.x; colorchunks[k][j++] = color.y; colorchunks[k][j++] = color.z;
							colorchunks[k][j++] = color.x; colorchunks[k][j++] = color.y; colorchunks[k][j++] = color.z;
							colorchunks[k][j++] = color.x; colorchunks[k][j++] = color.y; colorchunks[k][j++] = color.z;
						}
					} else {
						//dont worry about filling the memchunk
						//top
						if(top)
						{
							memchunks[c][smartinc(&c, &i, memchunks)] = x+chunk->pos.x*CHUNKSIZE; memchunks[c][smartinc(&c, &i, memchunks)] = y+chunk->pos.y*CHUNKSIZE+1; memchunks[c][smartinc(&c, &i, memchunks)] = z+chunk->pos.z*CHUNKSIZE;
							memchunks[c][smartinc(&c, &i, memchunks)] = x+chunk->pos.x*CHUNKSIZE; memchunks[c][smartinc(&c, &i, memchunks)] = y+chunk->pos.y*CHUNKSIZE+1; memchunks[c][smartinc(&c, &i, memchunks)] = z+chunk->pos.z*CHUNKSIZE+1;
							memchunks[c][smartinc(&c, &i, memchunks)] = x+chunk->pos.x*CHUNKSIZE+1; memchunks[c][smartinc(&c, &i, memchunks)] = y+chunk->pos.y*CHUNKSIZE+1; memchunks[c][smartinc(&c, &i, memchunks)] = z+chunk->pos.z*CHUNKSIZE;

							memchunks[c][smartinc(&c, &i, memchunks)] = x+chunk->pos.x*CHUNKSIZE+1; memchunks[c][smartinc(&c, &i, memchunks)] = y+chunk->pos.y*CHUNKSIZE+1; memchunks[c][smartinc(&c, &i, memchunks)] = z+chunk->pos.z*CHUNKSIZE+1;
							memchunks[c][smartinc(&c, &i, memchunks)] = x+chunk->pos.x*CHUNKSIZE+1; memchunks[c][smartinc(&c, &i, memchunks)] = y+chunk->pos.y*CHUNKSIZE+1; memchunks[c][smartinc(&c, &i, memchunks)] = z+chunk->pos.z*CHUNKSIZE;
							memchunks[c][smartinc(&c, &i, memchunks)] = x+chunk->pos.x*CHUNKSIZE; memchunks[c][smartinc(&c, &i, memchunks)] = y+chunk->pos.y*CHUNKSIZE+1; memchunks[c][smartinc(&c, &i, memchunks)] = z+chunk->pos.z*CHUNKSIZE+1;

							//color
							colorchunks[k][smartinc(&k, &j, colorchunks)] = color.x; colorchunks[k][smartinc(&k, &j, colorchunks)] = color.y; colorchunks[k][smartinc(&k, &j, colorchunks)] = color.z;
							colorchunks[k][smartinc(&k, &j, colorchunks)] = color.x; colorchunks[k][smartinc(&k, &j, colorchunks)] = color.y; colorchunks[k][smartinc(&k, &j, colorchunks)] = color.z;
							colorchunks[k][smartinc(&k, &j, colorchunks)] = color.x; colorchunks[k][smartinc(&k, &j, colorchunks)] = color.y; colorchunks[k][smartinc(&k, &j, colorchunks)] = color.z;
							colorchunks[k][smartinc(&k, &j, colorchunks)] = color.x; colorchunks[k][smartinc(&k, &j, colorchunks)] = color.y; colorchunks[k][smartinc(&k, &j, colorchunks)] = color.z;
							colorchunks[k][smartinc(&k, &j, colorchunks)] = color.x; colorchunks[k][smartinc(&k, &j, colorchunks)] = color.y; colorchunks[k][smartinc(&k, &j, colorchunks)] = color.z;
							colorchunks[k][smartinc(&k, &j, colorchunks)] = color.x; colorchunks[k][smartinc(&k, &j, colorchunks)] = color.y; colorchunks[k][smartinc(&k, &j, colorchunks)] = color.z;
						}
						//bottom
						if(bottom)
						{
							memchunks[c][smartinc(&c, &i, memchunks)] = x+chunk->pos.x*CHUNKSIZE; memchunks[c][smartinc(&c, &i, memchunks)] = y+chunk->pos.y*CHUNKSIZE; memchunks[c][smartinc(&c, &i, memchunks)] = z+chunk->pos.z*CHUNKSIZE;
							memchunks[c][smartinc(&c, &i, memchunks)] = x+chunk->pos.x*CHUNKSIZE+1; memchunks[c][smartinc(&c, &i, memchunks)] = y+chunk->pos.y*CHUNKSIZE; memchunks[c][smartinc(&c, &i, memchunks)] = z+chunk->pos.z*CHUNKSIZE;
							memchunks[c][smartinc(&c, &i, memchunks)] = x+chunk->pos.x*CHUNKSIZE; memchunks[c][smartinc(&c, &i, memchunks)] = y+chunk->pos.y*CHUNKSIZE; memchunks[c][smartinc(&c, &i, memchunks)] = z+chunk->pos.z*CHUNKSIZE+1;

							memchunks[c][smartinc(&c, &i, memchunks)] = x+chunk->pos.x*CHUNKSIZE+1; memchunks[c][smartinc(&c, &i, memchunks)] = y+chunk->pos.y*CHUNKSIZE; memchunks[c][smartinc(&c, &i, memchunks)] = z+chunk->pos.z*CHUNKSIZE+1;
							memchunks[c][smartinc(&c, &i, memchunks)] = x+chunk->pos.x*CHUNKSIZE; memchunks[c][smartinc(&c, &i, memchunks)] = y+chunk->pos.y*CHUNKSIZE; memchunks[c][smartinc(&c, &i, memchunks)] = z+chunk->pos.z*CHUNKSIZE+1;
							memchunks[c][smartinc(&c, &i, memchunks)] = x+chunk->pos.x*CHUNKSIZE+1; memchunks[c][smartinc(&c, &i, memchunks)] = y+chunk->pos.y*CHUNKSIZE; memchunks[c][smartinc(&c, &i, memchunks)] = z+chunk->pos.z*CHUNKSIZE;

							//color
							colorchunks[k][smartinc(&k, &j, colorchunks)] = color.x; colorchunks[k][smartinc(&k, &j, colorchunks)] = color.y; colorchunks[k][smartinc(&k, &j, colorchunks)] = color.z;
							colorchunks[k][smartinc(&k, &j, colorchunks)] = color.x; colorchunks[k][smartinc(&k, &j, colorchunks)] = color.y; colorchunks[k][smartinc(&k, &j, colorchunks)] = color.z;
							colorchunks[k][smartinc(&k, &j, colorchunks)] = color.x; colorchunks[k][smartinc(&k, &j, colorchunks)] = color.y; colorchunks[k][smartinc(&k, &j, colorchunks)] = color.z;
							colorchunks[k][smartinc(&k, &j, colorchunks)] = color.x; colorchunks[k][smartinc(&k, &j, colorchunks)] = color.y; colorchunks[k][smartinc(&k, &j, colorchunks)] = color.z;
							colorchunks[k][smartinc(&k, &j, colorchunks)] = color.x; colorchunks[k][smartinc(&k, &j, colorchunks)] = color.y; colorchunks[k][smartinc(&k, &j, colorchunks)] = color.z;
							colorchunks[k][smartinc(&k, &j, colorchunks)] = color.x; colorchunks[k][smartinc(&k, &j, colorchunks)] = color.y; colorchunks[k][smartinc(&k, &j, colorchunks)] = color.z;
						}
						//north
						if(north)
						{
							memchunks[c][smartinc(&c, &i, memchunks)] = x+chunk->pos.x*CHUNKSIZE; memchunks[c][smartinc(&c, &i, memchunks)] = y+chunk->pos.y*CHUNKSIZE+1; memchunks[c][smartinc(&c, &i, memchunks)] = z+chunk->pos.z*CHUNKSIZE;
							memchunks[c][smartinc(&c, &i, memchunks)] = x+chunk->pos.x*CHUNKSIZE+1; memchunks[c][smartinc(&c, &i, memchunks)] = y+chunk->pos.y*CHUNKSIZE+1; memchunks[c][smartinc(&c, &i, memchunks)] = z+chunk->pos.z*CHUNKSIZE;
							memchunks[c][smartinc(&c, &i, memchunks)] = x+chunk->pos.x*CHUNKSIZE; memchunks[c][smartinc(&c, &i, memchunks)] = y+chunk->pos.y*CHUNKSIZE; memchunks[c][smartinc(&c, &i, memchunks)] = z+chunk->pos.z*CHUNKSIZE;

							memchunks[c][smartinc(&c, &i, memchunks)] = x+chunk->pos.x*CHUNKSIZE+1; memchunks[c][smartinc(&c, &i, memchunks)] = y+chunk->pos.y*CHUNKSIZE; memchunks[c][smartinc(&c, &i, memchunks)] = z+chunk->pos.z*CHUNKSIZE;
							memchunks[c][smartinc(&c, &i, memchunks)] = x+chunk->pos.x*CHUNKSIZE; memchunks[c][smartinc(&c, &i, memchunks)] = y+chunk->pos.y*CHUNKSIZE; memchunks[c][smartinc(&c, &i, memchunks)] = z+chunk->pos.z*CHUNKSIZE;
							memchunks[c][smartinc(&c, &i, memchunks)] = x+chunk->pos.x*CHUNKSIZE+1; memchunks[c][smartinc(&c, &i, memchunks)] = y+chunk->pos.y*CHUNKSIZE+1; memchunks[c][smartinc(&c, &i, memchunks)] = z+chunk->pos.z*CHUNKSIZE;

							//color
							colorchunks[k][smartinc(&k, &j, colorchunks)] = color.x; colorchunks[k][smartinc(&k, &j, colorchunks)] = color.y; colorchunks[k][smartinc(&k, &j, colorchunks)] = color.z;
							colorchunks[k][smartinc(&k, &j, colorchunks)] = color.x; colorchunks[k][smartinc(&k, &j, colorchunks)] = color.y; colorchunks[k][smartinc(&k, &j, colorchunks)] = color.z;
							colorchunks[k][smartinc(&k, &j, colorchunks)] = color.x; colorchunks[k][smartinc(&k, &j, colorchunks)] = color.y; colorchunks[k][smartinc(&k, &j, colorchunks)] = color.z;
							colorchunks[k][smartinc(&k, &j, colorchunks)] = color.x; colorchunks[k][smartinc(&k, &j, colorchunks)] = color.y; colorchunks[k][smartinc(&k, &j, colorchunks)] = color.z;
							colorchunks[k][smartinc(&k, &j, colorchunks)] = color.x; colorchunks[k][smartinc(&k, &j, colorchunks)] = color.y; colorchunks[k][smartinc(&k, &j, colorchunks)] = color.z;
							colorchunks[k][smartinc(&k, &j, colorchunks)] = color.x; colorchunks[k][smartinc(&k, &j, colorchunks)] = color.y; colorchunks[k][smartinc(&k, &j, colorchunks)] = color.z;
						}
						//south
						if(south)
						{
							memchunks[c][smartinc(&c, &i, memchunks)] = x+chunk->pos.x*CHUNKSIZE; memchunks[c][smartinc(&c, &i, memchunks)] = y+chunk->pos.y*CHUNKSIZE+1; memchunks[c][smartinc(&c, &i, memchunks)] = z+chunk->pos.z*CHUNKSIZE+1;
							memchunks[c][smartinc(&c, &i, memchunks)] = x+chunk->pos.x*CHUNKSIZE; memchunks[c][smartinc(&c, &i, memchunks)] = y+chunk->pos.y*CHUNKSIZE; memchunks[c][smartinc(&c, &i, memchunks)] = z+chunk->pos.z*CHUNKSIZE+1;
							memchunks[c][smartinc(&c, &i, memchunks)] = x+chunk->pos.x*CHUNKSIZE+1; memchunks[c][smartinc(&c, &i, memchunks)] = y+chunk->pos.y*CHUNKSIZE+1; memchunks[c][smartinc(&c, &i, memchunks)] = z+chunk->pos.z*CHUNKSIZE+1;

							memchunks[c][smartinc(&c, &i, memchunks)] = x+chunk->pos.x*CHUNKSIZE+1; memchunks[c][smartinc(&c, &i, memchunks)] = y+chunk->pos.y*CHUNKSIZE; memchunks[c][smartinc(&c, &i, memchunks)] = z+chunk->pos.z*CHUNKSIZE+1;
							memchunks[c][smartinc(&c, &i, memchunks)] = x+chunk->pos.x*CHUNKSIZE+1; memchunks[c][smartinc(&c, &i, memchunks)] = y+chunk->pos.y*CHUNKSIZE+1; memchunks[c][smartinc(&c, &i, memchunks)] = z+chunk->pos.z*CHUNKSIZE+1;
							memchunks[c][smartinc(&c, &i, memchunks)] = x+chunk->pos.x*CHUNKSIZE; memchunks[c][smartinc(&c, &i, memchunks)] = y+chunk->pos.y*CHUNKSIZE; memchunks[c][smartinc(&c, &i, memchunks)] = z+chunk->pos.z*CHUNKSIZE+1;

							//color
							colorchunks[k][smartinc(&k, &j, colorchunks)] = color.x; colorchunks[k][smartinc(&k, &j, colorchunks)] = color.y; colorchunks[k][smartinc(&k, &j, colorchunks)] = color.z;
							colorchunks[k][smartinc(&k, &j, colorchunks)] = color.x; colorchunks[k][smartinc(&k, &j, colorchunks)] = color.y; colorchunks[k][smartinc(&k, &j, colorchunks)] = color.z;
							colorchunks[k][smartinc(&k, &j, colorchunks)] = color.x; colorchunks[k][smartinc(&k, &j, colorchunks)] = color.y; colorchunks[k][smartinc(&k, &j, colorchunks)] = color.z;
							colorchunks[k][smartinc(&k, &j, colorchunks)] = color.x; colorchunks[k][smartinc(&k, &j, colorchunks)] = color.y; colorchunks[k][smartinc(&k, &j, colorchunks)] = color.z;
							colorchunks[k][smartinc(&k, &j, colorchunks)] = color.x; colorchunks[k][smartinc(&k, &j, colorchunks)] = color.y; colorchunks[k][smartinc(&k, &j, colorchunks)] = color.z;
							colorchunks[k][smartinc(&k, &j, colorchunks)] = color.x; colorchunks[k][smartinc(&k, &j, colorchunks)] = color.y; colorchunks[k][smartinc(&k, &j, colorchunks)] = color.z;
						}
						//east
						if(east)
						{
							memchunks[c][smartinc(&c, &i, memchunks)] = x+chunk->pos.x*CHUNKSIZE+1; memchunks[c][smartinc(&c, &i, memchunks)] = y+chunk->pos.y*CHUNKSIZE+1; memchunks[c][smartinc(&c, &i, memchunks)] = z+chunk->pos.z*CHUNKSIZE;
							memchunks[c][smartinc(&c, &i, memchunks)] = x+chunk->pos.x*CHUNKSIZE+1; memchunks[c][smartinc(&c, &i, memchunks)] = y+chunk->pos.y*CHUNKSIZE+1; memchunks[c][smartinc(&c, &i, memchunks)] = z+chunk->pos.z*CHUNKSIZE+1;
							memchunks[c][smartinc(&c, &i, memchunks)] = x+chunk->pos.x*CHUNKSIZE+1; memchunks[c][smartinc(&c, &i, memchunks)] = y+chunk->pos.y*CHUNKSIZE; memchunks[c][smartinc(&c, &i, memchunks)] = z+chunk->pos.z*CHUNKSIZE;

							memchunks[c][smartinc(&c, &i, memchunks)] = x+chunk->pos.x*CHUNKSIZE+1; memchunks[c][smartinc(&c, &i, memchunks)] = y+chunk->pos.y*CHUNKSIZE; memchunks[c][smartinc(&c, &i, memchunks)] = z+chunk->pos.z*CHUNKSIZE+1;
							memchunks[c][smartinc(&c, &i, memchunks)] = x+chunk->pos.x*CHUNKSIZE+1; memchunks[c][smartinc(&c, &i, memchunks)] = y+chunk->pos.y*CHUNKSIZE; memchunks[c][smartinc(&c, &i, memchunks)] = z+chunk->pos.z*CHUNKSIZE;
							memchunks[c][smartinc(&c, &i, memchunks)] = x+chunk->pos.x*CHUNKSIZE+1; memchunks[c][smartinc(&c, &i, memchunks)] = y+chunk->pos.y*CHUNKSIZE+1; memchunks[c][smartinc(&c, &i, memchunks)] = z+chunk->pos.z*CHUNKSIZE+1;

							//color
							colorchunks[k][smartinc(&k, &j, colorchunks)] = color.x; colorchunks[k][smartinc(&k, &j, colorchunks)] = color.y; colorchunks[k][smartinc(&k, &j, colorchunks)] = color.z;
							colorchunks[k][smartinc(&k, &j, colorchunks)] = color.x; colorchunks[k][smartinc(&k, &j, colorchunks)] = color.y; colorchunks[k][smartinc(&k, &j, colorchunks)] = color.z;
							colorchunks[k][smartinc(&k, &j, colorchunks)] = color.x; colorchunks[k][smartinc(&k, &j, colorchunks)] = color.y; colorchunks[k][smartinc(&k, &j, colorchunks)] = color.z;
							colorchunks[k][smartinc(&k, &j, colorchunks)] = color.x; colorchunks[k][smartinc(&k, &j, colorchunks)] = color.y; colorchunks[k][smartinc(&k, &j, colorchunks)] = color.z;
							colorchunks[k][smartinc(&k, &j, colorchunks)] = color.x; colorchunks[k][smartinc(&k, &j, colorchunks)] = color.y; colorchunks[k][smartinc(&k, &j, colorchunks)] = color.z;
							colorchunks[k][smartinc(&k, &j, colorchunks)] = color.x; colorchunks[k][smartinc(&k, &j, colorchunks)] = color.y; colorchunks[k][smartinc(&k, &j, colorchunks)] = color.z;
						}
						//west
						if(west)
						{
							memchunks[c][smartinc(&c, &i, memchunks)] = x+chunk->pos.x*CHUNKSIZE; memchunks[c][smartinc(&c, &i, memchunks)] = y+chunk->pos.y*CHUNKSIZE+1; memchunks[c][smartinc(&c, &i, memchunks)] = z+chunk->pos.z*CHUNKSIZE;
							memchunks[c][smartinc(&c, &i, memchunks)] = x+chunk->pos.x*CHUNKSIZE; memchunks[c][smartinc(&c, &i, memchunks)] = y+chunk->pos.y*CHUNKSIZE; memchunks[c][smartinc(&c, &i, memchunks)] = z+chunk->pos.z*CHUNKSIZE;
							memchunks[c][smartinc(&c, &i, memchunks)] = x+chunk->pos.x*CHUNKSIZE; memchunks[c][smartinc(&c, &i, memchunks)] = y+chunk->pos.y*CHUNKSIZE+1; memchunks[c][smartinc(&c, &i, memchunks)] = z+chunk->pos.z*CHUNKSIZE+1;

							memchunks[c][smartinc(&c, &i, memchunks)] = x+chunk->pos.x*CHUNKSIZE; memchunks[c][smartinc(&c, &i, memchunks)] = y+chunk->pos.y*CHUNKSIZE; memchunks[c][smartinc(&c, &i, memchunks)] = z+chunk->pos.z*CHUNKSIZE+1;
							memchunks[c][smartinc(&c, &i, memchunks)] = x+chunk->pos.x*CHUNKSIZE; memchunks[c][smartinc(&c, &i, memchunks)] = y+chunk->pos.y*CHUNKSIZE+1; memchunks[c][smartinc(&c, &i, memchunks)] = z+chunk->pos.z*CHUNKSIZE+1;
							memchunks[c][smartinc(&c, &i, memchunks)] = x+chunk->pos.x*CHUNKSIZE; memchunks[c][smartinc(&c, &i, memchunks)] = y+chunk->pos.y*CHUNKSIZE; memchunks[c][smartinc(&c, &i, memchunks)] = z+chunk->pos.z*CHUNKSIZE;

							//color
							colorchunks[k][smartinc(&k, &j, colorchunks)] = color.x; colorchunks[k][smartinc(&k, &j, colorchunks)] = color.y; colorchunks[k][smartinc(&k, &j, colorchunks)] = color.z;
							colorchunks[k][smartinc(&k, &j, colorchunks)] = color.x; colorchunks[k][smartinc(&k, &j, colorchunks)] = color.y; colorchunks[k][smartinc(&k, &j, colorchunks)] = color.z;
							colorchunks[k][smartinc(&k, &j, colorchunks)] = color.x; colorchunks[k][smartinc(&k, &j, colorchunks)] = color.y; colorchunks[k][smartinc(&k, &j, colorchunks)] = color.z;
							colorchunks[k][smartinc(&k, &j, colorchunks)] = color.x; colorchunks[k][smartinc(&k, &j, colorchunks)] = color.y; colorchunks[k][smartinc(&k, &j, colorchunks)] = color.z;
							colorchunks[k][smartinc(&k, &j, colorchunks)] = color.x; colorchunks[k][smartinc(&k, &j, colorchunks)] = color.y; colorchunks[k][smartinc(&k, &j, colorchunks)] = color.z;
							colorchunks[k][smartinc(&k, &j, colorchunks)] = color.x; colorchunks[k][smartinc(&k, &j, colorchunks)] = color.y; colorchunks[k][smartinc(&k, &j, colorchunks)] = color.z;
						}
					}
				}
			}
		}
	}

	GLfloat *finaldata = (GLfloat *)malloc(sizeof(GLfloat) * ((long)65536*c + i));

	int w;
	for(w=0; w<c; w++)
	{
		memcpy(&(finaldata[(long)w*65536]), memchunks[w], (long)65536 * sizeof(GLfloat));
		free(memchunks[w]);
	}

	memcpy(&finaldata[(long)c*65536], memchunks[c], (i) * sizeof(GLfloat));
	free(memchunks[c]);

	mesh_t ret;
	ret.data = finaldata;

	ret.size = (long)65536*c + i;

	//color
	GLfloat *finalcolordata = (GLfloat *)malloc(sizeof(GLfloat) * ((long)65536*k + j));

	for(w=0; w<k; w++)
	{
		memcpy(&(finalcolordata[(long)w*65536]), colorchunks[w], (long)65536 * sizeof(GLfloat));
		free(colorchunks[w]);
	}

	memcpy(&finalcolordata[(long)k*65536], colorchunks[k], j * sizeof(GLfloat));
	free(colorchunks[k]);

	ret.colordata = finalcolordata;

	ret.colorsize = (long)65536*k + j;

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
