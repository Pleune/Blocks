#include "chunk.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <GL/glew.h>

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
chunk_getmesh(chunk_t chunk, block_t *chunkabove, block_t *chunkbelow, block_t *chunknorth, block_t *chunksouth, block_t *chunkeast, block_t *chunkwest)
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
				if(chunk.data[index].id)
				{
					int top, bottom, north, south, east, west;

					if(y==CHUNKSIZE-1)
					{
						top=1;
					} else {
						if(chunk.data[index+CHUNKSIZE].id)
							top = 0;
						else
							top = 1;
					}
					if(y==0)
					{
						bottom=1;
					} else {
						if(chunk.data[index-CHUNKSIZE].id)
							bottom = 0;
						else
							bottom = 1;
					}
					if(z==0)
					{
						north=1;
					} else {
						if(chunk.data[index-CHUNKSIZE*CHUNKSIZE].id)
							north = 0;
						else
							north = 1;
					}
					if(z==CHUNKSIZE-1)
					{
						south=1;
					} else {
						if(chunk.data[index+CHUNKSIZE*CHUNKSIZE].id)
							south = 0;
						else
							south = 1;
					}
					if(x==CHUNKSIZE-1)
					{
						east=1;
					} else {
						if(chunk.data[index+1].id)
							east = 0;
						else
							east = 1;
					}
					if(x==0)
					{
						west=1;
					} else {
						if(chunk.data[index-1].id)
							west = 0;
						else
							west = 1;
					}

					vec3_t color = block_getcolor(chunk.data[index].id);

					if(i<65400)
					{
						//dont worry about filling the memchunk
						//top
						if(top)
						{
							memchunks[c][i++] = x+chunk.pos[0]*CHUNKSIZE; memchunks[c][i++] = y+chunk.pos[1]*CHUNKSIZE+1; memchunks[c][i++] = z+chunk.pos[2]*CHUNKSIZE;
							memchunks[c][i++] = x+chunk.pos[0]*CHUNKSIZE; memchunks[c][i++] = y+chunk.pos[1]*CHUNKSIZE+1; memchunks[c][i++] = z+chunk.pos[2]*CHUNKSIZE+1;
							memchunks[c][i++] = x+chunk.pos[0]*CHUNKSIZE+1; memchunks[c][i++] = y+chunk.pos[1]*CHUNKSIZE+1; memchunks[c][i++] = z+chunk.pos[2]*CHUNKSIZE;

							memchunks[c][i++] = x+chunk.pos[0]*CHUNKSIZE+1; memchunks[c][i++] = y+chunk.pos[1]*CHUNKSIZE+1; memchunks[c][i++] = z+chunk.pos[2]*CHUNKSIZE+1;
							memchunks[c][i++] = x+chunk.pos[0]*CHUNKSIZE+1; memchunks[c][i++] = y+chunk.pos[1]*CHUNKSIZE+1; memchunks[c][i++] = z+chunk.pos[2]*CHUNKSIZE;
							memchunks[c][i++] = x+chunk.pos[0]*CHUNKSIZE; memchunks[c][i++] = y+chunk.pos[1]*CHUNKSIZE+1; memchunks[c][i++] = z+chunk.pos[2]*CHUNKSIZE+1;

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
							memchunks[c][i++] = x+chunk.pos[0]*CHUNKSIZE; memchunks[c][i++] = y+chunk.pos[1]*CHUNKSIZE; memchunks[c][i++] = z+chunk.pos[2]*CHUNKSIZE;
							memchunks[c][i++] = x+chunk.pos[0]*CHUNKSIZE+1; memchunks[c][i++] = y+chunk.pos[1]*CHUNKSIZE; memchunks[c][i++] = z+chunk.pos[2]*CHUNKSIZE;
							memchunks[c][i++] = x+chunk.pos[0]*CHUNKSIZE; memchunks[c][i++] = y+chunk.pos[1]*CHUNKSIZE; memchunks[c][i++] = z+chunk.pos[2]*CHUNKSIZE+1;

							memchunks[c][i++] = x+chunk.pos[0]*CHUNKSIZE+1; memchunks[c][i++] = y+chunk.pos[1]*CHUNKSIZE; memchunks[c][i++] = z+chunk.pos[2]*CHUNKSIZE+1;
							memchunks[c][i++] = x+chunk.pos[0]*CHUNKSIZE; memchunks[c][i++] = y+chunk.pos[1]*CHUNKSIZE; memchunks[c][i++] = z+chunk.pos[2]*CHUNKSIZE+1;
							memchunks[c][i++] = x+chunk.pos[0]*CHUNKSIZE+1; memchunks[c][i++] = y+chunk.pos[1]*CHUNKSIZE; memchunks[c][i++] = z+chunk.pos[2]*CHUNKSIZE;

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
							memchunks[c][i++] = x+chunk.pos[0]*CHUNKSIZE; memchunks[c][i++] = y+chunk.pos[1]*CHUNKSIZE+1; memchunks[c][i++] = z+chunk.pos[2]*CHUNKSIZE;
							memchunks[c][i++] = x+chunk.pos[0]*CHUNKSIZE+1; memchunks[c][i++] = y+chunk.pos[1]*CHUNKSIZE+1; memchunks[c][i++] = z+chunk.pos[2]*CHUNKSIZE;
							memchunks[c][i++] = x+chunk.pos[0]*CHUNKSIZE; memchunks[c][i++] = y+chunk.pos[1]*CHUNKSIZE; memchunks[c][i++] = z+chunk.pos[2]*CHUNKSIZE;

							memchunks[c][i++] = x+chunk.pos[0]*CHUNKSIZE+1; memchunks[c][i++] = y+chunk.pos[1]*CHUNKSIZE; memchunks[c][i++] = z+chunk.pos[2]*CHUNKSIZE;
							memchunks[c][i++] = x+chunk.pos[0]*CHUNKSIZE; memchunks[c][i++] = y+chunk.pos[1]*CHUNKSIZE; memchunks[c][i++] = z+chunk.pos[2]*CHUNKSIZE;
							memchunks[c][i++] = x+chunk.pos[0]*CHUNKSIZE+1; memchunks[c][i++] = y+chunk.pos[1]*CHUNKSIZE+1; memchunks[c][i++] = z+chunk.pos[2]*CHUNKSIZE;

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
							memchunks[c][i++] = x+chunk.pos[0]*CHUNKSIZE; memchunks[c][i++] = y+chunk.pos[1]*CHUNKSIZE+1; memchunks[c][i++] = z+chunk.pos[2]*CHUNKSIZE+1;
							memchunks[c][i++] = x+chunk.pos[0]*CHUNKSIZE; memchunks[c][i++] = y+chunk.pos[1]*CHUNKSIZE; memchunks[c][i++] = z+chunk.pos[2]*CHUNKSIZE+1;
							memchunks[c][i++] = x+chunk.pos[0]*CHUNKSIZE+1; memchunks[c][i++] = y+chunk.pos[1]*CHUNKSIZE+1; memchunks[c][i++] = z+chunk.pos[2]*CHUNKSIZE+1;

							memchunks[c][i++] = x+chunk.pos[0]*CHUNKSIZE+1; memchunks[c][i++] = y+chunk.pos[1]*CHUNKSIZE; memchunks[c][i++] = z+chunk.pos[2]*CHUNKSIZE+1;
							memchunks[c][i++] = x+chunk.pos[0]*CHUNKSIZE+1; memchunks[c][i++] = y+chunk.pos[1]*CHUNKSIZE+1; memchunks[c][i++] = z+chunk.pos[2]*CHUNKSIZE+1;
							memchunks[c][i++] = x+chunk.pos[0]*CHUNKSIZE; memchunks[c][i++] = y+chunk.pos[1]*CHUNKSIZE; memchunks[c][i++] = z+chunk.pos[2]*CHUNKSIZE+1;

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
							memchunks[c][i++] = x+chunk.pos[0]*CHUNKSIZE+1; memchunks[c][i++] = y+chunk.pos[1]*CHUNKSIZE+1; memchunks[c][i++] = z+chunk.pos[2]*CHUNKSIZE;
							memchunks[c][i++] = x+chunk.pos[0]*CHUNKSIZE+1; memchunks[c][i++] = y+chunk.pos[1]*CHUNKSIZE+1; memchunks[c][i++] = z+chunk.pos[2]*CHUNKSIZE+1;
							memchunks[c][i++] = x+chunk.pos[0]*CHUNKSIZE+1; memchunks[c][i++] = y+chunk.pos[1]*CHUNKSIZE; memchunks[c][i++] = z+chunk.pos[2]*CHUNKSIZE;

							memchunks[c][i++] = x+chunk.pos[0]*CHUNKSIZE+1; memchunks[c][i++] = y+chunk.pos[1]*CHUNKSIZE; memchunks[c][i++] = z+chunk.pos[2]*CHUNKSIZE+1;
							memchunks[c][i++] = x+chunk.pos[0]*CHUNKSIZE+1; memchunks[c][i++] = y+chunk.pos[1]*CHUNKSIZE; memchunks[c][i++] = z+chunk.pos[2]*CHUNKSIZE;
							memchunks[c][i++] = x+chunk.pos[0]*CHUNKSIZE+1; memchunks[c][i++] = y+chunk.pos[1]*CHUNKSIZE+1; memchunks[c][i++] = z+chunk.pos[2]*CHUNKSIZE+1;

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
							memchunks[c][i++] = x+chunk.pos[0]*CHUNKSIZE; memchunks[c][i++] = y+chunk.pos[1]*CHUNKSIZE+1; memchunks[c][i++] = z+chunk.pos[2]*CHUNKSIZE;
							memchunks[c][i++] = x+chunk.pos[0]*CHUNKSIZE; memchunks[c][i++] = y+chunk.pos[1]*CHUNKSIZE; memchunks[c][i++] = z+chunk.pos[2]*CHUNKSIZE;
							memchunks[c][i++] = x+chunk.pos[0]*CHUNKSIZE; memchunks[c][i++] = y+chunk.pos[1]*CHUNKSIZE+1; memchunks[c][i++] = z+chunk.pos[2]*CHUNKSIZE+1;

							memchunks[c][i++] = x+chunk.pos[0]*CHUNKSIZE; memchunks[c][i++] = y+chunk.pos[1]*CHUNKSIZE; memchunks[c][i++] = z+chunk.pos[2]*CHUNKSIZE+1;
							memchunks[c][i++] = x+chunk.pos[0]*CHUNKSIZE; memchunks[c][i++] = y+chunk.pos[1]*CHUNKSIZE+1; memchunks[c][i++] = z+chunk.pos[2]*CHUNKSIZE+1;
							memchunks[c][i++] = x+chunk.pos[0]*CHUNKSIZE; memchunks[c][i++] = y+chunk.pos[1]*CHUNKSIZE; memchunks[c][i++] = z+chunk.pos[2]*CHUNKSIZE;

							//color
							colorchunks[k][j++] = color.x; colorchunks[k][j++] = color.y; colorchunks[k][j++] = color.z;
							colorchunks[k][j++] = color.x; colorchunks[k][j++] = color.y; colorchunks[k][j++] = color.z;
							colorchunks[k][j++] = color.x; colorchunks[k][j++] = color.y; colorchunks[k][j++] = color.z;
							colorchunks[k][j++] = color.x; colorchunks[k][j++] = color.y; colorchunks[k][j++] = color.z;
							colorchunks[k][j++] = color.x; colorchunks[k][j++] = color.y; colorchunks[k][j++] = color.z;
							colorchunks[k][j++] = color.x; colorchunks[k][j++] = color.y; colorchunks[k][j++] = color.z;
						}
					} else {
					}
				}
			}
		}
	}

	GLfloat *finaldata = (GLfloat *)malloc(sizeof(GLfloat) * (65536*c + i));

	int w;
	for(w=0; w<c; w++)
	{
		memcpy(&(finaldata[w*65536]), memchunks[w], 65536 * sizeof(GLfloat));
		free(memchunks[w]);
	}

	memcpy(&finaldata[c*65536], memchunks[c], (i) * sizeof(GLfloat));
	free(memchunks[c]);

	mesh_t ret;
	ret.data = finaldata;

	ret.size = 65536*c + i;

	//color
	GLfloat *finalcolordata = (GLfloat *)malloc(sizeof(GLfloat) * (65536*k + j));

	for(w=0; w<k; w++)
	{
		memcpy(&(finalcolordata[w*65536]), colorchunks[w], 65536 * sizeof(GLfloat));
		free(colorchunks[w]);
	}

	memcpy(&finalcolordata[k*65536], colorchunks[k], j * sizeof(GLfloat));
	free(colorchunks[k]);

	ret.colordata = finalcolordata;

	ret.colorsize = 65536*k + j;

	return ret;
}

chunk_t
mallocchunk()
{
	chunk_t ret;

	ret.data = (block_t *)malloc(sizeof(block_t) * CHUNKSIZE*CHUNKSIZE*CHUNKSIZE);
	return ret;
}

chunk_t
callocchunk()
{
	chunk_t ret;

	ret.data = (block_t *)calloc(CHUNKSIZE*CHUNKSIZE*CHUNKSIZE, sizeof(block_t));
	return ret;
}

void
zerochunk(chunk_t chunk)
{
	memset(chunk.data, 0, CHUNKSIZE*CHUNKSIZE*CHUNKSIZE);
}
