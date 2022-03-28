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

static int test_specific_case1_spath_cb(void *data, void *ctx, bool *stop) {
    udg_node_t *n = (udg_node_t *)data;
    test_udg_data_t *tud = (test_udg_data_t *)n->n_data;
    int *path_iter = (int *)ctx;
    int err;
    
    switch (*path_iter) {
        case 0:
            assert(tud->tud_key == 1);
            break;
        case 1:
            assert(tud->tud_key == 4);
            break;
        case 2:
            assert(tud->tud_key == 6);
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
    linked_list_t *spath;
    int path_iter;
    
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
    
    assert(udg_shortest_path_df(g, n[0], n[4], &spath) == 0);
    assert(spath);
    
    path_iter = 0;
    assert(ll_iterate(spath, test_specific_case1_spath_cb, (void *)&path_iter) == 0);
    assert(path_iter == 3);
    
    //udg_dump(g);
    ll_destroy(spath);
    udg_destroy(g);
}

static void test_specific_cases(void) {
    test_specific_case1();
}

static void build_random_graph(ud_graph_t *g, test_udg_data_t *tud, int n) {
    udg_node_t **nodes;
    int nedges, err;
    
    assert(nodes = malloc(sizeof(udg_node_t *) * n));
    
    for (int i = 0; i < n; i++) {
        tud[i].tud_key = i;
        assert(udg_add_node(g, (void *)&tud[i], &nodes[i]) == 0);
    }
    
    // add edges randomly
    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
            if (rand() % 2)
                assert(udg_add_edge(g, nodes[i], nodes[j]) == 0);
        }
    }
    
    udg_check(g);
    free(nodes);
}

static void test_random_case(int n) {
    ud_graph_t *g;
    test_udg_data_t *tud;
    
    printf("test_random_case (n %d)\n", n);
    
    assert(g = udg_create(&test_udg_ops));
    assert(tud = malloc(sizeof(test_udg_data_t) * n));
    
    build_random_graph(g, tud, n);
    //udg_dump(g);
    
    udg_destroy(g);
    free(tud);
}

static int test_iterate_cb(void *data, void *ctx, bool *stop) {
    test_udg_data_t *tud = (test_udg_data_t *)data;
    //printf("    test_iterate_cb %d\n", tud->tud_key);
    return 0;
}

static void test_iterate(void) {
    ud_graph_t *g;
    test_udg_data_t *tud;
    udg_node_t *start_node;
    int n = 8;
    
    printf("test_iterate\n");
    
    assert(g = udg_create(&test_udg_ops));
    assert(tud = malloc(sizeof(test_udg_data_t) * n));
    
    build_random_graph(g, tud, n);
    
    assert(udg_get_node(g, (void *)&tud[rand() % n], &start_node) == 0);
    //printf("  doing udg_iterate_df with start %d\n", ((test_udg_data_t *)start_node->n_data)->tud_key);
    assert(udg_iterate_df(g, start_node, test_iterate_cb, NULL) == 0);
    //printf("  doing udg_iterate_bf with start %d\n", ((test_udg_data_t *)start_node->n_data)->tud_key);
    assert(udg_iterate_bf(g, start_node, test_iterate_cb, NULL) == 0);
    
    udg_destroy(g);
    free(tud);
}

static int test_shortest_path_specific_case4_cb(void *data, void *ctx, bool *stop) {
    udg_node_t *n = (udg_node_t *)data;
    test_udg_data_t *tud = (test_udg_data_t *)n->n_data;
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
            assert(tud->tud_key == 3);
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
//   0: 1, 4, 7
//   1: 0, 2
//   2: 1, 3
//   3: 2
//   4: 0, 5
//   5: 4, 6
//   6: 5
//   7: 0, 3, 8
//   8: 7, 9
//   9: 8, 10
//   10: 9
//
// same as case 3 but connect the 7 and 3 and make
// sure we still report 0-7-3 as the shortest path.
// shouldn't look at the 8 in this case after finding
// 3 on the 7 path, either
//
static void test_shortest_path_specific_case4(void) {
    ud_graph_t *g;
    test_udg_data_t tud[11];
    udg_node_t *n[11];
    linked_list_t *spath;
    int path_iter;
    
    printf("test_shortest_path_specific_case4\n");
    
    assert(g = udg_create(&test_udg_ops));
    memset(tud, 0, sizeof(test_udg_data_t) * 11);
    
    for (int i = 0; i < 11; i++) {
        tud[i].tud_key = i;
        assert(udg_add_node(g, (void *)&tud[i], &n[i]) == 0);
    }
    udg_check(g);
    
    assert(udg_add_edge(g, n[0], n[1]) == 0);
    assert(udg_add_edge(g, n[0], n[4]) == 0);
    assert(udg_add_edge(g, n[0], n[7]) == 0);
    assert(udg_add_edge(g, n[1], n[2]) == 0);
    assert(udg_add_edge(g, n[2], n[3]) == 0);
    assert(udg_add_edge(g, n[4], n[5]) == 0);
    assert(udg_add_edge(g, n[5], n[6]) == 0);
    assert(udg_add_edge(g, n[7], n[3]) == 0);
    assert(udg_add_edge(g, n[7], n[8]) == 0);
    assert(udg_add_edge(g, n[8], n[9]) == 0);
    assert(udg_add_edge(g, n[9], n[10]) == 0);
    udg_check(g);
    
    assert(udg_shortest_path_df(g, n[0], n[3], &spath) == 0);
    assert(spath);
    
    path_iter = 0;
    assert(ll_iterate(spath, test_shortest_path_specific_case4_cb, (void *)&path_iter) == 0);
    assert(path_iter == 3);
    
    //udg_dump(g);
    ll_destroy(spath);
    udg_destroy(g);
}

static int test_shortest_path_specific_case3_cb(void *data, void *ctx, bool *stop) {
    udg_node_t *n = (udg_node_t *)data;
    test_udg_data_t *tud = (test_udg_data_t *)n->n_data;
    int *path_iter = (int *)ctx;
    int err;
    
    switch (*path_iter) {
        case 0:
            assert(tud->tud_key == 0);
            break;
        case 1:
            assert(tud->tud_key == 1);
            break;
        case 2:
            assert(tud->tud_key == 2);
            break;
        case 3:
            assert(tud->tud_key == 3);
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
//   0: 1, 4, 7
//   1: 0, 2
//   2: 1, 3
//   3: 2
//   4: 0, 5
//   5: 4, 6
//   6: 5
//   7: 0, 8
//   8: 7, 9
//   9: 8, 10
//   10: 9
//
// get shortest path from 0 to 3. when we find the 3
// on the 1 path (0-1-2-3), this should cause us not
// to look at the 6, 9, or 10
//
static void test_shortest_path_specific_case3(void) {
    ud_graph_t *g;
    test_udg_data_t tud[11];
    udg_node_t *n[11];
    linked_list_t *spath;
    int path_iter;
    
    printf("test_shortest_path_specific_case3\n");
    
    assert(g = udg_create(&test_udg_ops));
    memset(tud, 0, sizeof(test_udg_data_t) * 11);
    
    for (int i = 0; i < 11; i++) {
        tud[i].tud_key = i;
        assert(udg_add_node(g, (void *)&tud[i], &n[i]) == 0);
    }
    udg_check(g);
    
    assert(udg_add_edge(g, n[0], n[1]) == 0);
    assert(udg_add_edge(g, n[0], n[4]) == 0);
    assert(udg_add_edge(g, n[0], n[7]) == 0);
    assert(udg_add_edge(g, n[1], n[2]) == 0);
    assert(udg_add_edge(g, n[2], n[3]) == 0);
    assert(udg_add_edge(g, n[4], n[5]) == 0);
    assert(udg_add_edge(g, n[5], n[6]) == 0);
    assert(udg_add_edge(g, n[7], n[8]) == 0);
    assert(udg_add_edge(g, n[8], n[9]) == 0);
    assert(udg_add_edge(g, n[9], n[10]) == 0);
    udg_check(g);
    
    assert(udg_shortest_path_df(g, n[0], n[3], &spath) == 0);
    assert(spath);
    
    path_iter = 0;
    assert(ll_iterate(spath, test_shortest_path_specific_case3_cb, (void *)&path_iter) == 0);
    assert(path_iter == 4);
    
    //udg_dump(g);
    ll_destroy(spath);
    udg_destroy(g);
}

static int test_shortest_path_specific_case2_cb(void *data, void *ctx, bool *stop) {
    udg_node_t *n = (udg_node_t *)data;
    test_udg_data_t *tud = (test_udg_data_t *)n->n_data;
    int *path_iter = (int *)ctx;
    int err;
    
    switch (*path_iter) {
        case 0:
            assert(tud->tud_key == 0);
            break;
        case 1:
            assert(tud->tud_key == 1);
            break;
        case 2:
            assert(tud->tud_key == 6);
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
//   0: 1, 2, 4
//   1: 0, 6
//   2: 0, 3
//   3: 2
//   4: 0, 5
//   5: 4
//   6: 1
//
// get shortest path from 0 to 6. when we find the 6
// on the 1 path (0-1-6), we should then see that we
// have the path when iterating 2 and 4, and should
// not even look at the 3 and 5
//
static void test_shortest_path_specific_case2(void) {
    ud_graph_t *g;
    test_udg_data_t tud[7];
    udg_node_t *n[7];
    linked_list_t *spath;
    int path_iter;
    
    printf("test_shortest_path_specific_case2\n");
    
    assert(g = udg_create(&test_udg_ops));
    memset(tud, 0, sizeof(test_udg_data_t) * 7);
    
    for (int i = 0; i < 7; i++) {
        tud[i].tud_key = i;
        assert(udg_add_node(g, (void *)&tud[i], &n[i]) == 0);
    }
    udg_check(g);
    
    assert(udg_add_edge(g, n[0], n[1]) == 0);
    assert(udg_add_edge(g, n[0], n[2]) == 0);
    assert(udg_add_edge(g, n[0], n[4]) == 0);
    assert(udg_add_edge(g, n[1], n[6]) == 0);
    assert(udg_add_edge(g, n[2], n[3]) == 0);
    assert(udg_add_edge(g, n[4], n[5]) == 0);
    udg_check(g);
    
    assert(udg_shortest_path_df(g, n[0], n[6], &spath) == 0);
    assert(spath);
    
    path_iter = 0;
    assert(ll_iterate(spath, test_shortest_path_specific_case2_cb, (void *)&path_iter) == 0);
    assert(path_iter == 3);
    
    //udg_dump(g);
    ll_destroy(spath);
    udg_destroy(g);
}

static int test_shortest_path_specific_case1_cb(void *data, void *ctx, bool *stop) {
    udg_node_t *n = (udg_node_t *)data;
    test_udg_data_t *tud = (test_udg_data_t *)n->n_data;
    int *path_iter = (int *)ctx;
    int err;
    
    switch (*path_iter) {
        case 0:
            assert(tud->tud_key == 0);
            break;
        case 1:
            assert(tud->tud_key == 4);
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
//   0: 1, 2, 3, 4, 5, 6, 7
//   1: 0
//   2: 0
//   3: 0
//   4: 0
//   5: 0
//   6: 0
//   7: 0
//
// get shortest path from 0 to 4. we should stop iterating
// adjacencies as soon as we find 4
//
static void test_shortest_path_specific_case1(void) {
    ud_graph_t *g;
    test_udg_data_t tud[8];
    udg_node_t *n[8];
    linked_list_t *spath;
    int path_iter;
    
    printf("test_shortest_path_specific_case1\n");
    
    assert(g = udg_create(&test_udg_ops));
    memset(tud, 0, sizeof(test_udg_data_t) * 8);
    
    for (int i = 0; i < 8; i++) {
        tud[i].tud_key = i;
        assert(udg_add_node(g, (void *)&tud[i], &n[i]) == 0);
    }
    udg_check(g);
    
    for (int i = 1; i < 8; i++) {
        assert(udg_add_edge(g, n[0], n[i]) == 0);
    }
    udg_check(g);
    
    assert(udg_shortest_path_df(g, n[0], n[4], &spath) == 0);
    assert(spath);
    
    path_iter = 0;
    assert(ll_iterate(spath, test_shortest_path_specific_case1_cb, (void *)&path_iter) == 0);
    assert(path_iter == 2);
    
    //udg_dump(g);
    ll_destroy(spath);
    udg_destroy(g);
}

static double diff_timespec(struct timespec tend, struct timespec tstart) {
    return (tend.tv_sec - tstart.tv_sec) + ((tend.tv_nsec - tstart.tv_nsec) / 1e9);
}

static void test_shortest_path_random(int n) {
    ud_graph_t *g;
    test_udg_data_t *tud, *_tud1, *_tud2;
    udg_node_t *from, *to;
    linked_list_t *dfspath, *bfspath;
    struct timespec start, end;
    double dftime, bftime;
    
    printf("test_shortest_path_random (n %d)\n", n);
    
    assert(g = udg_create(&test_udg_ops));
    assert(tud = malloc(sizeof(test_udg_data_t) * n));
    
    build_random_graph(g, tud, n);
    //udg_dump(g);
    
    _tud1 = &tud[rand() % n];
    _tud2 = &tud[rand() % n];
    assert(udg_get_node(g, (void *)_tud1, &from) == 0);
    assert(udg_get_node(g, (void *)_tud2, &to) == 0);
    
    //printf("  getting shortest path from %d to %d...\n", _tud1->tud_key, _tud2->tud_key);
    
    clock_gettime(CLOCK_MONOTONIC, &start);
    assert(udg_shortest_path_bf(g, from, to, &bfspath) == 0);
    clock_gettime(CLOCK_MONOTONIC, &end);
    bftime = diff_timespec(end, start);
    
    /*if (bfspath) {
        printf("    bfspath (time %fs): ", bftime);
        ll_dump(bfspath);
    } else {
        printf("    no path\n");
    }*/
    
    clock_gettime(CLOCK_MONOTONIC, &start);
    assert(udg_shortest_path_df(g, from, to, &dfspath) == 0);
    clock_gettime(CLOCK_MONOTONIC, &end);
    dftime = diff_timespec(end, start);
    
    /*if (dfspath) {
        printf("    dfspath (time %fs): ", dftime);
        ll_dump(dfspath);
    }*/
    
    assert((dfspath && bfspath) || (!dfspath && !bfspath));
    
    if (bfspath)
        ll_destroy(bfspath);
    
    if (dfspath)
        ll_destroy(dfspath);
    
    udg_destroy(g);
    free(tud);
}

static void test_shortest_path(int n) {
    test_shortest_path_random(n);
    test_shortest_path_specific_case1();
    test_shortest_path_specific_case2();
    test_shortest_path_specific_case3();
    test_shortest_path_specific_case4();
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
    test_random_case(num_elements);
    test_iterate();
    test_shortest_path(num_elements);
    
	return 0;
}
