#include "world.h"

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#include <SDL2/SDL_timer.h>

#include "defines.h"
#include "chunk.h"

#define MODULO(a, b) (((a) % (b) + (b)) % (b))

long3_t worldscope = {0, 0, 0};
long3_t worldcenter = {0, 0, 0};

int stopthread;
SDL_Thread *thread;

GLuint termtexture;
unsigned char termscreen[128*128*3] = { 0 };

struct {
	chunk_t *chunk;

	GLuint vbo;
	GLuint cbo;
	GLuint ebo;
	GLuint termbo;
	int termpoints;

	GLuint termpbo;
	void *mappedptr;
	int termpboloadnexttime;
	int termpbohasnewdata;
	int termcounter;

	int iscurrent;
	long points;
	mesh_t mesh;
	int ismeshcurrent;
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

static int
quickremeshachunk(int3_t *chunkpos)
{
	if(SDL_TryLockMutex(data[chunkpos->x][chunkpos->y][chunkpos->z].lock)!=0)
	{
		printf("Xzz: %i Yzz: %i Zzz: %i\n", chunkpos->x, chunkpos->y, chunkpos->z);
		data[chunkpos->x][chunkpos->y][chunkpos->z].iscurrent=0;
		return -1;
	}

	chunk_t *north=0;
	chunk_t *south=0;
	chunk_t *east=0;
	chunk_t *west=0;
	chunk_t *up=0;
	chunk_t *down=0;

	chunk_t *chunk = data[chunkpos->x][chunkpos->y][chunkpos->z].chunk;

	int3_t tempchunkindex;
	long3_t tempcpos = chunk_getpos(chunk);

	tempcpos.x++;
	if(isquickloaded(tempcpos, &tempchunkindex))
	{
		if(!SDL_TryLockMutex(data[MODULO(chunkpos->x+1,WORLDSIZE)][MODULO(chunkpos->y,WORLDSIZE)][MODULO(chunkpos->z,WORLDSIZE)].lock))
			east = data[tempchunkindex.x][tempchunkindex.y][tempchunkindex.z].chunk;
	}
	tempcpos.x -= 2;
	if(isquickloaded(tempcpos, &tempchunkindex))
	{
		if(!SDL_TryLockMutex(data[MODULO(chunkpos->x-1,WORLDSIZE)][MODULO(chunkpos->y,WORLDSIZE)][MODULO(chunkpos->z,WORLDSIZE)].lock))
			west = data[tempchunkindex.x][tempchunkindex.y][tempchunkindex.z].chunk;
	}
	tempcpos.x++;

	tempcpos.y++;
	if(isquickloaded(tempcpos, &tempchunkindex))
	{
		if(!SDL_TryLockMutex(data[MODULO(chunkpos->x,WORLDSIZE)][MODULO(chunkpos->y+1,WORLDSIZE)][MODULO(chunkpos->z,WORLDSIZE)].lock))
			up = data[tempchunkindex.x][tempchunkindex.y][tempchunkindex.z].chunk;
	}
	tempcpos.y -= 2;
	if(isquickloaded(tempcpos, &tempchunkindex))
	{
		if(!SDL_TryLockMutex(data[MODULO(chunkpos->x,WORLDSIZE)][MODULO(chunkpos->y-1,WORLDSIZE)][MODULO(chunkpos->z,WORLDSIZE)].lock))
			down = data[tempchunkindex.x][tempchunkindex.y][tempchunkindex.z].chunk;
	}
	tempcpos.y++;

	tempcpos.z++;
	if(isquickloaded(tempcpos, &tempchunkindex))
	{
		if(!SDL_TryLockMutex(data[MODULO(chunkpos->x,WORLDSIZE)][MODULO(chunkpos->y,WORLDSIZE)][MODULO(chunkpos->z+1,WORLDSIZE)].lock))
			south = data[tempchunkindex.x][tempchunkindex.y][tempchunkindex.z].chunk;
	}
	tempcpos.z -= 2;
	if(isquickloaded(tempcpos, &tempchunkindex))
	{
		if(!SDL_TryLockMutex(data[MODULO(chunkpos->x,WORLDSIZE)][MODULO(chunkpos->y,WORLDSIZE)][MODULO(chunkpos->z-1,WORLDSIZE)].lock))
			north = data[tempchunkindex.x][tempchunkindex.y][tempchunkindex.z].chunk;
	}


	//make sure the other thread dosent do anything stupid
	data[chunkpos->x][chunkpos->y][chunkpos->z].ismeshcurrent=1;

	//re set up the buffers
	data[chunkpos->x][chunkpos->y][chunkpos->z].mesh = chunk_getmesh(chunk, up,down,north,south,east,west);

	data[chunkpos->x][chunkpos->y][chunkpos->z].iscurrent = 1;
	data[chunkpos->x][chunkpos->y][chunkpos->z].ismeshcurrent=0;
	data[chunkpos->x][chunkpos->y][chunkpos->z].points = data[chunkpos->x][chunkpos->y][chunkpos->z].mesh.ebosize;
	data[chunkpos->x][chunkpos->y][chunkpos->z].termpoints = data[chunkpos->x][chunkpos->y][chunkpos->z].mesh.termscreensize / 5;
	SDL_UnlockMutex(data[chunkpos->x][chunkpos->y][chunkpos->z].lock);

	//TODO:optimize this and the locking above
	if(north)
		SDL_UnlockMutex(data[MODULO(chunkpos->x,WORLDSIZE)][MODULO(chunkpos->y,WORLDSIZE)][MODULO(chunkpos->z-1,WORLDSIZE)].lock);
	if(south)
		SDL_UnlockMutex(data[MODULO(chunkpos->x,WORLDSIZE)][MODULO(chunkpos->y,WORLDSIZE)][MODULO(chunkpos->z+1,WORLDSIZE)].lock);
	if(east)
		SDL_UnlockMutex(data[MODULO(chunkpos->x+1,WORLDSIZE)][MODULO(chunkpos->y,WORLDSIZE)][MODULO(chunkpos->z,WORLDSIZE)].lock);
	if(west)
		SDL_UnlockMutex(data[MODULO(chunkpos->x-1,WORLDSIZE)][MODULO(chunkpos->y,WORLDSIZE)][MODULO(chunkpos->z,WORLDSIZE)].lock);
	if(up)
		SDL_UnlockMutex(data[MODULO(chunkpos->x,WORLDSIZE)][MODULO(chunkpos->y+1,WORLDSIZE)][MODULO(chunkpos->z,WORLDSIZE)].lock);
	if(down)
		SDL_UnlockMutex(data[MODULO(chunkpos->x,WORLDSIZE)][MODULO(chunkpos->y-1,WORLDSIZE)][MODULO(chunkpos->z,WORLDSIZE)].lock);

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
					int3_t chunkindex;
					if(!isquickloaded(cpos, &chunkindex))
					{
						if(stopthread)
							break;
						//the chunk should be loaded but its not. load it.
						chunk_t *chunk = data[chunkindex.x][chunkindex.y][chunkindex.z].chunk;

						SDL_UnlockMutex(data[chunkindex.x][chunkindex.y][chunkindex.z].lock);
						data[chunkindex.x][chunkindex.y][chunkindex.z].iswritable = 0;
						SDL_UnlockMutex(data[chunkindex.x][chunkindex.y][chunkindex.z].lock);

						chunk_reloadchunk(cpos, chunk);

						data[chunkindex.x][chunkindex.y][chunkindex.z].iswritable = 1;
						data[chunkindex.x][chunkindex.y][chunkindex.z].iscurrent = 0;

						data[chunkindex.x == WORLDSIZE-1 ? 0 : chunkindex.x+1][chunkindex.y][chunkindex.z].iscurrent = 0;
						data[chunkindex.x == 0 ? WORLDSIZE-1 : chunkindex.x-1][chunkindex.y][chunkindex.z].iscurrent = 0;
						data[chunkindex.x][chunkindex.y == WORLDSIZE-1 ? 0 : chunkindex.y+1][chunkindex.z].iscurrent = 0;
						data[chunkindex.x][chunkindex.y == 0 ? WORLDSIZE-1 : chunkindex.y-1][chunkindex.z].iscurrent = 0;
						data[chunkindex.x][chunkindex.y][chunkindex.z == WORLDSIZE-1 ? 0 : chunkindex.z+1].iscurrent = 0;
						data[chunkindex.x][chunkindex.y][chunkindex.z == 0 ? WORLDSIZE-1 : chunkindex.z-1].iscurrent = 0;
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

					if(!data[i.x][i.y][i.z].iscurrent)
						quickremeshachunk(&i);

					//upload terminal textures
					if(data[i.x][i.y][i.z].mappedptr && !data[i.x][i.y][i.z].termpbohasnewdata)//god forbid this thread runs faster than the render thread
					{
						memcpy(data[i.x][i.y][i.z].mappedptr, termscreen, 128*128*3);
						data[i.x][i.y][i.z].termpbohasnewdata=1;
					}
				}
			}
		}

		if(!stopthread)
			SDL_Delay(300);
	}
	return 0;
}

void
world_init()
{
	int u, v;
	for(u=0; u<128; u++)
	{
		for(v=0; v<128; v++)
		{
			termscreen[(u + 128*v) *3]=0;
			termscreen[(u + 128*v) *3 + 1]=u;
			termscreen[(u + 128*v) *3 + 2]=v;
		}
	}

	termscreen[7000] = 0;

	int3_t chunkindex;
	for(chunkindex.x=0; chunkindex.x<WORLDSIZE; chunkindex.x++)
	{
		for(chunkindex.y=0; chunkindex.y<WORLDSIZE; chunkindex.y++)
		{
			for(chunkindex.z=0; chunkindex.z<WORLDSIZE; chunkindex.z++)
			{
				glGenBuffers(1, &data[chunkindex.x][chunkindex.y][chunkindex.z].ebo);
				glGenBuffers(1, &data[chunkindex.x][chunkindex.y][chunkindex.z].vbo);
				glGenBuffers(1, &data[chunkindex.x][chunkindex.y][chunkindex.z].cbo);
				glGenBuffers(1, &data[chunkindex.x][chunkindex.y][chunkindex.z].termbo);
				glGenBuffers(1, &data[chunkindex.x][chunkindex.y][chunkindex.z].termpbo);
				glGenTextures(1, &termtexture);//TODO: term texture cleanup
				glBindTexture(GL_TEXTURE_2D, termtexture);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

				data[chunkindex.x][chunkindex.y][chunkindex.z].mappedptr=0;
				data[chunkindex.x][chunkindex.y][chunkindex.z].termpboloadnexttime=1;
				data[chunkindex.x][chunkindex.y][chunkindex.z].termpbohasnewdata=0;
				data[chunkindex.x][chunkindex.y][chunkindex.z].termcounter=0;

				data[chunkindex.x][chunkindex.y][chunkindex.z].points = 0;
				data[chunkindex.x][chunkindex.y][chunkindex.z].iscurrent = 0;
				data[chunkindex.x][chunkindex.y][chunkindex.z].ismeshcurrent=1;

				data[chunkindex.x][chunkindex.y][chunkindex.z].mesh.vbodata=0;
				data[chunkindex.x][chunkindex.y][chunkindex.z].mesh.ebodata=0;
				data[chunkindex.x][chunkindex.y][chunkindex.z].mesh.colordata=0;
				data[chunkindex.x][chunkindex.y][chunkindex.z].mesh.termscreendata=0;

				data[chunkindex.x][chunkindex.y][chunkindex.z].iswritable=1;
				data[chunkindex.x][chunkindex.y][chunkindex.z].lock = SDL_CreateMutex();

				long3_t cchunkindex;
				cchunkindex.x = chunkindex.x + worldscope.x;
				cchunkindex.y = chunkindex.y + worldscope.y;
				cchunkindex.z = chunkindex.z + worldscope.z;
				data[chunkindex.x][chunkindex.y][chunkindex.z].chunk = chunk_loadchunk(cchunkindex);
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
				glDeleteBuffers(1, &data[chunkindex.x][chunkindex.y][chunkindex.z].cbo);
				glDeleteBuffers(1, &data[chunkindex.x][chunkindex.y][chunkindex.z].vbo);
				glDeleteBuffers(1, &data[chunkindex.x][chunkindex.y][chunkindex.z].ebo);
				glDeleteBuffers(1, &data[chunkindex.x][chunkindex.y][chunkindex.z].termbo);
				SDL_DestroyMutex(data[chunkindex.x][chunkindex.y][chunkindex.z].lock);
				chunk_freechunk(data[chunkindex.x][chunkindex.y][chunkindex.z].chunk);
				if(!data[chunkindex.x][chunkindex.y][chunkindex.z].ismeshcurrent)
				{
					free(data[chunkindex.x][chunkindex.y][chunkindex.z].mesh.ebodata);
					free(data[chunkindex.x][chunkindex.y][chunkindex.z].mesh.vbodata);
					free(data[chunkindex.x][chunkindex.y][chunkindex.z].mesh.colordata);
				}
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
				if(!data[x][y][z].ismeshcurrent)
				{
					//points buffer
						glBindBuffer(GL_ARRAY_BUFFER, data[x][y][z].vbo);
						glBufferData(GL_ARRAY_BUFFER, data[x][y][z].mesh.vbosize * sizeof(GLfloat), data[x][y][z].mesh.vbodata, GL_STATIC_DRAW);

					//element buffer
						glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, data[x][y][z].ebo);
						glBufferData(GL_ELEMENT_ARRAY_BUFFER, data[x][y][z].mesh.ebosize * sizeof(GLuint), data[x][y][z].mesh.ebodata, GL_STATIC_DRAW);

					//Color buffer
						glBindBuffer(GL_ARRAY_BUFFER, data[x][y][z].cbo);
						glBufferData(GL_ARRAY_BUFFER, data[x][y][z].mesh.colorsize * sizeof(GLfloat), data[x][y][z].mesh.colordata, GL_STATIC_DRAW);

					//terminal buffer
						glBindBuffer(GL_ARRAY_BUFFER, data[x][y][z].termbo);
						glBufferData(GL_ARRAY_BUFFER, data[x][y][z].mesh.termscreensize * sizeof(GLfloat), data[x][y][z].mesh.termscreendata, GL_STATIC_DRAW);

					if(data[x][y][z].mesh.ebodata)
						free(data[x][y][z].mesh.ebodata);
					if(data[x][y][z].mesh.vbodata)
						free(data[x][y][z].mesh.vbodata);
					if(data[x][y][z].mesh.colordata)
						free(data[x][y][z].mesh.colordata);
					if(data[x][y][z].mesh.termscreendata)
						free(data[x][y][z].mesh.termscreendata);
					data[x][y][z].ismeshcurrent=1;
				}

				//glUseProgram(drawprogram);
				glBindBuffer(GL_ARRAY_BUFFER, data[x][y][z].vbo);
				glVertexAttribPointer(
						0,
						3,
						GL_FLOAT,
						GL_FALSE,
						0,
						0);

				glBindBuffer(GL_ARRAY_BUFFER, data[x][y][z].cbo);
				glVertexAttribPointer(
						1,
						3,
						GL_FLOAT,
						GL_FALSE,
						0,
						0);

				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, data[x][y][z].ebo);
				glDrawElements(GL_TRIANGLES, data[x][y][z].points, GL_UNSIGNED_INT, 0);

			}
		}
	}

	glUseProgram(terminalscreensprogram);
	for(x=0; x<WORLDSIZE; x++)
	{
		for(y=0; y<WORLDSIZE; y++)
		{
			for(z=0; z<WORLDSIZE; z++)
			{
				long double dist;

				long3_t chunkpos = chunk_getpos(data[x][y][z].chunk);
				distlong3(&dist, &chunkpos, &worldcenter);

				if(dist < TERMINALRENDERDIST)
				{
					glActiveTexture(GL_TEXTURE0);
					glBindTexture(GL_TEXTURE_2D, termtexture);

					if(data[x][y][z].termpboloadnexttime)
					{
						glBindBuffer(GL_PIXEL_UNPACK_BUFFER, data[x][y][z].termpbo);
						glBufferData(GL_PIXEL_UNPACK_BUFFER, 128*128*3, 0, GL_STREAM_DRAW);

						data[x][y][z].mappedptr = glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
						glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

						data[x][y][z].termpboloadnexttime=0;
					}

					if(!data[x][y][z].mappedptr)
						data[x][y][z].termpboloadnexttime=1;

					data[x][y][z].termcounter++;
					if(data[x][y][z].termpbohasnewdata && (data[x][y][z].termcounter > 1000))
					{
						data[x][y][z].termcounter=0;

						glBindBuffer(GL_PIXEL_UNPACK_BUFFER, data[x][y][z].termpbo);

						glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
						data[x][y][z].mappedptr=0;

						glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 128, 128, 0,  GL_RGB, GL_UNSIGNED_BYTE, 0);
						glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

						data[x][y][z].termpbohasnewdata=0;
					}

					//TODO: put this in a proper place
					GLint uniform_mytexture = glGetUniformLocation(terminalscreensprogram, "myTextureSampler");
					glUniform1i(uniform_mytexture, 0);
					glBindBuffer(GL_ARRAY_BUFFER, data[x][y][z].termbo);
					glVertexAttribPointer(
							0,
							3,
							GL_FLOAT,
							GL_FALSE,
							5 * sizeof(GLfloat),//*sizeof(GLfloat),
							0);

					glVertexAttribPointer(
							1,
							2,
							GL_FLOAT,
							GL_FALSE,
							5*sizeof(GLfloat),
							(void *)(3*sizeof(GLfloat)));
					glDrawArrays(GL_TRIANGLES, 0, data[x][y][z].termpoints);
				}
			}
		}
	}
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
world_setblock(long x, long y, long z, block_t block, int loadnew)
{
	long3_t cpos = getchunkofspot(x, y, z);
	int3_t internalpos = getinternalspotofspot(x, y, z);

	int3_t chunkindex;
	if(isquickloaded(cpos, &chunkindex))
	{
		chunk_t *chunk = data[chunkindex.x][chunkindex.y][chunkindex.z].chunk;
		if(!data[chunkindex.x][chunkindex.y][chunkindex.z].iswritable)
			return -2;

		if(SDL_TryLockMutex(data[chunkindex.x][chunkindex.y][chunkindex.z].lock)==0)
		{
			chunk_setblock(chunk, internalpos.x, internalpos.y, internalpos.z, block);
			SDL_UnlockMutex(data[chunkindex.x][chunkindex.y][chunkindex.z].lock);

			quickremeshachunk(&chunkindex);

			cpos.x++;
			if(internalpos.x == CHUNKSIZE-1 && isquickloaded(cpos, &chunkindex))
				quickremeshachunk(&chunkindex);

			cpos.x -= 2;
			if(internalpos.x == 0 && isquickloaded(cpos, &chunkindex))
				quickremeshachunk(&chunkindex);

			cpos.x++;

			cpos.y++;
			if(internalpos.y == CHUNKSIZE-1 && isquickloaded(cpos, &chunkindex))
				quickremeshachunk(&chunkindex);

			cpos.y -= 2;
			if(internalpos.y == 0 && isquickloaded(cpos, &chunkindex))
				quickremeshachunk(&chunkindex);
			cpos.y++;

			cpos.z++;
			if(internalpos.z == CHUNKSIZE-1 && isquickloaded(cpos, &chunkindex))
				quickremeshachunk(&chunkindex);

			cpos.z -= 2;
			if(internalpos.z == 0 && isquickloaded(cpos, &chunkindex))
				quickremeshachunk(&chunkindex);

			return 0;
		}
		return -3;
	}
	return -1;
}
