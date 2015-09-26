#include "octree.h"

#include "modulo.h"

struct node_s {
	int8_t isleaf;

	union {
		block_t block;
		struct node_s *children[8];
	} data;
};

octree_t
*octree_create()
{
	octree_t *tree = malloc(sizeof(octree_t));
	tree->isleaf = 1;
	tree->data.block.id = AIR;
	return tree;
}

void
octree_destroy(octree_t *tree)
{
	int i;
	if(!tree->isleaf)
	{
		for(i=0; i<8; i++)
			octree_destroy(tree->data.children[i]);
	}
	free(tree);
}

void
octree_zero(octree_t *tree)
{
	int i;
	if(!tree->isleaf)
		for(i=0; i<8; i++)
			octree_destroy(tree->data.children[i]);
	tree->isleaf = 1;
	tree->data.block.id = AIR;
}

block_t
get(int8_t x, int8_t y, int8_t z, octree_t *tree, int8_t level)
{
	if(level < CHUNKLEVELS)
	{
		if(tree->isleaf)
			return tree->data.block;
		int8_t x_ = MODULO(x*2, CHUNKSIZE);
		int8_t y_ = MODULO(y*2, CHUNKSIZE);
		int8_t z_ = MODULO(z*2, CHUNKSIZE);
		return get(x_, y_, z_, tree->data.children[(x<CHUNKSIZE/2) + (y<CHUNKSIZE/2)*2 + (z<CHUNKSIZE/2)*4], level + 1);
	} else
		return tree->data.block;
}

block_t
octree_get(int8_t x, int8_t y, int8_t z, octree_t *tree)
{
	return get(x, y, z, tree, 0);
}

void
set(int8_t x, int8_t y, int8_t z, octree_t *tree, block_t *data, int8_t level)
{
	int i;
	if(level < CHUNKLEVELS)
	{
		int8_t x_ = MODULO(x*2, CHUNKSIZE);
		int8_t y_ = MODULO(y*2, CHUNKSIZE);
		int8_t z_ = MODULO(z*2, CHUNKSIZE);
		if(tree->isleaf)
		{
			if(!memcmp(data, &(tree->data.block), sizeof(block_t)))
				return;
			tree->isleaf = 0;
			block_t block = tree->data.block;
			for(i=0; i<8; i++)
			{
				tree->data.children[i] = malloc(sizeof(struct node_s));
				tree->data.children[i]->isleaf = 1;
				tree->data.children[i]->data.block = block;
			}
		}
		set(x_, y_, z_, tree->data.children[(x<CHUNKSIZE/2) + (y<CHUNKSIZE/2)*2 + (z<CHUNKSIZE/2)*4], data, level +1);
		if(tree->data.children[0]->isleaf)
		{
			block_t block = tree->data.children[0]->data.block;
			int childrens = 1;
			for(i=1; i<8; i++)
			{
				if(tree->data.children[i]->isleaf)
					if(!memcmp(&(tree->data.children[i]->data.block), &block, sizeof(block_t)))
						childrens++;
			}
			if(childrens == 8)
			{
				for(i=0; i<8; i++)
					free(tree->data.children[i]);
				tree->isleaf = 1;
				tree->data.block = block;
			}
		}
	} else
		tree->data.block = *data;
}

void
octree_set(int8_t x, int8_t y, int8_t z, octree_t *tree, block_t *data)
{
	set(x, y, z, tree, data, 0);
}
