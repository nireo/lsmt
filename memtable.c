#include "memtable.h"
#include "bloom.h"
#include "skiplist.h"
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

memtable*
memtable_new(size_t size)
{
  memtable* mt = malloc(sizeof(memtable));
  if (mt == NULL) {
    return NULL;
  }

  mt->bloom_filter = bloom_filter_new_default(size);
  mt->skiplist = skiplist_new();
  mt->taken_size = 0;
  mt->next = NULL;

  return mt;
}

memtable_res
memtable_insert(memtable* mt, const char* key, const char* value)
{
  // TODO: handle proper add to size of memtable for keys that are updated.
  if (bloom_filter_test_str(mt->bloom_filter, key)) {
    skiplist_remove(mt->skiplist, key);
  }

  mt->taken_size += 4 + strlen(key) + strlen(value); // 4 bytes for 2 uint16_t representing key and value lengths
  bloom_filter_put_str(mt->bloom_filter, key);
  skiplist_insert(mt->skiplist, key, value);

  return MEMTABLE_OK;
}

memtable_res
memtable_get(memtable* mt, const char* key, char** value)
{
  // not in bloom filter we can ignore this
  if (!bloom_filter_test_str(mt->bloom_filter, key)) {
    return MEMTABLE_FAILED;
  }

  skipnode* n = skiplist_search_by_key(mt->skiplist, key);
  *value = strdup(n->value);
  return MEMTABLE_OK;
}

void
memtable_free(memtable* mt)
{
  bloom_filter_free(mt->bloom_filter);
  skiplist_delete(mt->skiplist);
  free(mt);
}

#define WAL_PUT     1
#define WAL_DELETE  2

#define WAL_MAGIC   0x57CCCC48
#define WAL_VERSION 1

wal*
wal_open(const char* fname)
{
  wal* wl = calloc(1, sizeof(wal));
  if (!wl) {
    return NULL;
  }
  wl->filename = strdup(fname);
  wl->seq = 1;

  wl->fd = open(fname, O_RDWR, O_CREAT, 0644);
  if (wl->fd < 0) {
    free(wl->filename);
    free(wl);
    return NULL;
  }

  if (lseek(wl->fd, 0, SEEK_END) == 0) {
    wal_header header = {
      .magic = WAL_MAGIC,
      .version = WAL_VERSION,
      .seq = wl->seq
    };

    if (write(wl->fd, &header, sizeof(header)) != sizeof(header)) {
      close(wl->fd);
      free(wl->filename);
      free(wl);
      return NULL;
    }
  }

  return wl;
}

int
wal_put(wal* wl, const char* key, uint16_t keysize, const char* value, uint32_t valsize)
{
  wal_entry_header header = {
    .type = WAL_PUT,
    .key_size = keysize,
    .value_size = valsize,
    .checksum = 0,
  };
  // TODO: calculate crc

  if (write(wl->fd, &header, sizeof(header)) != sizeof(header)) {
    return 1;
  }

  if (write(wl->fd, value, valsize) != valsize) {
    return 1;
  }
  fsync(wl->fd);
  wl->seq++;

  return 0;
}

int
wal_delete(wal* wl, const char* key, uint16_t keysize)
{
  wal_entry_header header = {
    .type = WAL_DELETE,
    .key_size = keysize,
    .value_size = 0,
    .checksum = 0,
  };

  if (write(wl->fd, &header, sizeof(header)) != sizeof(header)) {
    return 1;
  }

  if (write(wl->fd, &key, keysize) != keysize) {
    return 1;
  }

  fsync(wl->fd);
  wl->seq++;
  return 0;
}

void wal_close(wal *wl) {
  if (wl) {
    if (wl->fd >= 0) {
      close(wl->fd);
    }
    free(wl->filename);
    free(wl);
  }
}

