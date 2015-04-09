#include "world.h"

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#include <SDL2/SDL_timer.h>

#include "defines.h"
#include "chunk.h"
#include "custommath.h"

#define MODULO(a, b) (((a) % (b) + (b)) % (b))

chunk_t *loadedchunks[WORLDSIZE * WORLDSIZE * WORLDSIZE];
long3_t worldscope = {0, 0, 0};

int stopthread;
SDL_Thread *thread;

GLuint termtexture;
unsigned char termscreen[128*128*3] = { 0 };

struct {
	GLuint vbo;
	GLuint cbo;
	GLuint ebo;
	GLuint termbo;
	int termpoints;

	GLuint termpbo;
	void *mappedptr;
	int termpboloadnexttime;
	int termpbohasnewdata;

	int iscurrent;
	long points;
	mesh_t mesh;
	int ismeshcurrent;
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
	int u, v;
	for(u=0; u<128; u++)
	{
		for(v=0; v<128; v++)
		{
			termscreen[u*v*3]=u;
			termscreen[u*v*3+1]=v;
			termscreen[u*v*3+2]=0;
		}
	}

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
				glGenBuffers(1, &blockvbos[x][y][z].termbo);
				glGenBuffers(1, &blockvbos[x][y][z].termpbo);
				glGenTextures(1, &termtexture);//TODO: cleanup

				blockvbos[x][y][z].mappedptr=0;
				blockvbos[x][y][z].termpboloadnexttime=1;
				blockvbos[x][y][z].termpbohasnewdata=0;

				blockvbos[x][y][z].points = 0;
				blockvbos[x][y][z].iscurrent = 0;
				blockvbos[x][y][z].ismeshcurrent=1;

				blockvbos[x][y][z].mesh.vbodata=0;
				blockvbos[x][y][z].mesh.ebodata=0;
				blockvbos[x][y][z].mesh.colordata=0;
				blockvbos[x][y][z].mesh.termscreendata=0;

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
	thread = SDL_CreateThread(world_threadentry, "world", 0);
}

void
world_cleanup()
{
	stopthread=1;

	int ret;
	SDL_WaitThread(thread, &ret);

	int x, y, z;
	for(x=0; x<WORLDSIZE; x++)
	{
		for(y=0; y<WORLDSIZE; y++)
		{
			for(z=0; z<WORLDSIZE; z++)
			{
				glDeleteBuffers(1, &blockvbos[x][y][z].cbo);
				glDeleteBuffers(1, &blockvbos[x][y][z].vbo);
				glDeleteBuffers(1, &blockvbos[x][y][z].ebo);
				glDeleteBuffers(1, &blockvbos[x][y][z].termbo);
				SDL_DestroyMutex(blockvbos[x][y][z].lock);
				chunk_freechunk(loadedchunks[x + y*WORLDSIZE + z*WORLDSIZE*WORLDSIZE]);
				if(!blockvbos[x][y][z].ismeshcurrent)
				{
					free(blockvbos[x][y][z].mesh.ebodata);
					free(blockvbos[x][y][z].mesh.vbodata);
					free(blockvbos[x][y][z].mesh.colordata);
				}
			}
		}
	}
}

void
world_render(GLuint drawprogram, GLuint terminalscreensprogram)
{
	glEnable(GL_DEPTH_TEST);

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

					//terminal buffer
						glBindBuffer(GL_ARRAY_BUFFER, blockvbos[x][y][z].termbo);
						glBufferData(GL_ARRAY_BUFFER, blockvbos[x][y][z].mesh.termscreensize * sizeof(GLfloat), blockvbos[x][y][z].mesh.termscreendata, GL_STATIC_DRAW);

					if(blockvbos[x][y][z].mesh.ebodata)
						free(blockvbos[x][y][z].mesh.ebodata);
					if(blockvbos[x][y][z].mesh.vbodata)
						free(blockvbos[x][y][z].mesh.vbodata);
					if(blockvbos[x][y][z].mesh.colordata)
						free(blockvbos[x][y][z].mesh.colordata);
					if(blockvbos[x][y][z].mesh.termscreendata)
						free(blockvbos[x][y][z].mesh.termscreendata);
					blockvbos[x][y][z].ismeshcurrent=1;
				}

				glUseProgram(drawprogram);
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
				glDisableVertexAttribArray(0);
				glDisableVertexAttribArray(1);

				glUseProgram(terminalscreensprogram);

				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, termtexture);
				if(blockvbos[x][y][z].termpboloadnexttime)
				{
					glBindBuffer(GL_PIXEL_UNPACK_BUFFER, blockvbos[x][y][z].termpbo);
					glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, 128*128*3, 0, GL_STREAM_DRAW_ARB);
					blockvbos[x][y][z].mappedptr = glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
					glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
					blockvbos[x][y][z].termpboloadnexttime=0;
					int i = glGetError();
					while(i)
					{
						printf("GLERR: %i\n", i);
						i=glGetError();
					}
				}
				if(!blockvbos[x][y][z].mappedptr)
					blockvbos[x][y][z].termpboloadnexttime=1;
					blockvbos[x][y][z].termpboloadnexttime=0;

				if(blockvbos[x][y][z].termpbohasnewdata)
				{
					glBindBuffer(GL_PIXEL_UNPACK_BUFFER, blockvbos[x][y][z].termpbo);
						glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
						blockvbos[x][y][z].mappedptr=0;
					glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 128, 128, 0,  GL_RGB, GL_UNSIGNED_BYTE, 0);
					glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
					blockvbos[x][y][z].termpbohasnewdata=0;
					blockvbos[x][y][z].termpboloadnexttime=1;
				}

				//TODO: put this in a proper place
				GLint uniform_mytexture = glGetUniformLocation(terminalscreensprogram, "myTextureSampler");
				glUniform1i(uniform_mytexture, 0);
				glBindBuffer(GL_ARRAY_BUFFER, blockvbos[x][y][z].termbo);
				glVertexAttribPointer(
						0,
						3,
						GL_FLOAT,
						GL_FALSE,
						5 * sizeof(GLfloat),//*sizeof(GLfloat),
						0);
				glEnableVertexAttribArray(0);

				glVertexAttribPointer(
						1,
						2,
						GL_FLOAT,
						GL_FALSE,
						5*sizeof(GLfloat),
						(void *)(3*sizeof(GLfloat)));
				glEnableVertexAttribArray(1);
				glDrawArrays(GL_TRIANGLES, 0, blockvbos[x][y][z].termpoints);
				glDisableVertexAttribArray(0);
				glDisableVertexAttribArray(1);
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
	blockvbos[i->x][i->y][i->z].mesh = chunk_getmesh(loadedchunks[getchunkarrayspotof(i->x,i->y,i->z)], up,down,north,south,east,west);

	blockvbos[i->x][i->y][i->z].iscurrent = 1;
	blockvbos[i->x][i->y][i->z].ismeshcurrent=0;
	blockvbos[i->x][i->y][i->z].points = blockvbos[i->x][i->y][i->z].mesh.ebosize;
	blockvbos[i->x][i->y][i->z].termpoints = blockvbos[i->x][i->y][i->z].mesh.termscreensize / 5;
	SDL_UnlockMutex(blockvbos[i->x][i->y][i->z].lock);

	//TODO:optimize this and the locking above
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
	stopthread=0;
	while(!stopthread)
	{

		long3_t cpos;

		for(cpos.x = worldscope.x; cpos.x< worldscope.x+WORLDSIZE; cpos.x++)
		{
			if(stopthread)
				break;
			for(cpos.y = worldscope.y; cpos.y< worldscope.y+WORLDSIZE; cpos.y++)
			{
				if(stopthread)
					break;
				for(cpos.z = worldscope.z; cpos.z< worldscope.z+WORLDSIZE; cpos.z++)
				{
					if(stopthread)
						break;
					long arrindex;
					if(!isquickloaded(cpos, &arrindex))
					{
						if(stopthread)
							break;
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

						blockvbos[bp.x == WORLDSIZE-1 ? 0 : bp.x+1][bp.y][bp.z].iscurrent = 0;
						blockvbos[bp.x == 0 ? WORLDSIZE-1 : bp.x-1][bp.y][bp.z].iscurrent = 0;
						blockvbos[bp.x][bp.y == WORLDSIZE-1 ? 0 : bp.y+1][bp.z].iscurrent = 0;
						blockvbos[bp.x][bp.y == 0 ? WORLDSIZE-1 : bp.y-1][bp.z].iscurrent = 0;
						blockvbos[bp.x][bp.y][bp.z == WORLDSIZE-1 ? 0 : bp.z+1].iscurrent = 0;
						blockvbos[bp.x][bp.y][bp.z == 0 ? WORLDSIZE-1 : bp.z-1].iscurrent = 0;
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
					if(!blockvbos[i.x][i.y][i.z].iscurrent)
					{
						quickremeshachunk(&i);

					}
					//upload terminal textures
					if(blockvbos[i.x][i.y][i.z].mappedptr && !blockvbos[i.x][i.y][i.z].termpbohasnewdata)//god forbid this thread runs faster than the render thread
					{
						memcpy(blockvbos[i.x][i.y][i.z].mappedptr, termscreen, 128*128*3);
						blockvbos[i.x][i.y][i.z].termpbohasnewdata=1;
						printf("TEST\n");
					}
				}
			}
		}

		if(!stopthread)
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

			quickremeshachunk(&i);

			//TODO: if a block is placed on a chunk boundry, update the chunk next door
			return 0;
		}
		return -3;
	}
	return -1;
}
