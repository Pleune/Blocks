#include "block.h"


const blockdata_t blockinfo[(enum block_id) ERR] = {
	[AIR] = {0, {0,0,0}, "Air"},
	[STONE] = {1, {0.2,0.2,0.22}, "Stone"},
	[DIRT] = {1, {0.185,0.09,0.05}, "Dirt"},
	[GRASS] = {1, {0.05,0.27,0.1}, "Grass"},
	[BEDROCK] = {1, {0.1,0.1,0.1}, "Hard Stone"},
	[WATER] = {1, {0.08,0.08,0.3}, "Water"}
};


int
block_issolid(block_t b)
{
	if(b.id != ERR)
		return blockinfo[b.id].issolid;
	else
		return 0;
}

vec3_t
block_getcolor(blockid_t id)
{
	return blockinfo[id].color;
}
