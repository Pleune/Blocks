#include "chunk.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <GL/glew.h>

static inline uint16_t
smartinc(int *c, uint16_t *i, GLfloat **memchunks)
{
	if(*i==65535)
	{
		*i=0;
		*c += 1;
		memchunks[*c] = (GLfloat *)malloc(sizeof(GLfloat) * 65536);
		return 65535;
	} else {
		*i += 1;
		return *i-1;
	}
}

mesh_t
getmesh(chunk_t chunk, block_t *chunkabove, block_t *chunkbelow, block_t *chunknorth, block_t *chunksouth, block_t *chunkeast, block_t *chunkwest)
{
	GLfloat *memchunks[256];
	int c = 0;
	uint16_t i = 0;

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
					if(i<65428)
					{
						//dont worry about filling the memchunk
						//top
						memchunks[c][i++] = x; memchunks[c][i++] = y+1; memchunks[c][i++] = z;
						memchunks[c][i++] = x; memchunks[c][i++] = y+1; memchunks[c][i++] = z+1;
						memchunks[c][i++] = x+1; memchunks[c][i++] = y+1; memchunks[c][i++] = z;

						memchunks[c][i++] = x+1; memchunks[c][i++] = y+1; memchunks[c][i++] = z+1;
						memchunks[c][i++] = x+1; memchunks[c][i++] = y+1; memchunks[c][i++] = z;
						memchunks[c][i++] = x; memchunks[c][i++] = y+1; memchunks[c][i++] = z+1;
						//bottom
						memchunks[c][i++] = x; memchunks[c][i++] = y; memchunks[c][i++] = z;
						memchunks[c][i++] = x+1; memchunks[c][i++] = y; memchunks[c][i++] = z;
						memchunks[c][i++] = x; memchunks[c][i++] = y; memchunks[c][i++] = z+1;

						memchunks[c][i++] = x+1; memchunks[c][i++] = y; memchunks[c][i++] = z+1;
						memchunks[c][i++] = x; memchunks[c][i++] = y; memchunks[c][i++] = z+1;
						memchunks[c][i++] = x+1; memchunks[c][i++] = y; memchunks[c][i++] = z;
						//north
						memchunks[c][i++] = x; memchunks[c][i++] = y+1; memchunks[c][i++] = z;
						memchunks[c][i++] = x+1; memchunks[c][i++] = y+1; memchunks[c][i++] = z;
						memchunks[c][i++] = x; memchunks[c][i++] = y; memchunks[c][i++] = z;

						memchunks[c][i++] = x+1; memchunks[c][i++] = y; memchunks[c][i++] = z;
						memchunks[c][i++] = x; memchunks[c][i++] = y; memchunks[c][i++] = z;
						memchunks[c][i++] = x+1; memchunks[c][i++] = y+1; memchunks[c][i++] = z;
						//south
						memchunks[c][i++] = x; memchunks[c][i++] = y+1; memchunks[c][i++] = z+1;
						memchunks[c][i++] = x; memchunks[c][i++] = y; memchunks[c][i++] = z+1;
						memchunks[c][i++] = x+1; memchunks[c][i++] = y+1; memchunks[c][i++] = z+1;

						memchunks[c][i++] = x+1; memchunks[c][i++] = y; memchunks[c][i++] = z+1;
						memchunks[c][i++] = x+1; memchunks[c][i++] = y+1; memchunks[c][i++] = z+1;
						memchunks[c][i++] = x; memchunks[c][i++] = y; memchunks[c][i++] = z+1;
						//east
						memchunks[c][i++] = x+1; memchunks[c][i++] = y+1; memchunks[c][i++] = z;
						memchunks[c][i++] = x+1; memchunks[c][i++] = y+1; memchunks[c][i++] = z+1;
						memchunks[c][i++] = x+1; memchunks[c][i++] = y; memchunks[c][i++] = z;

						memchunks[c][i++] = x+1; memchunks[c][i++] = y; memchunks[c][i++] = z+1;
						memchunks[c][i++] = x+1; memchunks[c][i++] = y; memchunks[c][i++] = z;
						memchunks[c][i++] = x+1; memchunks[c][i++] = y+1; memchunks[c][i++] = z+1;
						//west
						memchunks[c][i++] = x; memchunks[c][i++] = y+1; memchunks[c][i++] = z;
						memchunks[c][i++] = x; memchunks[c][i++] = y; memchunks[c][i++] = z;
						memchunks[c][i++] = x; memchunks[c][i++] = y+1; memchunks[c][i++] = z+1;

						memchunks[c][i++] = x; memchunks[c][i++] = y; memchunks[c][i++] = z+1;
						memchunks[c][i++] = x; memchunks[c][i++] = y+1; memchunks[c][i++] = z+1;
						memchunks[c][i++] = x; memchunks[c][i++] = y; memchunks[c][i++] = z;
					} else {
						//dont worry about filling the memchunk
						//top
						memchunks[c][smartinc(&c, &i, memchunks)] = x; memchunks[c][smartinc(&c, &i, memchunks)] = y+1; memchunks[c][smartinc(&c, &i, memchunks)] = z;
						memchunks[c][smartinc(&c, &i, memchunks)] = x; memchunks[c][smartinc(&c, &i, memchunks)] = y+1; memchunks[c][smartinc(&c, &i, memchunks)] = z+1;
						memchunks[c][smartinc(&c, &i, memchunks)] = x+1; memchunks[c][smartinc(&c, &i, memchunks)] = y+1; memchunks[c][smartinc(&c, &i, memchunks)] = z;

						memchunks[c][smartinc(&c, &i, memchunks)] = x+1; memchunks[c][smartinc(&c, &i, memchunks)] = y+1; memchunks[c][smartinc(&c, &i, memchunks)] = z+1;
						memchunks[c][smartinc(&c, &i, memchunks)] = x+1; memchunks[c][smartinc(&c, &i, memchunks)] = y+1; memchunks[c][smartinc(&c, &i, memchunks)] = z;
						memchunks[c][smartinc(&c, &i, memchunks)] = x; memchunks[c][smartinc(&c, &i, memchunks)] = y+1; memchunks[c][smartinc(&c, &i, memchunks)] = z+1;
						//bottom
						memchunks[c][smartinc(&c, &i, memchunks)] = x; memchunks[c][smartinc(&c, &i, memchunks)] = y; memchunks[c][smartinc(&c, &i, memchunks)] = z;
						memchunks[c][smartinc(&c, &i, memchunks)] = x+1; memchunks[c][smartinc(&c, &i, memchunks)] = y; memchunks[c][smartinc(&c, &i, memchunks)] = z;
						memchunks[c][smartinc(&c, &i, memchunks)] = x; memchunks[c][smartinc(&c, &i, memchunks)] = y; memchunks[c][smartinc(&c, &i, memchunks)] = z+1;

						memchunks[c][smartinc(&c, &i, memchunks)] = x+1; memchunks[c][smartinc(&c, &i, memchunks)] = y; memchunks[c][smartinc(&c, &i, memchunks)] = z+1;
						memchunks[c][smartinc(&c, &i, memchunks)] = x; memchunks[c][smartinc(&c, &i, memchunks)] = y; memchunks[c][smartinc(&c, &i, memchunks)] = z+1;
						memchunks[c][smartinc(&c, &i, memchunks)] = x+1; memchunks[c][smartinc(&c, &i, memchunks)] = y; memchunks[c][smartinc(&c, &i, memchunks)] = z;
						//north
						memchunks[c][smartinc(&c, &i, memchunks)] = x; memchunks[c][smartinc(&c, &i, memchunks)] = y+1; memchunks[c][smartinc(&c, &i, memchunks)] = z;
						memchunks[c][smartinc(&c, &i, memchunks)] = x+1; memchunks[c][smartinc(&c, &i, memchunks)] = y+1; memchunks[c][smartinc(&c, &i, memchunks)] = z;
						memchunks[c][smartinc(&c, &i, memchunks)] = x; memchunks[c][smartinc(&c, &i, memchunks)] = y; memchunks[c][smartinc(&c, &i, memchunks)] = z;

						memchunks[c][smartinc(&c, &i, memchunks)] = x+1; memchunks[c][smartinc(&c, &i, memchunks)] = y; memchunks[c][smartinc(&c, &i, memchunks)] = z;
						memchunks[c][smartinc(&c, &i, memchunks)] = x; memchunks[c][smartinc(&c, &i, memchunks)] = y; memchunks[c][smartinc(&c, &i, memchunks)] = z;
						memchunks[c][smartinc(&c, &i, memchunks)] = x+1; memchunks[c][smartinc(&c, &i, memchunks)] = y+1; memchunks[c][smartinc(&c, &i, memchunks)] = z;
						//south
						memchunks[c][smartinc(&c, &i, memchunks)] = x; memchunks[c][smartinc(&c, &i, memchunks)] = y+1; memchunks[c][smartinc(&c, &i, memchunks)] = z+1;
						memchunks[c][smartinc(&c, &i, memchunks)] = x; memchunks[c][smartinc(&c, &i, memchunks)] = y; memchunks[c][smartinc(&c, &i, memchunks)] = z+1;
						memchunks[c][smartinc(&c, &i, memchunks)] = x+1; memchunks[c][smartinc(&c, &i, memchunks)] = y+1; memchunks[c][smartinc(&c, &i, memchunks)] = z+1;

						memchunks[c][smartinc(&c, &i, memchunks)] = x+1; memchunks[c][smartinc(&c, &i, memchunks)] = y; memchunks[c][smartinc(&c, &i, memchunks)] = z+1;
						memchunks[c][smartinc(&c, &i, memchunks)] = x+1; memchunks[c][smartinc(&c, &i, memchunks)] = y+1; memchunks[c][smartinc(&c, &i, memchunks)] = z+1;
						memchunks[c][smartinc(&c, &i, memchunks)] = x; memchunks[c][smartinc(&c, &i, memchunks)] = y; memchunks[c][smartinc(&c, &i, memchunks)] = z+1;
						//east
						memchunks[c][smartinc(&c, &i, memchunks)] = x+1; memchunks[c][smartinc(&c, &i, memchunks)] = y+1; memchunks[c][smartinc(&c, &i, memchunks)] = z;
						memchunks[c][smartinc(&c, &i, memchunks)] = x+1; memchunks[c][smartinc(&c, &i, memchunks)] = y+1; memchunks[c][smartinc(&c, &i, memchunks)] = z+1;
						memchunks[c][smartinc(&c, &i, memchunks)] = x+1; memchunks[c][smartinc(&c, &i, memchunks)] = y; memchunks[c][smartinc(&c, &i, memchunks)] = z;

						memchunks[c][smartinc(&c, &i, memchunks)] = x+1; memchunks[c][smartinc(&c, &i, memchunks)] = y; memchunks[c][smartinc(&c, &i, memchunks)] = z+1;
						memchunks[c][smartinc(&c, &i, memchunks)] = x+1; memchunks[c][smartinc(&c, &i, memchunks)] = y; memchunks[c][smartinc(&c, &i, memchunks)] = z;
						memchunks[c][smartinc(&c, &i, memchunks)] = x+1; memchunks[c][smartinc(&c, &i, memchunks)] = y+1; memchunks[c][smartinc(&c, &i, memchunks)] = z+1;
						//west
						memchunks[c][smartinc(&c, &i, memchunks)] = x; memchunks[c][smartinc(&c, &i, memchunks)] = y+1; memchunks[c][smartinc(&c, &i, memchunks)] = z;
						memchunks[c][smartinc(&c, &i, memchunks)] = x; memchunks[c][smartinc(&c, &i, memchunks)] = y; memchunks[c][smartinc(&c, &i, memchunks)] = z;
						memchunks[c][smartinc(&c, &i, memchunks)] = x; memchunks[c][smartinc(&c, &i, memchunks)] = y+1; memchunks[c][smartinc(&c, &i, memchunks)] = z+1;

						memchunks[c][smartinc(&c, &i, memchunks)] = x; memchunks[c][smartinc(&c, &i, memchunks)] = y; memchunks[c][smartinc(&c, &i, memchunks)] = z+1;
						memchunks[c][smartinc(&c, &i, memchunks)] = x; memchunks[c][smartinc(&c, &i, memchunks)] = y+1; memchunks[c][smartinc(&c, &i, memchunks)] = z+1;
						memchunks[c][smartinc(&c, &i, memchunks)] = x; memchunks[c][smartinc(&c, &i, memchunks)] = y; memchunks[c][smartinc(&c, &i, memchunks)] = z;
					}
				}
			}
		}
	}
	GLfloat *finaldata = (GLfloat *)malloc(sizeof(GLfloat) * (i + 65536*c));

	int w;
	for(w=0; w<c; w++)
	{
		memcpy(&finaldata[w*65536], memchunks[w], 65563 * sizeof(GLfloat));
		free(memchunks[w]);
	}

	memcpy(&finaldata[c*65536], memchunks[c], i * sizeof(GLfloat));

	mesh_t ret;
	ret.data = finaldata;

	ret.size = i + 65536*c;

	free(memchunks[c]);
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
