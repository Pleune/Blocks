#ifndef OCTREE_H
#define OCTREE_H

#include <unistd.h>
#include <stdint.h>

#include "block.h"
#include "chunk.h"

typedef struct node_s octree_t;

octree_t *octree_create();
void octree_destroy(octree_t *tree);
void octree_zero(octree_t *tree);

block_t octree_get(int8_t x, int8_t y, int8_t z, octree_t *tree);
void octree_set(int8_t x, int8_t y, int8_t z, octree_t *tree, block_t *data);

size_t octree_dump(octree_t *tree, unsigned char **data);
octree_t *octree_read(const unsigned char *data);

#endif
