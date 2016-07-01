#include "octree.h"

#include <stdlib.h>
#include <string.h>

#include "modulo.h"
#include "stack.h"
#include "debug.h"

struct node_s {
	int8_t isleaf;

	union {
		block_t block;
		struct node_s *children;
	} data;
};

octree_t
*octree_create()
{
	octree_t *tree = calloc(1, sizeof(octree_t));
	tree->isleaf = 1;
	tree->data.block.id = AIR;
	return tree;
}

static void
destroy(octree_t *tree)
{
	int i;
	if(!tree->isleaf)
	{
		for(i=0; i<8; i++)
			destroy(&tree->data.children[i]);
		free(tree->data.children);
	}
}

void
octree_destroy(octree_t *tree)
{
	destroy(tree);
	free(tree);
}

void
octree_zero(octree_t *tree)
{
	destroy(tree);
	tree->isleaf = 1;
	tree->data.block.id = AIR;
}

static block_t
get(int8_t x, int8_t y, int8_t z, octree_t *tree, int8_t level)
{
	if(tree->isleaf)
		return tree->data.block;
	int8_t x_ = (x*2 % CHUNKSIZE);
	int8_t y_ = (y*2 % CHUNKSIZE);
	int8_t z_ = (z*2 % CHUNKSIZE);
	return get(x_, y_, z_, &tree->data.children[(x<CHUNKSIZE/2) | ((y<CHUNKSIZE/2) << 1) | ((z<CHUNKSIZE/2) << 2)], level + 1);
}

block_t
octree_get(int8_t x, int8_t y, int8_t z, octree_t *tree)
{
	return get(x, y, z, tree, 0);
}

static void
set(int8_t x, int8_t y, int8_t z, octree_t *tree, block_t *data, int8_t level)
{
	int i;
	if(level < CHUNK_LEVELS)
	{
		int8_t x_ = (x*2 % CHUNKSIZE);
		int8_t y_ = (y*2 % CHUNKSIZE);
		int8_t z_ = (z*2 % CHUNKSIZE);
		if(tree->isleaf)
		{
			if(!memcmp(data, &(tree->data.block), sizeof(block_t)))
				return;
			tree->isleaf = 0;
			block_t block = tree->data.block;

			tree->data.children = malloc(sizeof(struct node_s) * 8);
			for(i=0; i<8; i++)
			{
				tree->data.children[i].isleaf = 1;
				tree->data.children[i].data.block = block;
			}
		}
		set(x_, y_, z_, &tree->data.children[(x<CHUNKSIZE/2) + (y<CHUNKSIZE/2)*2 + (z<CHUNKSIZE/2)*4], data, level +1);
		if(tree->data.children[0].isleaf)
		{
			block_t block = tree->data.children[0].data.block;
			int childrens = 1;
			for(i=1; i<8; i++)
			{
				if(tree->data.children[i].isleaf)
					if(!memcmp(&(tree->data.children[i].data.block), &block, sizeof(block_t)))
						childrens++;
			}
			if(childrens == 8)
			{
				free(tree->data.children);
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

void write_node(octree_t *tree, struct stack *stack);

void
write_nonleaf(octree_t *tree, struct stack *stack)
{
	static char static_L = 'N';
	stack_push(stack, &static_L);

	int i;
	for(i=0; i<8; i++)
		write_node(&tree->data.children[i], stack);
}

void
write_leaf(octree_t *tree, struct stack *stack)
{
	static char static_B = 'L';
	stack_push(stack, &static_B);

	unsigned char tmp;
	tmp = tree->data.block.id;
	stack_push(stack, &tmp);
	tmp = tree->data.block.id >> 8;
	stack_push(stack, &tmp);

	tmp = tree->data.block.metadata.number;
	stack_push(stack, &tmp);
	tmp = tree->data.block.metadata.number >> 8;
	stack_push(stack, &tmp);
	tmp = tree->data.block.metadata.number >> 16;
	stack_push(stack, &tmp);
	tmp = tree->data.block.metadata.number >> 24;
	stack_push(stack, &tmp);

}

void
write_node(octree_t *tree, struct stack *stack)
{
	if(tree->isleaf)
		write_leaf(tree, stack);
	else
		write_nonleaf(tree, stack);
}

size_t
octree_dump(octree_t *tree, unsigned char **data)
{
	//TODO: constants
	struct stack stack;
	stack_init(&stack, 1, 10000, 2.0);

	write_node(tree, &stack);

	stack_trim(&stack);
	*data = stack.data;

	return stack.size;
}

size_t read_node(octree_t *tree, unsigned char *data);

size_t
read_nonleaf(octree_t *tree, unsigned char *data)
{
	size_t size = 1;
	tree->isleaf = 0;

	int i = 0;
	tree->data.children = malloc(sizeof(struct node_s) * 8);
	for(i=0; i<8; i++)
		size += read_node(&tree->data.children[i], data + size);

	return size;
}

size_t
read_leaf(octree_t *tree, unsigned char *data)
{
	unsigned char *index = data + 1;
	tree->isleaf = 1;
	tree->data.block.id =
		(enum block_id)(index[0]) |
		(enum block_id)(index[1]) << 8;
	tree->data.block.metadata.number =
		(enum block_id)(index[2]) |
		(enum block_id)(index[3]) << 8 |
		(enum block_id)(index[4]) << 16 |
		(enum block_id)(index[5]) << 24;

	return 7;
}

size_t
read_node(octree_t *tree, unsigned char *data)
{
	if(data[0] == 'L')
		return read_leaf(tree, data);
	else if(data[0] == 'N')
		return read_nonleaf(tree, data);
	else
		fail("byte not 'N' or 'L' reading octree from binary dump");

	return 0;
}

octree_t *
octree_read(unsigned char *data)
{
	octree_t *octree = octree_create();
	read_node(octree, data);
	return octree;
}
