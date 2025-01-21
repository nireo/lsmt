#include "lsmt.h"
#include "memtable.h"
#include "utils.h"
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

static int init_memtable_from_file(const char *path, memtable *mem) {
  return 0;
}

static int
init_tree_from_path(lsm_tree* tree)
{
  DIR* dir = opendir(tree->data_dir_path);
  if (!dir) {
    fprintf(stderr, "failed to open directory");
    return 1;
  }

  struct dirent* entry;
  struct stat file_stat;
  char fullpath[1024];

  while ((entry = readdir(dir)) != NULL) {
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
      continue;
    }
    snprintf(fullpath, sizeof(fullpath), "%s/%s", tree->data_dir_path, entry->d_name);
    if (stat(fullpath, &file_stat) == 0) {
      if (S_ISREG(file_stat.st_mode)) {
        const char *ext = get_file_ext(entry->d_name);

        if (strcmp(ext, "mem") == 0) {
        }
      }
    }
  }

  return 0;
}

lsm_tree*
lsm_tree_new(const char* data_dir_path)
{
  lsm_tree* tree = malloc(sizeof(lsm_tree));
  tree->data_dir_path = strdup(data_dir_path);

  if (dir_exists(data_dir_path)) {
  } else {
    mkdir(data_dir_path, 0777);
  }

  return tree;
}

void
lsm_tree_free(lsm_tree* tree)
{
  memtable_free(tree->active);

  memtable* curr = tree->old_memtables;
  while (curr != NULL) {
    memtable* next = curr->next;
    memtable_free(curr);
    curr = next;
  }

  free(tree->data_dir_path);
  free(tree);
}
