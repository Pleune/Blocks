#ifndef BLOCKPICK_H
#define BLOCKPICK_H

#include "custommath.h"
#include "block.h"

long3_t world_raypos(const vec3_t *start, const vec3_t *direction, int before, int dist);
void world_rayadd(const vec3_t *start, const vec3_t *direction, block_t block, int update, int before, int dist);
void world_raydel(const vec3_t *start, const vec3_t *direction, int update, int dist);

#endif
