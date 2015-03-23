#include "world.h"

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include <GL/glew.h>

#include "defines.h"
#include "chunk.h"

chunk_t loadedchunks[WORLDSIZE * WORLDSIZE * WORLDSIZE];
int3_t centerpos = {WORLDSIZE/2, WORLDSIZE/2, (WORLDSIZE)/2};

struct {
	GLuint vbo;
	int iscurrent;
	long points;
} blockvbos[WORLDSIZE][WORLDSIZE][WORLDSIZE];

static inline int
getspotof(int3_t pos)
{
	return (pos.x%WORLDSIZE) + (pos.y%WORLDSIZE)*WORLDSIZE + (pos.z%WORLDSIZE)*WORLDSIZE*WORLDSIZE;
}

static inline int3_t
getckunkof(int3_t blockpos)
{
	blockpos.x = floor((double)blockpos.x / CHUNKSIZE);
	blockpos.y = floor((double)blockpos.y / CHUNKSIZE);
	blockpos.z = floor((double)blockpos.z / CHUNKSIZE);
	return blockpos;
}

static inline int
isquickloaded(int3_t pos)
{
	return (centerpos.x - WORLDSIZE/2 <= pos.x && pos.x < centerpos.x + WORLDSIZE/2) && (centerpos.y - WORLDSIZE/2 <= pos.y && pos.y < centerpos.y + WORLDSIZE/2) && (centerpos.z - WORLDSIZE/2 <= pos.z && pos.z < centerpos.z + WORLDSIZE/2);
}

void
world_initalload()
{
	int x, y, z;
	for(x=0; x<WORLDSIZE; x++)
	{
		for(y=0; y<WORLDSIZE; y++)
		{
			for(z=0; z<WORLDSIZE; z++)
			{
				glGenBuffers(1, &blockvbos[x][y][z].vbo);
				glBindBuffer(GL_ARRAY_BUFFER, blockvbos[x][y][z].vbo);

				blockvbos[x][y][z].points = 0;
				blockvbos[x][y][z].iscurrent = 0;

				int3_t pos;
				pos.x = x;
				pos.y = y;
				pos.z = z;
				int spot = getspotof(pos);
				loadedchunks[spot] = callocchunk();
				loadedchunks[spot].pos[0] = x;
				loadedchunks[spot].pos[1] = y;
				loadedchunks[spot].pos[2] = z;

				loadedchunks[spot].data[0].id = 1;
				if(y==0)
				{
					int j,k;
					for(j=0; j<CHUNKSIZE; j++)
					{
						for(k=0; k<CHUNKSIZE; k++)
							loadedchunks[spot].data[j+k*CHUNKSIZE*CHUNKSIZE].id = 1;
					}
				}
			}
		}
	}
}

void
world_cleanup()
{	int x, y, z;
	for(x=0; x<WORLDSIZE; x++)
	{
		for(y=0; y<WORLDSIZE; y++)
		{
			for(z=0; z<WORLDSIZE; z++)
			{
				glDeleteBuffers(1, &blockvbos[x][y][z].vbo);
				free(loadedchunks[x + y*WORLDSIZE + z*WORLDSIZE*WORLDSIZE].data);
			}
		}
	}
}

void
world_render()
{
	int x=0;
	int y=0;
	int z=0;

	for(x=0; x<WORLDSIZE; x++)
	{
		for(y=0; y<WORLDSIZE; y++)
		{
			for(z=0; z<WORLDSIZE; z++)
			{
				glBindBuffer(GL_ARRAY_BUFFER, blockvbos[x][y][z].vbo);
				glVertexAttribPointer(
						0,
						3,
						GL_FLOAT,
						GL_FALSE,
						0,
						0);
				glEnableVertexAttribArray(0);
				if(!blockvbos[x][y][z].iscurrent)
				{
					int3_t pos;
					pos.x = x;
					pos.y = y;
					pos.z = z;

					mesh_t mesh = chunk_getmesh(loadedchunks[getspotof(pos)], 0,0,0,0,0,0);

					glBufferData(GL_ARRAY_BUFFER, mesh.size * sizeof(GLfloat), mesh.data, GL_STATIC_DRAW);

					blockvbos[x][y][z].points = mesh.size / 3;
					blockvbos[x][y][z].iscurrent = 1;
				}

				glDrawArrays(GL_TRIANGLES, 0, blockvbos[x][y][z].points);
			}
		}
	}
}

int
world_addblock(int3_t pos, block_t block, int loadnew)
{
	int3_t cpos = getckunkof(pos);
	int3_t internalpos;
	internalpos.x = pos.x - cpos.x*CHUNKSIZE;
	internalpos.y = pos.y - cpos.y*CHUNKSIZE;
	internalpos.z = pos.z - cpos.z*CHUNKSIZE;

	if(isquickloaded(cpos))
	{
		int arrindex = getspotof(cpos);
		loadedchunks[arrindex].data[internalpos.x + internalpos.y*CHUNKSIZE + internalpos.z*CHUNKSIZE*CHUNKSIZE] = block;
		blockvbos[cpos.x][cpos.y][cpos.z].iscurrent = 0;
		return 0;
	}
	else
		return -1;
}
