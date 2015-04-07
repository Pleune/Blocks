#include "world.h"

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#include <SDL2/SDL_timer.h>
#include <GL/glew.h>

#include "defines.h"
#include "chunk.h"
#include "custommath.h"

#define MODULO(a, b) (((a) % (b) + (b)) % (b))

chunk_t *loadedchunks[WORLDSIZE * WORLDSIZE * WORLDSIZE];
long3_t worldscope = {0, 0, 0};

struct {
	GLuint vbo;
	GLuint cbo;
	GLuint ebo;
	int iscurrent;
	long points;
	mesh_t mesh;
	int ismeshcurrent;
	int3_t coords;//must be hand asisgned before use
	SDL_mutex *lock;
	int iswritable;
} blockvbos[WORLDSIZE][WORLDSIZE][WORLDSIZE];

static inline long3_t
getinternalspotof(long x, long y, long z)
{
	long3_t blockpos;
	blockpos.x = MODULO(x, CHUNKSIZE);
	blockpos.y = MODULO(y, CHUNKSIZE);
	blockpos.z = MODULO(z, CHUNKSIZE);
	return blockpos;
}

static inline long3_t
getchunkspotof(long x, long y, long z)
{
	long3_t chunkpos;
	chunkpos.x = floor((double)x / CHUNKSIZE);
	chunkpos.y = floor((double)y / CHUNKSIZE);
	chunkpos.z = floor((double)z / CHUNKSIZE);
	return chunkpos;
}

static inline long
getchunkarrayspotof(long x, long y, long z)
{
	return MODULO(x, WORLDSIZE) + MODULO(y, WORLDSIZE)*WORLDSIZE + MODULO(z, WORLDSIZE)*WORLDSIZE*WORLDSIZE;
}

static inline int
shouldbequickloaded(long3_t pos)
{
	return worldscope.x <= pos.x && pos.x < worldscope.x + WORLDSIZE &&
		worldscope.y <= pos.y && pos.y < worldscope.y + WORLDSIZE &&
		worldscope.z <= pos.z && pos.z < worldscope.z + WORLDSIZE;
}

static inline int
isquickloaded(long3_t pos, long *arrindex)
{
	long ai = getchunkarrayspotof(pos.x, pos.y, pos.z);
	if(arrindex)
		*arrindex = ai;

	long3_t cpos = chunk_getpos(loadedchunks[ai]);
	return shouldbequickloaded(cpos) && !memcmp(&cpos, &pos, sizeof(long3_t));
}

void
world_setworldcenter(long x, long y, long z)
{
	worldscope = getchunkspotof(x, y, z);
	worldscope.x -= WORLDSIZE/2;
	worldscope.y -= WORLDSIZE/2;
	worldscope.z -= WORLDSIZE/2;
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
				glGenBuffers(1, &blockvbos[x][y][z].ebo);
				glGenBuffers(1, &blockvbos[x][y][z].vbo);
				glGenBuffers(1, &blockvbos[x][y][z].cbo);

				blockvbos[x][y][z].points = 0;
				blockvbos[x][y][z].iscurrent = 0;
				blockvbos[x][y][z].ismeshcurrent=1;

				blockvbos[x][y][z].mesh.vbodata=0;
				blockvbos[x][y][z].mesh.ebodata=0;
				blockvbos[x][y][z].mesh.colordata=0;

				blockvbos[x][y][z].iswritable=1;
				blockvbos[x][y][z].lock = SDL_CreateMutex();

				int spot = getchunkarrayspotof(x + worldscope.x, y + worldscope.y, z + worldscope.z);

				long3_t cpos;
				cpos.x = x + worldscope.x;
				cpos.y = y + worldscope.y;
				cpos.z = z + worldscope.z;
				chunk_loadchunk(cpos, &loadedchunks[spot]);
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
				glDeleteBuffers(1, &blockvbos[x][y][z].cbo);
				glDeleteBuffers(1, &blockvbos[x][y][z].vbo);
				SDL_DestroyMutex(blockvbos[x][y][z].lock);
				chunk_freechunk(loadedchunks[x + y*WORLDSIZE + z*WORLDSIZE*WORLDSIZE]);
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
				if(!blockvbos[x][y][z].ismeshcurrent)
				{
					//points buffer
						glBindBuffer(GL_ARRAY_BUFFER, blockvbos[x][y][z].vbo);
						glBufferData(GL_ARRAY_BUFFER, blockvbos[x][y][z].mesh.vbosize * sizeof(GLfloat), blockvbos[x][y][z].mesh.vbodata, GL_STATIC_DRAW);

					//element buffer
						glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, blockvbos[x][y][z].ebo);
						glBufferData(GL_ELEMENT_ARRAY_BUFFER, blockvbos[x][y][z].mesh.ebosize * sizeof(GLuint), blockvbos[x][y][z].mesh.ebodata, GL_STATIC_DRAW);

					//Color buffer
						glBindBuffer(GL_ARRAY_BUFFER, blockvbos[x][y][z].cbo);
						glBufferData(GL_ARRAY_BUFFER, blockvbos[x][y][z].mesh.colorsize * sizeof(GLfloat), blockvbos[x][y][z].mesh.colordata, GL_STATIC_DRAW);


					if(blockvbos[x][y][z].mesh.ebodata)
						free(blockvbos[x][y][z].mesh.ebodata);
					if(blockvbos[x][y][z].mesh.vbodata)
						free(blockvbos[x][y][z].mesh.vbodata);
					if(blockvbos[x][y][z].mesh.colordata)
						free(blockvbos[x][y][z].mesh.colordata);
					blockvbos[x][y][z].ismeshcurrent=1;
				}
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

				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, blockvbos[x][y][z].ebo);
				glDrawElements(GL_TRIANGLES, blockvbos[x][y][z].points, GL_UNSIGNED_INT, 0);
			}
		}
	}
}

static int
quickremeshachunk(void *ptr)
{
	int3_t *i = (int3_t *)ptr;

	long arrpos = getchunkarrayspotof(i->x, i->y, i->z);

	if(SDL_TryLockMutex(blockvbos[i->x][i->y][i->z].lock)!=0)
	{
		printf("Xzz: %i Yzz: %i Zzz: %i\n", i->x, i->y, i->z);
		blockvbos[i->x][i->y][i->z].iscurrent=0;
		return -1;
	}

	chunk_t *north=0;
	chunk_t *south=0;
	chunk_t *east=0;
	chunk_t *west=0;
	chunk_t *up=0;
	chunk_t *down=0;

	long temparrpos;
	long3_t tempcpos = chunk_getpos(loadedchunks[arrpos]);

	tempcpos.x++;
	if(isquickloaded(tempcpos, &temparrpos))
	{
		if(!SDL_TryLockMutex(blockvbos[MODULO(i->x+1,WORLDSIZE)][MODULO(i->y,WORLDSIZE)][MODULO(i->z,WORLDSIZE)].lock))
			east = loadedchunks[temparrpos];
	}
	tempcpos.x -= 2;
	if(isquickloaded(tempcpos, &temparrpos))
	{
		if(!SDL_TryLockMutex(blockvbos[MODULO(i->x-1,WORLDSIZE)][MODULO(i->y,WORLDSIZE)][MODULO(i->z,WORLDSIZE)].lock))
			west = loadedchunks[temparrpos];
	}
	tempcpos.x++;

	tempcpos.y++;
	if(isquickloaded(tempcpos, &temparrpos))
	{
		if(!SDL_TryLockMutex(blockvbos[MODULO(i->x,WORLDSIZE)][MODULO(i->y+1,WORLDSIZE)][MODULO(i->z,WORLDSIZE)].lock))
			up = loadedchunks[temparrpos];
	}
	tempcpos.y -= 2;
	if(isquickloaded(tempcpos, &temparrpos))
	{
		if(!SDL_TryLockMutex(blockvbos[MODULO(i->x,WORLDSIZE)][MODULO(i->y-1,WORLDSIZE)][MODULO(i->z,WORLDSIZE)].lock))
			down = loadedchunks[temparrpos];
	}
	tempcpos.y++;

	tempcpos.z++;
	if(isquickloaded(tempcpos, &temparrpos))
	{
		if(!SDL_TryLockMutex(blockvbos[MODULO(i->x,WORLDSIZE)][MODULO(i->y,WORLDSIZE)][MODULO(i->z+1,WORLDSIZE)].lock))
			south = loadedchunks[temparrpos];
	}
	tempcpos.z -= 2;
	if(isquickloaded(tempcpos, &temparrpos))
	{
		if(!SDL_TryLockMutex(blockvbos[MODULO(i->x,WORLDSIZE)][MODULO(i->y,WORLDSIZE)][MODULO(i->z-1,WORLDSIZE)].lock))
			north = loadedchunks[temparrpos];
	}


	//make sure the other thread dosent do anything stupid
	blockvbos[i->x][i->y][i->z].ismeshcurrent=1;

	//re set up the buffers
//	if(blockvbos[i->x][i->y][i->z].mesh.ebodata)
//		free(blockvbos[i->x][i->y][i->z].mesh.ebodata);
//	if(blockvbos[i->x][i->y][i->z].mesh.vbodata)
//		free(blockvbos[i->x][i->y][i->z].mesh.vbodata);
//	if(blockvbos[i->x][i->y][i->z].mesh.colordata)
//		free(blockvbos[i->x][i->y][i->z].mesh.colordata);

	blockvbos[i->x][i->y][i->z].mesh = chunk_getmesh(loadedchunks[getchunkarrayspotof(i->x,i->y,i->z)], up,down,north,south,east,west);

	blockvbos[i->x][i->y][i->z].iscurrent = 1;
	blockvbos[i->x][i->y][i->z].ismeshcurrent=0;
	blockvbos[i->x][i->y][i->z].points = blockvbos[i->x][i->y][i->z].mesh.ebosize;
	SDL_UnlockMutex(blockvbos[i->x][i->y][i->z].lock);

	if(north)
		SDL_UnlockMutex(blockvbos[MODULO(i->x,WORLDSIZE)][MODULO(i->y,WORLDSIZE)][MODULO(i->z-1,WORLDSIZE)].lock);
	if(south)
		SDL_UnlockMutex(blockvbos[MODULO(i->x,WORLDSIZE)][MODULO(i->y,WORLDSIZE)][MODULO(i->z+1,WORLDSIZE)].lock);
	if(east)
		SDL_UnlockMutex(blockvbos[MODULO(i->x+1,WORLDSIZE)][MODULO(i->y,WORLDSIZE)][MODULO(i->z,WORLDSIZE)].lock);
	if(west)
		SDL_UnlockMutex(blockvbos[MODULO(i->x-1,WORLDSIZE)][MODULO(i->y,WORLDSIZE)][MODULO(i->z,WORLDSIZE)].lock);
	if(up)
		SDL_UnlockMutex(blockvbos[MODULO(i->x,WORLDSIZE)][MODULO(i->y+1,WORLDSIZE)][MODULO(i->z,WORLDSIZE)].lock);
	if(down)
		SDL_UnlockMutex(blockvbos[MODULO(i->x,WORLDSIZE)][MODULO(i->y-1,WORLDSIZE)][MODULO(i->z,WORLDSIZE)].lock);

	return 0;
}

int
world_threadentry(void *ptr)
{
	while(1)
	{

		long3_t cpos;

		for(cpos.x = worldscope.x; cpos.x< worldscope.x+WORLDSIZE; cpos.x++)
		{
			for(cpos.y = worldscope.y; cpos.y< worldscope.y+WORLDSIZE; cpos.y++)
			{
				for(cpos.z = worldscope.z; cpos.z< worldscope.z+WORLDSIZE; cpos.z++)
				{
					long arrindex;
					if(!isquickloaded(cpos, &arrindex))
					{
						//the chunk should be loaded but its not. load it.
						chunk_t *chunk = loadedchunks[arrindex];

						int3_t bp;
						bp.x = MODULO(cpos.x,WORLDSIZE);
						bp.y = MODULO(cpos.y,WORLDSIZE);
						bp.z = MODULO(cpos.z,WORLDSIZE);

						SDL_UnlockMutex(blockvbos[bp.x][bp.y][bp.z].lock);
						blockvbos[bp.x][bp.y][bp.z].iswritable = 0;
						SDL_UnlockMutex(blockvbos[bp.x][bp.y][bp.z].lock);

						chunk_reloadchunk(cpos, chunk);

						blockvbos[bp.x][bp.y][bp.z].iswritable = 1;
						blockvbos[bp.x][bp.y][bp.z].iscurrent = 0;
					}
				}
			}
		}

		int3_t i;
		for(i.x = 0; i.x< WORLDSIZE; i.x++)
		{
			for(i.y = 0; i.y< WORLDSIZE; i.y++)
			{
				for(i.z = 0; i.z< WORLDSIZE; i.z++)
				{
					if(!blockvbos[i.x][i.y][i.z].iscurrent)
					{
						quickremeshachunk(&i);
					}
				}
			}
		}

		SDL_Delay(300);
	}
	return 0;
}



//TODO: loadnew
block_t
world_getblock(long x, long y, long z, int loadnew)
{
	long3_t cpos = getchunkspotof(x, y, z);
	long3_t internalpos = getinternalspotof(x,y,z);

	long arrindex;

	if(isquickloaded(cpos, &arrindex))
		return chunk_getblock(loadedchunks[arrindex], internalpos.x, internalpos.y, internalpos.z);
	block_t error;
	error.id = 255;
	return error;
}

//TODO: loadnew
int
world_setblock(long x, long y, long z, block_t block, int loadnew)
{
	long3_t cpos = getchunkspotof(x, y, z);
	long3_t internalpos = getinternalspotof(x, y, z);

	int3_t bp;
	bp.x = MODULO(cpos.x,WORLDSIZE);
	bp.y = MODULO(cpos.y,WORLDSIZE);
	bp.z = MODULO(cpos.z,WORLDSIZE);

	long arrindex;
	if(isquickloaded(cpos, &arrindex))
	{
		chunk_t *chunk = loadedchunks[arrindex];
		if(!blockvbos[bp.x][bp.y][bp.z].iswritable)
			return -2;

		if(SDL_TryLockMutex(blockvbos[bp.x][bp.y][bp.z].lock)==0)
		{
			chunk_setblock(chunk, internalpos.x, internalpos.y, internalpos.z, block);
			SDL_UnlockMutex(blockvbos[bp.x][bp.y][bp.z].lock);
			int3_t i;
			i.x = MODULO(cpos.x, WORLDSIZE);
			i.y = MODULO(cpos.y, WORLDSIZE);
			i.z = MODULO(cpos.z, WORLDSIZE);
			blockvbos[i.x][i.y][i.z].coords = i;

			SDL_CreateThread( quickremeshachunk, "quickblock", &blockvbos[i.x][i.y][i.z].coords);

			return 0;
		}
		return -3;
	}
	return -1;
}
