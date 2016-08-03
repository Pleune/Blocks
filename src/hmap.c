#include "hmap.h"

#include <string.h>

#include "hash.h"
#include "stack.h"
#include "standard.h"
#include "debug.h"

struct keypair {
	uint32_t hash;
	void *key;
	void *data;
};

struct hmap {
	stack_t **buckets;
	int size;
	hmap_hash hash_func;
	hmap_compare compare_func;
};

static int
get_bucket_no(struct hmap *hmap, const void *key, uint32_t *hash)
{
	*hash = hmap->hash_func(key);
	return *hash % hmap->size;
}

hmap_t *
hmap_create(hmap_hash hash_func, hmap_compare compare_func)
{
	size_t size = 60000;

	struct hmap *ret = malloc(sizeof(struct hmap));
	ret->hash_func = hash_func;
	ret->compare_func = compare_func;
	ret->size = size;

	ret->buckets = calloc(size, sizeof(stack_t *));

	return ret;
}

void
hmap_destroy(hmap_t *hmap, hmap_free free_key, hmap_free free_data)
{
	int i;
	for(i=0; i<hmap->size; ++i)
	{
		stack_t *bucket = hmap->buckets[i];
		if(bucket != 0)
		{
			if(free_key || free_data)
			{
				struct keypair keypair;
				while(stack_pop(bucket, &keypair))
				{
					if(free_key)
						free_key(keypair.key);
					if(free_data)
						free_data(keypair.data);
				}
			}
			stack_destroy(bucket);
		}
	}

	free(hmap->buckets);
	free(hmap);
}

int
hmap_insert(hmap_t *hmap, void *key, void *data)
{
	uint32_t hash;
	int bucket_no = get_bucket_no(hmap, key, &hash);

	struct keypair keypair;
	keypair.data = data;
	keypair.hash = hash;
	keypair.key = key;

	if(hmap_lookup(hmap, key) != 0)
		return BLOCKS_FAIL;

	if(hmap->buckets[bucket_no] == 0)
		hmap->buckets[bucket_no] = stack_create(sizeof(struct keypair), 1, 4);

	stack_push(hmap->buckets[bucket_no], &keypair);

	return BLOCKS_SUCCESS;
}

void *
hmap_lookup(hmap_t *hmap, const void *key)
{
	uint32_t hash;
	int bucket_no = get_bucket_no(hmap, key, &hash);

	if(hmap->buckets[bucket_no] == 0)
		return 0;

	struct keypair *keypair;

	int i;
	int max = stack_objects_get_num(hmap->buckets[bucket_no]);
	for(i=0; i<max; ++i)
	{
		keypair = stack_element_ref(hmap->buckets[bucket_no], i);
		if(keypair->hash == hash)
		{
			if(hmap->compare_func(keypair->key, key))
			{
				return keypair->data;
			}
		}
	}

	return 0;
}

int
hmap_remove(hmap_t *hmap, const void *key)
{

	uint32_t hash;
	int bucket_no = get_bucket_no(hmap, key, &hash);

	if(hmap->buckets[bucket_no] == 0)
		return BLOCKS_ERROR;

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

void
hmap_dump_array(hmap_t *hmap, struct hmap_keypair** array, size_t* len)
{
	stack_t *tmp_array = stack_create(sizeof(struct hmap_keypair), 100, 1.5);

	int i;
	for(i=0; i<hmap->size; ++i)
	{
		stack_t *bucket = hmap->buckets[i];
		if(bucket != 0)
		{
			int num_elements = stack_objects_get_num(bucket);
			int x;
			for(x=0; x<num_elements; ++x)
			{
				struct keypair *keypair = stack_element_ref(bucket, x);
				struct hmap_keypair data_to_push;
				data_to_push.key = keypair->key;
				data_to_push.data = keypair->data;
				stack_push(tmp_array, &data_to_push);
			}
		}
	}

	*len = stack_objects_get_num(tmp_array);
	if(*len > 0)
	{
		stack_trim(tmp_array);
		*array = stack_transform_dataptr(tmp_array);
	} else {
		stack_destroy(tmp_array);
	}
}

//Default hash functions
uint32_t
hmap_hash_uint32(const void *key)
{
	return hash_uint32(*(const uint32_t *)key);
}

uint32_t
hmap_hash_nullterminated(const void *key)
{
	return hash_nullterminated(key);
}

//Default compare functions
int
hmap_compare_uint32(const void *a, const void *b)
{
	const uint32_t *keya = a;
	const uint32_t *keyb = b;
	return keya == keyb;
}

int
hmap_compare_nullterminated(const void *a, const void *b)
{
	return strcmp(a, b) == 0;
}
