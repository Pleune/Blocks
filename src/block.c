#include "block.h"

int
block_issolid(block_t b)
{
	return b.id != AIR;// && b.id != ERR;
}

vec3_t
block_getcolor(blockid_t id)
{
	vec3_t c;
	switch(id)
	{
		case STONE:
			c.x = .2;
			c.y = .2;
			c.z = .2;
			break;
		case GRASS:
			c.x = .05;
			c.y = .27;
			c.z = .1;
			break;
		case WATER:
			c.x = .08;
			c.y = .08;
			c.z = .3;
			break;
		case SAND:
			c.x = .3;
			c.y = .3;
			c.z = .22;
			break;
		default:
			c.x = .6;
			c.y = .45;
			c.z = .25;
			break;
	}
	return c;
}
