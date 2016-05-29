#include "stack.h"

#include <string.h>
#include <stdio.h>

inline static void
resize(struct stack *stack, size_t size)
{
	size_t offset = stack->top - stack->data;

	stack->size = size;
	unsigned char *new_data = realloc(stack->data, size);
	if(!new_data)
	{
		//TODO: error
		printf("!!! ERROR 0!!!\n");
	} else {
		stack->data = new_data;
	}

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
	{
		//TODO: error
		printf("!!! ERROR 1!!!\n");
	}
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
stack_numobjects(struct stack *stack)
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
	{
		//TODO: error
		printf("!!! ERROR 2!!! %li\n", stack->size);
	} else {
		stack->data = new_data;
	}

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
		//TODO: error
		printf("!!! ERROR 3!!!\n");
		return;
	}

	memcpy(data, stack->top, stack->object_size);
	stack->top -= stack->object_size;
}
