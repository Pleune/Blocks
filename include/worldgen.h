#ifndef WORLDGEN_H
#define WORLDGEN_H

#include "chunk.h"

typedef struct worldgen_s worldgen_t;

worldgen_t *worldgen_createcontext();
void worldgen_destroycontext(worldgen_t *context);

void worldgen_genchunk(worldgen_t *context, chunk_t *chunk, long3_t *cpos);
long worldgen_getheightfrompos(worldgen_t *context, long x, long z);

#endif
