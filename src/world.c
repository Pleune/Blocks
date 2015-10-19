#include "world.h"

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <time.h>

#include <SDL2/SDL_timer.h>

#include "defines.h"
#include "chunk.h"
#include "modulo.h"

long3_t worldscope = {0, 0, 0};
long3_t worldcenter = {0, 0, 0};

uint32_t seed;

int stopthread;
SDL_Thread *thread;

struct {
	chunk_t *chunk;
} data[WORLDSIZE][WORLDSIZE][WORLDSIZE];

static inline int3_t
getchunkindexofchunk(long3_t pos)
{
	int3_t icpo = {
		MODULO(pos.x, WORLDSIZE),
		MODULO(pos.y, WORLDSIZE),
		MODULO(pos.z, WORLDSIZE)
	};
	return icpo;
}

static inline int
shouldbequickloaded(long3_t pos)
{
	return worldscope.x <= pos.x && pos.x < worldscope.x + WORLDSIZE &&
		worldscope.y <= pos.y && pos.y < worldscope.y + WORLDSIZE &&
		worldscope.z <= pos.z && pos.z < worldscope.z + WORLDSIZE;
}

static inline int
isquickloaded(long3_t pos, int3_t *chunkindex)
{
	int3_t ci = getchunkindexofchunk(pos);
	if(chunkindex)
		*chunkindex = ci;

	long3_t cpos = chunk_getpos(data[ci.x][ci.y][ci.z].chunk);
	return shouldbequickloaded(cpos) && !memcmp(&cpos, &pos, sizeof(long3_t));
}

static void
setworldcenter(vec3_t pos)
{
	worldcenter = chunk_getchunkofspot(pos.x, pos.y, pos.z);
	worldscope.x = worldcenter.x - WORLDSIZE/2;
	worldscope.y = worldcenter.y - WORLDSIZE/2;
	worldscope.z = worldcenter.z - WORLDSIZE/2;
}

static int
quickremeshachunk(int3_t *chunkindex, int instant)
{
	if(!instant)
	{
		chunk_setnotcurrent(data[chunkindex->x][chunkindex->y][chunkindex->z].chunk);
		return 0;
	}

	chunk_t *north=0;
	chunk_t *south=0;
	chunk_t *east=0;
	chunk_t *west=0;
	chunk_t *up=0;
	chunk_t *down=0;

	chunk_t *chunk = data[chunkindex->x][chunkindex->y][chunkindex->z].chunk;

	int3_t tempchunkindex;
	long3_t tempcpos = chunk_getpos(chunk);

	tempcpos.x++;
	if(isquickloaded(tempcpos, &tempchunkindex))
	{
			east = data[tempchunkindex.x][tempchunkindex.y][tempchunkindex.z].chunk;
	}
	tempcpos.x -= 2;
	if(isquickloaded(tempcpos, &tempchunkindex))
	{
			west = data[tempchunkindex.x][tempchunkindex.y][tempchunkindex.z].chunk;
	}
	tempcpos.x++;

	tempcpos.y++;
	if(isquickloaded(tempcpos, &tempchunkindex))
	{
			up = data[tempchunkindex.x][tempchunkindex.y][tempchunkindex.z].chunk;
	}
	tempcpos.y -= 2;
	if(isquickloaded(tempcpos, &tempchunkindex))
	{
			down = data[tempchunkindex.x][tempchunkindex.y][tempchunkindex.z].chunk;
	}
	tempcpos.y++;

	tempcpos.z++;
	if(isquickloaded(tempcpos, &tempchunkindex))
	{
			south = data[tempchunkindex.x][tempchunkindex.y][tempchunkindex.z].chunk;
	}
	tempcpos.z -= 2;
	if(isquickloaded(tempcpos, &tempchunkindex))
	{
			north = data[tempchunkindex.x][tempchunkindex.y][tempchunkindex.z].chunk;
	}


	//re set up the buffers
	chunk_remesh(chunk, up,down,north,south,east,west);

	return 0;
}

int
world_generation(void *ptr)
{
	stopthread=0;
	while(!stopthread)
	{
		long3_t cpos;
		for(cpos.x = worldscope.x; cpos.x< worldscope.x+WORLDSIZE; cpos.x++)
		{
			if(stopthread)
				break;
			for(cpos.z = worldscope.z; cpos.z< worldscope.z+WORLDSIZE; cpos.z++)
			{
				if(stopthread)
					break;
				for(cpos.y = worldscope.y; cpos.y< worldscope.y+WORLDSIZE; cpos.y++)
				{
					if(stopthread)
						break;
					int3_t chunkindex;
					if(!isquickloaded(cpos, &chunkindex))
					{
						if(stopthread)
							break;
						//the chunk should be loaded but its not. load it.
						chunk_t *chunk = data[chunkindex.x][chunkindex.y][chunkindex.z].chunk;

						chunk_reloadchunk(cpos, chunk);

						chunk_setnotcurrent(data[chunkindex.x == WORLDSIZE-1 ? 0 : chunkindex.x+1][chunkindex.y][chunkindex.z].chunk);
						chunk_setnotcurrent(data[chunkindex.x == 0 ? WORLDSIZE-1 : chunkindex.x-1][chunkindex.y][chunkindex.z].chunk);
						chunk_setnotcurrent(data[chunkindex.x][chunkindex.y == WORLDSIZE-1 ? 0 : chunkindex.y+1][chunkindex.z].chunk);
						chunk_setnotcurrent(data[chunkindex.x][chunkindex.y == 0 ? WORLDSIZE-1 : chunkindex.y-1][chunkindex.z].chunk);
						chunk_setnotcurrent(data[chunkindex.x][chunkindex.y][chunkindex.z == WORLDSIZE-1 ? 0 : chunkindex.z+1].chunk);
						chunk_setnotcurrent(data[chunkindex.x][chunkindex.y][chunkindex.z == 0 ? WORLDSIZE-1 : chunkindex.z-1].chunk);
					}
				}
			}
		}

		int3_t i;
		for(i.x = 0; i.x< WORLDSIZE; i.x++)
		{
			if(stopthread)
				break;
			for(i.y = 0; i.y< WORLDSIZE; i.y++)
			{
				if(stopthread)
					break;
				for(i.z = 0; i.z< WORLDSIZE; i.z++)
				{
					if(stopthread)
						break;

					if(!chunk_iscurrent(data[i.x][i.y][i.z].chunk))
						quickremeshachunk(&i, 1);
				}
			}
		}

		if(!stopthread)
			SDL_Delay(80);
	}
	return 0;
}

void
world_genseed()
{
	time_t t;
	srand((unsigned) time(&t));
	seed = rand();
}

void
world_init(vec3_t pos)
{
	setworldcenter(pos);

	long chunkno = 1;
	long3_t cpos;
	for(cpos.x = worldscope.x; cpos.x< worldscope.x+WORLDSIZE; cpos.x++)
	{
		for(cpos.z = worldscope.z; cpos.z< worldscope.z+WORLDSIZE; cpos.z++)
		{
			for(cpos.y = worldscope.y; cpos.y< worldscope.y+WORLDSIZE; cpos.y++)
			{
				printf("LOADING... %f%%\r", (float)(chunkno * 100.0f)/(WORLDSIZE*WORLDSIZE*WORLDSIZE));
				fflush(stdout);
				chunkno++;
				int3_t chunkindex = getchunkindexofchunk(cpos);
				data[chunkindex.x][chunkindex.y][chunkindex.z].chunk = chunk_loadchunk(cpos);
			}
		}
	}
	putchar('\n');

	thread = SDL_CreateThread(world_generation, "world_generation", 0);
}

void
world_cleanup()
{
	stopthread=1;

	int ret;
	SDL_WaitThread(thread, &ret);

	int3_t chunkindex;
	for(chunkindex.x=0; chunkindex.x<WORLDSIZE; chunkindex.x++)
	{
		for(chunkindex.y=0; chunkindex.y<WORLDSIZE; chunkindex.y++)
		{
			for(chunkindex.z=0; chunkindex.z<WORLDSIZE; chunkindex.z++)
			{
				chunk_freechunk(data[chunkindex.x][chunkindex.y][chunkindex.z].chunk);
			}
		}
	}
}

void
world_render(vec3_t pos)
{
	setworldcenter(pos);
	glEnable(GL_DEPTH_TEST);

	int x=0;
	int y=0;
	int z=0;

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);

	for(x=0; x<WORLDSIZE; x++)
	{
		for(y=0; y<WORLDSIZE; y++)
		{
			for(z=0; z<WORLDSIZE; z++)
			{
				chunk_render(data[x][y][z].chunk);
			}
		}
	}
}

//TODO: loadnew
block_t
world_getblock(long x, long y, long z, int loadnew)
{
	long3_t cpos = chunk_getchunkofspot(x, y, z);
	int3_t internalpos = chunk_getinternalspotofspot(x,y,z);

	int3_t icpo;

	if(isquickloaded(cpos, &icpo))
		return chunk_getblock(data[icpo.x][icpo.y][icpo.z].chunk, internalpos.x, internalpos.y, internalpos.z);
	block_t error;
	error.id = ERR;
	return error;
}

//TODO: loadnew
int
world_setblock(long x, long y, long z, block_t block, int loadnew, int instant)
{
	long3_t cpos = chunk_getchunkofspot(x, y, z);
	int3_t internalpos = chunk_getinternalspotofspot(x, y, z);

	int3_t chunkindex;
	if(isquickloaded(cpos, &chunkindex))
	{
		chunk_t *chunk = data[chunkindex.x][chunkindex.y][chunkindex.z].chunk;

		chunk_setblock(chunk, internalpos.x, internalpos.y, internalpos.z, block);

		quickremeshachunk(&chunkindex, instant);

		cpos.x++;
		if(internalpos.x == CHUNKSIZE-1 && isquickloaded(cpos, &chunkindex))
			quickremeshachunk(&chunkindex, instant);

		cpos.x -= 2;
		if(internalpos.x == 0 && isquickloaded(cpos, &chunkindex))
			quickremeshachunk(&chunkindex, instant);
		cpos.x++;

		cpos.y++;
		if(internalpos.y == CHUNKSIZE-1 && isquickloaded(cpos, &chunkindex))
			quickremeshachunk(&chunkindex, instant);

		cpos.y -= 2;
		if(internalpos.y == 0 && isquickloaded(cpos, &chunkindex))
			quickremeshachunk(&chunkindex, instant);
		cpos.y++;

		cpos.z++;
		if(internalpos.z == CHUNKSIZE-1 && isquickloaded(cpos, &chunkindex))
			quickremeshachunk(&chunkindex, instant);

		cpos.z -= 2;
		if(internalpos.z == 0 && isquickloaded(cpos, &chunkindex))
			quickremeshachunk(&chunkindex, instant);

		return 0;
	}
	return -1;
}

uint32_t
world_getseed()
{
	return seed;
}
