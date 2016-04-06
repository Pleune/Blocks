#ifndef CHUNK_H
#define CHUNK_H

#include <SDL2/SDL_thread.h>

#include "defines.h"
#include "custommath.h"
#include "block.h"
#include "update.h"
#include "cat.h"

#define CHUNKSIZE (int) CAT(0x1p, CHUNKLEVELS)

typedef struct chunk_s chunk_t;

long3_t chunk_getworldposfromchunkpos(long3_t cpos, int x, int y, int z);

void chunk_render(chunk_t *chunk);
void chunk_remesh(chunk_t *chunk, chunk_t *chunkabove, chunk_t *chunkbelow, chunk_t *chunknorth, chunk_t *chunksouth, chunk_t *chunkeast, chunk_t *chunkwest);

int chunk_iscurrent(chunk_t *chunk);
void chunk_setnotcurrent(chunk_t *chunk);

block_t chunk_getblock(chunk_t *c, int x, int y, int z);
blockid_t chunk_getblockid(chunk_t *c, int x, int y, int z);
void chunk_setblock(chunk_t *c, int x, int y, int z, block_t b);
void chunk_setblockid(chunk_t *c, int x, int y, int z, blockid_t id);
void chunk_setair(chunk_t *c, int x, int y, int z);

long3_t chunk_getpos(chunk_t *chunk);

void chunk_updatequeue(chunk_t *chunk, int x, int y, int z, uint8_t time, update_flags_t flags);
long chunk_updaterun(chunk_t *chunk);

chunk_t *chunk_loadchunk(long3_t pos);
chunk_t *chunk_loademptychunk(long3_t pos);
void chunk_freechunk(chunk_t *chunk);

int chunk_reloadchunk(long3_t pos, chunk_t *chunk);
void chunk_zerochunk(chunk_t *chunk);

#endif //CHUNK_H
