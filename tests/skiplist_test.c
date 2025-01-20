#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(__MACH__) && !defined(CLOCK_REALTIME)
#include <sys/time.h>

#define CLOCK_REALTIME  0
#define CLOCK_MONOTONIC 1

int
clock_gettime(int clk_id, struct timespec* t)
{
  struct timeval now;
  int rv = gettimeofday(&now, NULL);
  if (rv)
    return rv;
  t->tv_sec = now.tv_sec;
  t->tv_nsec = now.tv_usec * 1000;
  return 0;
}
#else
#include <time.h>
#endif

#include "../skiplist.h"

#define N                 1024 * 1024 // Reduced size for string test
#define MAX_STRING_LENGTH 16

static void
generate_random_string(char* str, size_t length)
{
  const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  if (length) {
    for (size_t i = 0; i < length - 1; i++) {
      int key = random() % (sizeof(charset) - 1);
      str[i] = charset[key];
    }
    str[length - 1] = '\0';
  }
}

int
main(void)
{
  struct timespec start, end;
  char** keys = (char**)malloc(N * sizeof(char*));
  char** values = (char**)malloc(N * sizeof(char*));

  if (keys == NULL || values == NULL) {
    fprintf(stderr, "Memory allocation failed\n");
    exit(-1);
  }

  for (int i = 0; i < N; i++) {
    keys[i] = (char*)malloc(MAX_STRING_LENGTH);
    values[i] = (char*)malloc(MAX_STRING_LENGTH);
    if (keys[i] == NULL || values[i] == NULL) {
      fprintf(stderr, "Memory allocation failed\n");
      exit(-1);
    }
  }

  skiplist* list = skiplist_new();
  if (list == NULL) {
    fprintf(stderr, "Skiplist creation failed\n");
    exit(-1);
  }

  printf("Test start!\n");
  printf("Adding %d nodes with string keys and values...\n", N);

  srandom(time(NULL));
  clock_gettime(CLOCK_MONOTONIC, &start);
  for (int i = 0; i < N; i++) {
    generate_random_string(keys[i], MAX_STRING_LENGTH);
    generate_random_string(values[i], MAX_STRING_LENGTH);
    skiplist_insert(list, keys[i], values[i]);
  }
  clock_gettime(CLOCK_MONOTONIC, &end);
  printf("Insertion time: %ldms\n",
      (end.tv_sec - start.tv_sec) * 1000 + (end.tv_nsec - start.tv_nsec) / 1000000);

  printf("Searching each node by key...\n");
  clock_gettime(CLOCK_MONOTONIC, &start);
  for (int i = 0; i < N; i++) {
    skipnode* node = skiplist_search_by_key(list, keys[i]);
    if (node != NULL) {
      if (strcmp(node->value, values[i]) != 0) {
        printf("Error: Value mismatch for key %s\n", keys[i]);
      }
    } else {
      printf("Error: Not found - Key: %s\n", keys[i]);
    }
  }
  clock_gettime(CLOCK_MONOTONIC, &end);
  printf("Search by key time: %ldms\n",
      (end.tv_sec - start.tv_sec) * 1000 + (end.tv_nsec - start.tv_nsec) / 1000000);

  if (N > 1) {
    printf("Testing range search...\n");
    range_spec range;
    range.min = keys[0];
    range.max = keys[1];
    range.min_exclusive = 0;
    range.max_exclusive = 0;

    clock_gettime(CLOCK_MONOTONIC, &start);
    if (key_in_range(list, &range)) {
      printf("Found keys in range [%s, %s]\n", range.min, range.max);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    printf("Range search time: %ldms\n",
        (end.tv_sec - start.tv_sec) * 1000 + (end.tv_nsec - start.tv_nsec) / 1000000);
  }

  printf("Removing all nodes...\n");
  clock_gettime(CLOCK_MONOTONIC, &start);
  for (int i = 0; i < N; i++) {
    skiplist_remove(list, keys[i]);
  }
  clock_gettime(CLOCK_MONOTONIC, &end);
  printf("Deletion time: %ldms\n",
      (end.tv_sec - start.tv_sec) * 1000 + (end.tv_nsec - start.tv_nsec) / 1000000);

  printf("End of Test.\n");

  skiplist_delete(list);
  for (int i = 0; i < N; i++) {
    free(keys[i]);
    free(values[i]);
  }
  free(keys);
  free(values);

  return 0;
}
