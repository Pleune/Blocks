#ifndef SAVE_H
#define SAVE_H

#include <stdint.h>
#include <string.h>

#include "stack.h"
#include "standard.h"

#define SAVE_SECTION_NAME_MAX_LEN 20

typedef struct save save_t;

save_t * save_open_file(const char *path);
int save_flush(save_t *save);
int save_close(save_t *save);

int save_section_new(save_t *save, const char *name, unsigned char *data, size_t data_len);
size_t save_section_append(save_t *save, const char *name, unsigned char *data, size_t data_len);//Returns new new len
int save_section_remove(save_t *save, const char *name);

uint8_t save_read_uint8(const unsigned char *data);
uint16_t save_read_uint16(const unsigned char *data);
uint32_t save_read_uint32(const unsigned char *data);
uint64_t save_read_uint64(const unsigned char *data);

void save_write_uint8(unsigned char *data, uint8_t a);
void save_write_uint16(unsigned char *data, uint16_t a);
void save_write_uint32(unsigned char *data, uint32_t a);
void save_write_uint64(unsigned char *data, uint64_t a);

/*
double save_read_double(save_t *save, size_t pos);
void save_write_double(save_t *save, double d, size_t pos);
*/

/*
long save_read_string(save_t *save, char *str, size_t max_len);
long save_write_string(save_t *save, char *str, size_t max_len);
*/

int save_write_section(save_t *save, const char *section, unsigned char *data, size_t len);
const unsigned char *save_get_section(save_t *save, const char *section);

#endif
