#ifndef CHUNK_H
#define CHUNK_H

#include <SDL2/SDL_thread.h>

#include "defines.h"
#include "custommath.h"

#include "block.h"
#include "mesh.h"

typedef struct {
	long3_t pos;
	block_t *data;
	SDL_mutex *lock;
	int writable;
} chunk_t;

//uses the adjacent chunks to cut out unneeded triangles. if the pointers are null, they work as a empty chunk.
mesh_t chunk_getmesh(chunk_t chunk, block_t *chunkabove, block_t *chunkbelow, block_t *chunknorth, block_t *chunksouth, block_t *chunkeast, block_t *chunkwest);

inline static block_t *chunk_mallocdata(){return (block_t *)malloc(sizeof(block_t) * CHUNKSIZE*CHUNKSIZE*CHUNKSIZE);}
inline static block_t *chunk_callocdata(){return (block_t *)calloc(CHUNKSIZE*CHUNKSIZE*CHUNKSIZE, sizeof(block_t));}

chunk_t chunk_initchunk(long3_t pos);
chunk_t chuhnk_mallocchunk(long3_t pos);
chunk_t chunk_callocchunk(long3_t pos);

void chunk_zerochunk(chunk_t chunk);

void chunk_freechunk(chunk_t chunk);

#endif //CHUNK_H
