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
	vec3_t velocity;
	vec3_t friction;
	vec3_t workingfrict;
	double m;
};

entity_t *
entity_create(double x, double y, double z, double w, double h, double m)
{
	entity_t *entity = malloc(sizeof(entity_t));

	vec3_t pos = {x,y,z};

	entity->pos = pos;
	entity->w = w;
	entity->h = h;
	entity->m = 1;

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

vec3_t
move(entity_t *entity, vec3_t *delta)
{
	vec3_t startpos = entity->pos;
	double halfw = entity->w / 2.0;
	long a, b;

	vec3_t friction = {0,0,0};

	entity->pos.x += delta->x;
	a = floor(startpos.x + halfw);
	b = floor(entity->pos.x + halfw);
	if(a < b)
	{
//		printf("x inc\n");
		long y, z;
		int flag = 0;
		for(y = floor(entity->pos.y); y < entity->pos.y + entity->h; y++)
		{
			for(z = floor(entity->pos.z - halfw); z < entity->pos.z + halfw; z++)
			{
				if(block_issolid(world_getblock(b, y, z, 0)))
				{
					entity->pos.x = b - halfw -.0001;
					entity->velocity.x = 0;
					flag = 1;
					break;
				}
			}
		}
		if(flag)
		{
			friction.y += 1;
			friction.z += 1;
		}

	}
	a = floor(startpos.x - halfw);
	b = floor(entity->pos.x - halfw);
	if(b < a)
	{
//		printf("x dec\n");
		long y, z;
		
		int flag = 0;
		for(y = floor(entity->pos.y); y < entity->pos.y + entity->h; y++)
		{
			for(z = floor(entity->pos.z - halfw); z < entity->pos.z + halfw; z++)
			{
				if(block_issolid(world_getblock(b, y, z, 0)))
				{
					entity->pos.x = b + 1 + halfw + .0001;
					entity->velocity.x = 0;
					flag = 1;
					break;
				}
			}
		}
		if(flag)
		{
			friction.y += 1;
			friction.z += 1;
		}
	}

	entity->pos.y += delta->y;
	a = floor(startpos.y + entity->h);
	b = floor(entity->pos.y + entity->h);
	if(a < b)
	{
//		printf("y inc\n");
		long x, z;
		int flag = 0;
		for(x = floor(entity->pos.x - halfw); x < entity->pos.x + halfw; x++)
		{
			for(z = floor(entity->pos.z - halfw); z < entity->pos.z + halfw; z++)
			{
				if(block_issolid(world_getblock(x, b, z, 0)))
				{
					entity->pos.y = b - entity->h - .0001;
					entity->velocity.y = 0;
					flag = 1;
					break;
				}
			}
		}
		if(flag)
		{
			friction.x += 1;
			friction.z += 1;
		}
	}
	a = floor(startpos.y);
	b = floor(entity->pos.y);
	if(b < a)
	{
//		printf("y dec\n");
		long x, z;
		
		int flag = 0;
		for(x = floor(entity->pos.x - halfw); x < entity->pos.x + halfw; x++)
		{
			for(z = floor(entity->pos.z - halfw); z < entity->pos.z + halfw; z++)
			{
				if(block_issolid(world_getblock(x, b, z, 0)))
				{
					entity->pos.y = b + 1;
					entity->velocity.y = 0;
					flag = 1;
					break;
				}
			}
		}
		if(flag)
		{
			friction.x += 1;
			friction.z += 1;
		}
	}

	entity->pos.z += delta->z;
	a = floor(startpos.z + halfw);
	b = floor(entity->pos.z + halfw);
	if(a < b)
	{
//		printf("z inc\n");
		long x, y;
		
		int flag = 0;
		for(y = floor(entity->pos.y); y < entity->pos.y + entity->h; y++)
		{
			for(x = floor(entity->pos.x - halfw); x < entity->pos.x + halfw; x++)
			{
				if(block_issolid(world_getblock(x, y, b, 0)))
				{
					entity->pos.z = b - halfw - .0001;
					entity->velocity.z = 0;
					flag = 1;
					break;
				}
			}
		}
		if(flag)
		{
			friction.x += 1;
			friction.y += 1;
		}
	}
	a = floor(startpos.z - halfw);
	b = floor(entity->pos.z - halfw);
	if(b < a)
	{
//		printf("z dec\n");
		long x, y;
		
		int flag = 0;
		for(y = floor(entity->pos.y); y < entity->pos.y + entity->h; y++)
		{
			for(x = floor(entity->pos.x - halfw); x < entity->pos.x + halfw; x++)
			{
				if(block_issolid(world_getblock(x, y, b, 0)))
				{
					entity->pos.z = b + 1 + halfw + .0001;
					entity->velocity.z = 0;
					flag = 1;
					break;
				}
			}
		}
		if(flag)
		{
			friction.x += 1;
			friction.y += 1;
		}
	}
	
	double mag = sqrt(friction.x*friction.x + friction.y*friction.y + friction.z*friction.z);
	
	if(mag)
	{
		friction.x *= entity->friction.x/mag;
		friction.y *= entity->friction.y/mag;
		friction.z *= entity->friction.z/mag;
	}

	return friction;
}

void
entity_move(entity_t *entity, vec3_t *delta)
{
	move(entity, delta);
}

void
entity_update(entity_t *entity, vec3_t *forces, double dt)
{
	entity->velocity.x += forces->x*dt/entity->m;
	entity->velocity.y += (forces->y/entity->m-9.8)*dt;
	entity->velocity.z += forces->z*dt/entity->m;
	
	vec3_t delta;
	delta.x = entity->velocity.x*dt;
	delta.y = entity->velocity.y*dt;
	delta.z = entity->velocity.z*dt;
	
	if(entity->velocity.x > 0)
	{
		entity->velocity.x -= entity->workingfrict.x*dt;
		entity->velocity.x = entity->velocity.x < 0 ? 0 : entity->velocity.x;
	}
	if(entity->velocity.x < 0)
	{
		entity->velocity.x += entity->workingfrict.x*dt;
		entity->velocity.x = entity->velocity.x > 0 ? 0 : entity->velocity.x;
	}
	
	if(entity->velocity.y > 0)
	{
		entity->velocity.y -= entity->workingfrict.y*dt;
		entity->velocity.y = entity->velocity.y < 0 ? 0 : entity->velocity.y;
	}
	if(entity->velocity.y < 0)
	{
		entity->velocity.y += entity->workingfrict.y*dt;
		entity->velocity.y = entity->velocity.y > 0 ? 0 : entity->velocity.y;
	}
	
	if(entity->velocity.z > 0)
	{
		entity->velocity.z -= entity->workingfrict.z*dt;
		entity->velocity.z = entity->velocity.z < 0 ? 0 : entity->velocity.z;
	}
	if(entity->velocity.z < 0)
	{
		entity->velocity.z += entity->workingfrict.z*dt;
		entity->velocity.z = entity->velocity.z > 0 ? 0 : entity->velocity.z;
	}
	
	entity->workingfrict = move(entity, &delta);
}
