#ifndef CHUNK_H
#define CHUNK_H

#include <SDL2/SDL_thread.h>

#include "defines.h"
#include "custommath.h"
#include "block.h"
#include "cat.h"
#include "modulo.h"

#define CHUNKSIZE (int) CAT(0x1p, CHUNKLEVELS)

typedef struct chunk_s chunk_t;

void chunk_remesh(chunk_t *chunk, chunk_t *chunkabove, chunk_t *chunkbelow, chunk_t *chunknorth, chunk_t *chunksouth, chunk_t *chunkeast, chunk_t *chunkwest);

void chunk_setnotcurrent(chunk_t *chunk);
int chunk_iscurrent(chunk_t *chunk);
void chunk_render(chunk_t *chunk);

block_t chunk_getblock(chunk_t *c, int x, int y, int z);
blockid_t chunk_getblockid(chunk_t *c, int x, int y, int z);
void chunk_setblock(chunk_t *c, int x, int y, int z, block_t b);
void chunk_setblockid(chunk_t *c, int x, int y, int z, blockid_t id);
void chunk_setair(chunk_t *c, int x, int y, int z);

long3_t chunk_getpos(chunk_t *chunk);

chunk_t * chunk_loadchunk(long3_t pos);
chunk_t * chunk_loademptychunk(long3_t pos);
void chunk_freechunk(chunk_t *chunk);

int chunk_reloadchunk(long3_t pos, chunk_t *chunk);
void chunk_zerochunk(chunk_t *chunk);

static inline long3_t
chunk_getchunkofspot(long x, long y, long z)
{
	long3_t chunkpos;
	chunkpos.x = floor((double)x / CHUNKSIZE);
	chunkpos.y = floor((double)y / CHUNKSIZE);
	chunkpos.z = floor((double)z / CHUNKSIZE);
	return chunkpos;
}

static inline int3_t
chunk_getinternalspotofspot(long x, long y, long z)
{
	int3_t blockpos;
	blockpos.x = MODULO(x, CHUNKSIZE);
	blockpos.y = MODULO(y, CHUNKSIZE);
	blockpos.z = MODULO(z, CHUNKSIZE);
	return blockpos;
}

#endif //CHUNK_H
