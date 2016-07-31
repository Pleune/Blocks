#ifndef HASH_H
#define HASH_H

#include <stdint.h>

uint32_t hash_uint32(const uint32_t a);
uint32_t hash_nullterminated(const char *a);

#endif
