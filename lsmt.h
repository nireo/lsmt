#include "memtable.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct lsm_tree {
  char *data_dir_path;
  memtable *active;
  memtable *old_memtables; // the memtables that are not yet flushed (linked list)
} lsm_tree;

lsm_tree *lsm_tree_new(const char *data_dir_path);
