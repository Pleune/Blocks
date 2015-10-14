#ifndef WORLDGEN_H
#define WORLDGEN_H

#include "chunk.h"

void worldgen_genchunk(chunk_t *chunk);
long worldgen_getheightfrompos(long x, long z);

#endif
