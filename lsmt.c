#include "lsmt.h"
#include "memtable.h"
#include "utils.h"
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static int
init_memtable_from_file(const char* path, memtable* mem)
{
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
  char basename[1024];

  while ((entry = readdir(dir)) != NULL) {
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
      continue;
    }

    const char* ext = get_file_ext(entry->d_name);
    if (strcmp(ext, "mem") == 0) {
      strncpy(basename, entry->d_name, strlen(entry->d_name) - 4);
      basename[strlen(entry->d_name) - 4] = '\0';

      char mem_path[1024], filter_path[1024];
      snprintf(mem_path, sizeof(mem_path), "%s/%s.mem", tree->data_dir_path, basename);
      snprintf(filter_path, sizeof(filter_path), "%s/%s.filter", tree->data_dir_path, basename);

      if (access(mem_path, F_OK) != 0 || access(filter_path, F_OK) != 0) {
        fprintf(stderr, "missing matching .mem or .filter file for %s\n", basename);
        closedir(dir);
        return 1;
      }

      memtable* mt = memtable_new(1024);
      if (mt == NULL) {
        fprintf(stderr, "failed to create memtable\n");
        closedir(dir);
        return 1;
      }

      bloom_filter* filter = bloom_filter_from_file(filter_path);
      if (filter == NULL) {
        fprintf(stderr, "failed to load bloom filter from %s\n", filter_path);
        memtable_free(mt);
        closedir(dir);
        return 1;
      }

      mt->bloom_filter = filter;

      int ret = init_memtable_from_file(mem_path, mt);
      if (ret != 0) {
        fprintf(stderr, "failed to init memtable from %s\n", mem_path);
        memtable_free(mt);
        closedir(dir);
        return 1;
      }

      if (tree->active == NULL) {
        tree->active = mt;
      } else {
        mt->next = tree->old_memtables;
        tree->old_memtables = mt;
      }
    }
  }

  closedir(dir);
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
