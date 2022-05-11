#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <getopt.h>

#include "udw_graph.h"

typedef struct test_udwg_data {
    int tud_key;
} test_udwg_data_t;

static int test_udwg_compare(void *data1, void *data2) {
    test_udwg_data_t *tud1, *tud2;
    
    tud1 = (test_udwg_data_t *)data1;
    tud2 = (test_udwg_data_t *)data2;
    
    if (tud1->tud_key > tud2->tud_key)
        return 1;
    else if (tud1->tud_key < tud2->tud_key)
        return -1;
    else // ==
        return 0;
}

static void test_udwg_dump_data(void *data) {
    test_udwg_data_t *tud = (test_udwg_data_t *)data;
    printf("%d ", tud->tud_key);
}

static udw_graph_ops_t test_udwg_ops = {
    .go_compare_fn = test_udwg_compare,
    .go_dump_data_fn = test_udwg_dump_data,
    .go_destroy_data_fn = NULL
};

static int test_specific_case1_spath_cb(void *data, void *ctx, bool *stop) {
    udwg_edge_t *edge = (udwg_edge_t *)data;
    udwg_node_t *n = edge->e_adj;
    test_udwg_data_t *tud = (test_udwg_data_t *)n->n_data;
    int *path_iter = (int *)ctx;
    int err;
    
    switch (*path_iter) {
        case 0:
            assert(tud->tud_key == 0);
            break;
        case 1:
            assert(tud->tud_key == 7);
            break;
        case 2:
            assert(tud->tud_key == 9);
            break;
        default:
            assert(0);
            break;
    }
    
    (*path_iter)++;
    
    return 0;
    
error_out:
    return err;
}

//
// graph definition:
// 0 : 1 (1) 5 (1) 6 (1) 7 (1)
// 1 : 0 (1) 5 (5) 6 (7)
// 3 : 5 (1) 9 (1)
// 5 : 0 (1) 1 (5) 3 (1)
// 6 : 0 (1) 1 (7) 7 (1) 9 (3)
// 7 : 0 (1) 6 (1) 9 (1)
// 9 : 3 (1) 6 (3) 7 (1)
//
// shortest path between 1 and 9: 1, 0, 7, 9
//
static void test_specific_case1(void) {
    udw_graph_t *g;
    test_udwg_data_t tud[7];
    udwg_node_t *n[7];
    linked_list_t *dfspath, *bfspath;
    int path_iter;
    
    printf("test_specific_case1\n");
    
    assert(g = udwg_create(&test_udwg_ops));
    memset(tud, 0, sizeof(test_udwg_data_t) * 6);
    
    tud[0].tud_key = 1;
    assert(udwg_add_node(g, (void *)&tud[0], &n[0]) == 0);
    tud[1].tud_key = 0;
    assert(udwg_add_node(g, (void *)&tud[1], &n[1]) == 0);
    tud[2].tud_key = 6;
    assert(udwg_add_node(g, (void *)&tud[2], &n[2]) == 0);
    tud[3].tud_key = 5;
    assert(udwg_add_node(g, (void *)&tud[3], &n[3]) == 0);
    tud[4].tud_key = 7;
    assert(udwg_add_node(g, (void *)&tud[4], &n[4]) == 0);
    tud[5].tud_key = 9;
    assert(udwg_add_node(g, (void *)&tud[5], &n[5]) == 0);
    tud[6].tud_key = 3;
    assert(udwg_add_node(g, (void *)&tud[6], &n[6]) == 0);
    udwg_check(g);
    assert(udwg_add_edge(g, n[0], n[1], 1) == 0); // 1-0
    assert(udwg_add_edge(g, n[0], n[2], 7) == 0); // 1-6
    assert(udwg_add_edge(g, n[0], n[3], 5) == 0); // 1-5
    assert(udwg_add_edge(g, n[1], n[2], 1) == 0); // 0-6
    assert(udwg_add_edge(g, n[1], n[3], 1) == 0); // 0-5
    assert(udwg_add_edge(g, n[1], n[4], 1) == 0); // 0-7
    assert(udwg_add_edge(g, n[2], n[4], 1) == 0); // 6-7
    assert(udwg_add_edge(g, n[2], n[5], 3) == 0); // 6-9
    assert(udwg_add_edge(g, n[3], n[6], 1) == 0); // 5-3
    assert(udwg_add_edge(g, n[4], n[5], 1) == 0); // 7-9
    assert(udwg_add_edge(g, n[5], n[6], 1) == 0); // 9-3
    udwg_check(g);
    //udwg_dump(g);
    
#if 0
    assert(udg_shortest_path_df(g, n[0], n[4], &dfspath) == 0);
    assert(dfspath);
    
    path_iter = 0;
    assert(ll_iterate(dfspath, test_specific_case1_spath_cb, (void *)&path_iter) == 0);
    assert(path_iter == 3);
    
    ll_destroy(dfspath);
#endif
    
    assert(udwg_shortest_path_bf(g, n[0], n[5], &bfspath) == 0);
    assert(bfspath);
    //ll_dump(bfspath);
    
    path_iter = 0;
    assert(ll_iterate(bfspath, test_specific_case1_spath_cb, (void *)&path_iter) == 0);
    assert(path_iter == 3);
    
    ll_destroy(bfspath);

    udwg_destroy(g);
}

static void test_specific_cases(void) {
    test_specific_case1();
}

#define DEFAULT_NUM_ELEMENTS (1 << 10)

int main(int argc, char **argv) {
    time_t seed = 0;
    int ch, num_elements = DEFAULT_NUM_ELEMENTS;
    
    struct option longopts[] = {
        { "num",    required_argument,   NULL,   'n' },
        { "seed",   required_argument,   NULL,   's' },
        { NULL,                0,        NULL,    0 }
    };

    while ((ch = getopt_long(argc, argv, "", longopts, NULL)) != -1) {
        switch (ch) {
            case 'n':
                num_elements = (int)strtol(optarg, NULL, 10);
                break;
            case 's':
                seed = (time_t)strtol(optarg, NULL, 10);
                break;
            default:
                printf("usage: %s [--num <num-elements>] [--seed <seed>]\n", argv[0]);
                return -1;
        }
    }
    
    if (!seed)
        srand(time(&seed));
    else
        srand(seed);
    
    printf("seed %llu\n", seed);
    
    test_specific_cases();
    
    return 0;
}

