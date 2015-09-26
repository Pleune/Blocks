#ifndef BLOCK_H
#define BLOCK_H

#include <stdint.h>
#include "custommath.h"
#include "directions.h"

enum block_id {AIR = 0, STONE, DIRT, GRASS, SAND, WATER, ERR};

#define BLOCK_ID_INVALID 255

typedef enum block_id blockid_t;

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
vec3_t block_getcolor(blockid_t);

#endif //BLOCK_H
