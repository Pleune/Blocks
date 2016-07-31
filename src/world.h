#ifndef WORLD_H
#define WORLD_H

#include <GL/glew.h>
#include <stdlib.h>
#include <math.h>

#include "custommath.h"
#include "block.h"
#include "modulo.h"
#include "chunk.h"
#include "entity.h"

int world_init_new(volatile int *status, const char *savename);
int world_init_load(const char *savename, volatile int *status);
int world_save();
void world_cleanup();

int world_is_initalized();
entity_t *world_get_player();

void world_seed_gen();
uint32_t world_get_seed();
void world_set_seed(uint32_t new_seed);

void world_render(vec3_t pos, GLuint modelmatrix);

block_t world_block_get(long x, long y, long z, int loadnew);
blockid_t world_block_get_id(long x, long y, long z, int loadnew);
int world_block_set(long x, long y, long z, block_t block, int update, int loadnew, int instant);
int world_block_set_id(long x, long y, long z, blockid_t id, int update, int loadnew, int instant);

void world_update_queue(long x, long y, long z, uint8_t time, update_flags_t flags);
long world_update_flush();

long world_get_trianglecount();

static inline long3_t
world_get_chunkpos_of_worldpos(long x, long y, long z)
{
	long3_t chunkpos;
	chunkpos.x = floor((double)x / CHUNKSIZE);
	chunkpos.y = floor((double)y / CHUNKSIZE);
	chunkpos.z = floor((double)z / CHUNKSIZE);
	return chunkpos;
}

static inline int3_t
world_get_internalpos_of_worldpos(long x, long y, long z)
{
	int3_t blockpos;
	blockpos.x = MODULO(x, CHUNKSIZE);
	blockpos.y = MODULO(y, CHUNKSIZE);
	blockpos.z = MODULO(z, CHUNKSIZE);
	return blockpos;
}
#endif
