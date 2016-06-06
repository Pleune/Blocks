#ifndef BLOCK_H
#define BLOCK_H

#include <stdint.h>
#include "custommath.h"
#include "directions.h"

#define BLOCK_NUM_TYPES 9
enum block_id {AIR = 0, STONE, DIRT, GRASS, SAND, BEDROCK, WATER, WATER_GEN, ERR};
typedef enum block_id blockid_t;

typedef struct {
	blockid_t id;
	union {
		void *pointer;
		uint32_t number;
	} metadata;
} block_t;

struct block_properties {
	uint8_t solid;
	vec3_t color;
	char *name;
};

extern const struct block_properties block_properties[BLOCK_NUM_TYPES];

#define BLOCK_PROPERTY_SOLID(id) (block_properties[id].solid)
#define BLOCK_PROPERTY_COLOR(id) (block_properties[id].color)

#endif
