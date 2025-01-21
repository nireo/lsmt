#ifndef __UTIL_H__
#define __UTIL_H__

#include <stddef.h>
#include <stdint.h>
#include <limits.h>
#include <stdbool.h>

#define BITS_IN_TYPE(ty) (CHAR_BIT * (sizeof(ty)))

typedef struct {
    uint32_t *mem;
    size_t size;
} bit_vec;

bit_vec *bit_vec_new(size_t num_bits);
void bit_vec_free(bit_vec *vec);
bool bit_vec_get(bit_vec *vec, size_t idx);
void bit_vec_set(bit_vec *vec, size_t idx, bool val);

bool dir_exists(const char *path);
const char *get_file_ext(const char *filename);

#endif
