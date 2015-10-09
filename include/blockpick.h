#ifndef BLOCKPICK_H
#define BLOCKPICK_H

#include "custommath.h"
#include "block.h"

void game_rayadd(const vec3_t *start, const vec3_t *direction, block_t block, int before);
void game_raydel(const vec3_t *start, const vec3_t *direction);

#endif
