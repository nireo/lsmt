#include "bloom.h"
#include "utils.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// TODO: test different hash functions here
uint32_t
hash_djb2(const void* buff, size_t length)
{
  uint32_t hash = 5381;
  const uint8_t* data = buff;
  for (size_t i = 0; i < length; i++) {
    hash = ((hash << 5) + hash) + data[i];
  }
  return hash;
}

uint32_t
hash_sdbm(const void* buff, size_t length)
{
  uint32_t hash = 0;
  const uint8_t* data = buff;
  for (size_t i = 0; i < length; i++) {
    hash = data[i] + (hash << 6) + (hash << 16) - hash;
  }
  return hash;
}

bloom_filter*
bloom_filter_new(size_t size, size_t num_functions, ...)
{
  va_list argp;
  bloom_filter* filter = malloc(sizeof(*filter));
  if (NULL == filter) {
    fprintf(stderr, "Out of memory.\n");
    exit(EXIT_FAILURE);
  }

  filter->num_items = 0;
  filter->vec = bit_vec_new(size);
  filter->num_functions = num_functions;
  filter->hash_functions = malloc(sizeof(hash32_func) * num_functions);
  if (NULL == filter->hash_functions) {
    fprintf(stderr, "Out of memory.\n");
    exit(EXIT_FAILURE);
  }
  va_start(argp, num_functions);
  for (int i = 0; i < num_functions; i++) {
    filter->hash_functions[i] = va_arg(argp, hash32_func);
  }
  va_end(argp);
  return filter;
}

bloom_filter*
bloom_filter_new_default(size_t size)
{
  return bloom_filter_new(size, 2, hash_djb2, hash_sdbm);
}

void
bloom_filter_free(bloom_filter* filter)
{
  bit_vec_free(filter->vec);
  free(filter->hash_functions);
  free(filter);
}

void
bloom_filter_put(bloom_filter* filter, const void* data, size_t length)
{
  for (int i = 0; i < filter->num_functions; i++) {
    uint32_t cur_hash = filter->hash_functions[i](data, length);
    bit_vec_set(filter->vec, cur_hash % filter->vec->size, true);
  }
  filter->num_items++;
}

void
bloom_filter_put_str(bloom_filter* filter, const char* str)
{
  bloom_filter_put(filter, str, strlen(str));
}

bool
bloom_filter_test(bloom_filter* filter, const void* data, size_t lentgth)
{
  for (int i = 0; i < filter->num_functions; i++) {
    uint32_t cur_hash = filter->hash_functions[i](data, lentgth);
    if (!bit_vec_get(filter->vec, cur_hash % filter->vec->size)) {
      return false;
    }
  }
  return true;
}

bool
bloom_filter_test_str(bloom_filter* filter, const char* str)
{
  return bloom_filter_test(filter, str, strlen(str));
}

int
bloom_filter_dump(bloom_filter* filter, const char* path)
{
  FILE* fp = fopen(path, "wb");
  if (!fp) {
    return -1;
  }

  if (fwrite(&filter->num_functions, sizeof(size_t), 1, fp) != 1 ||
      fwrite(&filter->num_items, sizeof(size_t), 1, fp) != 1 ||
      fwrite(&filter->vec->size, sizeof(size_t), 1, fp) != 1) {
    fclose(fp);
    return -1;
  }

  size_t mem_size = filter->vec->size / BITS_IN_TYPE(uint32_t);
  if (filter->vec->size % BITS_IN_TYPE(uint32_t)) {
    mem_size++;
  }

  if (fwrite(filter->vec->mem, sizeof(uint32_t), mem_size, fp) != mem_size) {
    fclose(fp);
    return -1;
  }
  fclose(fp);

  return 0;
}

bloom_filter*
bloom_filter_from_file(const char* path)
{
  FILE* fp = fopen(path, "rb");
  if (!fp) {
    return NULL;
  }

  bloom_filter* filter = malloc(sizeof(bloom_filter));
  if (!filter) {
    fclose(fp);
    return NULL;
  }

  if (fread(&filter->num_functions, sizeof(size_t), 1, fp) != 1 ||
      fread(&filter->num_items, sizeof(size_t), 1, fp) != 1) {
    free(filter);
    fclose(fp);
    return NULL;
  }

  size_t num_bits;
  if (fread(&num_bits, sizeof(size_t), 1, fp) != 1) {
    free(filter);
    fclose(fp);
    return NULL;
  }

  filter->vec = malloc(sizeof(bit_vec));
  if (!filter->vec) {
    free(filter);
    fclose(fp);
    return NULL;
  }

  size_t mem_size = num_bits / BITS_IN_TYPE(uint32_t);
  if (num_bits % BITS_IN_TYPE(uint32_t)) {
    mem_size++;
  }

  filter->vec->mem = malloc(mem_size * sizeof(uint32_t));
  if (!filter->vec->mem) {
    free(filter->vec);
    free(filter);
    fclose(fp);
    return NULL;
  }
  filter->vec->size = num_bits;

  if (fread(filter->vec->mem, sizeof(uint32_t), mem_size, fp) != mem_size) {
    free(filter->vec->mem);
    free(filter->vec);
    free(filter);
    fclose(fp);
    return NULL;
  }

  // TODO: maybe support different hash functions here
  filter->hash_functions = malloc(sizeof(hash32_func) * 2);
  filter->hash_functions[0] = hash_djb2;
  filter->hash_functions[1] = hash_sdbm;
  fclose(fp);

  return filter;
}
