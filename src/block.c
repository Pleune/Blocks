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
		case 1:
			c.x = 1;
			c.y = 1;
			c.z = 1;
		break;
		case 2:
			c.x = 1;
			c.y = 0;
			c.z = 0;
		break;
		case 3:
			c.x = 0;
			c.y = 1;
			c.z = 0;
		break;
		case 4:
			c.x = 0;
			c.y = 0;
			c.z = 1;
		break;
		default:
			c.x = .5;
			c.y = .5;
			c.z = .5;
		break;
	}
	return c;
}
