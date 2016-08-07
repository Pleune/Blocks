#ifndef HMAP_H
#define HMAP_H

#include <stdint.h>
#include "stack.h"

typedef struct hmap hmap_t;

typedef uint32_t (*hmap_hash)(const void *key);
typedef int (*hmap_compare)(const void *a, const void *b);
typedef void (*hmap_free)(void *ptr);

struct hmap_keypair {
	void *key;
	void *data;
};

hmap_t *hmap_create(hmap_hash hash_func, hmap_compare compare_func, hmap_free free_key, hmap_free free_data);
void hmap_destroy(hmap_t *hmap);

int hmap_insert(hmap_t *hmap, void *key, void *data);
void *hmap_lookup(hmap_t *hmap, const void *key);
int hmap_remove(hmap_t *hmap, const void *key);

void hmap_dump_array(hmap_t *hmap, struct hmap_keypair **array, size_t *len);

//Default hash functions
uint32_t hmap_hash_uint32(const void *key);
uint32_t hmap_hash_nullterminated(const void *key);

//Default compare functions
int hmap_compare_uint32(const void *a, const void *b);
int hmap_compare_nullterminated(const void *a, const void *b);

#endif
