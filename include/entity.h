#ifndef ENTITY_H
#define ENTITY_H

#include "custommath.h"
#include "directions.h"

typedef struct entity_s entity_t;

void entity_setsize(entity_t *entity, double width, double hight);

void entity_setpos(entity_t *entity, vec3_t pos);
vec3_t entity_getpos(entity_t *entity);
const vec3_t *entity_getposptr(entity_t *entity);

void entity_move(entity_t *entity, vec3_t *delta);
void entity_update(entity_t *entity, vec3_t *forces, double dt);

#endif
