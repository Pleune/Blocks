#include "entity.h"

#include <stdio.h>
#include <math.h>
#include <stdlib.h>

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

void
entity_move(entity_t *entity, vec3_t *delta)
{
	vec3_t startpos = entity->pos;
	entity->pos.x += delta->x;
	entity->pos.y += delta->y;
	entity->pos.z += delta->z;

	double a = floor(startpos.x);
	double b = floor(entity->pos.x);
	if(a < b)
	{
		printf("x inc\n");
	} else if(b < a)
	{
		printf("x dec\n");
	}

	a = floor(startpos.y);
	b = floor(entity->pos.y);
	if(a < b)
	{
		printf("y inc\n");
	} else if(b < a)
	{
		printf("y dec\n");
	}

	a = floor(startpos.z);
	b = floor(entity->pos.z);
	if(a < b)
	{
		printf("z inc\n");
	} else if(b < a)
	{
		printf("z dec\n");
	}
}

void
entity_update(entity_t *entity, vec3_t *forces, double dt)
{
}
