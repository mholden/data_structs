#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <stdio.h>

#include "ud_graph.h"
#include "queue.h"
#include "linked_list.h"

static int udg_compare_nodes(void *data1, void *data2) {
    udg_node_t *n1, *n2;
    ud_graph_t *g;
    
    n1 = (udg_node_t *)data1;
    n2 = (udg_node_t *)data2;
    assert(n1->n_graph == n2->n_graph);
    g = n1->n_graph;
    
    return g->g_ops->go_compare_fn(n1->n_data, n2->n_data);
}

static void udg_destroy_node(void *data) {
    udg_node_t *n = (udg_node_t *)data;
    ud_graph_t *g = n->n_graph;
    
    if (g->g_ops->go_destroy_data_fn)
        g->g_ops->go_destroy_data_fn(n->n_data);
    at_destroy(n->n_adjs);
    free(n);
}

static void udg_dump_node(void *data) {
    udg_node_t *n = (udg_node_t *)data;
    ud_graph_t *g = n->n_graph;
    
    printf("n_data: ");
    if (g->g_ops->go_dump_data_fn)
        g->g_ops->go_dump_data_fn(n->n_data);
    printf("(num adjacencies %d) ", n->n_adjs->at_nnodes);
}

static avl_tree_ops_t udg_aops = {
    .ato_compare_fn = udg_compare_nodes,
    .ato_destroy_data_fn = udg_destroy_node,
    .ato_dump_data_fn = udg_dump_node,
};

ud_graph_t *udg_create(ud_graph_ops_t *gops) {
    ud_graph_t *g = NULL;
    
    g = malloc(sizeof(ud_graph_t));
    if (!g)
        goto error_out;
    
    memset(g, 0, sizeof(g));
    
    g->g_nodes = at_create(&udg_aops);
    if (!g->g_nodes)
        goto error_out;
    
    g->g_ops = malloc(sizeof(ud_graph_ops_t));
    if (!g->g_ops)
        goto error_out;
    
    memcpy(g->g_ops, gops, sizeof(ud_graph_ops_t));
    
    return g;
    
error_out:
    if (g) {
        if (g->g_nodes)
            at_destroy(g->g_nodes);
        free(g);
    }
    
    return NULL;
}

void udg_destroy(ud_graph_t *g) {
    at_destroy(g->g_nodes);
    free(g->g_ops);
    free(g);
    return;
}

static void udg_dump_node_adj(void *data) {
    udg_node_t *adj = (udg_node_t *)data;
    ud_graph_t *g = adj->n_graph;
    
    if (g->g_ops->go_dump_data_fn)
        g->g_ops->go_dump_data_fn(adj->n_data);
}

static avl_tree_ops_t udg_node_aops = {
    .ato_compare_fn = udg_compare_nodes,
    .ato_destroy_data_fn = NULL,
    .ato_dump_data_fn = udg_dump_node_adj,
};

int udg_add_node(ud_graph_t *g, void *data, udg_node_t **new) {
    udg_node_t *n = NULL;
    int err;
    
    assert(data);
    
    n = malloc(sizeof(udg_node_t));
    if (!n) {
        err = ENOMEM;
        goto error_out;
    }
    
    memset(n, 0, sizeof(*n));
    n->n_data = data;
    
    n->n_adjs = at_create(&udg_node_aops);
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

int udg_add_edge(ud_graph_t *g, udg_node_t *n1, udg_node_t *n2) {
    int err;
    
    err = at_insert(n1->n_adjs, (void *)n2);
    if (err)
        goto error_out;
    
    err = at_insert(n2->n_adjs, (void *)n1);
    if (err) {
        printf("udg_add_edge: couldn't at_insert n1 into n2 adjs\n");
        goto error_out2;
    }
    
    return 0;
    
error_out2:
    if (at_remove(n1->n_adjs, n2))
        printf("udg_add_edge: failed to remove n2 from n1 adjs\n");
    
error_out:
    return err;
}

int udg_get_node(ud_graph_t *g, void *data, udg_node_t **node) {
    udg_node_t search_node;
    
    memset(&search_node, 0, sizeof(udg_node_t));
    search_node.n_data = data;
    search_node.n_graph = g;
    
    return at_find(g->g_nodes, (void *)&search_node, (void **)node);
}

static int udg_check_node_adjacencies_cb(void *data, void *ctx, bool *stop) {
    udg_node_t *adj, *n;
    
    adj = (udg_node_t *)data;
    n = (udg_node_t *)ctx;
    
    assert(at_find(adj->n_adjs, (void *)n, NULL) == 0);
    
    return 0;
}

static int udg_check_node_cb(void *data, void *ctx, bool *stop) {
    udg_node_t *n = (udg_node_t *)data;
    
    // make sure all our adjacencies have us in their adjacency list
    return at_iterate(n->n_adjs, udg_check_node_adjacencies_cb, (void *)n);
}

void udg_check(ud_graph_t *g) {
    assert(at_iterate(g->g_nodes, udg_check_node_cb, NULL) == 0);
}

static int udg_dump_node_adjs_cb(void *data, void *ctx, bool *stop) {
    udg_node_t *adj = (udg_node_t *)data;
    ud_graph_t *g = adj->n_graph;
    
    if (g->g_ops->go_dump_data_fn)
        g->g_ops->go_dump_data_fn(adj->n_data);
    
    return 0;
}

static void udg_dump_node_data(void *node) {
    udg_node_t *n = (udg_node_t *)node;
    ud_graph_t *g = n->n_graph;
    if (g->g_ops->go_dump_data_fn)
        g->g_ops->go_dump_data_fn(n->n_data);
}

static int udg_dump_node_cb(void *data, void *ctx, bool *stop) {
    udg_node_t *n = (udg_node_t *)data;
    ud_graph_t *g = n->n_graph;
    
    //printf("node %p ", n);
    //printf("n_graph: %p ", g);
    //printf("n_data: ");
    udg_dump_node_data((void *)n);
    printf(": ");
    at_iterate(n->n_adjs, udg_dump_node_adjs_cb, NULL);
    printf("\n");
    
    return 0;
}

void udg_dump(ud_graph_t *g) {
    at_iterate(g->g_nodes, udg_dump_node_cb, NULL);
}

typedef struct _udg_iterate_df_adjs_cb_ctx {
    avl_tree_t *visited;
    int (*callback)(void *, void *, bool *);
    void *ctx;
    bool *stop;
} _udg_iterate_df_adjs_cb_ctx_t;

static int _udg_iterate_df(udg_node_t *n, avl_tree_t *visited, int (*callback)(void *, void *, bool *), void *ctx, bool *stop);

static int _udg_iterate_df_adjs_cb(void *data, void *ctx, bool *stop) {
    udg_node_t *adj = (udg_node_t *)data;
    _udg_iterate_df_adjs_cb_ctx_t *ia_ctx = (_udg_iterate_df_adjs_cb_ctx_t *)ctx;
    int err;
    
    err = _udg_iterate_df(adj, ia_ctx->visited, ia_ctx->callback, ia_ctx->ctx, ia_ctx->stop);
    if (err)
        goto error_out;
    
    if (*ia_ctx->stop)
        *stop = true;
    
error_out:
    return err;
}

static int _udg_iterate_df(udg_node_t *n, avl_tree_t *visited, int (*callback)(void *, void *, bool *), void *ctx, bool *stop) {
    bool _stop = false;
    _udg_iterate_df_adjs_cb_ctx_t ia_ctx;
    int err;
    
    if (at_find(visited, (void *)n, NULL) == 0) // already visited this node
        goto out;
    
    err = at_insert(visited, (void *)n);
    if (err) {
        printf("_udg_iterate_df: at_insert failed\n");
        goto error_out;
    }
    
    memset(&ia_ctx, 0, sizeof(_udg_iterate_df_adjs_cb_ctx_t));
    ia_ctx.visited = visited;
    ia_ctx.callback = callback;
    ia_ctx.ctx = ctx;
    ia_ctx.stop = &_stop;
    
    // iterate all adjancencies
    err = at_iterate(n->n_adjs, _udg_iterate_df_adjs_cb, (void *)&ia_ctx);
    if (err)
        goto error_out;
    if (_stop) {
        if (stop)
            *stop = true;
        goto out;
    }
    
    err = callback(n->n_data, ctx, &_stop);
    if (err)
        goto error_out;
    if (_stop) {
        if (stop)
            *stop = true;
    }
    
out:
    return 0;
    
error_out:
    if (stop) // stop the iteration on error
        *stop = true;
    
    return err;
}

// depth first iteration
int udg_iterate_df(ud_graph_t *g, udg_node_t *start, int (*callback)(void *, void *, bool *), void *ctx) {
    avl_tree_t *visited = NULL;
    avl_tree_ops_t aops;
    int err;
    
    memset(&aops, 0, sizeof(avl_tree_ops_t));
    aops.ato_compare_fn = udg_compare_nodes;
    
    visited = at_create(&aops);
    if (!visited) {
        err = ENOMEM;
        goto error_out;
    }
    
    err = _udg_iterate_df(start, visited, callback, ctx, NULL);
    if (err)
        goto error_out;
    
    at_destroy(visited);
    
    return 0;
    
error_out:
    if (visited)
        at_destroy(visited);
    
    return err;
}

typedef struct _udg_iterate_bf_add_adjs_cb_ctx {
    avl_tree_t *visited;
    queue_t *q;
} _udg_iterate_bf_add_adjs_cb_ctx_t;

int _udg_iterate_bf_add_adjs_cb(void *data, void *ctx, bool *stop) {
    udg_node_t *adj = (udg_node_t *)data;
    _udg_iterate_bf_add_adjs_cb_ctx_t *ibaa_ctx = (_udg_iterate_bf_add_adjs_cb_ctx_t *)ctx;
    int err;
    
    if (at_find(ibaa_ctx->visited, (void *)adj, NULL) == 0) // already visited this node
        goto out;
    
    err = q_add(ibaa_ctx->q, (void *)adj);
    if (err)
        goto error_out;
    
    err = at_insert(ibaa_ctx->visited, (void *)adj);
    if (err)
        goto error_out;
    
out:
    return 0;
    
error_out:
    return err;
}

int _udg_iterate_bf(udg_node_t *start, avl_tree_t *visited, queue_t *q, int (*callback)(void *, void *, bool *), void *ctx) {
    udg_node_t *next;
    _udg_iterate_bf_add_adjs_cb_ctx_t ibaa_ctx;
    bool stop = false;
    int err;
    
    memset(&ibaa_ctx, 0, sizeof(_udg_iterate_bf_add_adjs_cb_ctx_t));
    ibaa_ctx.visited = visited;
    ibaa_ctx.q = q;
    
    err = q_add(q, (void *)start);
    if (err)
        goto error_out;
    
    err = at_insert(visited, (void *)start);
    if (err)
        goto error_out;
    
    while (!q_empty(q)) {
        err = q_get(q, (void **)&next);
        if (err)
            goto error_out;
        
        err = callback(next->n_data, ctx, &stop);
        if (err)
            goto error_out;
        if (stop)
            break;
        
        err = at_iterate(next->n_adjs, _udg_iterate_bf_add_adjs_cb, (void *)&ibaa_ctx);
        if (err)
            goto error_out;
    }
    
    return 0;
    
error_out:
    return err;
}

// breadth first iteration
int udg_iterate_bf(ud_graph_t *g, udg_node_t *start, int (*callback)(void *, void *, bool *), void *ctx) {
    avl_tree_t *visited = NULL;
    avl_tree_ops_t aops;
    queue_t *q;
    int err;
    
    memset(&aops, 0, sizeof(avl_tree_ops_t));
    aops.ato_compare_fn = udg_compare_nodes;
    
    visited = at_create(&aops);
    if (!visited) {
        err = ENOMEM;
        goto error_out;
    }
    
    q = q_create(g->g_nodes->at_nnodes);
    if (!q) {
        err = ENOMEM;
        goto error_out;
    }
    
    err = _udg_iterate_bf(start, visited, q, callback, ctx);
    if (err)
        goto error_out;
    
    at_destroy(visited);
    q_destroy(q);
    
    return 0;
    
error_out:
    if (visited)
        at_destroy(visited);
    if (q)
        q_destroy(q);
        
    return err;
}

typedef struct udg_shortest_path_df_adjs_cb_ctx {
    linked_list_t *curr_path;
    udg_node_t *to;
    linked_list_t *spath;
    size_t length_to_beat;
} udg_shortest_path_df_adjs_cb_ctx_t;

static int udg_shortest_path_df_adjs_cb(void *data, void *ctx, bool *stop);
static int udg_shortest_path_df_add_adjs_spath_cb(void *data, void *ctx, bool *stop);

static linked_list_ops_t udg_spath_ll_ops = {
    .llo_compare_fn = udg_compare_nodes,
    .llo_destroy_data_fn = NULL,
    .llo_dump_data_fn = udg_dump_node
};

static int _udg_shortest_path_df(udg_node_t *from, udg_node_t *to, linked_list_t *curr_path, size_t length_to_beat, linked_list_t **spath) {
    udg_shortest_path_df_adjs_cb_ctx_t spac;
    linked_list_t *_curr_path, *adjs_spath, *_spath = NULL;
    int err;
    
    _curr_path = ll_create_copy(curr_path);
    if (!_curr_path) {
        err = ENOMEM;
        goto error_out;
    }
    
    err = ll_insert(_curr_path, from);
    if (err)
        goto error_out;
    
    memset(&spac, 0, sizeof(udg_shortest_path_df_adjs_cb_ctx_t));
    spac.curr_path = _curr_path;
    spac.to = to;
    spac.length_to_beat = length_to_beat;
    
    err = at_iterate(from->n_adjs, udg_shortest_path_df_adjs_cb, (void *)&spac);
    if (err)
        goto error_out;
    
    ll_destroy(_curr_path);
    
    adjs_spath = spac.spath;
    
    if (adjs_spath) {
        _spath = ll_create(&udg_spath_ll_ops);
        if (!_spath) {
            err = ENOMEM;
            goto error_out;
        }
        err = ll_insert(_spath, from);
        if (err)
            goto error_out;
        err = ll_iterate(adjs_spath, udg_shortest_path_df_add_adjs_spath_cb, (void *)_spath);
        if (err)
            goto error_out;
        ll_destroy(adjs_spath);
    }
    
    *spath = _spath;
    
    return 0;
    
error_out:
    return err;
}

static int udg_shortest_path_df_adjs_cb(void *data, void *ctx, bool *stop) {
    udg_node_t *adj = (udg_node_t *)data;
    udg_shortest_path_df_adjs_cb_ctx_t *spac = (udg_shortest_path_df_adjs_cb_ctx_t *)ctx;
    size_t length_to_beat = 0;
    linked_list_t *spath = NULL;
    ud_graph_t *g = adj->n_graph;
    bool spath_is_shortest = false;
    int err;
    
    if (ll_find(spac->curr_path, (void *)adj, NULL) == 0) // already been here
        goto out;
    
    if (udg_compare_nodes(adj, spac->to) == 0) { // this is the destination node
        spath = ll_create(&udg_spath_ll_ops);
        if (!spath) {
            err = ENOMEM;
            goto error_out;
        }
        err = ll_insert(spath, (void *)adj);
        if (err)
            goto error_out;
        //
        // no point in continuing to look through our adjacencies. this
        // will be the shortest path
        //
        *stop = true;
    } else {
        if (spac->length_to_beat) {
            //
            // somwhere in the search we've found a shortest path
            // already. from this level of the graph, to get a *shorter*
            // path, it needs to be shorter than length_to_beat. calling
            // down to _udg_shortest_path_df will start searching our
            // adjacencies for their shortest paths. if we have to beat
            // length_to_beat, then they have to beat length_to_beat - 1.
            // it's impossible to beat length_to_beat == 1, so no point
            // in trying
            //
            length_to_beat = spac->length_to_beat - 1;
            if (length_to_beat == 1)
                goto out;
        }
        err = _udg_shortest_path_df(adj, spac->to, spac->curr_path, length_to_beat, &spath);
        if (err)
            goto error_out;
    }
    
    if (spath) {
        assert(ll_find(spath, (void *)spac->to, NULL) == 0);
        //
        // we found a path to the destination node. now check
        // if it's shorter than the best we've found so far
        //
        if (!spac->spath || // haven't even found a viable path yet
            (ll_length(spath) < ll_length(spac->spath))) { // have one but this one is shorter
            spath_is_shortest = true;
        }
        
        if (spath_is_shortest) {
            if (spac->spath)
                ll_destroy(spac->spath);
            
            spac->spath = spath;
            spac->length_to_beat = ll_length(spath);
        } else {
            ll_destroy(spath);
        }
    }
    
out:
    return 0;
    
error_out:
    return err;
}

static int udg_shortest_path_df_add_adjs_spath_cb(void *data, void *ctx, bool *stop) {
    udg_node_t *n = (udg_node_t *)data;
    linked_list_t *path = (linked_list_t *)ctx;
    int err;
    
    err = ll_insert(path, (void *)n);
    if (err)
        goto error_out;
    
    return 0;
    
error_out:
    return err;
}

int udg_shortest_path_df(ud_graph_t *g, udg_node_t *from, udg_node_t *to, linked_list_t **spath) {
    linked_list_t *curr_path = NULL, *adjs_spath, *_spath = NULL;
    udg_shortest_path_df_adjs_cb_ctx_t spac;
    int err;
    
    if (at_find(g->g_nodes, (void *)to, NULL)) { // destination doesn't exist
        err = ENOENT;
        goto error_out;
    }
    
    if (at_empty(to->n_adjs)) // destination isn't connected to any other node
        goto out;
    
    curr_path = ll_create(&udg_spath_ll_ops);
    if (!curr_path) {
        err = ENOMEM;
        goto error_out;
    }
    
    err = ll_insert(curr_path, from);
    if (err)
        goto error_out;
    
    memset(&spac, 0, sizeof(udg_shortest_path_df_adjs_cb_ctx_t));
    spac.curr_path = curr_path;
    spac.to = to;
    
    err = at_iterate(from->n_adjs, udg_shortest_path_df_adjs_cb, (void *)&spac);
    if (err)
        goto error_out;
    
    ll_destroy(curr_path);
    
    adjs_spath = spac.spath;
    
    if (adjs_spath) {
        _spath = ll_create(&udg_spath_ll_ops);
        if (!_spath) {
            err = ENOMEM;
            goto error_out;
        }
        err = ll_insert(_spath, from);
        if (err)
            goto error_out;
        err = ll_iterate(adjs_spath, udg_shortest_path_df_add_adjs_spath_cb, (void *)_spath);
        if (err)
            goto error_out;
        ll_destroy(adjs_spath);
    }
    
    *spath = _spath;
    
out:
    return 0;
    
error_out:
    if (curr_path)
        ll_destroy(curr_path);
    if (_spath)
        ll_destroy(_spath);
    
    return err;
}

int udg_shortest_path_bf(ud_graph_t *g, udg_node_t *from, udg_node_t *to, linked_list_t **spath) {
    assert(0);
    return 0;
}
