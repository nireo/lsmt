#ifndef __MEMTABLE_H__
#define __MEMTABLE_H__

#include "bloom.h"
#include "skiplist.h"
#include "utils.h"

// TODO: make this better 
typedef enum {
  MEMTABLE_OK,
  MEMTABLE_FAILED,
} memtable_res;

typedef struct {
  bloom_filter* bloom_filter; // we can have this to speed up look ups.
  skiplist* skiplist;
} memtable;

memtable* memtable_new(size_t size);
memtable_res memtable_insert(memtable *mt, const char *key, const char *value);
memtable_res memtable_get(memtable *mt, const char *key, char **value);
void memtable_free(memtable *mt);

#endif
