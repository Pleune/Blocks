#ifndef BLOCK_H
#define BLOCK_H

#include <stdint.h>
#include "custommath.h"

#define AIR 0;
#define STONE 1;

typedef struct {
	uint8_t id;
} block_t;

int block_issolid(block_t b);
vec3_t block_getcolor(uint8_t id);

#endif //BLOCK_H
