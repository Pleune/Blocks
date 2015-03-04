#ifndef CHUNK_H
#define CHUNK_H

#include "defines.h"

#include "block.h"
#include "mesh.h"

typedef struct {
	int points;
	block_t *data;
} chunk_t;

//uses the adjacent chunks to cut out unneeded triangles. if the pointers are null, they work as a empty chunk.
mesh_t getmesh(chunk_t chunk, block_t *chunkabove, block_t *chunkbelow, block_t *chunknorth, block_t *chunksouth, block_t *chunkeast, block_t *chunkwest);

chunk_t mallocchunk();
chunk_t callocchunk();

void zerochunk(chunk_t chunk);

#endif //CHUNK_H
