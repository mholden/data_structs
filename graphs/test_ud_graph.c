#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <getopt.h>

#include "ud_graph.h"

typedef struct test_udg_data {
    int tud_key;
} test_udg_data_t;

static int test_udg_compare(void *data1, void *data2) {
    test_udg_data_t *tud1, *tud2;
    
    tud1 = (test_udg_data_t *)data1;
    tud2 = (test_udg_data_t *)data2;
    
    if (tud1->tud_key > tud2->tud_key)
        return 1;
    else if (tud1->tud_key < tud2->tud_key)
        return -1;
    else // ==
        return 0;
}

static void test_udg_dump_data(void *data) {
    test_udg_data_t *tud = (test_udg_data_t *)data;
    printf("%d ", tud->tud_key);
}

static ud_graph_ops_t test_udg_ops = {
    .go_compare_fn = test_udg_compare,
    .go_dump_data_fn = test_udg_dump_data,
    .go_destroy_data_fn = NULL
};

//
// graph definition:
//   1: 3, 4
//   3: 1, 4, 5
//   4: 1, 3, 5, 6, 8
//   5: 3, 4, 6
//   6: 4, 5, 8
//   8: 4, 6
//
//  shortest path between 1 and 6: 1, 4, 6
//
static void test_specific_case1(void) {
    ud_graph_t *g;
    test_udg_data_t tud[6];
    udg_node_t *n[6];
    
    printf("test_specific_case1\n");
    
    assert(g = udg_create(&test_udg_ops));
    memset(tud, 0, sizeof(test_udg_data_t) * 6);
    
    tud[0].tud_key = 1;
    assert(udg_add_node(g, (void *)&tud[0], &n[0]) == 0);
    tud[1].tud_key = 3;
    assert(udg_add_node(g, (void *)&tud[1], &n[1]) == 0);
    tud[2].tud_key = 4;
    assert(udg_add_node(g, (void *)&tud[2], &n[2]) == 0);
    tud[3].tud_key = 5;
    assert(udg_add_node(g, (void *)&tud[3], &n[3]) == 0);
    tud[4].tud_key = 6;
    assert(udg_add_node(g, (void *)&tud[4], &n[4]) == 0);
    tud[5].tud_key = 8;
    assert(udg_add_node(g, (void *)&tud[5], &n[5]) == 0);
    udg_check(g);
    assert(udg_add_edge(g, n[0], n[1]) == 0); // 1-3
    assert(udg_add_edge(g, n[0], n[2]) == 0); // 1-4
    assert(udg_add_edge(g, n[1], n[2]) == 0); // 3-4
    assert(udg_add_edge(g, n[1], n[3]) == 0); // 3-5
    assert(udg_add_edge(g, n[2], n[3]) == 0); // 4-5
    assert(udg_add_edge(g, n[2], n[4]) == 0); // 4-6
    assert(udg_add_edge(g, n[2], n[5]) == 0); // 4-8
    assert(udg_add_edge(g, n[3], n[4]) == 0); // 5-6
    assert(udg_add_edge(g, n[4], n[5]) == 0); // 6-8
    udg_check(g);
    
    //udg_dump(g);
    
    udg_destroy(g);
}

static void test_specific_cases(void) {
    test_specific_case1();
}

typedef struct test_udg_add_edge_fallback_cb_ctx {
    udg_node_t **nodes;
    udg_node_t *node;
    test_udg_data_t next_expected;
} test_udg_add_edge_fallback_cb_ctx_t;

static int test_udg_add_edge_fallback_cb(void *data, void *ctx) {
    udg_node_t *n = (udg_node_t *)data;
    test_udg_add_edge_fallback_cb_ctx_t *aef_ctx = (test_udg_add_edge_fallback_cb_ctx_t *)ctx;
    test_udg_data_t *expected, *actual;
    
    expected = &aef_ctx->next_expected;
    actual = (test_udg_data_t *)n->n_data;
    
    if (expected->tud_key < actual->tud_key) {
        assert(udg_add_edge(aef_ctx->node->n_graph, aef_ctx->node, aef_ctx->nodes[expected->tud_key]) == 0);
        return 1; // stop the iteration
    } else {
        expected->tud_key++;
        return 0;
    }
}

static void test_random_case(int n) {
    ud_graph_t *g;
    test_udg_data_t *tud;
    udg_node_t **nodes;
    test_udg_add_edge_fallback_cb_ctx_t aef_ctx;
    int nedges, err;
    
    printf("test_random_case\n");
    
    assert(g = udg_create(&test_udg_ops));
    assert(tud = malloc(sizeof(test_udg_data_t) * n));
    assert(nodes = malloc(sizeof(udg_node_t *) * n));
    
    for (int i = 0; i < n; i++) {
        tud[i].tud_key = i;
        assert(udg_add_node(g, (void *)&tud[i], &nodes[i]) == 0);
        nedges = rand() % (i + 1);
        for (int j = 0; j < nedges; j++) {
            err = udg_add_edge(g, nodes[i], nodes[rand() % i]);
            assert(err == 0 || err == EEXIST);
            if (err == EEXIST) { // if random edge doesn't work, force-add one manually
                memset(&aef_ctx, 0, sizeof(test_udg_add_edge_fallback_cb_ctx_t));
                aef_ctx.nodes = nodes;
                aef_ctx.node = nodes[i];
                aef_ctx.next_expected.tud_key = 0;
                at_iterate(nodes[i]->n_adjs, test_udg_add_edge_fallback_cb, (void *)&aef_ctx);
            }
        }
        //udg_check(g);
    }
    
    udg_check(g);
    //udg_dump(g);
    
    udg_destroy(g);
    free(tud);
    free(nodes);
}

int main(int argc, char **argv) {
    int ch, num_elements = 0;
    time_t seed;
    
    struct option longopts[] = {
        { "num",    required_argument,   NULL,   'n' },
        { NULL,                0,        NULL,    0 }
    };

    while ((ch = getopt_long(argc, argv, "", longopts, NULL)) != -1) {
        switch (ch) {
            case 'n':
                num_elements = (int)strtol(optarg, NULL, 10);
                break;
            default:
                printf("usage: %s [--num <num-elements>]\n", argv[0]);
                return -1;
        }
    }
    
    srand(time(&seed));
    
    printf("seed %llu\n", seed);
    
    test_specific_cases();
    if (num_elements)
        test_random_case(num_elements);
    
	return 0;
}
