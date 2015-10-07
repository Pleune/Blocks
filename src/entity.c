#include "entity.h"

struct entity_s {
	vec3_t pos;
	double w;
	double h;

	vec3_t velocities;
};

void
entity_setsize(entity_t *entity, double width, double height)
{
	entity->w = width;
	entity->h = height;
	entity->velocities.x = 0;
	entity->velocities.y = 0;
	entity->velocities.z = 0;
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
}

void
entity_update(entity_t *entity, vec3_t *forces, double dt)
{
}
