#ifndef ENTITY_H
#define ENTITY_H

#include "custommath.h"

typedef struct entity_s entity_t;

void setsize();

void setpos(entity_t *entity, vec3_t pos);
vec3_t getpos(entity_t *entity);

void setforces(entity_t *entity, vec3_t forces);

void update(entity_t *entity, double dt);

#endif
