#ifndef WORLD_H
#define WORLD_H

#include <GL/glew.h>

#include "custommath.h"
#include "block.h"

void world_init(vec3_t pos);
void world_cleanup();

void world_render(vec3_t pos);

int world_threadentry(void *ptr);

block_t world_getblock(long x, long y, long z, int loadnew);
int world_setblock(long x, long y, long z, block_t block, int update, int loadnew, int instant);

void world_genseed();
uint32_t world_getseed();
#endif //WORLD_H
