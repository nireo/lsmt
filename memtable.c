#include "memtable.h"
#include "bloom.h"
#include "skiplist.h"
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
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

memtable*
memtable_new_dir(size_t size, char* dir_path)
{
  memtable* mt = memtable_new(size);
  mt->wal = wal_open(dir_path);
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

  if (mt->wal) {
    int res = wal_put(mt->wal, key, strlen(key), value, strlen(value));
    if (res != 0) {
      return MEMTABLE_FAILED;
    }
  }

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
  if (mt->wal) {
    wal_close(mt->wal);
  }
  free(mt);
}

#define WAL_PUT     1
#define WAL_DELETE  2

#define WAL_MAGIC   0x57CCCC48
#define WAL_VERSION 1

wal*
wal_open(char* dir_path)
{
  wal* wl = calloc(1, sizeof(wal));
  if (!wl) {
    return NULL;
  }

  // Get Unix timestamp
  time_t now = time(NULL);
  char timestamp[32];
  snprintf(timestamp, sizeof(timestamp), "%ld", (long)now);

  // Construct full path: dir_path/timestamp.mem
  size_t path_len = strlen(dir_path) + strlen(timestamp) + 6; // +6 for '/', '.mem', and '\0'
  char* fname = malloc(path_len);
  if (!fname) {
    free(wl);
    return NULL;
  }
  snprintf(fname, path_len, "%s/%s.mem", dir_path, timestamp);

  wl->filename = fname;
  wl->seq = 1;

  wl->fd = open(fname, O_RDWR | O_CREAT, 0644);
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

  if (write(wl->fd, key, keysize) != keysize) {
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

void
wal_close(wal* wl)
{
  if (wl) {
    if (wl->fd >= 0) {
      close(wl->fd);
    }
    free(wl->filename);
    free(wl);
  }
}

memtable*
memtable_recover_from_wal(size_t size, const char* wal_path)
{
  memtable* mt = memtable_new(size);
  if (!mt) {
    return NULL;
  }

  int fd = open(wal_path, O_RDONLY);
  if (fd < 0) {
    memtable_free(mt);
    return NULL;
  }

  wal_header header;
  if (read(fd, &header, sizeof(header)) != sizeof(header)) {
    close(fd);
    memtable_free(mt);
    return NULL;
  }

  if (header.magic != WAL_MAGIC || header.version != WAL_VERSION) {
    close(fd);
    memtable_free(mt);
    return NULL;
  }

  while (1) {
    wal_entry_header entry_header;
    ssize_t read_size = read(fd, &entry_header, sizeof(entry_header));

    if (read_size == 0) { // EOF
      break;
    }
    if (read_size != sizeof(entry_header)) {
      close(fd);
      memtable_free(mt);
      return NULL;
    }

    char* key = malloc(entry_header.key_size + 1);
    char* value = NULL;
    if (!key) {
      close(fd);
      memtable_free(mt);
      return NULL;
    }

    if (read(fd, key, entry_header.key_size) != entry_header.key_size) {
      free(key);
      close(fd);
      memtable_free(mt);
      return NULL;
    }
    key[entry_header.key_size] = '\0';

    if (entry_header.type == WAL_PUT) {
      value = malloc(entry_header.value_size + 1);
      if (!value) {
        free(key);
        close(fd);
        memtable_free(mt);
        return NULL;
      }

      if (read(fd, value, entry_header.value_size) != entry_header.value_size) {
        free(key);
        free(value);
        close(fd);
        memtable_free(mt);
        return NULL;
      }
      value[entry_header.value_size] = '\0';

      memtable_insert(mt, key, value);
      free(value);
    } else if (entry_header.type == WAL_DELETE) {
      skiplist_remove(mt->skiplist, key);
    }

    free(key);
  }

  close(fd);
  return mt;
}
