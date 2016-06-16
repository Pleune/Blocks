#include "world.h"

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <limits.h>

#include <SDL_timer.h>

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
} data[WORLD_CHUNKS_PER_EDGE][WORLD_CHUNKS_PER_EDGE][WORLD_CHUNKS_PER_EDGE];

static inline long3_t
get_worldpos_from_chunkpos(long3_t *cpos)
{
	long3_t ret;
	ret.x = cpos->x * CHUNKSIZE;
	ret.y = cpos->y * CHUNKSIZE;
	ret.z = cpos->z * CHUNKSIZE;
	return ret;
}

inline static long3_t
get_worldpos_from_internalpos(long3_t *cpos, int x, int y, int z)
{
	long3_t ret;
	ret.x = cpos->x * CHUNKSIZE + x;
	ret.y = cpos->y * CHUNKSIZE + y;
	ret.z = cpos->z * CHUNKSIZE + z;
	return ret;
}

static inline int3_t
getchunkindexofchunk(long3_t pos)
{
	int3_t icpo = {
		MODULO(pos.x, WORLD_CHUNKS_PER_EDGE),
		MODULO(pos.y, WORLD_CHUNKS_PER_EDGE),
		MODULO(pos.z, WORLD_CHUNKS_PER_EDGE)
	};
	return icpo;
}

static inline int
shouldbequickloaded(long3_t pos)
{
	return worldscope.x <= pos.x && pos.x < worldscope.x + WORLD_CHUNKS_PER_EDGE &&
		worldscope.y <= pos.y && pos.y < worldscope.y + WORLD_CHUNKS_PER_EDGE &&
		worldscope.z <= pos.z && pos.z < worldscope.z + WORLD_CHUNKS_PER_EDGE;
}

static inline int
isquickloaded(long3_t pos, int3_t *chunkindex)
{
	int3_t ci = getchunkindexofchunk(pos);
	if(chunkindex)
		*chunkindex = ci;

	long3_t cpos = chunk_pos_get(data[ci.x][ci.y][ci.z].chunk);
	return shouldbequickloaded(cpos) && !memcmp(&cpos, &pos, sizeof(long3_t));
}

static void
setworldcenter(vec3_t pos)
{
	worldcenterpos = pos;

	worldcenter = world_get_chunkpos_of_worldpos(pos.x, pos.y, pos.z);
	worldscope.x = worldcenter.x - WORLD_CHUNKS_PER_EDGE/2;
	worldscope.y = worldcenter.y - WORLD_CHUNKS_PER_EDGE/2;
	worldscope.z = worldcenter.z - WORLD_CHUNKS_PER_EDGE/2;
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
	long3_t tempcpos = chunk_pos_get(chunk);

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

	chunk_mesh_clear_current(data[chunkindex->x][chunkindex->y][chunkindex->z].chunk);
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

						chunk_mesh_clear_current(data[chunkindex.x == WORLD_CHUNKS_PER_EDGE-1 ? 0 : chunkindex.x+1][chunkindex.y][chunkindex.z].chunk);
						chunk_mesh_clear_current(data[chunkindex.x == 0 ? WORLD_CHUNKS_PER_EDGE-1 : chunkindex.x-1][chunkindex.y][chunkindex.z].chunk);
						chunk_mesh_clear_current(data[chunkindex.x][chunkindex.y == WORLD_CHUNKS_PER_EDGE-1 ? 0 : chunkindex.y+1][chunkindex.z].chunk);
						chunk_mesh_clear_current(data[chunkindex.x][chunkindex.y == 0 ? WORLD_CHUNKS_PER_EDGE-1 : chunkindex.y-1][chunkindex.z].chunk);
						chunk_mesh_clear_current(data[chunkindex.x][chunkindex.y][chunkindex.z == WORLD_CHUNKS_PER_EDGE-1 ? 0 : chunkindex.z+1].chunk);
						chunk_mesh_clear_current(data[chunkindex.x][chunkindex.y][chunkindex.z == 0 ? WORLD_CHUNKS_PER_EDGE-1 : chunkindex.z-1].chunk);

						if(counter)
						{
							++(*counter);

							char string[] = "                    ";
							float percent = (float)(*counter)/(WORLD_CHUNKS_PER_EDGE*WORLD_CHUNKS_PER_EDGE*WORLD_CHUNKS_PER_EDGE);
							memset(string, '#', (sizeof(string) - 1) * percent);
							printf("LOADING... [%s] %f%%\r", string, percent * 100.0f);
							if(*counter == (WORLD_CHUNKS_PER_EDGE*WORLD_CHUNKS_PER_EDGE*WORLD_CHUNKS_PER_EDGE))
								putchar('\n');
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
		for(i.x = 0; i.x< WORLD_CHUNKS_PER_EDGE; ++i.x)
		{
			if(stopthreads)
				break;
			for(i.y = 0; i.y< WORLD_CHUNKS_PER_EDGE; ++i.y)
			{
				if(stopthreads)
					break;
				for(i.z = 0; i.z< WORLD_CHUNKS_PER_EDGE; ++i.z)
				{
					if(stopthreads)
						break;

					if(!chunk_mesh_is_current(data[i.x][i.y][i.z].chunk))
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
			worldcenter.x - WORLD_CHUNKS_PER_EDGE/6,
			worldcenter.y - WORLD_CHUNKS_PER_EDGE/6,
			worldcenter.z - WORLD_CHUNKS_PER_EDGE/6
		};
		long3_t highbound = {
			worldcenter.x + WORLD_CHUNKS_PER_EDGE/6,
			worldcenter.y + WORLD_CHUNKS_PER_EDGE/6,
			worldcenter.z + WORLD_CHUNKS_PER_EDGE/6
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

					if(!chunk_mesh_is_current(data[icpo.x][icpo.y][icpo.z].chunk))
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
		for(i.x = -WORLD_CHUNKS_PER_EDGE; i.x < WORLD_CHUNKS_PER_EDGE; ++i.x)
		{
			if(stopthreads)
				break;
			for(i.y = -WORLD_CHUNKS_PER_EDGE; i.y < WORLD_CHUNKS_PER_EDGE; ++i.y)
			{
				if(stopthreads)
					break;
				for(i.z = -WORLD_CHUNKS_PER_EDGE; i.z < WORLD_CHUNKS_PER_EDGE; ++i.z)
				{
					if(stopthreads)
						break;

					vec3_t begin_point = worldcenterpos;
					begin_point.y += PLAYER_EYEHEIGHT;

					long3_t pos = world_ray_pos(&begin_point, &i, 0, 1000);
					pos = world_get_chunkpos_of_worldpos(pos.x, pos.y, pos.z);

					int3_t icpo = getchunkindexofchunk(pos);

					if(!chunk_mesh_is_current(data[icpo.x][icpo.y][icpo.z].chunk))
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
		for(i.x = 0; i.x< WORLD_CHUNKS_PER_EDGE; ++i.x)
		{
			if(stopthreads)
				break;
			for(i.y = 0; i.y< WORLD_CHUNKS_PER_EDGE; ++i.y)
			{
				if(stopthreads)
					break;
				for(i.z = 0; i.z< WORLD_CHUNKS_PER_EDGE; ++i.z)
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
world_seed_gen()
{
	time_t t;
	srand((unsigned) time(&t));
	seed = rand();
}

void
world_init(vec3_t pos)
{
	setworldcenter(pos);
	chunk_static_init();

	int3_t cpos;
	for(cpos.x = 0; cpos.x<WORLD_CHUNKS_PER_EDGE; ++cpos.x)
	for(cpos.z = 0; cpos.z<WORLD_CHUNKS_PER_EDGE; ++cpos.z)
	for(cpos.y = 0; cpos.y<WORLD_CHUNKS_PER_EDGE; ++cpos.y)
	{
		long3_t long3max = {LONG_MAX, LONG_MAX, LONG_MAX};

		data[cpos.x][cpos.y][cpos.z].chunk = chunk_load_empty(long3max);
		data[cpos.x][cpos.y][cpos.z].instantremesh = 0;
	}

	stopthreads = 0;
	int wgcounter = 0;
	struct world_genthread_s wginfo = { 0, 0, 0,
		{0, 0, 0},
		{WORLD_CHUNKS_PER_EDGE, WORLD_CHUNKS_PER_EDGE, WORLD_CHUNKS_PER_EDGE}
	};
	wginfo.initalized = SDL_CreateSemaphore(0);
	wginfo.counter = &wgcounter;

	SDL_Thread *wgthreads[INIT_WORLDGEN_THREADS];
	worldgen_t *wgcontexts[INIT_WORLDGEN_THREADS];

	int i;
	for(i=0; i<INIT_WORLDGEN_THREADS; ++i)
	{
		wginfo.low.x = (WORLD_CHUNKS_PER_EDGE / (double)INIT_WORLDGEN_THREADS)*i;
		wginfo.high.x = (WORLD_CHUNKS_PER_EDGE / (double)INIT_WORLDGEN_THREADS)*(i+1);

		wginfo.context = worldgen_context_create();
		wgcontexts[i] = wginfo.context;

		wgthreads[i] = SDL_CreateThread(generationthreadfunc, "world_generation_init", &wginfo);
		SDL_SemWait(wginfo.initalized);
	}

	for(i=0; i<INIT_WORLDGEN_THREADS; ++i)
	{
		SDL_WaitThread(wgthreads[i], 0);
		worldgen_context_destroy(wgcontexts[i]);
	}

	wginfo.continuous = 1;
	wginfo.low.x = 0;
	wginfo.high.x = WORLD_CHUNKS_PER_EDGE;
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
	for(chunkindex.x=0; chunkindex.x<WORLD_CHUNKS_PER_EDGE; ++chunkindex.x)
	for(chunkindex.y=0; chunkindex.y<WORLD_CHUNKS_PER_EDGE; ++chunkindex.y)
	for(chunkindex.z=0; chunkindex.z<WORLD_CHUNKS_PER_EDGE; ++chunkindex.z)
		chunk_free(data[chunkindex.x][chunkindex.y][chunkindex.z].chunk);

	chunk_static_cleanup();
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

	for(x=0; x<WORLD_CHUNKS_PER_EDGE; ++x)
	for(y=0; y<WORLD_CHUNKS_PER_EDGE; ++y)
	for(z=0; z<WORLD_CHUNKS_PER_EDGE; ++z)
	{
		long3_t chunkpos = chunk_pos_get(data[x][y][z].chunk);
		long3_t worldpos = get_worldpos_from_chunkpos(&chunkpos);
		mat4_t matrix = gettranslatematrix(worldpos.x - pos.x, worldpos.y - pos.y, worldpos.z - pos.z);
		glUniformMatrix4fv(modelmatrix, 1, GL_FALSE, matrix.mat);

		points += chunk_render(data[x][y][z].chunk);
	}

	totalpoints = points;
}

//TODO: loadnew
block_t
world_block_get(long x, long y, long z, int loadnew)
{
	long3_t cpos = world_get_chunkpos_of_worldpos(x, y, z);
	int3_t internalpos = world_get_internalpos_of_worldpos(x,y,z);

	int3_t icpo;

	if(isquickloaded(cpos, &icpo))
		return chunk_block_get(data[icpo.x][icpo.y][icpo.z].chunk, internalpos.x, internalpos.y, internalpos.z);
	block_t error;
	error.id = ERR;
	return error;
}

//TODO: loadnew
blockid_t
world_block_get_id(long x, long y, long z, int loadnew)
{
	long3_t cpos = world_get_chunkpos_of_worldpos(x, y, z);
	int3_t internalpos = world_get_internalpos_of_worldpos(x,y,z);

	int3_t icpo;
	if(isquickloaded(cpos, &icpo))
		return chunk_block_get_id(data[icpo.x][icpo.y][icpo.z].chunk, internalpos.x, internalpos.y, internalpos.z);

	return ERR;
}

//TODO: loadnew
int
world_block_set(long x, long y, long z, block_t block, int update, int loadnew, int instant)
{
	long3_t cpos = world_get_chunkpos_of_worldpos(x, y, z);
	int3_t internalpos = world_get_internalpos_of_worldpos(x, y, z);

	int3_t chunkindex;
	if(isquickloaded(cpos, &chunkindex))
	{
		chunk_t *chunk = data[chunkindex.x][chunkindex.y][chunkindex.z].chunk;

		chunk_block_set(chunk, internalpos.x, internalpos.y, internalpos.z, block);

		if(update)
		{
			world_update_queue(x,y,z, update-1, 0);
			world_update_queue(x+1,y,z, update-1, 0);
			world_update_queue(x,y+1,z, update-1, 0);
			world_update_queue(x,y,z+1, update-1, 0);
			world_update_queue(x-1,y,z, update-1, 0);
			world_update_queue(x,y-1,z, update-1, 0);
			world_update_queue(x,y,z-1, update-1, 0);
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
world_block_set_id(long x, long y, long z, blockid_t id, int update, int loadnew, int instant)
{
	long3_t cpos = world_get_chunkpos_of_worldpos(x, y, z);
	int3_t internalpos = world_get_internalpos_of_worldpos(x, y, z);

	int3_t chunkindex;
	if(isquickloaded(cpos, &chunkindex))
	{
		chunk_t *chunk = data[chunkindex.x][chunkindex.y][chunkindex.z].chunk;

		chunk_block_set_id(chunk, internalpos.x, internalpos.y, internalpos.z, id);

		if(update)
		{
			world_update_queue(x,y,z, update-1, 0);
			world_update_queue(x+1,y,z, update-1, 0);
			world_update_queue(x,y+1,z, update-1, 0);
			world_update_queue(x,y,z+1, update-1, 0);
			world_update_queue(x-1,y,z, update-1, 0);
			world_update_queue(x,y-1,z, update-1, 0);
			world_update_queue(x,y,z-1, update-1, 0);
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
world_get_seed()
{
	return seed;
}

void
world_update_queue(long x, long y, long z, uint8_t time, update_flags_t flags)
{
	long3_t cpos = world_get_chunkpos_of_worldpos(x, y, z);
	int3_t internalpos = world_get_internalpos_of_worldpos(x, y, z);

	int3_t chunkindex;
	if(isquickloaded(cpos, &chunkindex))
	{
		chunk_t *chunk = data[chunkindex.x][chunkindex.y][chunkindex.z].chunk;

		chunk_update_queue(chunk, internalpos.x, internalpos.y, internalpos.z, time, flags);
	}
}

long
world_update_flush()
{
	long num = 0;
	int x, y, z;
	for(x=0; x<WORLD_CHUNKS_PER_EDGE; ++x)
	for(y=0; y<WORLD_CHUNKS_PER_EDGE; ++y)
	for(z=0; z<WORLD_CHUNKS_PER_EDGE; ++z)
		num += chunk_update_run(data[x][y][z].chunk);

	return num;
}

long
world_get_trianglecount()
{
	return totalpoints / 3;
}
