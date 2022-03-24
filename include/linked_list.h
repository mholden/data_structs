#ifndef _LINKED_LIST_H_
#define _LINKED_LIST_H_

#include <stddef.h>
#include <stdbool.h>

typedef struct ll_node ll_node_t;
struct ll_node {
    void *ln_data;
	ll_node_t *ln_next;
};

typedef struct ll_ops {
    int (*llo_compare_fn)(void *, void *);
    void (*llo_destroy_data_fn)(void *);
    void (*llo_dump_data_fn)(void *);
} ll_ops_t;

typedef struct linked_list {
	ll_node_t *ll_root;
    ll_ops_t *ll_ops;
    size_t ll_nnodes;
} linked_list_t;

linked_list_t *ll_create(ll_ops_t *lops);
void ll_destroy(linked_list_t *ll);

int ll_insert(linked_list_t *ll, void *data);
int ll_find(linked_list_t *ll, void *to_find, void **data);
int ll_remove(linked_list_t *ll, void *to_remove);

bool ll_empty(linked_list_t *ll);

void ll_dump(linked_list_t *ll);
void ll_check(linked_list_t *ll);

#endif /* _LINKED_LIST_H_ */
