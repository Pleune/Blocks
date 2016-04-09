#ifndef BLOCK_H
#define BLOCK_H

#include <stdint.h>
#include "custommath.h"
#include "directions.h"
#include "update.h"

enum block_id {AIR = 0, STONE, DIRT, GRASS, SAND, BEDROCK, WATER, WATER_GEN, ERR};

typedef enum block_id blockid_t;

typedef struct {
	blockid_t id;
	union {
		void *pointer;
		uint32_t number;
	} metadata;
} block_t;

typedef struct {
	uint8_t issolid;
	vec3_t color;
	char *name;
} blockdata_t;

int block_issolid(block_t b);
vec3_t block_getcolor(blockid_t);

void block_updaterun(block_t b, long3_t pos, update_flags_t flags);

#endif //BLOCK_H
