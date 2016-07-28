#ifndef HMAP_H
#define HMAP_H

#include <stdint.h>
#include "stack.h"

typedef struct hmap hmap_t;

typedef uint32_t (*hmap_hash)(void *key);
typedef int (*hmap_compare)(void *a, void *b);

hmap_t *hmap_create(hmap_hash hash_func, hmap_compare compare_func, int size);
void hmap_destroy(hmap_t *hmap);

void hmap_insert(hmap_t *hmap, void *key, void *data);
void *hmap_lookup(hmap_t *hmap, void *key);
int hmap_remove(hmap_t *hmap, void *key);

//Default hash functions
uint32_t hmap_hash_uint32(void *key);
uint32_t hmap_hash_nullterminated(void *key);

//Default compare functions
int hmap_compare_uint32(void *a, void *b);
int hmap_compare_nullterminated(void *a, void *b);

#endif
