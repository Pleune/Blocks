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
} chunk_p;

//uses the adjacent chunks to cut out unneeded triangles. if the pointers are null, they work as a empty chunk.
mesh_t chunk_getmesh(chunk_p *chunk, chunk_p *chunkabove, chunk_p *chunkbelow, chunk_p *chunknorth, chunk_p *chunksouth, chunk_p *chunkeast, chunk_p *chunkwest);

inline static block_t *chunk_mallocdata(){return (block_t *)malloc(sizeof(block_t) * CHUNKSIZE*CHUNKSIZE*CHUNKSIZE);}
inline static block_t *chunk_callocdata(){return (block_t *)calloc(CHUNKSIZE*CHUNKSIZE*CHUNKSIZE, sizeof(block_t));}

chunk_p chunk_initchunk(long3_t pos);
chunk_p chuhnk_mallocchunk(long3_t pos);
chunk_p chunk_callocchunk(long3_t pos);

void chunk_zerochunk(chunk_p *chunk);

void chunk_freechunk(chunk_p *chunk);

block_t chunk_getblock(chunk_p *c, int x, int y, int z);

#endif //CHUNK_H
