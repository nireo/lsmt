#ifndef __SKIPLIST_H__
#define __SKIPLIST_H__

typedef struct sk_range_s {
    int min;
    int max;
    int minex;
    int maxex;
} sk_range;

typedef struct sk_link_s {
    struct sk_link_s *next;
    struct sk_link_s *prev;
    int span;
} sk_link;

typedef struct sk_node_s {
} sk_node;

typedef struct skiplist_s {
    int level;
    int count;
} skiplist;

#endif
