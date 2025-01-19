#ifndef __SKIPLIST_H__
#define __SKIPLIST_H__

#define MAX_LEVEL 32

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
    const char *key;
    const char *value;
    sk_link link[0];
} sk_node;

typedef struct skiplist_s {
    int level;
    int count;
    sk_link head[MAX_LEVEL];
} skiplist;

skiplist *skiplist_new(void);
sk_node *skiplist_insert(skiplist *list, const char *key, const char *value);
void skiplist_remove(skiplist *list, const char *key);

#define list_entry(ptr, type, member) \
        ((type *)((char *)(ptr) - (size_t)(&((type *)0)->member)))

#define skiplist_foreach_forward(pos, end) \
        for (; pos != end; pos = (pos)->next)

#define skiplist_foreach_forward_safe(pos, n, end) \
        for (n = (pos)->next; pos != end; pos = n, n = (pos)->next)

#define skiplist_foreach_backward(pos, end) \
        for (; pos != end; pos = (pos)->prev)

#define skiplist_foreach_backward_safe(pos, n, end) \
        for (n = (pos)->prev; pos != end; pos = n, n = (pos)->prev)

#endif
