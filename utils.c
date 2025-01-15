#include "utils.h"
#include <stdio.h>
#include <stdlib.h>

// bit_vec allocates a new bit vector
bit_vec *bit_vec_new(size_t num_bits) {
  bit_vec *vec = malloc(sizeof(*vec));
  if (vec == NULL) {
    fprintf(stderr, "out of memory.\n");
    exit(EXIT_FAILURE);
  }

  size_t mem_size = num_bits / BITS_IN_TYPE(uint32_t);
  if (!(num_bits % BITS_IN_TYPE(u_int32_t))) {
    mem_size++;
  }

  vec->mem = calloc(mem_size, sizeof(*(vec->mem)));
  if (vec->mem == NULL) {
    fprintf(stderr, "out of memory\n");
    exit(EXIT_FAILURE);
  }

  vec->size = num_bits;
  return vec;
}

void bit_vec_free(bit_vec *vec) {
  free(vec->mem);
  free(vec);
}

bool bit_vec_get(bit_vec *vec, size_t bit_idx) {
  if (bit_idx >= vec->size) {
    fprintf(stderr, "Out of bounds bit_idx=%zu, vect->size=%zu\n", bit_idx,
            vec->size);
    exit(EXIT_FAILURE);
  }
  size_t chunk_offset = bit_idx / BITS_IN_TYPE(uint32_t);
  size_t bit_offset = bit_idx & (BITS_IN_TYPE(uint32_t) - 1);
  uint32_t byte = vec->mem[chunk_offset];
  return (byte >> bit_offset) & 1;
}

void bit_vec_set(bit_vec *vec, size_t bit_idx, bool val) {
  if (bit_idx >= vec->size) {
    fprintf(stderr, "out of bounds bit_idx=%zu, vect->size=%zu\n", bit_idx,
            vec->size);
    exit(EXIT_FAILURE);
  }
  size_t chunk_offset = bit_idx / BITS_IN_TYPE(uint32_t);
  size_t bit_offset = bit_idx & (BITS_IN_TYPE(uint32_t) - 1);
  uint32_t *byte = &(vec->mem[chunk_offset]);
  if (val) {
    *byte |= ((uint32_t)1) << bit_offset;
  } else {
    *byte &= ~(1 << bit_offset);
  }
}
