#include "../memtable.h"
#include <assert.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

void
test_basic_operations()
{
  printf("Testing basic operations...\n");

  memtable* mt = memtable_new(1000);
  assert(mt != NULL && "Memtable creation failed");

  const char* test_key = "test_key";
  const char* test_value = "test_value";

  memtable_res res = memtable_insert(mt, test_key, test_value);
  assert(res == MEMTABLE_OK && "Insert failed");

  char* retrieved_value = NULL;
  res = memtable_get(mt, test_key, &retrieved_value);

  assert(res == MEMTABLE_OK && "Get failed");
  assert(strcmp(retrieved_value, test_value) == 0 && "Retrieved value doesn't match");

  free(retrieved_value);
  printf("Basic single insert/get test passed\n");

  const char* keys[] = { "key1", "key2", "key3", "key4", "key5" };
  const char* values[] = { "value1", "value2", "value3", "value4", "value5" };
  const int num_entries = 5;

  for (int i = 0; i < num_entries; i++) {
    res = memtable_insert(mt, keys[i], values[i]);
    assert(res == MEMTABLE_OK && "Multiple insert failed");
  }

  for (int i = 0; i < num_entries; i++) {
    retrieved_value = NULL;
    res = memtable_get(mt, keys[i], &retrieved_value);
    assert(res == MEMTABLE_OK && "Multiple get failed");
    assert(strcmp(retrieved_value, values[i]) == 0 && "Multiple retrieved value doesn't match");
    free(retrieved_value);
  }
  printf("Multiple inserts/gets test passed\n");

  // Test non-existent key
  retrieved_value = NULL;
  res = memtable_get(mt, "nonexistent_key", &retrieved_value);
  assert(res == MEMTABLE_FAILED && "Non-existent key test failed");

  printf("Non-existent key test passed\n");

  const char* update_value = "updated_value";
  res = memtable_insert(mt, keys[0], update_value);
  assert(res == MEMTABLE_OK && "Update insert failed");

  retrieved_value = NULL;
  res = memtable_get(mt, keys[0], &retrieved_value);
  assert(res == MEMTABLE_OK && "Update get failed");
  assert(strcmp(retrieved_value, update_value) == 0 && "Updated value doesn't match");
  free(retrieved_value);

  printf("Update existing key test passed\n");

  memtable_free(mt);
  printf("All basic operation tests passed!\n\n");
}

void
test_edge_cases()
{
  printf("Testing edge cases...\n");

  memtable* mt = memtable_new(1000);
  assert(mt != NULL && "Memtable creation failed");

  memtable_res res = memtable_insert(mt, "", "empty_key");
  assert(res == MEMTABLE_OK && "Empty key insert failed");

  res = memtable_insert(mt, "empty_value", "");
  assert(res == MEMTABLE_OK && "Empty value insert failed");

  char* retrieved_value = NULL;
  res = memtable_get(mt, "", &retrieved_value);
  assert(res == MEMTABLE_OK && "Empty key get failed");
  assert(strcmp(retrieved_value, "empty_key") == 0 && "Empty key value doesn't match");
  free(retrieved_value);

  retrieved_value = NULL;
  res = memtable_get(mt, "empty_value", &retrieved_value);
  assert(res == MEMTABLE_OK && "Empty value get failed");
  assert(strcmp(retrieved_value, "") == 0 && "Empty value doesn't match");
  free(retrieved_value);

  printf("Empty string tests passed\n");

  char long_key[1024];
  char long_value[1024];
  memset(long_key, 'k', 1023);
  memset(long_value, 'v', 1023);
  long_key[1023] = '\0';
  long_value[1023] = '\0';

  res = memtable_insert(mt, long_key, long_value);
  assert(res == MEMTABLE_OK && "Long string insert failed");

  retrieved_value = NULL;
  res = memtable_get(mt, long_key, &retrieved_value);
  assert(res == MEMTABLE_OK && "Long string get failed");
  assert(strcmp(retrieved_value, long_value) == 0 && "Long string value doesn't match");
  free(retrieved_value);

  printf("Long string tests passed\n");

  memtable_free(mt);
  printf("All edge case tests passed!\n\n");
}

void
test_wal_operations()
{
  printf("Testing WAL operations...\n");

  const char* test_dir = "test_wal_dir";
  mkdir(test_dir, 0777);

  memtable* mt = memtable_new_dir(1000, (char*)test_dir);
  assert(mt != NULL && "Failed to create memtable with WAL");
  assert(mt->wal != NULL && "WAL not initialized");

  const char* test_keys[] = { "key1", "key2", "key3" };
  const char* test_values[] = { "value1", "value2", "value3" };

  for (int i = 0; i < 3; i++) {
    memtable_res res = memtable_insert(mt, test_keys[i], test_values[i]);
    assert(res == MEMTABLE_OK && "Failed to insert with WAL");
  }

  char* wal_path = strdup(mt->wal->filename);

  memtable_free(mt);

  memtable* recovered_mt = memtable_recover_from_wal(1000, wal_path);
  assert(recovered_mt != NULL && "Failed to recover from WAL");

  for (int i = 0; i < 3; i++) {
    char* value = NULL;
    memtable_res res = memtable_get(recovered_mt, test_keys[i], &value);
    assert(res == MEMTABLE_OK && "Failed to get recovered value");
    assert(strcmp(value, test_values[i]) == 0 && "Recovered value doesn't match");
    free(value);
  }

  printf("Basic WAL recovery test passed\n");

  DIR* dir = opendir(test_dir);
  struct dirent* entry;
  while ((entry = readdir(dir)) != NULL) {
    if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
      char path[1024];
      snprintf(path, sizeof(path), "%s/%s", test_dir, entry->d_name);
      remove(path);
    }
  }
  closedir(dir);
  rmdir(test_dir);

  printf("All WAL operation tests passed!\n\n");
}

int
main()
{
  printf("Starting memtable tests...\n\n");

  test_basic_operations();
  // test_edge_cases();
  test_wal_operations();

  printf("All tests passed successfully!\n");
  return 0;
}
