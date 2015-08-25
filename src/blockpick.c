#include "blockpick.h"

#include <math.h>

#include "world.h"

void
game_rayadd(vec3_t start, vec3_t direction, block_t block, int before)
{
	long3_t p;
	p.x = floorf(start.x);
	p.y = floorf(start.y);
	p.z = floorf(start.z);

	vec3_t b;
	b.x = start.x - p.x;
	b.y = start.y - p.y;
	b.z = start.z - p.z;

	int dirx = direction.x > 0 ? 1 : -1;
	int diry = direction.y > 0 ? 1 : -1;
	int dirz = direction.z > 0 ? 1 : -1;

	int3_t iszero = {
		direction.x == 0,
		direction.y == 0,
		direction.z == 0
	};

	vec3_t rt;
	vec3_t delta = {0};
	if(!iszero.x)
	{
		rt.y = direction.y / direction.x;
		rt.z = direction.z / direction.x;
		delta.x = sqrtf(1 + rt.y*rt.y + rt.z*rt.z);
	}
	if(!iszero.y)
	{
		rt.x = direction.x / direction.y;
		rt.z = direction.z / direction.y;
		delta.y = sqrtf(rt.x*rt.x + 1 + rt.z*rt.z);
	}
	if(!iszero.z)
	{
		rt.x = direction.x / direction.z;
		rt.y = direction.y / direction.z;
		delta.z = sqrtf(rt.x*rt.x + rt.y*rt.y + 1);
	}

	vec3_t max = {
		delta.x * (direction.x > 0 ? (1 - b.x) : b.x),
		delta.y * (direction.y > 0 ? (1 - b.y) : b.y),
		delta.z * (direction.z > 0 ? (1 - b.z) : b.z)
	};

	int i;
	for(i=0; i<1000; i++)
	{
		if((max.x <= max.y || iszero.y) && (max.x <= max.z || iszero.z) && !iszero.x)
		{
			max.x += delta.x;
			p.x += dirx;
			if(block_issolid(world_getblock(p.x,p.y,p.z,0)))
			{
				if(before)
					p.x -= dirx;
				break;
			}
		}
		else if((max.y <= max.x || iszero.x) && (max.y <= max.z || iszero.z) && !iszero.y)
		{
			max.y += delta.y;
			p.y += diry;
			if(block_issolid(world_getblock(p.x,p.y,p.z,0)))
			{
				if(before)
					p.y -= diry;
				break;
			}
		}
		else if((max.z <= max.x || iszero.x) && (max.z <= max.y || iszero.y) && !iszero.z)
		{
			max.z += delta.z;
			p.z += dirz;
			if(block_issolid(world_getblock(p.x,p.y,p.z,0)))
			{
				if(before)
					p.z -= dirz;
				break;
			}
		}
	}
	if(!block_issolid(world_getblock(p.x,p.y,p.z,0)) || !block.id)
		world_setblock(p.x, p.y, p.z, block, 0, 1);
}

void
game_raydel(vec3_t start, vec3_t direction)
{
	block_t b;
	b.id = 0;
	game_rayadd(start, direction, b, 0);
}
