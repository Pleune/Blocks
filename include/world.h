#ifndef WORLD_H
#define WORLD_H

#include <GL/glew.h>

#include "custommath.h"
#include "block.h"
#include "modulo.h"
#include "chunk.h"

void world_init(vec3_t pos);
void world_cleanup();

void world_genseed();
uint32_t world_getseed();

void world_render(vec3_t pos);

block_t world_getblock(long x, long y, long z, int loadnew);
int world_setblock(long x, long y, long z, block_t block, int update, int loadnew, int instant);

void world_updatequeue(long x, long y, long z, uint8_t time, update_flags_t flags);
long world_updaterun();

static inline long3_t
world_getchunkposofworldpos(long x, long y, long z)
{
	long3_t chunkpos;
	chunkpos.x = floor((double)x / CHUNKSIZE);
	chunkpos.y = floor((double)y / CHUNKSIZE);
	chunkpos.z = floor((double)z / CHUNKSIZE);
	return chunkpos;
}

static inline int3_t
world_getinternalposofworldpos(long x, long y, long z)
{
	int3_t blockpos;
	blockpos.x = MODULO(x, CHUNKSIZE);
	blockpos.y = MODULO(y, CHUNKSIZE);
	blockpos.z = MODULO(z, CHUNKSIZE);
	return blockpos;
}
#endif //WORLD_H
