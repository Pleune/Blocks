#include "stack.h"

#include <string.h>
#include <stdio.h>

#include "debug.h"

struct stack {
	size_t object_size;
	size_t size;

	double resize_factor;

	unsigned char *data;
	unsigned char *top; //non inclusive
	unsigned char *end; //non inclusive
};

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

struct stack *
stack_create(size_t object_size, size_t object_count, double resize_factor)
{
	struct stack *stack = malloc(sizeof(struct stack));

	stack->object_size = object_size;
	stack->size = object_count * object_size;

	stack->data = malloc(stack->size);
	if(!stack->data)
		fail("stack_init malloc failed");

	stack->top = stack->data;
	stack->end = stack->data + stack->size;

	stack->resize_factor = resize_factor;

	return stack;
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
		size_t new_size = stack->size * stack->resize_factor;
		if(new_size == 0)
			new_size = stack->object_size;

		resize(stack, new_size);
	}

	memcpy(stack->top, data, stack->object_size);
	stack->top += stack->object_size;
}

void *
stack_pop(struct stack *stack, void *data)
{
	if(stack->top == stack->data)
		return 0;

	stack->top -= stack->object_size;
	memcpy(data, stack->top, stack->object_size);

	return data;
}

void *
stack_element_ref(stack_t* stack, size_t index)
{
	void *ref = stack->data + stack->object_size * index;
	if(ref + stack->object_size <= (void *)stack->top)
	{
		return ref;
	} else {
		error("stack_element_ref(): index out of bounds");
		return 0;
	}
}

void *
stack_element_replace_from_end(stack_t* stack, size_t index)
{
	void *ref;
	if((ref = stack_element_ref(stack, index)) == 0)
		return 0;//error message printed above

	stack_pop(stack, ref);
	return ref;
}

void
stack_push_mult(struct stack* stack, void* data, size_t count)
{
	size_t current_size = stack->size;
	size_t needed_size = (stack->top - stack->data) + stack->object_size*count;

	while(current_size <= needed_size)
		current_size *= stack->resize_factor;

	if(current_size > stack->size)
		resize(stack, current_size);

	memcpy(stack->top, data, stack->object_size * count);
	stack->top += stack->object_size * count;
}

void *
stack_transform_dataptr(stack_t *stack)
{
	void *ret = stack->data;
	free(stack);
	return ret;
}
