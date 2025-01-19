#include "memtable.h"
#include "bloom.h"
#include "skiplist.h"
#include <string.h>

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
