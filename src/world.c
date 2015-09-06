#include "world.h"

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#include <SDL2/SDL_timer.h>

#include "defines.h"
#include "chunk.h"
#include "modulo.h"

long3_t worldscope = {0, 0, 0};
long3_t worldcenter = {0, 0, 0};

int stopthread;
SDL_Thread *thread;

GLuint termtexture;
unsigned char termscreen[128*128*3] = { 0 };

struct {
	chunk_t *chunk;
	SDL_mutex *lock;
	int iswritable;
} data[WORLDSIZE][WORLDSIZE][WORLDSIZE];

static inline int3_t
getinternalspotofspot(long x, long y, long z)
{
	int3_t blockpos;
	blockpos.x = MODULO(x, CHUNKSIZE);
	blockpos.y = MODULO(y, CHUNKSIZE);
	blockpos.z = MODULO(z, CHUNKSIZE);
	return blockpos;
}

static inline long3_t
getchunkofspot(long x, long y, long z)
{
	long3_t chunkpos;
	chunkpos.x = floor((double)x / CHUNKSIZE);
	chunkpos.y = floor((double)y / CHUNKSIZE);
	chunkpos.z = floor((double)z / CHUNKSIZE);
	return chunkpos;
}

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
	worldcenter = getchunkofspot(pos.x, pos.y, pos.z);
	worldscope.x = worldcenter.x - WORLDSIZE/2;
	worldscope.y = worldcenter.y - WORLDSIZE/2;
	worldscope.z = worldcenter.z - WORLDSIZE/2;
}

void
setnotwriteable(int3_t *chunkindex)
{
	SDL_LockMutex(data[chunkindex->x][chunkindex->y][chunkindex->z].lock);
	data[chunkindex->x][chunkindex->y][chunkindex->z].iswritable = 0;
	SDL_UnlockMutex(data[chunkindex->x][chunkindex->y][chunkindex->z].lock);
}

inline void
setwriteable(int3_t *chunkindex)
{
	data[chunkindex->x][chunkindex->y][chunkindex->z].iswritable = 1;
}

inline void
beginwrite(int3_t *chunkindex)
{
	SDL_LockMutex(data[chunkindex->x][chunkindex->y][chunkindex->z].lock);
}

inline void
endwrite(int3_t *chunkindex)
{
	SDL_UnlockMutex(data[chunkindex->x][chunkindex->y][chunkindex->z].lock);
}

static int
quickremeshachunk(int3_t *chunkindex, int instant)
{
	if(!instant)
	{
		chunk_setnotcurrent(data[chunkindex->x][chunkindex->y][chunkindex->z].chunk);
		return 0;
	}

	if(SDL_TryLockMutex(data[chunkindex->x][chunkindex->y][chunkindex->z].lock)!=0)
	{
		chunk_setnotcurrent(data[chunkindex->x][chunkindex->y][chunkindex->z].chunk);
		return -1;
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
		if(!SDL_TryLockMutex(data[MODULO(chunkindex->x+1,WORLDSIZE)][MODULO(chunkindex->y,WORLDSIZE)][MODULO(chunkindex->z,WORLDSIZE)].lock))
			east = data[tempchunkindex.x][tempchunkindex.y][tempchunkindex.z].chunk;
	}
	tempcpos.x -= 2;
	if(isquickloaded(tempcpos, &tempchunkindex))
	{
		if(!SDL_TryLockMutex(data[MODULO(chunkindex->x-1,WORLDSIZE)][MODULO(chunkindex->y,WORLDSIZE)][MODULO(chunkindex->z,WORLDSIZE)].lock))
			west = data[tempchunkindex.x][tempchunkindex.y][tempchunkindex.z].chunk;
	}
	tempcpos.x++;

	tempcpos.y++;
	if(isquickloaded(tempcpos, &tempchunkindex))
	{
		if(!SDL_TryLockMutex(data[MODULO(chunkindex->x,WORLDSIZE)][MODULO(chunkindex->y+1,WORLDSIZE)][MODULO(chunkindex->z,WORLDSIZE)].lock))
			up = data[tempchunkindex.x][tempchunkindex.y][tempchunkindex.z].chunk;
	}
	tempcpos.y -= 2;
	if(isquickloaded(tempcpos, &tempchunkindex))
	{
		if(!SDL_TryLockMutex(data[MODULO(chunkindex->x,WORLDSIZE)][MODULO(chunkindex->y-1,WORLDSIZE)][MODULO(chunkindex->z,WORLDSIZE)].lock))
			down = data[tempchunkindex.x][tempchunkindex.y][tempchunkindex.z].chunk;
	}
	tempcpos.y++;

	tempcpos.z++;
	if(isquickloaded(tempcpos, &tempchunkindex))
	{
		if(!SDL_TryLockMutex(data[MODULO(chunkindex->x,WORLDSIZE)][MODULO(chunkindex->y,WORLDSIZE)][MODULO(chunkindex->z+1,WORLDSIZE)].lock))
			south = data[tempchunkindex.x][tempchunkindex.y][tempchunkindex.z].chunk;
	}
	tempcpos.z -= 2;
	if(isquickloaded(tempcpos, &tempchunkindex))
	{
		if(!SDL_TryLockMutex(data[MODULO(chunkindex->x,WORLDSIZE)][MODULO(chunkindex->y,WORLDSIZE)][MODULO(chunkindex->z-1,WORLDSIZE)].lock))
			north = data[tempchunkindex.x][tempchunkindex.y][tempchunkindex.z].chunk;
	}


	//re set up the buffers
	chunk_remesh(chunk, up,down,north,south,east,west);

	SDL_UnlockMutex(data[chunkindex->x][chunkindex->y][chunkindex->z].lock);

	//TODO:optimize this and the locking above
	if(north)
		SDL_UnlockMutex(data[MODULO(chunkindex->x,WORLDSIZE)][MODULO(chunkindex->y,WORLDSIZE)][MODULO(chunkindex->z-1,WORLDSIZE)].lock);
	if(south)
		SDL_UnlockMutex(data[MODULO(chunkindex->x,WORLDSIZE)][MODULO(chunkindex->y,WORLDSIZE)][MODULO(chunkindex->z+1,WORLDSIZE)].lock);
	if(east)
		SDL_UnlockMutex(data[MODULO(chunkindex->x+1,WORLDSIZE)][MODULO(chunkindex->y,WORLDSIZE)][MODULO(chunkindex->z,WORLDSIZE)].lock);
	if(west)
		SDL_UnlockMutex(data[MODULO(chunkindex->x-1,WORLDSIZE)][MODULO(chunkindex->y,WORLDSIZE)][MODULO(chunkindex->z,WORLDSIZE)].lock);
	if(up)
		SDL_UnlockMutex(data[MODULO(chunkindex->x,WORLDSIZE)][MODULO(chunkindex->y+1,WORLDSIZE)][MODULO(chunkindex->z,WORLDSIZE)].lock);
	if(down)
		SDL_UnlockMutex(data[MODULO(chunkindex->x,WORLDSIZE)][MODULO(chunkindex->y-1,WORLDSIZE)][MODULO(chunkindex->z,WORLDSIZE)].lock);

	return 0;
}

int
world_threadentry(void *ptr)
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

						setnotwriteable(&chunkindex);

						chunk_reloadchunk(cpos, chunk);

						setwriteable(&chunkindex);

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

					//upload terminal textures
//					if(data[i.x][i.y][i.z].mappedptr && !data[i.x][i.y][i.z].termpbohasnewdata)//god forbid this thread runs faster than the render thread
//					{
//						memcpy(data[i.x][i.y][i.z].mappedptr, termscreen, 128*128*3);
//						data[i.x][i.y][i.z].termpbohasnewdata=1;
//					}
				}
			}
		}

		if(!stopthread)
			SDL_Delay(80);
	}
	return 0;
}

void
world_init(vec3_t pos)
{
	setworldcenter(pos);
//				glGenTextures(1, &termtexture);//TODO: term texture cleanup
//				glBindTexture(GL_TEXTURE_2D, termtexture);
//				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
//				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
//
	long3_t cpos;
	for(cpos.x = worldscope.x; cpos.x< worldscope.x+WORLDSIZE; cpos.x++)
	{
		for(cpos.z = worldscope.z; cpos.z< worldscope.z+WORLDSIZE; cpos.z++)
		{
			for(cpos.y = worldscope.y; cpos.y< worldscope.y+WORLDSIZE; cpos.y++)
			{
				int3_t chunkindex = getchunkindexofchunk(cpos);
				data[chunkindex.x][chunkindex.y][chunkindex.z].iswritable=1;
				data[chunkindex.x][chunkindex.y][chunkindex.z].lock = SDL_CreateMutex();
				data[chunkindex.x][chunkindex.y][chunkindex.z].chunk = chunk_loadchunk(cpos);
			}
		}
	}

	thread = SDL_CreateThread(world_threadentry, "world", 0);
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
				SDL_DestroyMutex(data[chunkindex.x][chunkindex.y][chunkindex.z].lock);
				chunk_freechunk(data[chunkindex.x][chunkindex.y][chunkindex.z].chunk);
			}
		}
	}
}

void
world_render(GLuint drawprogram, GLuint terminalscreensprogram, vec3_t pos)
{
	setworldcenter(pos);
	glEnable(GL_DEPTH_TEST);

	int x=0;
	int y=0;
	int z=0;

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);

	glUseProgram(drawprogram);

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

//	glUseProgram(terminalscreensprogram);
//	for(x=0; x<WORLDSIZE; x++)
//	{
//		for(y=0; y<WORLDSIZE; y++)
//		{
//			for(z=0; z<WORLDSIZE; z++)
//			{
//				long double dist;
//
//				long3_t chunkpos = chunk_getpos(data[x][y][z].chunk);
//				distlong3(&dist, &chunkpos, &worldcenter);
//
//				if(dist < TERMINALRENDERDIST)
//				{
//					glActiveTexture(GL_TEXTURE0);
//					glBindTexture(GL_TEXTURE_2D, termtexture);
//
//					if(data[x][y][z].termpboloadnexttime)
//					{
//						glBindBuffer(GL_PIXEL_UNPACK_BUFFER, data[x][y][z].termpbo);
//						glBufferData(GL_PIXEL_UNPACK_BUFFER, 128*128*3, 0, GL_STREAM_DRAW);
//
//						data[x][y][z].mappedptr = glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
//						glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
//
//						data[x][y][z].termpboloadnexttime=0;
//					}
//
//					if(!data[x][y][z].mappedptr)
//						data[x][y][z].termpboloadnexttime=1;
//
//					data[x][y][z].termcounter++;
//					if(data[x][y][z].termpbohasnewdata && (data[x][y][z].termcounter > 1000))
//					{
//						data[x][y][z].termcounter=0;
//
//						glBindBuffer(GL_PIXEL_UNPACK_BUFFER, data[x][y][z].termpbo);
//
//						glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
//						data[x][y][z].mappedptr=0;
//
//						glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 128, 128, 0,  GL_RGB, GL_UNSIGNED_BYTE, 0);
//						glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
//
//						data[x][y][z].termpbohasnewdata=0;
//					}
//
//					//TODO: put this in a proper place
//					GLint uniform_mytexture = glGetUniformLocation(terminalscreensprogram, "myTextureSampler");
//					glUniform1i(uniform_mytexture, 0);
//					glBindBuffer(GL_ARRAY_BUFFER, data[x][y][z].termbo);
//					glVertexAttribPointer(
//							0,
//							3,
//							GL_FLOAT,
//							GL_FALSE,
//							5 * sizeof(GLfloat),//*sizeof(GLfloat),
//							0);
//
//					glVertexAttribPointer(
//							1,
//							2,
//							GL_FLOAT,
//							GL_FALSE,
//							5*sizeof(GLfloat),
//							(void *)(3*sizeof(GLfloat)));
//					glDrawArrays(GL_TRIANGLES, 0, data[x][y][z].termpoints);
//				}
//			}
//		}
//	}
}

//TODO: loadnew
block_t
world_getblock(long x, long y, long z, int loadnew)
{
	long3_t cpos = getchunkofspot(x, y, z);
	int3_t internalpos = getinternalspotofspot(x,y,z);

	int3_t icpo;

	if(isquickloaded(cpos, &icpo))
		return chunk_getblock(data[icpo.x][icpo.y][icpo.z].chunk, internalpos.x, internalpos.y, internalpos.z);
	block_t error;
	error.id = 255;
	return error;
}

//TODO: loadnew
int
world_setblock(long x, long y, long z, block_t block, int loadnew, int instant)
{
	long3_t cpos = getchunkofspot(x, y, z);
	int3_t internalpos = getinternalspotofspot(x, y, z);

	int3_t chunkindex;
	if(isquickloaded(cpos, &chunkindex))
	{
		chunk_t *chunk = data[chunkindex.x][chunkindex.y][chunkindex.z].chunk;
		if(!data[chunkindex.x][chunkindex.y][chunkindex.z].iswritable)
			return -2;

		beginwrite(&chunkindex);
		chunk_setblock(chunk, internalpos.x, internalpos.y, internalpos.z, block);
		endwrite(&chunkindex);

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
