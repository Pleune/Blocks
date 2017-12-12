#include "hash.h"

uint32_t
hash_uint32(const uint32_t a)
{
	uint32_t tmp = a;
	tmp = (a+0x7ed55d16) + (tmp<<12);
	tmp = (a^0xc761c23c) ^ (tmp>>19);
	tmp = (a+0x165667b1) + (tmp<<5);
	tmp = (a+0xd3a2646c) ^ (tmp<<9);
	tmp = (a+0xfd7046c5) + (tmp<<3);
	tmp = (a^0xb55a4f09) ^ (tmp>>16);
	return tmp;
}

//TODO: better string hash
uint32_t
hash_nullterminated(const char* a)
{
	uint32_t sum = 0;
	while(*a != 0)
	{
		sum += *a;
		++a;
	}

	return hash_uint32(sum);
}
