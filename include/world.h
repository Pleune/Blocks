#ifndef WORLD_H
#define WORLD_H

#include "block.h"

typedef struct {
	int x;
	int y;
	int z;
} int3_t;

void world_initalload();
void world_render();

int world_addblock(int3_t pos, block_t block, int loadnew);
#endif //WORLD_H
