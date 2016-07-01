#include "stack.h"

#include <string.h>
#include <stdio.h>

#include "debug.h"

inline static void
resize(struct stack *stack, size_t size)
{
	size_t offset = stack->top - stack->data;

	stack->size = size;
	unsigned char *new_data = realloc(stack->data, size);
	if(!new_data)
		fail("stack resize realloc fail");
	else
		stack->data = new_data;

	stack->top = offset + stack->data;
	stack->end = stack->data + size;
}

void
stack_init(struct stack *stack, size_t object_size, size_t object_count, double resize_factor)
{
	stack->object_size = object_size;
	stack->size = object_count * object_size;

	stack->data = malloc(stack->size);
	if(!stack->data)
		fail("stack_init malloc failed");

	stack->top = stack->data;
	stack->end = stack->data + stack->size;

	stack->resize_factor = resize_factor;
}

void
stack_destroy(struct stack *stack)
{
	free(stack->data);
}

void
stack_resize(struct stack *stack, size_t object_count)
{
	resize(stack, object_count * stack->object_size);
}

long
stack_objects_get_num(struct stack *stack)
{
	return stack->size / stack->object_size;
}

void
stack_trim(struct stack *stack)
{
	stack->size = stack->top - stack->data;

	if(stack->size == 0)
		return;

	unsigned char *new_data = realloc(stack->data, stack->size);
	if(!new_data)
		error("stack_trim realloc failed");
	else
		stack->data = new_data;

	stack->end = stack->data + stack->size;
}

void
stack_push(struct stack *stack, void *data)
{
	if(stack->top >= stack->end)
	{
		resize(stack, stack->size * stack->resize_factor);
	}

	memcpy(stack->top, data, stack->object_size);
	stack->top += stack->object_size;
}

void
stack_pop(struct stack *stack, void *data)
{
	if(stack->top == stack->data)
	{
		error("stack_pop no elements to pop");
		return;
	}

	memcpy(data, stack->top, stack->object_size);
	stack->top -= stack->object_size;
}

void
stack_push_mult(struct stack* stack, void* data, size_t count)
{
	size_t current_size = stack->size;
	size_t needed_size = (stack->top - stack->data) + stack->object_size*count;

	while(current_size < needed_size)
		current_size *= stack->resize_factor;

	if(current_size > stack->size)
		resize(stack, current_size);

	memcpy(stack->top, data, stack->object_size * count);
	stack->top += stack->object_size * count;
}
