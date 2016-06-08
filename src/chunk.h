#ifndef CHUNK_H
#define CHUNK_H

#include <SDL_thread.h>

#include "defines.h"
#include "custommath.h"
#include "block.h"
#include "update.h"
#include "cat.h"

#define CHUNKSIZE (int) CAT(0x1p, CHUNK_LEVELS)

typedef struct chunk chunk_t;

void chunk_static_init();
void chunk_static_cleanup();

chunk_t *chunk_load_empty(long3_t pos);
void chunk_free(chunk_t *chunk);
void chunk_fill_air(chunk_t *chunk);

long3_t chunk_pos_get(chunk_t *chunk);
int chunk_recenter(chunk_t *chunk, long3_t *pos);

long chunk_render(chunk_t *chunk);
void chunk_remesh(chunk_t *chunk, chunk_t *chunkabove, chunk_t *chunkbelow, chunk_t *chunknorth, chunk_t *chunksouth, chunk_t *chunkeast, chunk_t *chunkwest);

void chunk_lock(chunk_t *chunk);
void chunk_unlock(chunk_t *chunk);

int chunk_mesh_is_current(chunk_t *chunk);
void chunk_mesh_clear_current(chunk_t *chunk);
void chunk_mesh_clear(chunk_t *chunk);

block_t chunk_block_get(chunk_t *c, int x, int y, int z);
blockid_t chunk_block_get_id(chunk_t *c, int x, int y, int z);
void chunk_block_set(chunk_t *c, int x, int y, int z, block_t b);
void chunk_block_set_id(chunk_t *c, int x, int y, int z, blockid_t id);

void chunk_update_queue(chunk_t *chunk, int x, int y, int z, int time, update_flags_t flags);
long chunk_update_run(chunk_t *chunk);

#endif
