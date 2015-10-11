#include "entity.h"

#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include "block.h"
#include "world.h"

struct entity_s {
	vec3_t pos;
	double w;
	double h;

	vec3_t velocities;
};

entity_t *
entity_create(double x, double y, double z, double w, double h)
{
	entity_t *entity = malloc(sizeof(entity_t));

	vec3_t pos = {x,y,z};
	entity->pos = pos;
	entity->w = w;
	entity->h = h;

	return entity;
}

void
entity_destroy(entity_t *entity)
{
	free(entity);
}

void
entity_setsize(entity_t *entity, double width, double height)
{
	entity->w = width;
	entity->h = height;
}

void
entity_setpos(entity_t *entity, vec3_t pos)
{
	entity->pos = pos;
}

vec3_t
entity_getpos(entity_t *entity)
{
	return entity->pos;
}

const vec3_t *
entity_getposptr(entity_t *entity)
{
	return &(entity->pos);
}

int
iscolliding(entity_t *entity)
{
	return 0;
}

void
entity_move(entity_t *entity, vec3_t *delta)
{
	vec3_t startpos = entity->pos;
	double halfw = entity->w / 2.0;
	long a, b;

	entity->pos.x += delta->x;
	a = floor(startpos.x + halfw);
	b = floor(entity->pos.x + halfw);
	if(a < b)
	{
//		printf("x inc\n");
		long y, z;
		for(y = floor(entity->pos.y); y < entity->pos.y + entity->h; y++)
		{
			for(z = floor(entity->pos.z - halfw); z < entity->pos.z + halfw; z++)
			{
				if(block_issolid(world_getblock(b, y, z, 0)))
				{
					entity->pos.x = b - halfw -.01;
					entity->velocities.x = 0;
					break;
				}
			}
		}

	}
	a = floor(startpos.x - halfw);
	b = floor(entity->pos.x - halfw);
	if(b < a)
	{
//		printf("x dec\n");
		long y, z;
		for(y = floor(entity->pos.y); y < entity->pos.y + entity->h; y++)
		{
			for(z = floor(entity->pos.z - halfw); z < entity->pos.z + halfw; z++)
			{
				if(block_issolid(world_getblock(b, y, z, 0)))
				{
					entity->pos.x = b + 1 + halfw + .01;
					entity->velocities.x = 0;
					break;
				}
			}
		}
	}

	entity->pos.y += delta->y;
	a = floor(startpos.y + entity->h);
	b = floor(entity->pos.y + entity->h);
	if(a < b)
	{
//		printf("y inc\n");
		long x, z;
		for(x = floor(entity->pos.x - halfw); x < entity->pos.x + halfw; x++)
		{
			for(z = floor(entity->pos.z - halfw); z < entity->pos.z + halfw; z++)
			{
				if(block_issolid(world_getblock(x, b, z, 0)))
				{
					entity->pos.y = b - entity->h - .01;
					entity->velocities.y = 0;
					break;
				}
			}
		}
	}
	a = floor(startpos.y);
	b = floor(entity->pos.y);
	if(b < a)
	{
//		printf("y dec\n");
		long x, z;
		for(x = floor(entity->pos.x - halfw); x < entity->pos.x + halfw; x++)
		{
			for(z = floor(entity->pos.z - halfw); z < entity->pos.z + halfw; z++)
			{
				if(block_issolid(world_getblock(x, b, z, 0)))
				{
					entity->pos.y = b + 1;
					entity->velocities.y = 0;
					break;
				}
			}
		}
	}

	entity->pos.z += delta->z;
	a = floor(startpos.z + halfw);
	b = floor(entity->pos.z + halfw);
	if(a < b)
	{
//		printf("z inc\n");
		long x, y;
		for(y = floor(entity->pos.y); y < entity->pos.y + entity->h; y++)
		{
			for(x = floor(entity->pos.x - halfw); x < entity->pos.x + halfw; x++)
			{
				if(block_issolid(world_getblock(x, y, b, 0)))
				{
					entity->pos.z = b - halfw - .01;
					entity->velocities.z = 0;
					break;
				}
			}
		}
	}
	a = floor(startpos.z - halfw);
	b = floor(entity->pos.z - halfw);
	if(b < a)
	{
//		printf("z dec\n");
		long x, y;
		for(y = floor(entity->pos.y); y < entity->pos.y + entity->h; y++)
		{
			for(x = floor(entity->pos.x - halfw); x < entity->pos.x + halfw; x++)
			{
				if(block_issolid(world_getblock(x, y, b, 0)))
				{
					entity->pos.z = b + 1 + halfw + .01;
					entity->velocities.z = 0;
					break;
				}
			}
		}
	}
}

void
entity_update(entity_t *entity, vec3_t *forces, double dt)
{
}
