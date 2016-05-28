#include "entity.h"

#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include "block.h"
#include "world.h"
#include "defines.h"

struct entity_s {
	vec3_t pos;
	double w;
	double h;
	vec3_t velocity;
	vec3_t friction;
	double m;
	uint8_t hasjumped;
};

entity_t *
entity_create(double x, double y, double z, double w, double h, double m)
{
	entity_t *entity = malloc(sizeof(entity_t));
	memset(&(entity->velocity), 0, sizeof(vec3_t));

	vec3_t pos = {x,y,z};

	entity->pos = pos;
	entity->w = w;
	entity->h = h;
	entity->m = 1;
	entity->hasjumped=0;

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
entity_setfriction(entity_t *entity, vec3_t f)
{
	entity->friction = f;
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
entity_jump(entity_t *entity, double y)
{
	if(entity->hasjumped)
		return;
	entity->velocity.y = y;
	entity->hasjumped=1;
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
		long y, z;
		for(y = floor(entity->pos.y); y < entity->pos.y + entity->h; y++)
		{
			for(z = floor(entity->pos.z - halfw); z < entity->pos.z + halfw; z++)
			{
				if(block_issolid(world_getblock(b, y, z, 0).id))
				{
					entity->pos.x = b - halfw -.0001;
					entity->velocity.x = 0;
					break;
				}
			}
		}

	}
	a = floor(startpos.x - halfw);
	b = floor(entity->pos.x - halfw);
	if(b < a)
	{
		long y, z;
		for(y = floor(entity->pos.y); y < entity->pos.y + entity->h; y++)
		{
			for(z = floor(entity->pos.z - halfw); z < entity->pos.z + halfw; z++)
			{
				if(block_issolid(world_getblock(b, y, z, 0).id))
				{
					entity->pos.x = b + 1 + halfw + .0001;
					entity->velocity.x = 0;
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
		long x, z;
		for(x = floor(entity->pos.x - halfw); x < entity->pos.x + halfw; x++)
		{
			for(z = floor(entity->pos.z - halfw); z < entity->pos.z + halfw; z++)
			{
				if(block_issolid(world_getblock(x, b, z, 0).id))
				{
					entity->pos.y = b - entity->h - .0001;
					entity->velocity.y = 0;
					break;
				}
			}
		}
	}
	a = floor(startpos.y);
	b = floor(entity->pos.y);
	if(b < a)
	{
		long x, z;
		for(x = floor(entity->pos.x - halfw); x < entity->pos.x + halfw; x++)
		{
			for(z = floor(entity->pos.z - halfw); z < entity->pos.z + halfw; z++)
			{
				if(block_issolid(world_getblock(x, b, z, 0).id))
				{
					entity->pos.y = b + 1;
					entity->velocity.y = 0;
					entity->hasjumped=0;
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
		long x, y;
		for(y = floor(entity->pos.y); y < entity->pos.y + entity->h; y++)
		{
			for(x = floor(entity->pos.x - halfw); x < entity->pos.x + halfw; x++)
			{
				if(block_issolid(world_getblock(x, y, b, 0).id))
				{
					entity->pos.z = b - halfw - .0001;
					entity->velocity.z = 0;
					break;
				}
			}
		}
	}
	a = floor(startpos.z - halfw);
	b = floor(entity->pos.z - halfw);
	if(b < a)
	{
		long x, y;
		for(y = floor(entity->pos.y); y < entity->pos.y + entity->h; y++)
		{
			for(x = floor(entity->pos.x - halfw); x < entity->pos.x + halfw; x++)
			{
				if(block_issolid(world_getblock(x, y, b, 0).id))
				{
					entity->pos.z = b + 1 + halfw + .0001;
					entity->velocity.z = 0;
					break;
				}
			}
		}
	}
}

void
entity_update(entity_t *entity, vec3_t *forces, double dt)
{
	entity->velocity.x += forces->x*dt/entity->m - entity->friction.x*entity->velocity.x*dt;
	entity->velocity.y += (forces->y/entity->m-GRAVITY)*dt;
	entity->velocity.z += forces->z*dt/entity->m - entity->friction.z*entity->velocity.z*dt;
	double mag = sqrt(entity->velocity.x*entity->velocity.x + entity->velocity.z*entity->velocity.z);
	if(mag > SPEED)
	{
		entity->velocity.x *= SPEED/mag;
		entity->velocity.z *= SPEED/mag;
	}



	vec3_t delta;
	delta.x = entity->velocity.x*dt;
	delta.y = entity->velocity.y*dt;
	delta.z = entity->velocity.z*dt;
	entity_move(entity, &delta);
}
