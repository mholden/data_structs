#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <string.h>

#include "udw_graph.h"
#include "heap.h"

static int udwg_compare_nodes(void *data1, void *data2) {
    udwg_node_t *n1, *n2;
    udw_graph_t *g;
    
    n1 = (udwg_node_t *)data1;
    n2 = (udwg_node_t *)data2;
    assert(n1->n_graph == n2->n_graph);
    g = n1->n_graph;
    
    return g->g_ops->go_compare_fn(n1->n_data, n2->n_data);
}

static void udwg_destroy_node(void *data) {
    udwg_node_t *n = (udwg_node_t *)data;
    udw_graph_t *g = n->n_graph;
    
    if (g->g_ops->go_destroy_data_fn)
        g->g_ops->go_destroy_data_fn(n->n_data);
    at_destroy(n->n_adjs);
    free(n);
}

static void udwg_dump_node(void *data) {
    udwg_node_t *n = (udwg_node_t *)data;
    udw_graph_t *g = n->n_graph;
    
    printf("n_data: ");
    if (g->g_ops->go_dump_data_fn)
        g->g_ops->go_dump_data_fn(n->n_data);
    printf("(num adjacencies %d) ", n->n_adjs->at_nnodes);
}

static avl_tree_ops_t udwg_aops = {
    .ato_compare_fn = udwg_compare_nodes,
    .ato_destroy_data_fn = udwg_destroy_node,
    .ato_dump_data_fn = udwg_dump_node,
};

udw_graph_t *udwg_create(udw_graph_ops_t *gops) {
    udw_graph_t *g = NULL;
    
    g = malloc(sizeof(udw_graph_t));
    if (!g)
        goto error_out;
    
    memset(g, 0, sizeof(g));
    
    g->g_nodes = at_create(&udwg_aops);
    if (!g->g_nodes)
        goto error_out;
    
    g->g_ops = malloc(sizeof(udw_graph_ops_t));
    if (!g->g_ops)
        goto error_out;
    
    memcpy(g->g_ops, gops, sizeof(udw_graph_ops_t));
    
    return g;
    
error_out:
    if (g) {
        if (g->g_nodes)
            at_destroy(g->g_nodes);
        free(g);
    }
    
    return NULL;
}

void udwg_destroy(udw_graph_t *g) {
    at_destroy(g->g_nodes);
    free(g->g_ops);
    free(g);
    return;
}

static int udwg_compare_node_adjs(void *data1, void *data2) {
    udwg_edge_t *e1, *e2;
    udwg_node_t *n1, *n2;
    udw_graph_t *g;
    
    e1 = (udwg_edge_t *)data1;
    e2 = (udwg_edge_t *)data2;
    
    n1 = e1->e_adj;
    n2 = e2->e_adj;
    assert(n1->n_graph == n2->n_graph);
    g = n1->n_graph;
    
    return g->g_ops->go_compare_fn(n1->n_data, n2->n_data);
}

static void udwg_destroy_node_adj(void *data) {
    udwg_edge_t *edge = (udwg_edge_t *)data;
    free(edge);
}

static void udwg_dump_node_adj(void *data) {
    udwg_edge_t *edge = (udwg_edge_t *)data;
    udwg_node_t *adj = edge->e_adj;
    udw_graph_t *g = adj->n_graph;
    
    printf("adj: ");
    if (g->g_ops->go_dump_data_fn)
        g->g_ops->go_dump_data_fn(adj->n_data);
    printf("weight %" PRId64 " ", edge->e_weight);
}

static avl_tree_ops_t udwg_node_aops = {
    .ato_compare_fn = udwg_compare_node_adjs,
    .ato_destroy_data_fn = udwg_destroy_node_adj,
    .ato_dump_data_fn = udwg_dump_node_adj,
};

int udwg_add_node(udw_graph_t *g, void *data, udwg_node_t **new) {
    udwg_node_t *n = NULL;
    int err;
    
    assert(data);
    
    n = malloc(sizeof(udwg_node_t));
    if (!n) {
        err = ENOMEM;
        goto error_out;
    }
    
    memset(n, 0, sizeof(*n));
    n->n_data = data;
    
    n->n_adjs = at_create(&udwg_node_aops);
    if (!n->n_adjs) {
        err = ENOMEM;
        goto error_out;
    }
    
    n->n_graph = g;
    
    err = at_insert(g->g_nodes, (void *)n);
    if (err)
        goto error_out;
    
    if (new)
        *new = n;
    
    return 0;
    
error_out:
    if (n) {
        if (n->n_adjs)
            at_destroy(n->n_adjs);
        free(n);
    }
    
    return err;
}

int udwg_add_edge(udw_graph_t *g, udwg_node_t *n1, udwg_node_t *n2, int64_t weight) {
    udwg_edge_t *e1 = NULL, *e2 = NULL;
    int err;
    
    e1 = malloc(sizeof(udwg_edge_t));
    if (!e1) {
        err = ENOMEM;
        goto error_out;
    }
    
    e2 = malloc(sizeof(udwg_edge_t));
    if (!e2) {
        err = ENOMEM;
        goto error_out;
    }
    
    memset(e1, 0, sizeof(udwg_edge_t));
    e1->e_adj = n2;
    e1->e_weight = weight;
    
    err = at_insert(n1->n_adjs, (void *)e1);
    if (err)
        goto error_out;

    memset(e2, 0, sizeof(udwg_edge_t));
    e2->e_adj = n1;
    e2->e_weight = weight;
    
    err = at_insert(n2->n_adjs, (void *)e2);
    if (err) {
        printf("udwg_add_edge: couldn't at_insert e2 into n2 adjs\n");
        goto error_out2;
    }
    
    return 0;
    
error_out2:
    if (at_remove(n1->n_adjs, e1))
        printf("udwg_add_edge: failed to remove e1 from n1 adjs\n");
    
error_out:
    if (e1)
        free(e1);
    if (e2)
        free(e2);
    
    return err;
}

int udwg_get_node(udw_graph_t *g, void *data, udwg_node_t **node){
    assert(0);
}

int udwg_iterate_df(udw_graph_t *g, udwg_node_t *start, int (*callback)(void *data, void *ctx, bool *stop), void *ctx){
    assert(0);
}

int udwg_iterate_bf(udw_graph_t *g, udwg_node_t *start, int (*callback)(void *data, void *ctx, bool *stop), void *ctx) {
    assert(0);
}

static int udwg_check_node_adjacencies_cb(void *data, void *ctx, bool *stop) {
    udwg_edge_t *edge = (udwg_edge_t *)data, search_edge, *found_edge;
    udwg_node_t *adj, *n;
    
    adj = edge->e_adj;
    n = (udwg_node_t *)ctx;
    
    memset(&search_edge, 0, sizeof(udwg_edge_t));
    search_edge.e_adj = n;
    
    assert(at_find(adj->n_adjs, (void *)&search_edge, (void **)&found_edge) == 0 &&
            (found_edge->e_weight == edge->e_weight));
    
    return 0;
}

static int udwg_check_node_cb(void *data, void *ctx, bool *stop) {
    udwg_node_t *n = (udwg_node_t *)data;
    
    // make sure all our adjacencies have us in their adjacency list
    return at_iterate(n->n_adjs, udwg_check_node_adjacencies_cb, (void *)n);
}

void udwg_check(udw_graph_t *g) {
    assert(at_iterate(g->g_nodes, udwg_check_node_cb, NULL) == 0);
}

static void udwg_dump_edge(void *data) {
    udwg_edge_t *edge = (udwg_edge_t *)data;
    udwg_node_t *adj = edge->e_adj;
    udw_graph_t *g = adj->n_graph;
    if (g->g_ops->go_dump_data_fn)
        g->g_ops->go_dump_data_fn(adj->n_data);
    printf("(%" PRId64 ") ", edge->e_weight);
}

static int udwg_dump_node_adjs_cb(void *data, void *ctx, bool *stop) {
    udwg_dump_edge(data);
    return 0;
}

static void udwg_dump_node_data(void *node) {
    udwg_node_t *n = (udwg_node_t *)node;
    udw_graph_t *g = n->n_graph;
    if (g->g_ops->go_dump_data_fn)
        g->g_ops->go_dump_data_fn(n->n_data);
}

static int udwg_dump_node_cb(void *data, void *ctx, bool *stop) {
    udwg_node_t *n = (udwg_node_t *)data;
    udw_graph_t *g = n->n_graph;
    
    //printf("node %p ", n);
    //printf("n_graph: %p ", g);
    //printf("n_data: ");
    udwg_dump_node_data((void *)n);
    printf(": ");
    at_iterate(n->n_adjs, udwg_dump_node_adjs_cb, NULL);
    printf("\n");
    
    return 0;
}

void udwg_dump(udw_graph_t *g) {
    at_iterate(g->g_nodes, udwg_dump_node_cb, NULL);
}

int udwg_shortest_path_df(udw_graph_t *g, udwg_node_t *from, udwg_node_t *to, linked_list_t **path) {
    assert(0);
}

static int udwg_path_len_cb(void *data, void *ctx, bool *stop) {
    udwg_edge_t *edge = (udwg_edge_t *)data;
    int64_t *plen = (int64_t *)ctx;
    
    *plen += edge->e_weight;
    
    return 0;
}

static int64_t udwg_path_len(linked_list_t *path) {
    int64_t plen = 0;
    ll_iterate(path, udwg_path_len_cb, &plen);
    return plen;
}

typedef struct udwg_spbf_cb_ctx {
    linked_list_t *path;
    heap_t *pq;
    udwg_node_t *to;
    linked_list_t *best_path;
} udwg_spbf_cb_ctx_t;

static int udwg_spbf_cb(void *data, void *ctx, bool *stop) {
    udwg_edge_t *edge = (udwg_edge_t *)data;
    udwg_spbf_cb_ctx_t *gsp_ctx = (udwg_spbf_cb_ctx_t *)ctx;
    udwg_node_t *node = edge->e_adj, *to = gsp_ctx->to;
    linked_list_t *path = gsp_ctx->path, *adj_path = NULL;
    heap_t *pq = gsp_ctx->pq;
    int err;
    
    if (ll_find(path, (void *)edge, NULL) == 0) // already visited this node
        goto out;
    
    adj_path = ll_create_copy(path);
    if (!adj_path) {
        err = ENOMEM;
        goto error_out;
    }
    
    err = ll_insert(adj_path, (void *)edge);
    if (err)
        goto error_out;
    
    if (udwg_compare_nodes(node, to) == 0) { // this is our destination node
        if (!gsp_ctx->best_path ||
                (udwg_path_len(adj_path) < udwg_path_len(gsp_ctx->best_path))) {
            //
            // and this path to it is better than anything we've seen
            // so far. so this is our new best path
            //
            if (gsp_ctx->best_path)
                ll_destroy(gsp_ctx->best_path);
            gsp_ctx->best_path = adj_path;
        }
        goto out;
    }
    
    err = heap_insert(pq, adj_path);
    if (err)
        goto error_out;
    
out:
    return 0;
    
error_out:
    if (adj_path)
        ll_destroy(adj_path);
    
    return err;
}

static int udwg_compare_paths(void *element1, void *element2) {
    linked_list_t *path1, *path2;
    int64_t plen1, plen2;
    
    path1 = (linked_list_t *)element1;
    path2 = (linked_list_t *)element2;
    
    plen1 = udwg_path_len(path1);
    plen2 = udwg_path_len(path2);
    
    // we want a min heap, so > -> -1
    if (plen1 > plen2)
        return -1;
    else if (plen2 > plen1)
        return 1;
    else // ==
        return 0;
}

static void udwg_destroy_path(void *element) {
    linked_list_t *path = (linked_list_t *)element;
    ll_destroy(path);
}

static void udwg_dump_path(void *element) {
    linked_list_t *path = (linked_list_t *)element;
    printf("plen %" PRIu64 " ", udwg_path_len(path));
    ll_dump(path);
}

static heap_ops_t spbf_pq_ops = {
    .heap_compare = udwg_compare_paths,
    .heap_destroy_element = udwg_destroy_path,
    .heap_dump_element = udwg_dump_path
};

static int udwg_compare_edges(void *data1, void *data2) {
    udwg_edge_t *e1, *e2;
    udwg_node_t *n1, *n2;
    udw_graph_t *g;
    
    e1 = (udwg_edge_t *)data1;
    e2 = (udwg_edge_t *)data2;
    n1 = e1->e_adj;
    n2 = e2->e_adj;
    assert(n1->n_graph == n2->n_graph);
    g = n1->n_graph;
    return g->g_ops->go_compare_fn(n1->n_data, n2->n_data);
}

static void udwg_destroy_edge(void *data) {
    udwg_edge_t *e = (udwg_edge_t *)data;
    free(e);
}

static linked_list_ops_t spbf_ll_ops = {
    .llo_compare_fn = udwg_compare_edges,
    .llo_destroy_data_fn = NULL,
    .llo_dump_data_fn = udwg_dump_edge
};

//
// the general strategy here is to explore shortest paths first. this could be thought of as
// 'best first' instead of 'breadth first'. we maintain a priority queue of paths with a heap.
//
// note: the path we return does not include 'from'. it's a linked list of edges (node + weight)
// that leads to 'to' from 'from', but does not include 'from'.
//
int udwg_shortest_path_bf(udw_graph_t *g, udwg_node_t *from, udwg_node_t *to, linked_list_t **path) {
    heap_t *pq = NULL; // priority queue
    linked_list_t *_path = NULL;
    udwg_edge_t *edge;
    udwg_node_t *node;
    udwg_spbf_cb_ctx_t gsp_ctx;
    bool found = false;
    int err;
    
    pq = heap_create(&spbf_pq_ops);
    if (!pq) {
        err = ENOMEM;
        goto error_out;
    }
    
    _path = ll_create(&spbf_ll_ops);
    if (!_path) {
        err = ENOMEM;
        goto error_out;
    }
    
    memset(&gsp_ctx, 0, sizeof(udwg_spbf_cb_ctx_t));
    gsp_ctx.pq = pq;
    gsp_ctx.to = to;
    gsp_ctx.path = _path;
    
    err = at_iterate(from->n_adjs, udwg_spbf_cb, &gsp_ctx);
    if (err)
        goto error_out;
    
    ll_destroy(_path);
    
    while (!heap_empty(pq)) {
        _path = heap_pop(pq);
        assert(_path);
        
        if (gsp_ctx.best_path && udwg_path_len(_path) >= udwg_path_len(gsp_ctx.best_path)) {
            // current best path can't be beat, so we're done
            found = true;
            ll_destroy(_path);
            goto out;
        }
            
        err = ll_last(_path, (void **)&edge);
        if (err)
            goto error_out;
        
        node = edge->e_adj;
        
        //
        // XX: could further optimize this by keeping track of all nodes we've visited +
        // the shortest path we've seen to that node. if we've visited this node already
        // and the shortest path we have stored for it is less that udwg_path_len(_path)
        // then there's no point in further exploring this path
        //
        
        gsp_ctx.path = _path;
        err = at_iterate(node->n_adjs, udwg_spbf_cb, &gsp_ctx);
        if (err)
            goto error_out;
        
        ll_destroy(_path);
    }
    
    if (!found) {
        err = ENOENT;
        goto error_out;
    }
    
out:
    heap_destroy(pq);
    
    *path = gsp_ctx.best_path;
    
    return 0;
    
error_out:
    if (pq)
        heap_destroy(pq);
    if (_path)
        ll_destroy(_path);
    
    return err;
}
