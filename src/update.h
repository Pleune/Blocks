#ifndef UPDATE_H
#define UPDATE_H

#include "block.h"

typedef uint8_t update_flags_t;

#define UPDATE_FLAGS_FLOW_WATER 0b1
#define UPDATE_FLAGS_FALL 0b10

void update_run(block_t b, long3_t pos, update_flags_t flags);

#endif
