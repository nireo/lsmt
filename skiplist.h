#ifndef _SKIPLIST_H
#define _SKIPLIST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct sk_link_s {
    struct sk_link_s *next, *prev;
    int span;
} sk_link;

static inline void
list_init(sk_link* link)
{
    link->prev = link;
    link->next = link;
}

static inline void
__list_add(sk_link* link, sk_link* prev, sk_link* next)
{
    link->next = next;
    link->prev = prev;
    next->prev = link;
    prev->next = link;
}

static inline void
__list_del(sk_link* prev, sk_link* next)
{
    prev->next = next;
    next->prev = prev;
}

static inline void
list_add(sk_link* link, sk_link* next)
{
    __list_add(link, next->prev, next);
}

static inline void
list_del(sk_link* link)
{
    __list_del(link->prev, link->next);
    list_init(link);
}

static inline int
list_empty(sk_link* link)
{
    return link->next == link;
}

#define list_entry(ptr, type, member) \
    ((type*)((char*)(ptr) - (size_t)(&((type*)0)->member)))

#define skiplist_foreach_forward(pos, end) \
    for (; pos != end; pos = (pos)->next)

#define skiplist_foreach_forward_safe(pos, n, end) \
    for (n = (pos)->next; pos != end; pos = n, n = (pos)->next)

#define skiplist_foreach_backward(pos, end) \
    for (; pos != end; pos = (pos)->prev)

#define skiplist_foreach_backward_safe(pos, n, end) \
    for (n = (pos)->prev; pos != end; pos = n, n = (pos)->prev)

#define MAX_LEVEL 32 /* Should be enough for 2^32 elements */

typedef struct {
    char* min;
    char* max;
    bool min_exclusive;
    bool max_exclusive; // If true, max is exclusive
} range_spec;

typedef struct skiplist_s {
    int level;
    int count;
    sk_link head[MAX_LEVEL];
} skiplist;

typedef struct skipnode_s {
    char* key;
    char* value;
    sk_link link[0];
} skipnode;

static skipnode*
skipnode_new(int level, const char* key, const char* value)
{
    skipnode* node = (skipnode*)malloc(sizeof(*node) + level * sizeof(sk_link));
    if (node != NULL) {
        node->key = strdup(key);
        node->value = strdup(value);
        if (node->key == NULL || node->value == NULL) {
            free(node->key);
            free(node->value);
            free(node);
            return NULL;
        }
    }
    return node;
}

static void
skipnode_delete(skipnode* node)
{
    free(node->key);
    free(node->value);
    free(node);
}

static skiplist*
skiplist_new(void)
{
    int i;
    skiplist* list = (skiplist*)malloc(sizeof(*list));
    if (list != NULL) {
        list->level = 1;
        list->count = 0;
        for (i = 0; i < sizeof(list->head) / sizeof(list->head[0]); i++) {
            list_init(&list->head[i]);
            list->head[i].span = 0;
        }
    }
    return list;
}

static void
skiplist_delete(skiplist* list)
{
    sk_link* n;
    skipnode* node;
    sk_link* pos = list->head[0].next;
    skiplist_foreach_forward_safe(pos, n, &list->head[0])
    {
        node = list_entry(pos, skipnode, link[0]);
        skipnode_delete(node);
    }
    free(list);
}

static int
random_level(void)
{
    int level = 1;
    const double p = 0.25;
    while ((random() & 0xffff) < 0xffff * p) {
        level++;
    }
    return level > MAX_LEVEL ? MAX_LEVEL : level;
}

static void
__remove(skiplist* list, skipnode* node, int level, sk_link** update)
{
    int i;
    int remain_level = list->level;
    for (i = 0; i < list->level; i++) {
        if (i < level) {
            list_del(&node->link[i]);
            update[i] = node->link[i].next;
            update[i]->span += node->link[i].span - 1;
        } else {
            update[i]->span--;
        }

        if (list_empty(&list->head[i])) {
            if (remain_level == list->level) {
                remain_level = i + 1;
            }
        }
    }

    skipnode_delete(node);
    list->count--;
    list->level = remain_level;
}

static skipnode*
skiplist_insert(skiplist* list, const char* key, const char* value)
{
    skipnode* nd;
    int rank[MAX_LEVEL];
    sk_link* update[MAX_LEVEL];
    int level = random_level();

    if (level > list->level) {
        list->level = level;
    }

    skipnode* node = skipnode_new(level, key, value);
    if (node != NULL) {
        int i = list->level - 1;
        sk_link* pos = &list->head[i];
        sk_link* end = &list->head[i];

        for (; i >= 0; i--) {
            rank[i] = i == list->level - 1 ? 0 : rank[i + 1];
            pos = pos->next;
            skiplist_foreach_forward(pos, end)
            {
                nd = list_entry(pos, skipnode, link[i]);
                if (strcmp(nd->key, key) >= 0) {
                    end = &nd->link[i];
                    break;
                }
                rank[i] += nd->link[i].span;
            }

            update[i] = end;
            pos = end->prev;
            pos--;
            end--;
        }

        for (i = 0; i < list->level; i++) {
            if (i < level) {
                list_add(&node->link[i], update[i]);
                node->link[i].span = rank[0] - rank[i] + 1;
                update[i]->span -= node->link[i].span - 1;
            } else {
                update[i]->span++;
            }
        }

        list->count++;
    }

    return node;
}

static void
skiplist_remove(skiplist* list, const char* key)
{
    skipnode* node;
    int i = list->level - 1;
    sk_link* pos = &list->head[i];
    sk_link* end = &list->head[i];
    sk_link *n, *update[MAX_LEVEL];

    for (; i >= 0; i--) {
        pos = pos->next;
        skiplist_foreach_forward_safe(pos, n, end)
        {
            node = list_entry(pos, skipnode, link[i]);
            if (strcmp(node->key, key) > 0) {
                end = &node->link[i];
                break;
            } else if (strcmp(node->key, key) == 0) {
                __remove(list, node, i + 1, update);
                return;
            }
        }
        update[i] = end;
        pos = end->prev;
        pos--;
        end--;
    }
}

static int
key_gte_min(const char* key, range_spec* range)
{
    int cmp = strcmp(key, range->min);
    return range->min_exclusive ? (cmp > 0) : (cmp >= 0);
}

static int
key_lte_max(const char* key, range_spec* range)
{
    int cmp = strcmp(key, range->max);
    return range->max_exclusive ? (cmp < 0) : (cmp <= 0);
}

static int
key_in_range(skiplist* list, range_spec* range)
{
    if (strcmp(range->min, range->max) > 0 ||
        (strcmp(range->min, range->max) == 0 && (range->min_exclusive || range->max_exclusive))) {
        return 0;
    }

    if (list_empty(&list->head[0])) {
        return 0;
    }

    sk_link* link = list->head[0].next;
    skipnode* node = list_entry(link, skipnode, link[0]);
    if (!key_lte_max(node->key, range)) {
        return 0;
    }

    link = list->head[0].prev;
    node = list_entry(link, skipnode, link[0]);
    if (!key_gte_min(node->key, range)) {
        return 0;
    }

    return 1;
}

static skipnode*
skiplist_search_by_key(skiplist* list, const char* key)
{
    int i = list->level - 1;
    sk_link* pos = &list->head[i];
    sk_link* end = &list->head[i];
    skipnode* node;

    for (; i >= 0; i--) {
        pos = pos->next;
        skiplist_foreach_forward(pos, end)
        {
            node = list_entry(pos, skipnode, link[i]);
            if (strcmp(node->key, key) == 0) {
                return node;
            }
            if (strcmp(node->key, key) > 0) {
                end = &node->link[i];
                break;
            }
        }
        pos = end->prev;
        pos--;
        end--;
    }

    return NULL;
}

static skipnode*
skiplist_search_by_rank(skiplist* list, int rank)
{
    if (rank == 0 || rank > list->count) {
        return NULL;
    }

    int i = list->level - 1;
    int traversed = 0;
    sk_link* pos = &list->head[i];
    sk_link* end = &list->head[i];
    skipnode* node;

    for (; i >= 0; i--) {
        pos = pos->next;
        skiplist_foreach_forward(pos, end)
        {
            node = list_entry(pos, skipnode, link[i]);
            if (traversed + node->link[i].span > rank) {
                end = &node->link[i];
                break;
            }
            traversed += node->link[i].span;
            if (rank == traversed) {
                return node;
            }
        }
        pos = end->prev;
        pos--;
        end--;
    }

    return NULL;
}

static void
skiplist_dump(skiplist* list)
{
    int traversed = 0;
    skipnode* node;
    int i = list->level - 1;
    sk_link* pos = &list->head[i];
    sk_link* end = &list->head[i];

    printf("\nTotal %d nodes: \n", list->count);
    for (; i >= 0; i--) {
        traversed = 0;
        pos = pos->next;
        printf("level %d:\n", i + 1);
        skiplist_foreach_forward(pos, end)
        {
            node = list_entry(pos, skipnode, link[i]);
            traversed += node->link[i].span;
            printf("key:%s value:%s rank:%d\n", node->key, node->value, traversed);
        }
        pos = &list->head[i];
        pos--;
        end--;
    }
}

#endif
