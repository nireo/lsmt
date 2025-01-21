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

typedef struct memtable_s {
  bloom_filter* bloom_filter; // we can have this to speed up look ups.
  skiplist* skiplist;
  size_t taken_size;
  struct memtable_s *next;
} memtable;

memtable* memtable_new(size_t size);
memtable_res memtable_insert(memtable *mt, const char *key, const char *value);
memtable_res memtable_get(memtable *mt, const char *key, char **value);
void memtable_free(memtable *mt);

typedef struct wal_entry_header_s {
  uint32_t type;
  uint16_t key_size;
  uint32_t value_size;
  uint32_t checksum;
} wal_entry_header;

typedef struct wal_header_s {
  uint32_t magic;
  uint32_t version;
  uint64_t seq;
} wal_header;

typedef struct wal_s {
  int fd;
  char *filename;
  uint64_t seq;
} wal;

wal *wal_open(const char *filename);
int wal_put(wal *wl, const char *key, uint16_t key_size, const char *value, uint32_t value_size);
int wal_delete(wal *wl, const char *key, uint16_t key_size);
void wal_close(wal *wl);

#endif
