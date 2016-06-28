#include "block.h"

const struct block_properties block_properties[BLOCK_NUM_TYPES] = {
	[AIR] = {0, {0,0,0}, "Air"},
	[STONE] = {1, {0.2,0.2,0.22}, "Stone"},
	[DIRT] = {1, {0.185,0.09,0.05}, "Dirt"},
	[GRASS] = {1, {0.05,0.27,0.1}, "Grass"},
	[SAND] = {1, {0.28,0.3,0.15}, "Sand"},
	[BEDROCK] = {1, {0.1,0.1,0.1}, "Hard Stone"},
	[WATER] = {1, {0.08,0.08,0.3}, "Water"},
	[WATER_GEN] = {1, {.8,.8,.8}, "Water Generator"},
	[ERR] = {0, {1,0,0}, "Error"}
};
