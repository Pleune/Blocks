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

#include "worldgen.h"

#define MODULO(a, b) (((a) % (b) + (b)) % (b))

chunk_p loadedchunks[WORLDSIZE * WORLDSIZE * WORLDSIZE];
long3_t worldscope = {-2, -2, -2};

struct {
	GLuint vbo;
	GLuint cbo;
	int iscurrent;
	long points;
	mesh_t mesh;
	int ismeshcurrent;
	int3_t coords;//must be hand asisgned before use
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
isquickloaded(long3_t cpos, long *arrindex)
{
	long ai = getchunkarrayspotof(cpos.x, cpos.y, cpos.z);
	if(arrindex)
		*arrindex = ai;
	return shouldbequickloaded(cpos) && !memcmp(&cpos, &loadedchunks[ai].pos, sizeof(long3_t));
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
				glGenBuffers(1, &blockvbos[x][y][z].vbo);
				glGenBuffers(1, &blockvbos[x][y][z].cbo);

				blockvbos[x][y][z].points = 0;
				blockvbos[x][y][z].iscurrent = 0;
				blockvbos[x][y][z].ismeshcurrent=1;

				blockvbos[x][y][z].mesh.data=0;
				blockvbos[x][y][z].mesh.colordata=0;

				int spot = getchunkarrayspotof(x + worldscope.x, y + worldscope.y, z + worldscope.z);

				long3_t cpos;
				cpos.x = x + worldscope.x;
				cpos.y = y + worldscope.y;
				cpos.z = z + worldscope.z;
				loadedchunks[spot] = chunk_initchunk(cpos);
				worldgen_genchunk(&loadedchunks[spot]);
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
				chunk_freechunk(&loadedchunks[x + y*WORLDSIZE + z*WORLDSIZE*WORLDSIZE]);
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
						glBufferData(GL_ARRAY_BUFFER, blockvbos[x][y][z].mesh.size * sizeof(GLfloat), blockvbos[x][y][z].mesh.data, GL_STATIC_DRAW);

						if(blockvbos[x][y][z].mesh.data)
						{
							free(blockvbos[x][y][z].mesh.data);
							blockvbos[x][y][z].mesh.data=0;
						}

					//Color buffer
						glBindBuffer(GL_ARRAY_BUFFER, blockvbos[x][y][z].cbo);
						glBufferData(GL_ARRAY_BUFFER, blockvbos[x][y][z].mesh.colorsize * sizeof(GLfloat), blockvbos[x][y][z].mesh.colordata, GL_STATIC_DRAW);

						if(blockvbos[x][y][z].mesh.colordata)
						{
							free(blockvbos[x][y][z].mesh.colordata);
							blockvbos[x][y][z].mesh.colordata=0;
						}

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
				glDrawArrays(GL_TRIANGLES, 0, blockvbos[x][y][z].points);
			}
		}
	}
}

static int
quickremeshachunk(void *ptr)
{
	int3_t *i = (int3_t *)ptr;

	long arrpos = getchunkarrayspotof(i->x, i->y, i->z);

	if(SDL_TryLockMutex(loadedchunks[arrpos].lock)!=0)
	{
		printf("Xzz: %i Yzz: %i Zzz: %i\n", i->x, i->y, i->z);
		blockvbos[i->x][i->y][i->z].iscurrent=0;
		return -1;
	}

	chunk_p *north=0;
	chunk_p *south=0;
	chunk_p *east=0;
	chunk_p *west=0;
	chunk_p *up=0;
	chunk_p *down=0;

	long temparrpos;
	long3_t tempcpos = loadedchunks[arrpos].pos;



	tempcpos.x++;
	if(isquickloaded(tempcpos, &temparrpos))
	{
		if(!SDL_TryLockMutex(loadedchunks[temparrpos].lock))
			east = &loadedchunks[temparrpos];
	}
	tempcpos.x -= 2;
	if(isquickloaded(tempcpos, &temparrpos))
	{
		if(!SDL_TryLockMutex(loadedchunks[temparrpos].lock))
			west = &loadedchunks[temparrpos];
	}
	tempcpos.x++;

	tempcpos.y++;
	if(isquickloaded(tempcpos, &temparrpos))
	{
		if(!SDL_TryLockMutex(loadedchunks[temparrpos].lock))
			up = &loadedchunks[temparrpos];
	}
	tempcpos.y -= 2;
	if(isquickloaded(tempcpos, &temparrpos))
	{
		if(!SDL_TryLockMutex(loadedchunks[temparrpos].lock))
			down = &loadedchunks[temparrpos];
	}
	tempcpos.y++;

	tempcpos.z++;
	if(isquickloaded(tempcpos, &temparrpos))
	{
		if(!SDL_TryLockMutex(loadedchunks[temparrpos].lock))
			south = &loadedchunks[temparrpos];
	}
	tempcpos.z -= 2;
	if(isquickloaded(tempcpos, &temparrpos))
	{
		if(!SDL_TryLockMutex(loadedchunks[temparrpos].lock))
			north = &loadedchunks[temparrpos];
	}


	//make sure the other thread dosent do anything stupid
	blockvbos[i->x][i->y][i->z].ismeshcurrent=1;

	//re set up the buffers
	if(blockvbos[i->x][i->y][i->z].mesh.data)
		free(blockvbos[i->x][i->y][i->z].mesh.data);
	if(blockvbos[i->x][i->y][i->z].mesh.colordata)
		free(blockvbos[i->x][i->y][i->z].mesh.colordata);

	blockvbos[i->x][i->y][i->z].mesh = chunk_getmesh(&loadedchunks[getchunkarrayspotof(i->x,i->y,i->z)], up,down,north,south,east,west);

	blockvbos[i->x][i->y][i->z].iscurrent = 1;
	blockvbos[i->x][i->y][i->z].ismeshcurrent=0;
	blockvbos[i->x][i->y][i->z].points = blockvbos[i->x][i->y][i->z].mesh.size / 3;
	SDL_UnlockMutex(loadedchunks[arrpos].lock);

	if(north)
		SDL_UnlockMutex(north->lock);
	if(south)
		SDL_UnlockMutex(south->lock);
	if(east)
		SDL_UnlockMutex(east->lock);
	if(west)
		SDL_UnlockMutex(west->lock);
	if(up)
		SDL_UnlockMutex(up->lock);
	if(down)
		SDL_UnlockMutex(down->lock);

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
						chunk_p *chunk = &loadedchunks[arrindex];

						SDL_LockMutex(chunk->lock);
						chunk->writable = 0;
						SDL_UnlockMutex(chunk->lock);

						free(chunk->data);
						chunk->pos = cpos;

						worldgen_genchunk(chunk);

						chunk->writable = 1;
						blockvbos[MODULO(cpos.x, WORLDSIZE)][MODULO(cpos.y, WORLDSIZE)][MODULO(cpos.z, WORLDSIZE)].iscurrent = 0;
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
		return chunk_getblock(&loadedchunks[arrindex], internalpos.x, internalpos.y, internalpos.z);
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

	long arrindex;
	if(isquickloaded(cpos, &arrindex))
	{
		chunk_p *chunk = &loadedchunks[arrindex];
		if(!chunk->writable)
			return -2;

		if(SDL_TryLockMutex(chunk->lock)==0)
		{
			chunk_setblock(chunk, internalpos.x, internalpos.y, internalpos.z, block);
			SDL_UnlockMutex(chunk->lock);
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
