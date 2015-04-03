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
	GLuint cbo;
	int iscurrent;
	long points;
} blockvbos[WORLDSIZE][WORLDSIZE][WORLDSIZE];

static inline int3_t
getinternalspotof(long x, long y, long z)
{
	int3_t blockpos;
	blockpos.x = x%CHUNKSIZE;
	blockpos.y = y%CHUNKSIZE;
	blockpos.z = z%CHUNKSIZE;
	return blockpos;
}

static inline int3_t
getchunkspotof(long x, long y, long z)
{
	int3_t chunkpos;
	chunkpos.x = floor((double)x / CHUNKSIZE);
	chunkpos.y = floor((double)y / CHUNKSIZE);
	chunkpos.z = floor((double)z / CHUNKSIZE);
	return chunkpos;
}

static inline long
getinternalarrayspotof(long x, long y, long z)
{
	return (x) + (y)*CHUNKSIZE + (z)*CHUNKSIZE*CHUNKSIZE;
}

static inline long
getchunkarrayspotof(long x, long y, long z)
{
	return (x) + (y)*WORLDSIZE + (z)*WORLDSIZE*WORLDSIZE;
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
				glGenBuffers(1, &blockvbos[x][y][z].cbo);

				blockvbos[x][y][z].points = 0;
				blockvbos[x][y][z].iscurrent = 0;

				int spot = getchunkarrayspotof(x, y, z);
				loadedchunks[spot] = callocchunk();
				loadedchunks[spot].pos[0] = x;
				loadedchunks[spot].pos[1] = y;
				loadedchunks[spot].pos[2] = z;

				//loadedchunks[spot].data[0].id = 1;
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
				if(!blockvbos[x][y][z].iscurrent)
				{
					//re set up the buffers
					mesh_t mesh = chunk_getmesh(loadedchunks[getchunkarrayspotof(x,y,z)], 0,0,0,0,0,0);
					//points buffer
						glBindBuffer(GL_ARRAY_BUFFER, blockvbos[x][y][z].vbo);
						glBufferData(GL_ARRAY_BUFFER, mesh.size * sizeof(GLfloat), mesh.data, GL_STATIC_DRAW);

						free(mesh.data);

						//do the stuff the else case needs to do
						glVertexAttribPointer(
							0,
							3,
							GL_FLOAT,
							GL_FALSE,
							0,
							0);
						glEnableVertexAttribArray(0);
					//Color buffer
						glBindBuffer(GL_ARRAY_BUFFER, blockvbos[x][y][z].cbo);
						glBufferData(GL_ARRAY_BUFFER, mesh.colorsize * sizeof(GLfloat), mesh.colordata, GL_STATIC_DRAW);

						free(mesh.colordata);

						//do the stuff the else case needs to do
						glVertexAttribPointer(
								1,
								3,
								GL_FLOAT,
								GL_FALSE,
								0,
								0);
						glEnableVertexAttribArray(1);

					blockvbos[x][y][z].points = mesh.size / 3;
					blockvbos[x][y][z].iscurrent = 1;
				} else {
					glBindBuffer(GL_ARRAY_BUFFER, blockvbos[x][y][z].vbo);
					glVertexAttribPointer(
							0,
							3,
							GL_FLOAT,
							GL_FALSE,
							0,
							0);
					glEnableVertexAttribArray(0);

					glBindBuffer(GL_ARRAY_BUFFER, blockvbos[x][y][z].cbo);
					glVertexAttribPointer(
							1,
							3,
							GL_FLOAT,
							GL_FALSE,
							0,
							0);
					glEnableVertexAttribArray(1);
				}
				glDrawArrays(GL_TRIANGLES, 0, blockvbos[x][y][z].points);
			}
		}
	}
}

//TODO: loadnew
block_t
world_getblock(long x, long y, long z, int loadnew)
{
	int3_t cpos = getchunkspotof(x, y, z);
	int3_t internalpos = getinternalspotof(x,y,z);

	if(isquickloaded(cpos))
		return loadedchunks[getchunkarrayspotof(cpos.x, cpos.y, cpos.z)].data[getinternalarrayspotof(internalpos.x, internalpos.y, internalpos.z)];
	block_t error;
	error.id = 255;
	return error;
}

int
world_setblock(long x, long y, long z, block_t block, int loadnew)
{
	int3_t cpos = getchunkspotof(x, y, z);
	int3_t internalpos = getinternalspotof(x, y, z);

	if(isquickloaded(cpos))
	{
		int arrindex = getchunkarrayspotof(cpos.x, cpos.y, cpos.z);
		loadedchunks[arrindex].data[internalpos.x + internalpos.y*CHUNKSIZE + internalpos.z*CHUNKSIZE*CHUNKSIZE] = block;
		blockvbos[cpos.x][cpos.y][cpos.z].iscurrent = 0;
		return 0;
	}
	else
		return -1;
}
