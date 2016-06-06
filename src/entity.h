#ifndef ENTITY_H
#define ENTITY_H

#include "custommath.h"
#include "directions.h"

typedef struct entity_s entity_t;

entity_t *entity_create(double x, double y, double z, double w, double h, double m);
void entity_destroy(entity_t *entity);

void entity_size_set(entity_t *entity, double width, double hight);
void entity_friction_set(entity_t *entity, vec3_t f);
void entity_pos_set(entity_t *entity, vec3_t pos);
vec3_t entity_pos_get(entity_t *entity);
const vec3_t *entity_pos_get_ptr(entity_t *entity);

void entity_move(entity_t *entity, vec3_t *delta);
void entity_update(entity_t *entity, vec3_t *forces, double dt);
void entity_jump(entity_t *entity, double y);

#endif
