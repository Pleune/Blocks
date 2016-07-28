#ifndef SAVE_H
#define SAVE_H

#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "stack.h"
#include "standard.h"

inline static void
save_write_uint16(stack_t *stack, uint16_t d)
{
	unsigned char tmp;
	int i;
	for(i=0; i<2; i++)
	{
		tmp = d;
		stack_push(stack, &tmp);
		d >>= 8;
	}
}

inline static void
save_write_uint32(stack_t *stack, uint32_t d)
{
	unsigned char tmp;
	int i;
	for(i=0; i<4; i++)
	{
		tmp = d;
		stack_push(stack, &tmp);
		d >>= 8;
	}
}

inline static void
save_write_uint64(stack_t *stack, uint64_t d)
{
	unsigned char tmp;
	int i;
	for(i=0; i<8; i++)
	{
		tmp = d;
		stack_push(stack, &tmp);
		d >>= 8;
	}
}

inline static size_t
save_read_uint16(unsigned char *data, void *ret, size_t ret_size)
{
	int i;
	uint16_t *d = ret;
	memset(ret, 0, ret_size);
	for(i=0; i<2; i++)
		*d |= (uint16_t)(data[i]) << (i*8);
	return 2;
}

inline static size_t
save_read_uint32(unsigned char *data, void *ret, size_t ret_size)
{
	int i;
	uint32_t *d = ret;
	memset(ret, 0, ret_size);
	for(i=0; i<4; i++)
		*d |= (uint32_t)(data[i]) << (i*8);
	return 4;
}

inline static size_t
save_read_uint64(unsigned char *data, void *ret, size_t ret_size)
{
	int i;
	uint64_t *d = ret;
	memset(ret, 0, ret_size);
	for(i=0; i<8; i++)
		*d |= (uint64_t)(data[i]) << (i*8);
	return 8;
}

#endif
