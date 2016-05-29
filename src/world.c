#include "world.h"

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <limits.h>

#include <SDL2/SDL_timer.h>

#include "custommath.h"
#include "defines.h"
#include "chunk.h"
#include "modulo.h"
#include "worldgen.h"
#include "blockpick.h"

static long3_t worldscope = {0, 0, 0};
static vec3_t worldcenterpos = {0, 0, 0};
static long3_t worldcenter = {0, 0, 0};

static uint32_t seed;
static long totalpoints=0;

static int stopthreads;
static SDL_Thread *generationthread;
static SDL_Thread *remeshthreadA; //entire world (slow)
static SDL_Thread *remeshthreadB; //center of world (faster)
static SDL_Thread *remeshthreadC; //Only "raycasted" updates (fasterer)
static SDL_Thread *remeshthreadD; //Only "instant" updates (fastest)

struct {
	chunk_t *chunk;
	uint8_t instantremesh;
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
	worldcenterpos = pos;

	worldcenter = world_getchunkposofworldpos(pos.x, pos.y, pos.z);
	worldscope.x = worldcenter.x - WORLDSIZE/2;
	worldscope.y = worldcenter.y - WORLDSIZE/2;
	worldscope.z = worldcenter.z - WORLDSIZE/2;
}

static void
remesh(int3_t *chunkindex)
{
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
}

static void
queueremesh(int3_t *chunkindex, int instant)
{
	if(instant)
	{
		data[chunkindex->x][chunkindex->y][chunkindex->z].instantremesh = 1;
	}

	chunk_setnotcurrent(data[chunkindex->x][chunkindex->y][chunkindex->z].chunk);
	return;
}

struct world_genthread_s {
	SDL_sem *initalized;
	int continuous;
	worldgen_t* context;
	int3_t low;
	int3_t high;

	int *counter;
};

static int
generationthreadfunc(void *ptr)
{
	struct world_genthread_s *info = (struct world_genthread_s *)ptr;
	int3_t low = info->low;
	int3_t high = info->high;
	int continuous = info->continuous;
	worldgen_t *context = info->context;
	int *counter = info->counter;

	SDL_SemPost(info->initalized);

	do
	{
		long3_t cpos;
		for(cpos.x = worldscope.x + low.x; cpos.x < worldscope.x+high.x; ++cpos.x)
		{
			if(stopthreads)
				break;
			for(cpos.z = worldscope.z + low.z; cpos.z < worldscope.z+high.z; ++cpos.z)
			{
				if(stopthreads)
					break;
				for(cpos.y = worldscope.y + low.y; cpos.y < worldscope.y+high.y; ++cpos.y)
				{
					if(stopthreads)
						break;
					int3_t chunkindex;
					if(!isquickloaded(cpos, &chunkindex))
					{
						if(stopthreads)
							break;

						chunk_t *chunk = data[chunkindex.x][chunkindex.y][chunkindex.z].chunk;

						worldgen_genchunk(context, chunk, &cpos);

						chunk_setnotcurrent(data[chunkindex.x == WORLDSIZE-1 ? 0 : chunkindex.x+1][chunkindex.y][chunkindex.z].chunk);
						chunk_setnotcurrent(data[chunkindex.x == 0 ? WORLDSIZE-1 : chunkindex.x-1][chunkindex.y][chunkindex.z].chunk);
						chunk_setnotcurrent(data[chunkindex.x][chunkindex.y == WORLDSIZE-1 ? 0 : chunkindex.y+1][chunkindex.z].chunk);
						chunk_setnotcurrent(data[chunkindex.x][chunkindex.y == 0 ? WORLDSIZE-1 : chunkindex.y-1][chunkindex.z].chunk);
						chunk_setnotcurrent(data[chunkindex.x][chunkindex.y][chunkindex.z == WORLDSIZE-1 ? 0 : chunkindex.z+1].chunk);
						chunk_setnotcurrent(data[chunkindex.x][chunkindex.y][chunkindex.z == 0 ? WORLDSIZE-1 : chunkindex.z-1].chunk);

						if(counter)
						{
							++(*counter);

							char string[] = "                    ";
							float percent = (float)(*counter)/(WORLDSIZE*WORLDSIZE*WORLDSIZE);
							memset(string, '#', (sizeof(string) - 1) * percent);
							printf("LOADING... [%s] %f%%\r", string, percent * 100.0f);
							fflush(stdout);
						}
					}
				}
			}
		}

		if(!stopthreads)
			SDL_Delay(80);
	} while(!stopthreads && continuous);
	return 0;
}

//FULL REMESH
static int
remeshthreadfuncA(void *ptr)
{
	while(!stopthreads)
	{
		int3_t i;
		for(i.x = 0; i.x< WORLDSIZE; ++i.x)
		{
			if(stopthreads)
				break;
			for(i.y = 0; i.y< WORLDSIZE; ++i.y)
			{
				if(stopthreads)
					break;
				for(i.z = 0; i.z< WORLDSIZE; ++i.z)
				{
					if(stopthreads)
						break;

					if(!chunk_iscurrent(data[i.x][i.y][i.z].chunk))
						remesh(&i);
				}
			}
		}

		if(!stopthreads)
			SDL_Delay(80);
	}
	return 0;
}

//MIDDLE WORLD REMESH
static int
remeshthreadfuncB(void *ptr)
{
	while(!stopthreads)
	{
		long3_t i;
		long3_t lowbound = {
			worldcenter.x - WORLDSIZE/6,
			worldcenter.y - WORLDSIZE/6,
			worldcenter.z - WORLDSIZE/6
		};
		long3_t highbound = {
			worldcenter.x + WORLDSIZE/6,
			worldcenter.y + WORLDSIZE/6,
			worldcenter.z + WORLDSIZE/6
		};
		for(i.x = lowbound.x; i.x< highbound.x; ++i.x)
		{
			if(stopthreads)
				break;
			for(i.y = lowbound.y; i.y< highbound.y; ++i.y)
			{
				if(stopthreads)
					break;
				for(i.z = lowbound.z; i.z< highbound.z; ++i.z)
				{
					int3_t icpo = getchunkindexofchunk(i);
					if(stopthreads)
						break;

					if(!chunk_iscurrent(data[icpo.x][icpo.y][icpo.z].chunk))
						remesh(&icpo);
				}
			}
		}

		if(!stopthreads)
			SDL_Delay(80);
	}
	return 0;
}

//RAYCAST REMESH
static int
remeshthreadfuncC(void *ptr)
{
	{
		vec3_t i;
		for(i.x = -WORLDSIZE; i.x < WORLDSIZE; ++i.x)
		{
			if(stopthreads)
				break;
			for(i.y = -WORLDSIZE; i.y < WORLDSIZE; ++i.y)
			{
				if(stopthreads)
					break;
				for(i.z = -WORLDSIZE; i.z < WORLDSIZE; ++i.z)
				{
					if(stopthreads)
						break;

					long3_t pos = world_raypos(&worldcenterpos, &i, 0, 1000);
					pos = world_getchunkposofworldpos(pos.x, pos.y, pos.z);

					int3_t icpo = getchunkindexofchunk(pos);

					if(!chunk_iscurrent(data[icpo.x][icpo.y][icpo.z].chunk))
						remesh(&icpo);

				}
			}
		}

		if(!stopthreads)
			SDL_Delay(80);
	}
	return 0;
}


//FAST REMESH
static int
remeshthreadfuncD(void *ptr)
{
	while(!stopthreads)
	{
		uint32_t ticks = SDL_GetTicks();
		int3_t i;
		for(i.x = 0; i.x< WORLDSIZE; ++i.x)
		{
			if(stopthreads)
				break;
			for(i.y = 0; i.y< WORLDSIZE; ++i.y)
			{
				if(stopthreads)
					break;
				for(i.z = 0; i.z< WORLDSIZE; ++i.z)
				{
					if(stopthreads)
						break;

					if(data[i.x][i.y][i.z].instantremesh)
					{
						data[i.x][i.y][i.z].instantremesh = 0;
						remesh(&i);
					}
				}
			}
		}

		uint32_t ticksdiff = SDL_GetTicks() - ticks;
		if(ticksdiff < 100)
			SDL_Delay(100 - ticksdiff); //Don't run this function more than 10 times a seccond
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
	chunk_initindexbuffers();

	int3_t cpos;
	for(cpos.x = 0; cpos.x<WORLDSIZE; ++cpos.x)
	for(cpos.z = 0; cpos.z<WORLDSIZE; ++cpos.z)
	for(cpos.y = 0; cpos.y<WORLDSIZE; ++cpos.y)
	{
		long3_t long3max = {LONG_MAX, LONG_MAX, LONG_MAX};

		data[cpos.x][cpos.y][cpos.z].chunk = chunk_loademptychunk(long3max);
		data[cpos.x][cpos.y][cpos.z].instantremesh = 0;
	}

	stopthreads = 0;
	int wgcounter = 0;
	struct world_genthread_s wginfo = { 0, 0, 0,
		{0, 0, 0},
		{WORLDSIZE, WORLDSIZE, WORLDSIZE}
	};
	wginfo.initalized = SDL_CreateSemaphore(0);
	wginfo.counter = &wgcounter;

	SDL_Thread *wgthreads[INIT_WORLDGEN_THREADS];
	worldgen_t *wgcontexts[INIT_WORLDGEN_THREADS];

	int i;
	for(i=0; i<INIT_WORLDGEN_THREADS; ++i)
	{
		wginfo.low.x = (WORLDSIZE / (double)INIT_WORLDGEN_THREADS)*i;
		wginfo.high.x = (WORLDSIZE / (double)INIT_WORLDGEN_THREADS)*(i+1);

		wginfo.context = worldgen_createcontext();
		wgcontexts[i] = wginfo.context;

		wgthreads[i] = SDL_CreateThread(generationthreadfunc, "world_generation_init", &wginfo);
		SDL_SemWait(wginfo.initalized);
	}

	for(i=0; i<INIT_WORLDGEN_THREADS; ++i)
	{
		SDL_WaitThread(wgthreads[i], 0);
		worldgen_destroycontext(wgcontexts[i]);
	}

	wginfo.continuous = 1;
	wginfo.low.x = 0;
	wginfo.high.x = WORLDSIZE;
	wginfo.context = 0;
	wginfo.counter = 0;

	generationthread = SDL_CreateThread(generationthreadfunc, "world_generation", &wginfo);
	SDL_SemWait(wginfo.initalized);
	SDL_DestroySemaphore(wginfo.initalized);

	remeshthreadA = SDL_CreateThread(remeshthreadfuncA, "world_remeshA", 0);
	remeshthreadB = SDL_CreateThread(remeshthreadfuncB, "world_remeshB", 0);
	remeshthreadC = SDL_CreateThread(remeshthreadfuncC, "world_remeshC", 0);
	remeshthreadD = SDL_CreateThread(remeshthreadfuncD, "world_remeshD", 0);
}

void
world_cleanup()
{
	stopthreads=1;

	SDL_WaitThread(generationthread, 0);
	SDL_WaitThread(remeshthreadA, 0);
	SDL_WaitThread(remeshthreadB, 0);
	SDL_WaitThread(remeshthreadC, 0);
	SDL_WaitThread(remeshthreadD, 0);

	int3_t chunkindex;
	for(chunkindex.x=0; chunkindex.x<WORLDSIZE; ++chunkindex.x)
	for(chunkindex.y=0; chunkindex.y<WORLDSIZE; ++chunkindex.y)
	for(chunkindex.z=0; chunkindex.z<WORLDSIZE; ++chunkindex.z)
		chunk_freechunk(data[chunkindex.x][chunkindex.y][chunkindex.z].chunk);

	chunk_destroyindexbuffers();
}

void
world_render(vec3_t pos, GLuint modelmatrix)
{
	setworldcenter(pos);
	glEnable(GL_DEPTH_TEST);

	int x=0;
	int y=0;
	int z=0;

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);

	long points = 0;

	for(x=0; x<WORLDSIZE; ++x)
	for(y=0; y<WORLDSIZE; ++y)
	for(z=0; z<WORLDSIZE; ++z)
	{
		long3_t chunkpos = chunk_getworldpos(data[x][y][z].chunk);
		mat4_t matrix = gettranslatematrix(chunkpos.x - pos.x, chunkpos.y - pos.y, chunkpos.z - pos.z);
		glUniformMatrix4fv(modelmatrix, 1, GL_FALSE, matrix.mat);

		points += chunk_render(data[x][y][z].chunk);
	}

	totalpoints = points;
}

//TODO: loadnew
block_t
world_getblock(long x, long y, long z, int loadnew)
{
	long3_t cpos = world_getchunkposofworldpos(x, y, z);
	int3_t internalpos = world_getinternalposofworldpos(x,y,z);

	int3_t icpo;

	if(isquickloaded(cpos, &icpo))
		return chunk_getblock(data[icpo.x][icpo.y][icpo.z].chunk, internalpos.x, internalpos.y, internalpos.z);
	block_t error;
	error.id = ERR;
	return error;
}

//TODO: loadnew
blockid_t
world_getblockid(long x, long y, long z, int loadnew)
{
	long3_t cpos = world_getchunkposofworldpos(x, y, z);
	int3_t internalpos = world_getinternalposofworldpos(x,y,z);

	int3_t icpo;
	if(isquickloaded(cpos, &icpo))
		return chunk_getblockid(data[icpo.x][icpo.y][icpo.z].chunk, internalpos.x, internalpos.y, internalpos.z);

	return ERR;
}

//TODO: loadnew
int
world_setblock(long x, long y, long z, block_t block, int update, int loadnew, int instant)
{
	long3_t cpos = world_getchunkposofworldpos(x, y, z);
	int3_t internalpos = world_getinternalposofworldpos(x, y, z);

	int3_t chunkindex;
	if(isquickloaded(cpos, &chunkindex))
	{
		chunk_t *chunk = data[chunkindex.x][chunkindex.y][chunkindex.z].chunk;

		chunk_setblock(chunk, internalpos.x, internalpos.y, internalpos.z, block);

		if(update)
		{
			world_updatequeue(x,y,z, update-1, 0);
			world_updatequeue(x+1,y,z, update-1, 0);
			world_updatequeue(x,y+1,z, update-1, 0);
			world_updatequeue(x,y,z+1, update-1, 0);
			world_updatequeue(x-1,y,z, update-1, 0);
			world_updatequeue(x,y-1,z, update-1, 0);
			world_updatequeue(x,y,z-1, update-1, 0);
		}

		queueremesh(&chunkindex, instant);

		cpos.x++;
		if(internalpos.x == CHUNKSIZE-1 && isquickloaded(cpos, &chunkindex))
			queueremesh(&chunkindex, instant);

		cpos.x -= 2;
		if(internalpos.x == 0 && isquickloaded(cpos, &chunkindex))
			queueremesh(&chunkindex, instant);
		cpos.x++;

		cpos.y++;
		if(internalpos.y == CHUNKSIZE-1 && isquickloaded(cpos, &chunkindex))
			queueremesh(&chunkindex, instant);

		cpos.y -= 2;
		if(internalpos.y == 0 && isquickloaded(cpos, &chunkindex))
			queueremesh(&chunkindex, instant);
		cpos.y++;

		cpos.z++;
		if(internalpos.z == CHUNKSIZE-1 && isquickloaded(cpos, &chunkindex))
			queueremesh(&chunkindex, instant);

		cpos.z -= 2;
		if(internalpos.z == 0 && isquickloaded(cpos, &chunkindex))
			queueremesh(&chunkindex, instant);

		return 0;
	}
	return -1;
}

//TODO: loadnew
int
world_setblockid(long x, long y, long z, blockid_t id, int update, int loadnew, int instant)
{
	long3_t cpos = world_getchunkposofworldpos(x, y, z);
	int3_t internalpos = world_getinternalposofworldpos(x, y, z);

	int3_t chunkindex;
	if(isquickloaded(cpos, &chunkindex))
	{
		chunk_t *chunk = data[chunkindex.x][chunkindex.y][chunkindex.z].chunk;

		chunk_setblockid(chunk, internalpos.x, internalpos.y, internalpos.z, id);

		if(update)
		{
			world_updatequeue(x,y,z, update-1, 0);
			world_updatequeue(x+1,y,z, update-1, 0);
			world_updatequeue(x,y+1,z, update-1, 0);
			world_updatequeue(x,y,z+1, update-1, 0);
			world_updatequeue(x-1,y,z, update-1, 0);
			world_updatequeue(x,y-1,z, update-1, 0);
			world_updatequeue(x,y,z-1, update-1, 0);
		}

		queueremesh(&chunkindex, instant);

		cpos.x++;
		if(internalpos.x == CHUNKSIZE-1 && isquickloaded(cpos, &chunkindex))
			queueremesh(&chunkindex, instant);

		cpos.x -= 2;
		if(internalpos.x == 0 && isquickloaded(cpos, &chunkindex))
			queueremesh(&chunkindex, instant);
		cpos.x++;

		cpos.y++;
		if(internalpos.y == CHUNKSIZE-1 && isquickloaded(cpos, &chunkindex))
			queueremesh(&chunkindex, instant);

		cpos.y -= 2;
		if(internalpos.y == 0 && isquickloaded(cpos, &chunkindex))
			queueremesh(&chunkindex, instant);
		cpos.y++;

		cpos.z++;
		if(internalpos.z == CHUNKSIZE-1 && isquickloaded(cpos, &chunkindex))
			queueremesh(&chunkindex, instant);

		cpos.z -= 2;
		if(internalpos.z == 0 && isquickloaded(cpos, &chunkindex))
			queueremesh(&chunkindex, instant);

		return 0;
	}
	return -1;
}

uint32_t
world_getseed()
{
	return seed;
}

void
world_updatequeue(long x, long y, long z, uint8_t time, update_flags_t flags)
{
	long3_t cpos = world_getchunkposofworldpos(x, y, z);
	int3_t internalpos = world_getinternalposofworldpos(x, y, z);

	int3_t chunkindex;
	if(isquickloaded(cpos, &chunkindex))
	{
		chunk_t *chunk = data[chunkindex.x][chunkindex.y][chunkindex.z].chunk;

		chunk_updatequeue(chunk, internalpos.x, internalpos.y, internalpos.z, time, flags);
	}
}

long
world_updaterun()
{
	long num = 0;
	int x, y, z;
	for(x=0; x<WORLDSIZE; ++x)
	for(y=0; y<WORLDSIZE; ++y)
	for(z=0; z<WORLDSIZE; ++z)
		num += chunk_updaterun(data[x][y][z].chunk);

	return num;
}

long
world_gettrianglecount()
{
	return totalpoints / 3;
}
