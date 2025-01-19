#include <stdio.h>

#include "../bloom.h"

int
main(int argc, char* argv[])
{

  // test basic operations
  bloom_filter* filter = bloom_filter_new_default(1024);
  bloom_filter_put_str(filter, "abc");
  printf("%d\n", bloom_filter_test_str(filter, "abc"));
  printf("%d\n", bloom_filter_test_str(filter, "bcd"));
  printf("%d\n", bloom_filter_test_str(filter, "0"));
  printf("%d\n", bloom_filter_test_str(filter, "1"));
  bloom_filter_put_str(filter, "2");
  printf("%d\n", bloom_filter_test_str(filter, "2"));

  // test file dumps
  if (bloom_filter_dump(filter, "test_bloom.filter") != 0) {
    fprintf(stderr, "failed to save filter\n");
    bloom_filter_free(filter);
    return 1;
  }
  bloom_filter_free(filter);

  bloom_filter* loaded = bloom_filter_from_file("test_bloom.filter");
  if (!loaded) {
    fprintf(stderr, "failed to loead filter\n");
    return 1;
  }
  printf("\nLoaded filter tests:\n");
  printf("'abc' present: %d\n", bloom_filter_test_str(loaded, "abc"));
  printf("'bcd' present: %d\n", bloom_filter_test_str(loaded, "bcd"));
  printf("'2' present: %d\n", bloom_filter_test_str(loaded, "2"));

  bloom_filter_put_str(loaded, "new_item");
  printf("\nAfter adding 'new_item':\n");
  printf("'new_item' present: %d\n", bloom_filter_test_str(loaded, "new_item"));

  bloom_filter_free(loaded);
  remove("test_bloom.filter");
  printf("\nTest file cleaned up\n");

  return 0;
}
