#include "bloom.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// TODO: replace these
uint32_t djb2(const void *buff, size_t length) {
    uint32_t hash = 5381;
    const uint8_t *data = buff;
    for(size_t i = 0; i < length; i++) {
         hash = ((hash << 5) + hash) + data[i]; 
    }
    return hash;
}
uint32_t sdbm(const void *buff, size_t length) {
    uint32_t hash = 0;
    const uint8_t *data = buff;
    for(size_t i = 0; i < length; i++) {
        hash = data[i] + (hash << 6) + (hash << 16) - hash;
    }
    return hash;
}

bloom_filter *bloom_filter_new(size_t size, size_t num_functions, ...) {
    va_list argp;
    bloom_filter *filter = malloc(sizeof(*filter));
    if (NULL==filter) {
        fprintf(stderr, "Out of memory.\n");
        exit(EXIT_FAILURE);
    }

    filter->num_items = 0;
    filter->vec = bit_vec_new(size);
    filter->num_functions = num_functions;
    filter->hash_functions = malloc(sizeof(hash32_func)*num_functions);
    if (NULL==filter->hash_functions) {
        fprintf(stderr, "Out of memory.\n");
        exit(EXIT_FAILURE);    
    }
    va_start(argp, num_functions);
    for(int i = 0; i < num_functions; i++) {
        filter->hash_functions[i] = va_arg(argp, hash32_func);
    }
    va_end(argp);
    return filter;
}

bloom_filter *bloom_filter_new_default(size_t size) {
    return bloom_filter_new(size, 2, djb2, sdbm);
} 

void bloom_filter_free(bloom_filter *filter) {
    bit_vec_free(filter->vec);
    free(filter->hash_functions);
    free(filter);
}

void bloom_filter_put(bloom_filter *filter, const void *data, size_t length){
    for(int i = 0; i < filter->num_functions; i++) {
        uint32_t cur_hash = filter->hash_functions[i](data, length);
        bit_vec_set(filter->vec, cur_hash % filter->vec->size, true);
    }
    filter->num_items++;
}

void bloom_filter_put_str(bloom_filter *filter, const char *str) {
    bloom_filter_put(filter, str, strlen(str));
}

bool bloom_filter_test(bloom_filter *filter, const void *data, size_t lentgth) {
    for(int i = 0; i < filter->num_functions; i++) {
        uint32_t cur_hash = filter->hash_functions[i](data, lentgth);
        if (!bit_vec_get(filter->vec, cur_hash % filter->vec->size)) {
            return false;
        }
    }
    return true;
}
bool bloom_filter_test_str(bloom_filter *filter, const char *str) {
    return bloom_filter_test(filter, str, strlen(str));
}
