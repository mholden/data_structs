#ifndef _AVL_TREE_H_
#define _AVL_TREE_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef struct avl_tree_node avl_tree_node_t;

struct avl_tree_node {
    int8_t atn_balance;
    void *atn_data;
    avl_tree_node_t *atn_rchild;
    avl_tree_node_t *atn_lchild;
};

typedef struct avl_tree_ops {
    int (*ato_compare_fn)(void *, void *);
    void (*ato_destroy_data_fn)(void *);
    void (*ato_dump_data_fn)(void *);
} avl_tree_ops_t;

typedef struct avl_tree {
    avl_tree_node_t *at_root;
    avl_tree_ops_t *at_ops;
    size_t at_nnodes;
} avl_tree_t;

avl_tree_t *at_create(avl_tree_ops_t *ato);
void at_destroy(avl_tree_t *at);

int at_insert(avl_tree_t *at, void *data);
int at_find(avl_tree_t *at, void *to_find, void **data);
int at_remove(avl_tree_t *at, void *data);

void at_iterate(avl_tree_t *at, void (*callback)(void *data, void *ctx, bool *stop), void *ctx);

void at_check(avl_tree_t *at);
void at_dump(avl_tree_t *at);

#endif // _AVL_TREE_H_
