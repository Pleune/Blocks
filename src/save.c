#include "save.h"

#include <stdio.h>
#include <unistd.h>

//TODO: windows compat!
#include <sys/stat.h>

#include <SDL.h>

#include "debug.h"
#include "hmap.h"
#include "stack.h"
#include "minmax.h"

#define VERSION 0x01

/*
 * !!! FORMAT !!!
 *
 * HEADER:
 * {
 *     string: "BLKSAV" (no null termination)
 *     byte: VERSION
 * }
 * SECTIONS:
 * {
 *     uint32: num_sections
 *     {
 *         string: section_name
 *         uint64: section_len
 *         uint64: section_ptr
 *     }
 *     {
 *         string: section_name
 *         uint64: section_len
 *         uint64: section_ptr
 *     }
 *     {
 *         string: section_name
 *         uint64: section_len
 *         uint64: section_ptr
 *     }
 *     ...
 * }
 */

struct header {
	char name[SAVE_SECTION_NAME_MAX_LEN+1];
	size_t len_data;
};

struct section_malloc {
	unsigned char *data;
	size_t len;
};

struct save {
	char *path;
	hmap_t *sections; //map: NAME > struct section_malloc
	uint32_t num_sections;
	SDL_mutex *mutex;
};

#define READ_UINT(type)							\
	type##_t read_##type(FILE *f)				\
	{											\
		size_t i;								\
		size_t size = sizeof(type##_t);			\
		type##_t ret = 0;						\
		unsigned char data[size];				\
		fread(data, 1, size, f);				\
		for(i=0; i<size; ++i)					\
			ret |= data[i] << i*8;				\
		return ret;								\
	}

#define WRITE_UINT(type)						\
	void write_##type(FILE *f, type##_t a)		\
	{											\
	    size_t i;								\
		size_t size = sizeof(type##_t);			\
		unsigned char data[size];				\
		for(i=0; i<size; ++i)					\
			data[i] = a >> i*8;					\
		fwrite(data, 1, size, f);				\
	}

READ_UINT(uint8)
READ_UINT(uint16)
READ_UINT(uint32)
READ_UINT(uint64)

WRITE_UINT(uint8)
WRITE_UINT(uint16)
WRITE_UINT(uint32)
WRITE_UINT(uint64)

#define SAVE_READ_UINT(type)											\
	type##_t save_read_##type(const unsigned char *data)				\
	{																	\
		size_t size = sizeof(type##_t);									\
		size_t i;														\
		type##_t ret = 0;												\
		for(i=0; i<size; ++i)											\
		    ret |= data[i] << i*8;										\
		return ret;														\
	}

#define SAVE_WRITE_UINT(type)											\
	void save_write_##type(unsigned char *data, type##_t a)				\
	{																	\
		size_t size = sizeof(type##_t);									\
		size_t i;														\
		for(i=0; i<size; ++i)											\
			data[i] = a >> i*8;											\
	}

SAVE_READ_UINT(uint8)
SAVE_READ_UINT(uint16)
SAVE_READ_UINT(uint32)
SAVE_READ_UINT(uint64)

SAVE_WRITE_UINT(uint8)
SAVE_WRITE_UINT(uint16)
SAVE_WRITE_UINT(uint32)
SAVE_WRITE_UINT(uint64)

#define SAVE_READ_INT(type)												\
	type##_t save_read_##type(const unsigned char *data)				\
	{																	\
		size_t size = sizeof(type##_t);									\
		size_t i;														\
		unsigned char tmp[size];										\
		memcpy(tmp, data, size);										\
		int sign = (tmp[size-1] & 0b10000000) ? 1 : 0;					\
		tmp[size-1] &= 0b01111111;										\
		type##_t ret = 0;												\
		for(i=0; i<size; ++i)											\
		    ret |= tmp[i] << i*8;										\
		if(sign==0)														\
			ret *= -1;													\
		return ret;														\
	}

#define SAVE_WRITE_INT(type)											\
	void save_write_##type(unsigned char *data, type##_t a)				\
	{																	\
		size_t size = sizeof(type##_t);									\
		size_t i;														\
		type##_t tmp;													\
		if(a < 0)														\
			tmp = a *-1;												\
		else															\
			tmp = a;													\
		for(i=0; i<size; ++i)											\
			data[i] = tmp >> (i*8);										\
		data[size-1] &= 0b01111111;										\
		if(a > 0)														\
			data[size-1] |= 0b10000000;									\
	}

SAVE_READ_INT(int8)
SAVE_READ_INT(int16)
SAVE_READ_INT(int32)
SAVE_READ_INT(int64)

SAVE_WRITE_INT(int8)
SAVE_WRITE_INT(int16)
SAVE_WRITE_INT(int32)
SAVE_WRITE_INT(int64)

void
free_section_malloc(struct section_malloc *ptr)
{
	free(ptr->data);
	free(ptr);
}

static long
read_string(FILE *f, char *str, size_t max_len)
{
	char name[max_len];
	size_t len_name = 0;

	for(len_name=0; len_name<max_len; ++len_name)
	{
		name[len_name] = getc(f);

		if(name[len_name] == 0)
			break;

		if(feof(f))
		{
			error("save string not teminated (EOF reached) %s", name);
			return -1;
		}

	}

	strcpy(str, name);
	return len_name;
}

inline static int
write_string(FILE *f, const char *str)
{
	size_t len = strlen(str) + 1;
	fwrite(str, 1, len, f);
	return len;
}

char *
string_dupe(const char *string)
{
	char *ret = malloc(strlen(string)+1);
	strcpy(ret, string);
	return ret;
}

static int
file_exists(const char *filename)
{
    struct stat st;
    int result = stat(filename, &st);
    return result == 0;
}

static void
header_write(FILE *f)
{
	fwrite("BLKSAV", 1, 6, f);
	putc(VERSION, f);
}

static int
header_read(FILE *f)
{
	const char data[7];
	size_t ret = fread((unsigned char *)data, 1, 7, f);

	if(ferror(f))
	{
		error("save_open_file(): ferror reading header");
		return BLOCKS_FAIL;
	}

	if(ret != 7)
	{
		error("save_open_file(): could not read header bytes");
		return BLOCKS_FAIL;
	}

	if(strncmp(data, "BLKSAV", 6) != 0)
	{
		error("save_open_file(): wrong file type!");
		return BLOCKS_FAIL;
	}

	if(data[6] != VERSION)
	{
		error("save_open_file(): wrong save version!");
		return BLOCKS_FAIL;
	}

	return BLOCKS_SUCCESS;
}

static void
sections_write(FILE *f, save_t *save)
{
	write_uint32(f, save->num_sections);

	struct hmap_keypair *keypairs;
	size_t num_keypairs;
	hmap_dump_array(save->sections, &keypairs, &num_keypairs);

	stack_t *sections_ptrs = stack_create(sizeof(size_t), save->num_sections, 1.5);
	stack_t *sections_ptrs_positions = stack_create(sizeof(size_t), save->num_sections, 1.5);

	uint32_t i;
	for(i=0; i<save->num_sections; ++i)
	{
		write_string(f, keypairs[i].key);
		size_t section_len = ((struct section_malloc *)(keypairs[i].data))->len;
		write_uint64(f, section_len);
		size_t pos = ftell(f);
		write_uint64(f, 0x4E4E);//"NN" for empty
		stack_push(sections_ptrs, &pos);
	}

	for(i=0; i<save->num_sections; ++i)
	{
		size_t pos = ftell(f);
		stack_push(sections_ptrs_positions, &pos);
		size_t section_len = ((struct section_malloc *)(keypairs[i].data))->len;
		unsigned char *data = ((struct section_malloc *)(keypairs[i].data))->data;
		fwrite(data, 1, section_len, f);
	}

	for(i=0; i<save->num_sections; ++i)
	{
		size_t *ptr_pos = stack_element_ref(sections_ptrs, i);
		size_t *ptr_data = stack_element_ref(sections_ptrs_positions, i);

		fseek(f, *ptr_pos, SEEK_SET);
		write_uint64(f, *ptr_data);
	}

	if(num_keypairs > 0)
		free(keypairs);

	stack_destroy(sections_ptrs);
	stack_destroy(sections_ptrs_positions);
}

static int
sections_read(FILE *f, save_t *save)
{
	save->num_sections = read_uint32(f);

	struct section_info {
		char *name_ptr;
		size_t len_section;
		size_t ptr;
	};

	stack_t *sections = stack_create(sizeof(struct section_info), 100, 1.5);

	uint32_t i;
	for(i=0; i<save->num_sections; ++i)
	{
		struct section_info section;

		//read name:
		char name[256];
		long len_name = read_string(f, name, sizeof(name));
		if(len_name == -1)
		{
			stack_destroy(sections);
			return BLOCKS_FAIL;
		}
		section.name_ptr = string_dupe(name);
		//read len:
		section.len_section = read_uint64(f);
		//read ptr:
		section.ptr = read_uint64(f);

		stack_push(sections, &section);
	}

	for(i=0; i<save->num_sections; ++i)
	{
		struct section_info *section = stack_element_ref(sections, i);
		struct section_malloc *section_data = malloc(sizeof(struct section_malloc));
		section_data->data = malloc(section->len_section);
		void *section_data_ptr = section_data->data;
		if(fseek(f, section->ptr, SEEK_SET) != 0)
		{
			error(
				"save_open_file(): could not feek() to section pointer (section %s index: %i). invalid pointer?",
				section->name_ptr, i);
			stack_destroy(sections);
			free_section_malloc(section_data);
			return BLOCKS_FAIL;
		}

		if(fread(section_data_ptr, 1, section->len_section, f) != section->len_section)
		{
			error(
				"save_open_file(): could not fread() to section (section %s index: %i). invalid section len?",
				section->name_ptr, i);
			free_section_malloc(section_data);
			return BLOCKS_FAIL;
		}

		if(hmap_insert(save->sections, section->name_ptr, section_data) != BLOCKS_SUCCESS)
		{
			error("save_open_file(): failed to add section to map");
			free_section_malloc(section_data);
		}
	}

	stack_destroy(sections);

	info("save_open_file(): %s %i sections read sucessfully.", save->path, save->num_sections);

	return BLOCKS_SUCCESS;
}

save_t *
open_new(const char *path)
{
	struct save *save = malloc(sizeof(struct save));

	save->path = malloc(strlen(path)+1);
	strcpy(save->path, path);

	save->num_sections = 0;
	save->sections = hmap_create(hmap_hash_nullterminated, hmap_compare_nullterminated, (hmap_free)free, (hmap_free)free_section_malloc);

	save->mutex = SDL_CreateMutex();

	return save;
}

save_t *
save_open_file(const char *path)
{
	if(!file_exists(path))
		return open_new(path);

	FILE *file = fopen(path, "r+b");

	struct save *save = malloc(sizeof(struct save));

	save->path = string_dupe(path);

	if(header_read(file) != BLOCKS_SUCCESS)
	{
		free(save->path);
		free(save);
		fclose(file);
		return 0; //error printed in header_read()
	}

	save->num_sections = 0;
	save->sections = hmap_create(hmap_hash_nullterminated, hmap_compare_nullterminated, (hmap_free)free, (hmap_free)free_section_malloc);

	if(sections_read(file, save) != BLOCKS_SUCCESS)
	{
		free(save->path);
		hmap_destroy(save->sections);
		free(save);
		fclose(file);
		return 0; //error printed in sections_read()
	}

	fclose(file);

	save->mutex = SDL_CreateMutex();

	return save;
}

int
backup_make(save_t *save)
{
	char *new_path = malloc(strlen(save->path) + 5);
	strcpy(new_path, save->path);
	strcat(new_path, ".bak");

	if(file_exists(new_path))
	{
		if(remove(new_path) != 0)
		{
			free(new_path);
			return BLOCKS_FAIL;
		}
	}

	int ret = rename(save->path, new_path);
	free(new_path);

	if(ret != 0)
		return BLOCKS_FAIL;
	else
		return BLOCKS_SUCCESS;
}

int backup_restore(save_t *save)
{
	char *new_path = malloc(strlen(save->path) + 5);
	strcpy(new_path, save->path);
	strcat(new_path, ".bak");

	if(file_exists(save->path))
	{
		if(remove(save->path) != 0)
		{
			free(new_path);
			return BLOCKS_FAIL;
		}
	}

	int ret = rename(new_path, save->path);
	free(new_path);

	if(ret != 0)
		return BLOCKS_FAIL;
	else
		return BLOCKS_SUCCESS;
}

int
save_flush(save_t *save)
{
	SDL_LockMutex(save->mutex);
	backup_make(save);
	FILE *file = fopen(save->path, "w+b");
	fseek(file, 0, SEEK_SET);
	if(file == 0)
		return BLOCKS_FAIL;

	header_write(file);
	sections_write(file, save);

	fclose(file);

	SDL_UnlockMutex(save->mutex);

	return BLOCKS_SUCCESS;
}

int
save_close(save_t *save)
{
	if(save_flush(save) != BLOCKS_SUCCESS)
		return BLOCKS_FAIL;

	hmap_destroy(save->sections);
	free(save->path);
	SDL_DestroyMutex(save->mutex);
	free(save);

	return BLOCKS_SUCCESS;
}

int
save_section_new(save_t *save, const char *name, unsigned char *data, size_t data_len)
{
	SDL_LockMutex(save->mutex);

	char *name_ptr = string_dupe(name);

	struct section_malloc *section_data = malloc(sizeof(struct section_malloc));
	section_data->len = data_len;
	section_data->data = data;
	if(hmap_insert(save->sections, name_ptr, section_data) != BLOCKS_SUCCESS)
	{
		free(section_data);
		free(name_ptr);
		SDL_UnlockMutex(save->mutex);
		error("save_section_new(): could not add section. duplicate key?");
		return BLOCKS_FAIL;
	}

	save->num_sections++;

	SDL_UnlockMutex(save->mutex);

	return BLOCKS_SUCCESS;
}

size_t save_section_append(save_t *save, const char *name, unsigned char *data, size_t data_len);//Returns new new len
int save_section_remove(save_t *save, const char *name);

int
save_write_section(save_t *save, const char *section, unsigned char *data, size_t len)
{
	SDL_LockMutex(save->mutex);

	struct section_malloc *section_data = hmap_lookup(save->sections, section);
	if(section_data)
	{
		//TODO: change to save_section_remove()
		hmap_remove(save->sections, section);
		save->num_sections--;
	}

	save_section_new(save, section, data, len);

	SDL_UnlockMutex(save->mutex);

	return BLOCKS_SUCCESS;
}

const unsigned char *
save_get_section(save_t *save, const char *section)
{
	SDL_LockMutex(save->mutex);
	struct section_malloc *section_data = hmap_lookup(save->sections, section);
	SDL_UnlockMutex(save->mutex);
	if(section_data)
		return section_data->data;
	return 0;
}
