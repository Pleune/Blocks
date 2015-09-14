#include "block.h"

int
block_issolid(block_t b)
{
	return b.id != 0 && b.id != 255;
}

vec3_t
block_getcolor(uint8_t id)
{
	vec3_t c;
	switch(id)
	{
		case BLOCK_ID_STONE:
			c.x = .4;
			c.y = .4;
			c.z = .4;
		break;
		case BLOCK_ID_DIRT:
			c.x = .08;
			c.y = .5;
			c.z = .16;
		break;
		default:
			c.x = .6;
			c.y = .45;
			c.z = .25;
		break;
	}
	return c;
}
