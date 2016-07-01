#ifndef STACK_H
#define STACK_H

#include "stdlib.h"

struct stack {
	size_t object_size;
	size_t size;

	double resize_factor;

	unsigned char *data;
	unsigned char *top; //non inclusive
	unsigned char *end; //non inclusive
};

void stack_init(struct stack *stack, size_t object_size, size_t object_count, double resize_factor);
void stack_destroy(struct stack *stack);

void stack_trim(struct stack *stack);
void stack_resize(struct stack *stack, size_t size);

long stack_objects_get_num(struct stack *stack);

void stack_push(struct stack *stack, void *data);
void stack_pop(struct stack *stack, void *data);

void stack_push_mult(struct stack *stack, void *data, size_t count);

#endif
