#include "hmap.h"

#include <string.h>

#include "hash.h"
#include "stack.h"
#include "standard.h"

struct keypair {
	uint32_t hash;
	void *key;
	void *data;
};

struct hmap {
	stack_t ** buckets;
	int size;
	hmap_hash hash_func;
	hmap_compare compare_func;
};

static int
get_bucket_no(struct hmap *hmap, void *key, uint32_t *hash)
{
	*hash = hmap->hash_func(key);
	return *hash % hmap->size;
}

hmap_t *
hmap_create(hmap_hash hash_func, hmap_compare compare_func, int size)
{
	struct hmap *ret = malloc(sizeof(struct hmap));
	ret->hash_func = hash_func;
	ret->compare_func = compare_func;
	ret->size = size;

	ret->buckets = calloc(size, sizeof(stack_t *));

	int i;
	for(i=0; i<size; ++i)
		ret->buckets[i] = stack_create(sizeof(struct keypair), 0, 2.0);

	return ret;
}

void
hmap_destroy(hmap_t *hmap)
{
	int i;
	for(i=0; i<hmap->size; ++i)
	{
		struct keypair keypair;
		while(stack_pop(hmap->buckets[i], &keypair))
		{
			free(keypair.data);
			free(keypair.key);
		};

		stack_destroy(hmap->buckets[i]);
	}

	free(hmap->buckets);
	free(hmap);
}

void
hmap_insert(hmap_t *hmap, void *key, void *data)
{
	uint32_t hash;
	int bucket_no = get_bucket_no(hmap, key, &hash);

	struct keypair keypair;
	keypair.data = data;
	keypair.hash = hash;
	keypair.key = key;

	stack_push(hmap->buckets[bucket_no], &keypair);
}

void *
hmap_lookup(hmap_t *hmap, void *key)
{
	uint32_t hash;
	int bucket_no = get_bucket_no(hmap, key, &hash);

	struct keypair *keypair;

	int i;
	for(i=0; (keypair = stack_element_ref(hmap->buckets[bucket_no], i)) != 0; ++i)
	{
		if(keypair->hash == hash)
			if(hmap->compare_func(keypair->key, key))
				return keypair->data;
	}

	return 0;
}

int hmap_remove(hmap_t *hmap, void *key)
{

	uint32_t hash;
	int bucket_no = get_bucket_no(hmap, key, &hash);

	struct keypair *keypair;
	int i;
	for(i=0; (keypair = stack_element_ref(hmap->buckets[bucket_no], i)) != 0; ++i)
	{
		if(keypair->hash == hash)
			if(hmap->compare_func(keypair->key, key))
			{
				stack_element_replace_from_end(hmap->buckets[bucket_no], i);
				return BLOCKS_SUCCESS;
			}
	}

	return BLOCKS_FAIL;
}

//Default hash functions
uint32_t
hmap_hash_uint32(void *key)
{
	return hash_uint32(*(uint32_t *)key);
}

uint32_t
hmap_hash_nullterminated(void *key)
{
	return hash_nullterminated(key);
}

//Default compare functions
int
hmap_compare_uint32(void *a, void *b)
{
	uint32_t *keya = a;
	uint32_t *keyb = b;
	return keya == keyb;
}

int
hmap_compare_nullterminated(void *a, void *b)
{
	return strcmp(a, b) == 0;
}
