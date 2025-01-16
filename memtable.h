#ifndef __MEMTABLE_H__
#define __MEMTABLE_H__

#include "bloom.h"
#include "utils.h"

typedef struct {
    bloom_filter *bloom_filter; // we can have this to speed up look ups.
} memtable;

#endif
