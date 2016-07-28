#ifndef NOISE_H
#define NOISE_H

#include <stdint.h>

#include "hash.h"

inline static uint32_t
noise1D(uint32_t x, uint32_t seed)
{
	return hash_uint32(x+seed);
}

inline static uint32_t
noise2D(uint32_t x, uint32_t y, uint32_t seed)
{
	return hash_uint32(((hash_uint32(x)<<16) ^ hash_uint32(y)) + seed);
}

inline static uint32_t
noise3D(uint32_t x, uint32_t y, uint32_t z, uint32_t seed)
{
	return hash_uint32((hash_uint32(x) ^ (hash_uint32(y)<<12) ^ (hash_uint32(z) << 24)) + seed);
}

#endif
