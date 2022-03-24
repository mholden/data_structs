#ifndef _UD_GRAPH_H_
#define _UD_GRAPH_H_

#include <stdbool.h>

#include "avl-tree.h"
#include "linked_list.h"

typedef struct udg_path {
    linked_list_t *p_path; // linked list of udg_node_t's
} udg_path_t;

typedef struct ud_graph_ops {
    int (*go_compare_fn)(void *, void *);
    void (*go_destroy_data_fn)(void *);
    void (*go_dump_data_fn)(void *);
} ud_graph_ops_t;

typedef struct ud_graph {
    avl_tree_t *g_nodes; // avl tree of udg_node_t's
    ud_graph_ops_t *g_ops;
} ud_graph_t;

typedef struct udg_node {
    ud_graph_t *n_graph;
    void *n_data;
    avl_tree_t *n_adjs; // adjacencies: avl tree of udg_node_t's
} udg_node_t;

ud_graph_t *udg_create(ud_graph_ops_t *gops);
void udg_destroy(ud_graph_t *g);

int udg_add_node(ud_graph_t *g, void *data, udg_node_t **new_node);
int udg_add_edge(ud_graph_t *g, udg_node_t *n1, udg_node_t *n2);

int udg_get_node(ud_graph_t *g, void *data, udg_node_t **node);

int udg_iterate_df(ud_graph_t *g, udg_node_t *start, int (*callback)(void *data, void *ctx, bool *stop), void *ctx);
int udg_iterate_bf(ud_graph_t *g, udg_node_t *start, int (*callback)(void *data, void *ctx, bool *stop), void *ctx);

void udg_check(ud_graph_t *g);
void udg_dump(ud_graph_t *g);

int udg_shortest_path_df(ud_graph_t *g, udg_node_t *from, udg_node_t *to, udg_path_t **path);
int udg_shortest_path_bf(ud_graph_t *g, udg_node_t *from, udg_node_t *to, udg_path_t **path);

#endif /* _UD_GRAPH_H_ */
