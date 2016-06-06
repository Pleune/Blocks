#ifndef WORLDGEN_H
#define WORLDGEN_H

#include "chunk.h"

typedef struct worldgen_s worldgen_t;

worldgen_t *worldgen_context_create();
void worldgen_context_destroy(worldgen_t *context);

void worldgen_genchunk(worldgen_t *context, chunk_t *chunk, long3_t *cpos);
long worldgen_get_height_of_pos(worldgen_t *context, long x, long z);

#endif
