#ifndef _UDW_GRAPH_H_
#define _UDW_GRAPH_H_

#include <stdbool.h>

#include "avl_tree.h"
#include "linked_list.h"

typedef struct udw_graph_ops {
    int (*go_compare_fn)(void *, void *);
    void (*go_destroy_data_fn)(void *);
    void (*go_dump_data_fn)(void *);
} udw_graph_ops_t;

typedef struct udw_graph {
    avl_tree_t *g_nodes; // avl tree of udwg_node_t's
    udw_graph_ops_t *g_ops;
} udw_graph_t;

typedef struct udwg_node {
    udw_graph_t *n_graph;
    void *n_data;
    avl_tree_t *n_adjs; // adjacencies: avl tree of udwg_edge_t's
} udwg_node_t;

typedef struct udw_graph_edge {
    udwg_node_t *e_adj; // adjacent node
    int64_t e_weight;
} udwg_edge_t;

udw_graph_t *udwg_create(udw_graph_ops_t *gops);
void udwg_destroy(udw_graph_t *g);

int udwg_add_node(udw_graph_t *g, void *data, udwg_node_t **new_node);
int udwg_add_edge(udw_graph_t *g, udwg_node_t *n1, udwg_node_t *n2, int64_t weight);

int udwg_get_node(udw_graph_t *g, void *data, udwg_node_t **node);

int udwg_iterate_df(udw_graph_t *g, udwg_node_t *start, int (*callback)(void *data, void *ctx, bool *stop), void *ctx);
int udwg_iterate_bf(udw_graph_t *g, udwg_node_t *start, int (*callback)(void *data, void *ctx, bool *stop), void *ctx);

void udwg_check(udw_graph_t *g);
void udwg_dump(udw_graph_t *g);

int udwg_shortest_path_df(udw_graph_t *g, udwg_node_t *from, udwg_node_t *to, linked_list_t **path);
int udwg_shortest_path_bf(udw_graph_t *g, udwg_node_t *from, udwg_node_t *to, linked_list_t **path);



#endif /* _UDW_GRAPH_H_ */
