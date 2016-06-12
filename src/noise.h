#ifndef NOISE_H
#define NOISE_H

#include <stdint.h>

inline static uint32_t
hash( uint32_t a)
{
	a = (a+0x7ed55d16) + (a<<12);
	a = (a^0xc761c23c) ^ (a>>19);
	a = (a+0x165667b1) + (a<<5);
	a = (a+0xd3a2646c) ^ (a<<9);
	a = (a+0xfd7046c5) + (a<<3);
	a = (a^0xb55a4f09) ^ (a>>16);
	return a;
}

inline static uint32_t
noise1D(uint32_t x, uint32_t seed)
{
	return hash(x+seed);
}

inline static uint32_t
noise2D(uint32_t x, uint32_t y, uint32_t seed)
{
	return hash(((hash(x)<<16) ^ hash(y)) + seed);
}

inline static uint32_t
noise3D(uint32_t x, uint32_t y, uint32_t z, uint32_t seed)
{
	return hash((hash(x) ^ (hash(y)<<12) ^ (hash(z) << 24)) + seed);
}

#endif
