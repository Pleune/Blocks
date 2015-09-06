#ifndef BLOCK_H
#define BLOCK_H

#include <stdint.h>
#include "custommath.h"

#define BLOCK_ID_AIR 0
#define BLOCK_ID_STONE 1
#define BLOCK_ID_DIRT 2
#define BLOCK_ID_TERMINAL 3
#define BLOCK_ID_INVALID 255

typedef uint8_t blockid_t;

typedef struct {
	blockid_t id;
	union {
		void *pointer;
		uint8_t number;
	} metadata;
} block_t;

struct block_term_t {
	uint8_t face;
};

int block_issolid(block_t b);
vec3_t block_getcolor(uint8_t id);

#endif //BLOCK_H
