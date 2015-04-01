#include "block.h"

int
block_issolid(block_t b)
{
	return b.id != 0 && b.id != 255;
}
