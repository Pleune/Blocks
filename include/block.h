#ifndef BLOCK_H
#define BLOCK_H

#include <stdint.h>
#include "custommath.h"
#include "directions.h"
#include "update.h"

enum block_id {AIR = 0, STONE, DIRT, GRASS, SAND, BEDROCK, WATER, WATER_GEN, ERR};
typedef enum block_id blockid_t;
#define BLOCK_NUM_TYPES 9

typedef struct {
	blockid_t id;
	union {
		void *pointer;
		uint32_t number;
	} metadata;
} block_t;

struct blockproperties {
	uint8_t issolid;
	vec3_t color;
	char *name;
};
extern const struct blockproperties block_properties[BLOCK_NUM_TYPES];

inline int
block_issolid(blockid_t b)
{
	return block_properties[b].issolid;
};

inline vec3_t
block_getcolor(blockid_t b)
{
	return block_properties[b].color;
};

void block_updaterun(block_t b, long3_t pos, update_flags_t flags);

#endif //BLOCK_H
