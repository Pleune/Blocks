#ifndef BLOCK_H
#define BLOCK_H

#include <stdint.h>

#define AIR 0;
#define STONE 1;

typedef struct {
	uint8_t id;
} block_t;

int block_issolid(block_t b);

#endif //BLOCK_H
