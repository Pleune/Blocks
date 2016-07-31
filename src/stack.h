#ifndef STACK_H
#define STACK_H

#include <stdlib.h>

typedef struct stack stack_t;

stack_t *stack_create(size_t object_size, size_t object_count, double resize_factor);
void stack_destroy(stack_t *stack);

void stack_trim(stack_t *stack);
void stack_resize(stack_t *stack, size_t size);
void stack_ensure_size(stack_t *stack, size_t size);

size_t stack_objects_get_num(stack_t *stack);

void stack_push(stack_t *stack, const void *data);
void *stack_pop(stack_t *stack, void *data);
void *stack_element_ref(stack_t *stack, size_t index);
void *stack_element_replace_from_end(stack_t *stack, size_t index);//returns ref to index

void stack_push_mult(stack_t *stack, const void *data, size_t count);
void stack_advance(stack_t *stack, size_t count);

void *stack_transform_dataptr(stack_t *stack);

#endif
