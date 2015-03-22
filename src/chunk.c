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

	memchunks[0] = (GLfloat *)malloc(sizeof(GLfloat) * 65536);

	int x, y, z;
	for(x=0; x<CHUNKSIZE; x++)
	{
		for(y=0; y<CHUNKSIZE; y++)
		{
			for(z=0; z<CHUNKSIZE; z++)
			{
				if(chunk.data[x+CHUNKSIZE*y+CHUNKSIZE*CHUNKSIZE*z].id)
				{
					if(0)//i<65427)
					{
						//dont worry about filling the memchunk
						//top
						memchunks[c][i++] = x+chunk.pos[0]*CHUNKSIZE; memchunks[c][i++] = y+chunk.pos[1]*CHUNKSIZE+1; memchunks[c][i++] = z+chunk.pos[2]*CHUNKSIZE;
						memchunks[c][i++] = x+chunk.pos[0]*CHUNKSIZE; memchunks[c][i++] = y+chunk.pos[1]*CHUNKSIZE+1; memchunks[c][i++] = z+chunk.pos[2]*CHUNKSIZE+1;
						memchunks[c][i++] = x+chunk.pos[0]*CHUNKSIZE+1; memchunks[c][i++] = y+chunk.pos[1]*CHUNKSIZE+1; memchunks[c][i++] = z+chunk.pos[2]*CHUNKSIZE;

						memchunks[c][i++] = x+chunk.pos[0]*CHUNKSIZE+1; memchunks[c][i++] = y+chunk.pos[1]*CHUNKSIZE+1; memchunks[c][i++] = z+chunk.pos[2]*CHUNKSIZE+1;
						memchunks[c][i++] = x+chunk.pos[0]*CHUNKSIZE+1; memchunks[c][i++] = y+chunk.pos[1]*CHUNKSIZE+1; memchunks[c][i++] = z+chunk.pos[2]*CHUNKSIZE;
						memchunks[c][i++] = x+chunk.pos[0]*CHUNKSIZE; memchunks[c][i++] = y+chunk.pos[1]*CHUNKSIZE+1; memchunks[c][i++] = z+chunk.pos[2]*CHUNKSIZE+1;
						//bottom
						memchunks[c][i++] = x+chunk.pos[0]*CHUNKSIZE; memchunks[c][i++] = y+chunk.pos[1]*CHUNKSIZE; memchunks[c][i++] = z+chunk.pos[2]*CHUNKSIZE;
						memchunks[c][i++] = x+chunk.pos[0]*CHUNKSIZE+1; memchunks[c][i++] = y+chunk.pos[1]*CHUNKSIZE; memchunks[c][i++] = z+chunk.pos[2]*CHUNKSIZE;
						memchunks[c][i++] = x+chunk.pos[0]*CHUNKSIZE; memchunks[c][i++] = y+chunk.pos[1]*CHUNKSIZE; memchunks[c][i++] = z+chunk.pos[2]*CHUNKSIZE+1;

						memchunks[c][i++] = x+chunk.pos[0]*CHUNKSIZE+1; memchunks[c][i++] = y+chunk.pos[1]*CHUNKSIZE; memchunks[c][i++] = z+chunk.pos[2]*CHUNKSIZE+1;
						memchunks[c][i++] = x+chunk.pos[0]*CHUNKSIZE; memchunks[c][i++] = y+chunk.pos[1]*CHUNKSIZE; memchunks[c][i++] = z+chunk.pos[2]*CHUNKSIZE+1;
						memchunks[c][i++] = x+chunk.pos[0]*CHUNKSIZE+1; memchunks[c][i++] = y+chunk.pos[1]*CHUNKSIZE; memchunks[c][i++] = z+chunk.pos[2]*CHUNKSIZE;
						//north
						memchunks[c][i++] = x+chunk.pos[0]*CHUNKSIZE; memchunks[c][i++] = y+chunk.pos[1]*CHUNKSIZE+1; memchunks[c][i++] = z+chunk.pos[2]*CHUNKSIZE;
						memchunks[c][i++] = x+chunk.pos[0]*CHUNKSIZE+1; memchunks[c][i++] = y+chunk.pos[1]*CHUNKSIZE+1; memchunks[c][i++] = z+chunk.pos[2]*CHUNKSIZE;
						memchunks[c][i++] = x+chunk.pos[0]*CHUNKSIZE; memchunks[c][i++] = y+chunk.pos[1]*CHUNKSIZE; memchunks[c][i++] = z+chunk.pos[2]*CHUNKSIZE;

						memchunks[c][i++] = x+chunk.pos[0]*CHUNKSIZE+1; memchunks[c][i++] = y+chunk.pos[1]*CHUNKSIZE; memchunks[c][i++] = z+chunk.pos[2]*CHUNKSIZE;
						memchunks[c][i++] = x+chunk.pos[0]*CHUNKSIZE; memchunks[c][i++] = y+chunk.pos[1]*CHUNKSIZE; memchunks[c][i++] = z+chunk.pos[2]*CHUNKSIZE;
						memchunks[c][i++] = x+chunk.pos[0]*CHUNKSIZE+1; memchunks[c][i++] = y+chunk.pos[1]*CHUNKSIZE+1; memchunks[c][i++] = z+chunk.pos[2]*CHUNKSIZE;
						//south
						memchunks[c][i++] = x+chunk.pos[0]*CHUNKSIZE; memchunks[c][i++] = y+chunk.pos[1]*CHUNKSIZE+1; memchunks[c][i++] = z+chunk.pos[2]*CHUNKSIZE+1;
						memchunks[c][i++] = x+chunk.pos[0]*CHUNKSIZE; memchunks[c][i++] = y+chunk.pos[1]*CHUNKSIZE; memchunks[c][i++] = z+chunk.pos[2]*CHUNKSIZE+1;
						memchunks[c][i++] = x+chunk.pos[0]*CHUNKSIZE+1; memchunks[c][i++] = y+chunk.pos[1]*CHUNKSIZE+1; memchunks[c][i++] = z+chunk.pos[2]*CHUNKSIZE+1;

						memchunks[c][i++] = x+chunk.pos[0]*CHUNKSIZE+1; memchunks[c][i++] = y+chunk.pos[1]*CHUNKSIZE; memchunks[c][i++] = z+chunk.pos[2]*CHUNKSIZE+1;
						memchunks[c][i++] = x+chunk.pos[0]*CHUNKSIZE+1; memchunks[c][i++] = y+chunk.pos[1]*CHUNKSIZE+1; memchunks[c][i++] = z+chunk.pos[2]*CHUNKSIZE+1;
						memchunks[c][i++] = x+chunk.pos[0]*CHUNKSIZE; memchunks[c][i++] = y+chunk.pos[1]*CHUNKSIZE; memchunks[c][i++] = z+chunk.pos[2]*CHUNKSIZE+1;
						//east
						memchunks[c][i++] = x+chunk.pos[0]*CHUNKSIZE+1; memchunks[c][i++] = y+chunk.pos[1]*CHUNKSIZE+1; memchunks[c][i++] = z+chunk.pos[2]*CHUNKSIZE;
						memchunks[c][i++] = x+chunk.pos[0]*CHUNKSIZE+1; memchunks[c][i++] = y+chunk.pos[1]*CHUNKSIZE+1; memchunks[c][i++] = z+chunk.pos[2]*CHUNKSIZE+1;
						memchunks[c][i++] = x+chunk.pos[0]*CHUNKSIZE+1; memchunks[c][i++] = y+chunk.pos[1]*CHUNKSIZE; memchunks[c][i++] = z+chunk.pos[2]*CHUNKSIZE;

						memchunks[c][i++] = x+chunk.pos[0]*CHUNKSIZE+1; memchunks[c][i++] = y+chunk.pos[1]*CHUNKSIZE; memchunks[c][i++] = z+chunk.pos[2]*CHUNKSIZE+1;
						memchunks[c][i++] = x+chunk.pos[0]*CHUNKSIZE+1; memchunks[c][i++] = y+chunk.pos[1]*CHUNKSIZE; memchunks[c][i++] = z+chunk.pos[2]*CHUNKSIZE;
						memchunks[c][i++] = x+chunk.pos[0]*CHUNKSIZE+1; memchunks[c][i++] = y+chunk.pos[1]*CHUNKSIZE+1; memchunks[c][i++] = z+chunk.pos[2]*CHUNKSIZE+1;
						//west
						memchunks[c][i++] = x+chunk.pos[0]*CHUNKSIZE; memchunks[c][i++] = y+chunk.pos[1]*CHUNKSIZE+1; memchunks[c][i++] = z+chunk.pos[2]*CHUNKSIZE;
						memchunks[c][i++] = x+chunk.pos[0]*CHUNKSIZE; memchunks[c][i++] = y+chunk.pos[1]*CHUNKSIZE; memchunks[c][i++] = z+chunk.pos[2]*CHUNKSIZE;
						memchunks[c][i++] = x+chunk.pos[0]*CHUNKSIZE; memchunks[c][i++] = y+chunk.pos[1]*CHUNKSIZE+1; memchunks[c][i++] = z+chunk.pos[2]*CHUNKSIZE+1;

						memchunks[c][i++] = x+chunk.pos[0]*CHUNKSIZE; memchunks[c][i++] = y+chunk.pos[1]*CHUNKSIZE; memchunks[c][i++] = z+chunk.pos[2]*CHUNKSIZE+1;
						memchunks[c][i++] = x+chunk.pos[0]*CHUNKSIZE; memchunks[c][i++] = y+chunk.pos[1]*CHUNKSIZE+1; memchunks[c][i++] = z+chunk.pos[2]*CHUNKSIZE+1;
						memchunks[c][i++] = x+chunk.pos[0]*CHUNKSIZE; memchunks[c][i++] = y+chunk.pos[1]*CHUNKSIZE; memchunks[c][i++] = z+chunk.pos[2]*CHUNKSIZE;
					} else {
						//dont worry+chunk.pos[0]*CHUNKSIZE about filling the memchunk
						//top
						memchunks[c][smartinc(&c, &i, memchunks)] = x+chunk.pos[0]*CHUNKSIZE; memchunks[c][smartinc(&c, &i, memchunks)] = y+chunk.pos[1]*CHUNKSIZE+1; memchunks[c][smartinc(&c, &i, memchunks)] = z+chunk.pos[2]*CHUNKSIZE;
						memchunks[c][smartinc(&c, &i, memchunks)] = x+chunk.pos[0]*CHUNKSIZE; memchunks[c][smartinc(&c, &i, memchunks)] = y+chunk.pos[1]*CHUNKSIZE+1; memchunks[c][smartinc(&c, &i, memchunks)] = z+chunk.pos[2]*CHUNKSIZE+1;
						memchunks[c][smartinc(&c, &i, memchunks)] = x+chunk.pos[0]*CHUNKSIZE+1; memchunks[c][smartinc(&c, &i, memchunks)] = y+chunk.pos[1]*CHUNKSIZE+1; memchunks[c][smartinc(&c, &i, memchunks)] = z+chunk.pos[2]*CHUNKSIZE;

						memchunks[c][smartinc(&c, &i, memchunks)] = x+chunk.pos[0]*CHUNKSIZE+1; memchunks[c][smartinc(&c, &i, memchunks)] = y+chunk.pos[1]*CHUNKSIZE+1; memchunks[c][smartinc(&c, &i, memchunks)] = z+chunk.pos[2]*CHUNKSIZE+1;
						memchunks[c][smartinc(&c, &i, memchunks)] = x+chunk.pos[0]*CHUNKSIZE+1; memchunks[c][smartinc(&c, &i, memchunks)] = y+chunk.pos[1]*CHUNKSIZE+1; memchunks[c][smartinc(&c, &i, memchunks)] = z+chunk.pos[2]*CHUNKSIZE;
						memchunks[c][smartinc(&c, &i, memchunks)] = x+chunk.pos[0]*CHUNKSIZE; memchunks[c][smartinc(&c, &i, memchunks)] = y+chunk.pos[1]*CHUNKSIZE+1; memchunks[c][smartinc(&c, &i, memchunks)] = z+chunk.pos[2]*CHUNKSIZE+1;
						//bottom
						memchunks[c][smartinc(&c, &i, memchunks)] = x+chunk.pos[0]*CHUNKSIZE; memchunks[c][smartinc(&c, &i, memchunks)] = y+chunk.pos[1]*CHUNKSIZE; memchunks[c][smartinc(&c, &i, memchunks)] = z+chunk.pos[2]*CHUNKSIZE;
						memchunks[c][smartinc(&c, &i, memchunks)] = x+chunk.pos[0]*CHUNKSIZE+1; memchunks[c][smartinc(&c, &i, memchunks)] = y+chunk.pos[1]*CHUNKSIZE; memchunks[c][smartinc(&c, &i, memchunks)] = z+chunk.pos[2]*CHUNKSIZE;
						memchunks[c][smartinc(&c, &i, memchunks)] = x+chunk.pos[0]*CHUNKSIZE; memchunks[c][smartinc(&c, &i, memchunks)] = y+chunk.pos[1]*CHUNKSIZE; memchunks[c][smartinc(&c, &i, memchunks)] = z+chunk.pos[2]*CHUNKSIZE+1;

						memchunks[c][smartinc(&c, &i, memchunks)] = x+chunk.pos[0]*CHUNKSIZE+1; memchunks[c][smartinc(&c, &i, memchunks)] = y+chunk.pos[1]*CHUNKSIZE; memchunks[c][smartinc(&c, &i, memchunks)] = z+chunk.pos[2]*CHUNKSIZE+1;
						memchunks[c][smartinc(&c, &i, memchunks)] = x+chunk.pos[0]*CHUNKSIZE; memchunks[c][smartinc(&c, &i, memchunks)] = y+chunk.pos[1]*CHUNKSIZE; memchunks[c][smartinc(&c, &i, memchunks)] = z+chunk.pos[2]*CHUNKSIZE+1;
						memchunks[c][smartinc(&c, &i, memchunks)] = x+chunk.pos[0]*CHUNKSIZE+1; memchunks[c][smartinc(&c, &i, memchunks)] = y+chunk.pos[1]*CHUNKSIZE; memchunks[c][smartinc(&c, &i, memchunks)] = z+chunk.pos[2]*CHUNKSIZE;
						//north
						memchunks[c][smartinc(&c, &i, memchunks)] = x+chunk.pos[0]*CHUNKSIZE; memchunks[c][smartinc(&c, &i, memchunks)] = y+chunk.pos[1]*CHUNKSIZE+1; memchunks[c][smartinc(&c, &i, memchunks)] = z+chunk.pos[2]*CHUNKSIZE;
						memchunks[c][smartinc(&c, &i, memchunks)] = x+chunk.pos[0]*CHUNKSIZE+1; memchunks[c][smartinc(&c, &i, memchunks)] = y+chunk.pos[1]*CHUNKSIZE+1; memchunks[c][smartinc(&c, &i, memchunks)] = z+chunk.pos[2]*CHUNKSIZE;
						memchunks[c][smartinc(&c, &i, memchunks)] = x+chunk.pos[0]*CHUNKSIZE; memchunks[c][smartinc(&c, &i, memchunks)] = y+chunk.pos[1]*CHUNKSIZE; memchunks[c][smartinc(&c, &i, memchunks)] = z+chunk.pos[2]*CHUNKSIZE;

						memchunks[c][smartinc(&c, &i, memchunks)] = x+chunk.pos[0]*CHUNKSIZE+1; memchunks[c][smartinc(&c, &i, memchunks)] = y+chunk.pos[1]*CHUNKSIZE; memchunks[c][smartinc(&c, &i, memchunks)] = z+chunk.pos[2]*CHUNKSIZE;
						memchunks[c][smartinc(&c, &i, memchunks)] = x+chunk.pos[0]*CHUNKSIZE; memchunks[c][smartinc(&c, &i, memchunks)] = y+chunk.pos[1]*CHUNKSIZE; memchunks[c][smartinc(&c, &i, memchunks)] = z+chunk.pos[2]*CHUNKSIZE;
						memchunks[c][smartinc(&c, &i, memchunks)] = x+chunk.pos[0]*CHUNKSIZE+1; memchunks[c][smartinc(&c, &i, memchunks)] = y+chunk.pos[1]*CHUNKSIZE+1; memchunks[c][smartinc(&c, &i, memchunks)] = z+chunk.pos[2]*CHUNKSIZE;
						//south
						memchunks[c][smartinc(&c, &i, memchunks)] = x+chunk.pos[0]*CHUNKSIZE; memchunks[c][smartinc(&c, &i, memchunks)] = y+chunk.pos[1]*CHUNKSIZE+1; memchunks[c][smartinc(&c, &i, memchunks)] = z+chunk.pos[2]*CHUNKSIZE+1;
						memchunks[c][smartinc(&c, &i, memchunks)] = x+chunk.pos[0]*CHUNKSIZE; memchunks[c][smartinc(&c, &i, memchunks)] = y+chunk.pos[1]*CHUNKSIZE; memchunks[c][smartinc(&c, &i, memchunks)] = z+chunk.pos[2]*CHUNKSIZE+1;
						memchunks[c][smartinc(&c, &i, memchunks)] = x+chunk.pos[0]*CHUNKSIZE+1; memchunks[c][smartinc(&c, &i, memchunks)] = y+chunk.pos[1]*CHUNKSIZE+1; memchunks[c][smartinc(&c, &i, memchunks)] = z+chunk.pos[2]*CHUNKSIZE+1;

						memchunks[c][smartinc(&c, &i, memchunks)] = x+chunk.pos[0]*CHUNKSIZE+1; memchunks[c][smartinc(&c, &i, memchunks)] = y+chunk.pos[1]*CHUNKSIZE; memchunks[c][smartinc(&c, &i, memchunks)] = z+chunk.pos[2]*CHUNKSIZE+1;
						memchunks[c][smartinc(&c, &i, memchunks)] = x+chunk.pos[0]*CHUNKSIZE+1; memchunks[c][smartinc(&c, &i, memchunks)] = y+chunk.pos[1]*CHUNKSIZE+1; memchunks[c][smartinc(&c, &i, memchunks)] = z+chunk.pos[2]*CHUNKSIZE+1;
						memchunks[c][smartinc(&c, &i, memchunks)] = x+chunk.pos[0]*CHUNKSIZE; memchunks[c][smartinc(&c, &i, memchunks)] = y+chunk.pos[1]*CHUNKSIZE; memchunks[c][smartinc(&c, &i, memchunks)] = z+chunk.pos[2]*CHUNKSIZE+1;
						//east
						memchunks[c][smartinc(&c, &i, memchunks)] = x+chunk.pos[0]*CHUNKSIZE+1; memchunks[c][smartinc(&c, &i, memchunks)] = y+chunk.pos[1]*CHUNKSIZE+1; memchunks[c][smartinc(&c, &i, memchunks)] = z+chunk.pos[2]*CHUNKSIZE;
						memchunks[c][smartinc(&c, &i, memchunks)] = x+chunk.pos[0]*CHUNKSIZE+1; memchunks[c][smartinc(&c, &i, memchunks)] = y+chunk.pos[1]*CHUNKSIZE+1; memchunks[c][smartinc(&c, &i, memchunks)] = z+chunk.pos[2]*CHUNKSIZE+1;
						memchunks[c][smartinc(&c, &i, memchunks)] = x+chunk.pos[0]*CHUNKSIZE+1; memchunks[c][smartinc(&c, &i, memchunks)] = y+chunk.pos[1]*CHUNKSIZE; memchunks[c][smartinc(&c, &i, memchunks)] = z+chunk.pos[2]*CHUNKSIZE;

						memchunks[c][smartinc(&c, &i, memchunks)] = x+chunk.pos[0]*CHUNKSIZE+1; memchunks[c][smartinc(&c, &i, memchunks)] = y+chunk.pos[1]*CHUNKSIZE; memchunks[c][smartinc(&c, &i, memchunks)] = z+chunk.pos[2]*CHUNKSIZE+1;
						memchunks[c][smartinc(&c, &i, memchunks)] = x+chunk.pos[0]*CHUNKSIZE+1; memchunks[c][smartinc(&c, &i, memchunks)] = y+chunk.pos[1]*CHUNKSIZE; memchunks[c][smartinc(&c, &i, memchunks)] = z+chunk.pos[2]*CHUNKSIZE;
						memchunks[c][smartinc(&c, &i, memchunks)] = x+chunk.pos[0]*CHUNKSIZE+1; memchunks[c][smartinc(&c, &i, memchunks)] = y+chunk.pos[1]*CHUNKSIZE+1; memchunks[c][smartinc(&c, &i, memchunks)] = z+chunk.pos[2]*CHUNKSIZE+1;
						//west
						memchunks[c][smartinc(&c, &i, memchunks)] = x+chunk.pos[0]*CHUNKSIZE; memchunks[c][smartinc(&c, &i, memchunks)] = y+chunk.pos[1]*CHUNKSIZE+1; memchunks[c][smartinc(&c, &i, memchunks)] = z+chunk.pos[2]*CHUNKSIZE;
						memchunks[c][smartinc(&c, &i, memchunks)] = x+chunk.pos[0]*CHUNKSIZE; memchunks[c][smartinc(&c, &i, memchunks)] = y+chunk.pos[1]*CHUNKSIZE; memchunks[c][smartinc(&c, &i, memchunks)] = z+chunk.pos[2]*CHUNKSIZE;
						memchunks[c][smartinc(&c, &i, memchunks)] = x+chunk.pos[0]*CHUNKSIZE; memchunks[c][smartinc(&c, &i, memchunks)] = y+chunk.pos[1]*CHUNKSIZE+1; memchunks[c][smartinc(&c, &i, memchunks)] = z+chunk.pos[2]*CHUNKSIZE+1;

						memchunks[c][smartinc(&c, &i, memchunks)] = x+chunk.pos[0]*CHUNKSIZE; memchunks[c][smartinc(&c, &i, memchunks)] = y+chunk.pos[1]*CHUNKSIZE; memchunks[c][smartinc(&c, &i, memchunks)] = z+chunk.pos[2]*CHUNKSIZE+1;
						memchunks[c][smartinc(&c, &i, memchunks)] = x+chunk.pos[0]*CHUNKSIZE; memchunks[c][smartinc(&c, &i, memchunks)] = y+chunk.pos[1]*CHUNKSIZE+1; memchunks[c][smartinc(&c, &i, memchunks)] = z+chunk.pos[2]*CHUNKSIZE+1;
						memchunks[c][smartinc(&c, &i, memchunks)] = x+chunk.pos[0]*CHUNKSIZE; memchunks[c][smartinc(&c, &i, memchunks)] = y+chunk.pos[1]*CHUNKSIZE; memchunks[c][smartinc(&c, &i, memchunks)] = z+chunk.pos[2]*CHUNKSIZE;
					}
				}
			}
		}
	}

	GLfloat *finaldata = (GLfloat *)malloc(sizeof(GLfloat) * (65536*c + i));

	int w;
	for(w=0; w<(c); w++)
	{
		memcpy(&(finaldata[w*65536]), memchunks[w], 65536 * sizeof(GLfloat));
		free(memchunks[w]);
	}

	memcpy(&finaldata[c*65536], memchunks[c], (i) * sizeof(GLfloat));
	free(memchunks[c]);

	mesh_t ret;
	ret.data = finaldata;

	ret.size = 65536*c + i;

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
