#ifndef CHUNK_H
#define CHUNK_H

#include <SDL2/SDL_thread.h>

#include "defines.h"
#include "custommath.h"

#include "block.h"
#include "mesh.h"

typedef struct chunk_s chunk_t;

//uses the adjacent chunks to cut out unneeded triangles. if the pointers are null, they work as a empty chunk.
mesh_t chunk_getmesh(chunk_t *chunk, chunk_t *chunkabove, chunk_t *chunkbelow, chunk_t *chunknorth, chunk_t *chunksouth, chunk_t *chunkeast, chunk_t *chunkwest);

block_t chunk_getblock(chunk_t *c, int x, int y, int z);
void chunk_setblock(chunk_t *c, int x, int y, int z, block_t b);

long3_t chunk_getpos(chunk_t *chunk);

//loads compleatly new chunks -- basically malloc
chunk_t * chunk_loadchunk(long3_t pos);
chunk_t * chunk_loademptychunk(long3_t pos);

//releases a chunk from a load call
void chunk_freechunk(chunk_t *chunk);

int chunk_reloadchunk(long3_t pos, chunk_t *chunk);

void chunk_zerochunk(chunk_t *chunk);

#endif //CHUNK_H
