#ifndef UPDATE_H
#define UPDATE_H

#include <stdlib.h>
#include <SDL.h>

#include "block.h"

typedef uint8_t update_flags_t;

#define UPDATE_FLAGS_FLOW_WATER 0b1
#define UPDATE_FLAGS_FALL 0b10

struct update_node {
	struct update_node *next;

	update_flags_t flags;

	long3_t pos;
	int time;
};

struct update_stack {
	struct update_node *queue;
	SDL_mutex *mutex;
	int misses;
};

typedef struct update_stack update_stack_t;

update_stack_t *update_stack_create();
void update_stack_destroy(update_stack_t *stack);
void update_stack_clear(update_stack_t *stack);

void update_queue(update_stack_t *stack, long x, long y, long z, int time, update_flags_t flags);
int update_run(update_stack_t *stack);
void update_run_single(const struct update_node *update);

void update_fail_once(update_stack_t *stack);

size_t update_dump(update_stack_t *stack, unsigned char **data);
void update_read(update_stack_t *stack, const long3_t *cpos, const unsigned char *data, size_t size);

#endif
