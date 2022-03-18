#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <time.h>
#include <getopt.h>
#include <errno.h>

#include "avl-tree.h"

#define TADF_NO_FREE_ON_DESTROY 0x00000001

typedef struct test_at_data {
    int tad_key;
    int tad_flags;
} test_at_data_t;

static int test_at_compare(void *data1, void *data2){
    test_at_data_t *tad1, *tad2;
    
    tad1 = (test_at_data_t *)data1;
    tad2 = (test_at_data_t *)data2;
    
    if (tad1->tad_key > tad2->tad_key)
        return 1;
    else if (tad2->tad_key > tad1->tad_key)
        return -1;
    else
        return 0;
}

static void test_at_destroy_data(void *data) {
    test_at_data_t *tad = (test_at_data_t *)data;
    if (!(tad->tad_flags & TADF_NO_FREE_ON_DESTROY))
        free(tad);
}

static void test_at_dump_data(void *data) {
    test_at_data_t *tad = (test_at_data_t *)data;
    printf("tad_key: %d", tad->tad_key);
}

static avl_tree_ops_t test_at_ops = {
    .ato_compare_fn = test_at_compare,
    .ato_destroy_data_fn = test_at_destroy_data,
    .ato_dump_data_fn = test_at_dump_data
};

//
// case 4:              4b:
//          n0                  n0
//         /  \                /  \
//        n1   n2             n1   n2
//            /  \                /  \
//           n3   n4             n3   n4
//          /                      \
//         n5                       n5
//
// this will trigger a right rotation at n2 followed by
// a left rotation at n0 (vice versa on the other side)
//
// case 4:
//          n0                  n3
//         /  \                /  \
//  ->    n1   n3       ->    n0   n2
//            /  \           /  \    \
//           n5   n2        n1   n5   n4
//                 \
//                  n4
//
// 4b:
//        n0                  n3
//       /  \                /  \
//  ->  n1   n3       ->    n0   n2
//             \           /    /  \
//              n2        n1   n5   n4
//             /  \
//            n5   n4
//
static void test_case_4(void) {
    avl_tree_t *at;
    test_at_data_t *tad[6];
    
    printf("test_case_4\n");
    
    assert(at = at_create(&test_at_ops));
    
    for (int i = 0; i < 6; i++) {
        assert(tad[i] = malloc(sizeof(test_at_data_t)));
        memset(tad[i], 0, sizeof(test_at_data_t));
        tad[i]->tad_key = i;
    }
    
    assert(at_insert(at, tad[1]) == 0);
    assert(at_insert(at, tad[0]) == 0);
    assert(at_insert(at, tad[4]) == 0);
    assert(at_insert(at, tad[3]) == 0);
    assert(at_insert(at, tad[5]) == 0);
    assert(at_insert(at, tad[2]) == 0);
    at_check(at);
    at_destroy(at);
}

static void test_case_4b(void) {
    avl_tree_t *at;
    test_at_data_t *tad[6];
    
    printf("test_case_4b\n");
    
    assert(at = at_create(&test_at_ops));
    
    for (int i = 0; i < 6; i++) {
        assert(tad[i] = malloc(sizeof(test_at_data_t)));
        memset(tad[i], 0, sizeof(test_at_data_t));
        tad[i]->tad_key = i;
    }
    
    assert(at_insert(at, tad[1]) == 0);
    assert(at_insert(at, tad[0]) == 0);
    assert(at_insert(at, tad[4]) == 0);
    assert(at_insert(at, tad[2]) == 0);
    assert(at_insert(at, tad[5]) == 0);
    assert(at_insert(at, tad[3]) == 0);
    at_check(at);
    at_destroy(at);
}

//
// case 3:              3b:
//          n0                  n0
//         /  \                /  \
//        n1   n2             n1   n2
//            /  \                /  \
//           n3   n4             n3   n4
//                 \                 /
//                 n5               n5
//
// this will trigger a left rotation at n0 without needing
// a right rotation first at n2 (vice versa on the other side)
//
// case 3:              3b:
//          n2                  n2
//         /  \                /   \
//   ->  n0    n4        ->   n0    n4
//       / \    \            / \    /
//      n1  n3   n5        n1  n3  n5
//
//
static void test_case_3(void) {
    avl_tree_t *at;
    test_at_data_t *tad[6];
    
    printf("test_case_3\n");
    
    assert(at = at_create(&test_at_ops));
    
    for (int i = 0; i < 6; i++) {
        assert(tad[i] = malloc(sizeof(test_at_data_t)));
        memset(tad[i], 0, sizeof(test_at_data_t));
        tad[i]->tad_key = i;
    }
    
    assert(at_insert(at, tad[1]) == 0);
    assert(at_insert(at, tad[0]) == 0);
    assert(at_insert(at, tad[3]) == 0);
    assert(at_insert(at, tad[2]) == 0);
    assert(at_insert(at, tad[4]) == 0);
    assert(at_insert(at, tad[5]) == 0);
    at_check(at);
    at_destroy(at);
}

static void test_case_3b(void) {
    avl_tree_t *at;
    test_at_data_t *tad[6];
    
    printf("test_case_3b\n");
    
    assert(at = at_create(&test_at_ops));
    
    for (int i = 0; i < 6; i++) {
        assert(tad[i] = malloc(sizeof(test_at_data_t)));
        memset(tad[i], 0, sizeof(test_at_data_t));
        tad[i]->tad_key = i;
    }
    
    assert(at_insert(at, tad[1]) == 0);
    assert(at_insert(at, tad[0]) == 0);
    assert(at_insert(at, tad[3]) == 0);
    assert(at_insert(at, tad[2]) == 0);
    assert(at_insert(at, tad[5]) == 0);
    assert(at_insert(at, tad[4]) == 0);
    at_check(at);
    at_destroy(at);
}

//
// case 2:
//          n0        n0          n2
//           \         \         /  \
//            n1  ->   n2   ->  n0   n1
//           /           \
//          n2           n1
//
// this will trigger a right rotation at n1 followed by
// a left rotation at n0 (vice versa on the other side)
//
static void test_case_2(void) {
    avl_tree_t *at;
    test_at_data_t *tad[3];
    
    printf("test_case_2\n");
    
    assert(at = at_create(&test_at_ops));
    
    for (int i = 0; i < 3; i++) {
        assert(tad[i] = malloc(sizeof(test_at_data_t)));
        memset(tad[i], 0, sizeof(test_at_data_t));
        tad[i]->tad_key = i;
    }
    
    assert(at_insert(at, tad[0]) == 0);
    assert(at_insert(at, tad[2]) == 0);
    assert(at_insert(at, tad[1]) == 0);
    at_check(at);
    at_destroy(at);
}

//
// case 1:
//          n0              n1
//           \             /  \
//            n1    ->    n0   n2
//             \
//              n2
//
// simple left rotation with no right rotation required
// first
//
static void test_case_1(void) {
    avl_tree_t *at;
    test_at_data_t *tad[3];
    
    printf("test_case_1\n");
    
    assert(at = at_create(&test_at_ops));
    
    for (int i = 0; i < 3; i++) {
        assert(tad[i] = malloc(sizeof(test_at_data_t)));
        memset(tad[i], 0, sizeof(test_at_data_t));
        tad[i]->tad_key = i;
    }
    
    assert(at_insert(at, tad[0]) == 0);
    assert(at_insert(at, tad[1]) == 0);
    assert(at_insert(at, tad[2]) == 0);
    at_check(at);
    at_destroy(at);
}

static void test_specific_insertion_cases(void) {
    test_case_1();
    test_case_2();
    test_case_3();
    test_case_3b();
    test_case_4();
    test_case_4b();
}

// 12 nodes cases

//
// this triggers a rebalance of root->rchild that results
// in a height change that must propagate up to root
//
static void test_rcase_12(void) {
    avl_tree_t *at;
    test_at_data_t *tad[12];
    
    printf("test_rcase_12\n");
    
    assert(at = at_create(&test_at_ops));
    
    for (int i = 0; i < 12; i++) {
        assert(tad[i] = malloc(sizeof(test_at_data_t)));
        memset(tad[i], 0, sizeof(test_at_data_t));
        tad[i]->tad_key = i;
        tad[i]->tad_flags = TADF_NO_FREE_ON_DESTROY;
    }
    
    assert(at_insert(at, tad[4]) == 0);
    assert(at_insert(at, tad[2]) == 0);
    assert(at_insert(at, tad[7]) == 0);
    assert(at_insert(at, tad[1]) == 0);
    assert(at_insert(at, tad[3]) == 0);
    assert(at_insert(at, tad[6]) == 0);
    assert(at_insert(at, tad[9]) == 0);
    assert(at_insert(at, tad[0]) == 0);
    assert(at_insert(at, tad[5]) == 0);
    assert(at_insert(at, tad[8]) == 0);
    assert(at_insert(at, tad[10]) == 0);
    assert(at_insert(at, tad[11]) == 0);
    at_check(at);
    assert(at_remove(at, tad[5]) == 0);
    at_check(at);
    assert(at_find(at, tad[5], NULL) == ENOENT);
    assert(at_find(at, tad[1], NULL) == 0);
    assert(at_find(at, tad[2], NULL) == 0);
    assert(at_find(at, tad[3], NULL) == 0);
    assert(at_find(at, tad[4], NULL) == 0);
    assert(at_find(at, tad[6], NULL) == 0);
    assert(at_find(at, tad[7], NULL) == 0);
    assert(at_find(at, tad[8], NULL) == 0);
    assert(at_find(at, tad[9], NULL) == 0);
    assert(at_find(at, tad[10], NULL) == 0);
    assert(at_find(at, tad[11], NULL) == 0);
    at_destroy(at);
    for (int i = 0; i < 12; i++)
        free(tad[i]);
}


// 7 nodes cases

// this triggers a rebalance at root
static void test_rcase_7(void) {
    avl_tree_t *at;
    test_at_data_t *tad[7];
    
    printf("test_rcase_7\n");
    
    assert(at = at_create(&test_at_ops));
    
    for (int i = 0; i < 7; i++) {
        assert(tad[i] = malloc(sizeof(test_at_data_t)));
        memset(tad[i], 0, sizeof(test_at_data_t));
        tad[i]->tad_key = i;
        tad[i]->tad_flags = TADF_NO_FREE_ON_DESTROY;
    }
    
    assert(at_insert(at, tad[2]) == 0);
    assert(at_insert(at, tad[1]) == 0);
    assert(at_insert(at, tad[4]) == 0);
    assert(at_insert(at, tad[0]) == 0);
    assert(at_insert(at, tad[3]) == 0);
    assert(at_insert(at, tad[5]) == 0);
    assert(at_insert(at, tad[6]) == 0);
    at_check(at);
    assert(at_remove(at, tad[0]) == 0);
    at_check(at);
    assert(at_find(at, tad[0], NULL) == ENOENT);
    assert(at_find(at, tad[1], NULL) == 0);
    assert(at_find(at, tad[2], NULL) == 0);
    assert(at_find(at, tad[3], NULL) == 0);
    assert(at_find(at, tad[4], NULL) == 0);
    assert(at_find(at, tad[5], NULL) == 0);
    assert(at_find(at, tad[6], NULL) == 0);
    at_destroy(at);
    for (int i = 0; i < 7; i++)
        free(tad[i]);
}

// this triggers a rebalance at root
static void test_rcase_7b(void) {
    avl_tree_t *at;
    test_at_data_t *tad[7];
    
    printf("test_rcase_7b\n");
    
    assert(at = at_create(&test_at_ops));
    
    for (int i = 0; i < 7; i++) {
        assert(tad[i] = malloc(sizeof(test_at_data_t)));
        memset(tad[i], 0, sizeof(test_at_data_t));
        tad[i]->tad_key = i;
        tad[i]->tad_flags = TADF_NO_FREE_ON_DESTROY;
    }
    
    assert(at_insert(at, tad[2]) == 0);
    assert(at_insert(at, tad[1]) == 0);
    assert(at_insert(at, tad[5]) == 0);
    assert(at_insert(at, tad[0]) == 0);
    assert(at_insert(at, tad[4]) == 0);
    assert(at_insert(at, tad[6]) == 0);
    assert(at_insert(at, tad[3]) == 0);
    at_check(at);
    assert(at_remove(at, tad[0]) == 0);
    at_check(at);
    assert(at_find(at, tad[0], NULL) == ENOENT);
    assert(at_find(at, tad[1], NULL) == 0);
    assert(at_find(at, tad[2], NULL) == 0);
    assert(at_find(at, tad[3], NULL) == 0);
    assert(at_find(at, tad[4], NULL) == 0);
    assert(at_find(at, tad[5], NULL) == 0);
    assert(at_find(at, tad[6], NULL) == 0);
    at_destroy(at);
    for (int i = 0; i < 7; i++)
        free(tad[i]);
}

// 5 nodes cases

// this triggers a rebalance at root
static void test_rcase_5(void) {
    avl_tree_t *at;
    test_at_data_t *tad[5];
    
    printf("test_rcase_5\n");
    
    assert(at = at_create(&test_at_ops));
    
    for (int i = 0; i < 5; i++) {
        assert(tad[i] = malloc(sizeof(test_at_data_t)));
        memset(tad[i], 0, sizeof(test_at_data_t));
        tad[i]->tad_key = i;
        tad[i]->tad_flags = TADF_NO_FREE_ON_DESTROY;
    }
    
    assert(at_insert(at, tad[1]) == 0);
    assert(at_insert(at, tad[0]) == 0);
    assert(at_insert(at, tad[3]) == 0);
    assert(at_insert(at, tad[2]) == 0);
    assert(at_insert(at, tad[4]) == 0);
    at_check(at);
    assert(at_remove(at, tad[0]) == 0);
    at_check(at);
    assert(at_find(at, tad[0], NULL) == ENOENT);
    assert(at_find(at, tad[1], NULL) == 0);
    assert(at_find(at, tad[2], NULL) == 0);
    assert(at_find(at, tad[3], NULL) == 0);
    assert(at_find(at, tad[4], NULL) == 0);
    at_destroy(at);
    for (int i = 0; i < 5; i++)
        free(tad[i]);
}

// remove root; this triggers a rebalance at root
static void test_rcase_5b(void) {
    avl_tree_t *at;
    test_at_data_t *tad[5];
    
    printf("test_rcase_5b\n");
    
    assert(at = at_create(&test_at_ops));
    
    for (int i = 0; i < 5; i++) {
        assert(tad[i] = malloc(sizeof(test_at_data_t)));
        memset(tad[i], 0, sizeof(test_at_data_t));
        tad[i]->tad_key = i;
        tad[i]->tad_flags = TADF_NO_FREE_ON_DESTROY;
    }
    
    assert(at_insert(at, tad[1]) == 0);
    assert(at_insert(at, tad[0]) == 0);
    assert(at_insert(at, tad[3]) == 0);
    assert(at_insert(at, tad[2]) == 0);
    assert(at_insert(at, tad[4]) == 0);
    at_check(at);
    assert(at_remove(at, tad[1]) == 0);
    at_check(at);
    assert(at_find(at, tad[1], NULL) == ENOENT);
    assert(at_find(at, tad[0], NULL) == 0);
    assert(at_find(at, tad[2], NULL) == 0);
    assert(at_find(at, tad[3], NULL) == 0);
    assert(at_find(at, tad[4], NULL) == 0);
    at_destroy(at);
    for (int i = 0; i < 5; i++)
        free(tad[i]);
}

static void test_rcase_5c(void) {
    avl_tree_t *at;
    test_at_data_t *tad[5];
    
    printf("test_rcase_5c\n");
    
    assert(at = at_create(&test_at_ops));
    
    for (int i = 0; i < 5; i++) {
        assert(tad[i] = malloc(sizeof(test_at_data_t)));
        memset(tad[i], 0, sizeof(test_at_data_t));
        tad[i]->tad_key = i;
        tad[i]->tad_flags = TADF_NO_FREE_ON_DESTROY;
    }
    
    assert(at_insert(at, tad[1]) == 0);
    assert(at_insert(at, tad[0]) == 0);
    assert(at_insert(at, tad[3]) == 0);
    assert(at_insert(at, tad[2]) == 0);
    assert(at_insert(at, tad[4]) == 0);
    at_check(at);
    assert(at_remove(at, tad[3]) == 0);
    at_check(at);
    assert(at_find(at, tad[3], NULL) == ENOENT);
    assert(at_find(at, tad[0], NULL) == 0);
    assert(at_find(at, tad[1], NULL) == 0);
    assert(at_find(at, tad[2], NULL) == 0);
    assert(at_find(at, tad[4], NULL) == 0);
    at_destroy(at);
    for (int i = 0; i < 5; i++)
        free(tad[i]);
}

// 4 nodes cases
static void test_rcase_4(void) {
    avl_tree_t *at;
    test_at_data_t *tad[4];
    
    printf("test_rcase_4\n");
    
    assert(at = at_create(&test_at_ops));
    
    for (int i = 0; i < 4; i++) {
        assert(tad[i] = malloc(sizeof(test_at_data_t)));
        memset(tad[i], 0, sizeof(test_at_data_t));
        tad[i]->tad_key = i;
        tad[i]->tad_flags = TADF_NO_FREE_ON_DESTROY;
    }
    
    assert(at_insert(at, tad[1]) == 0);
    assert(at_insert(at, tad[0]) == 0);
    assert(at_insert(at, tad[2]) == 0);
    assert(at_insert(at, tad[3]) == 0);
    at_check(at);
    assert(at_remove(at, tad[2]) == 0);
    at_check(at);
    assert(at_find(at, tad[2], NULL) == ENOENT);
    assert(at_find(at, tad[0], NULL) == 0);
    assert(at_find(at, tad[1], NULL) == 0);
    assert(at_find(at, tad[3], NULL) == 0);
    at_destroy(at);
    for (int i = 0; i < 4; i++)
        free(tad[i]);
}

// remove left node; triggers a rebalance
static void test_rcase_4b(void) {
    avl_tree_t *at;
    test_at_data_t *tad[4];
    
    printf("test_rcase_4b\n");
    
    assert(at = at_create(&test_at_ops));
    
    for (int i = 0; i < 4; i++) {
        assert(tad[i] = malloc(sizeof(test_at_data_t)));
        memset(tad[i], 0, sizeof(test_at_data_t));
        tad[i]->tad_key = i;
        tad[i]->tad_flags = TADF_NO_FREE_ON_DESTROY;
    }
    
    assert(at_insert(at, tad[1]) == 0);
    assert(at_insert(at, tad[0]) == 0);
    assert(at_insert(at, tad[2]) == 0);
    assert(at_insert(at, tad[3]) == 0);
    at_check(at);
    assert(at_remove(at, tad[0]) == 0);
    at_check(at);
    assert(at_find(at, tad[0], NULL) == ENOENT);
    assert(at_find(at, tad[1], NULL) == 0);
    assert(at_find(at, tad[2], NULL) == 0);
    assert(at_find(at, tad[3], NULL) == 0);
    at_destroy(at);
    for (int i = 0; i < 4; i++)
        free(tad[i]);
}

// remove root; triggers a rebalance
static void test_rcase_4c(void) {
    avl_tree_t *at;
    test_at_data_t *tad[4];
    
    printf("test_rcase_4c\n");
    
    assert(at = at_create(&test_at_ops));
    
    for (int i = 0; i < 4; i++) {
        assert(tad[i] = malloc(sizeof(test_at_data_t)));
        memset(tad[i], 0, sizeof(test_at_data_t));
        tad[i]->tad_key = i;
        tad[i]->tad_flags = TADF_NO_FREE_ON_DESTROY;
    }
    
    assert(at_insert(at, tad[1]) == 0);
    assert(at_insert(at, tad[0]) == 0);
    assert(at_insert(at, tad[2]) == 0);
    assert(at_insert(at, tad[3]) == 0);
    at_check(at);
    assert(at_remove(at, tad[1]) == 0);
    at_check(at);
    assert(at_find(at, tad[1], NULL) == ENOENT);
    assert(at_find(at, tad[0], NULL) == 0);
    assert(at_find(at, tad[2], NULL) == 0);
    assert(at_find(at, tad[3], NULL) == 0);
    at_destroy(at);
    for (int i = 0; i < 4; i++)
        free(tad[i]);
}

// 3 nodes case: remove root
static void test_rcase_3(void) {
    avl_tree_t *at;
    test_at_data_t *tad[3];
    
    printf("test_rcase_3\n");
    
    assert(at = at_create(&test_at_ops));
    
    for (int i = 0; i < 3; i++) {
        assert(tad[i] = malloc(sizeof(test_at_data_t)));
        memset(tad[i], 0, sizeof(test_at_data_t));
        tad[i]->tad_key = i;
        tad[i]->tad_flags = TADF_NO_FREE_ON_DESTROY;
    }
    
    assert(at_insert(at, tad[1]) == 0);
    assert(at_insert(at, tad[0]) == 0);
    assert(at_insert(at, tad[2]) == 0);
    at_check(at);
    assert(at_remove(at, tad[1]) == 0);
    at_check(at);
    assert(at_find(at, tad[1], NULL) == ENOENT);
    assert(at_find(at, tad[0], NULL) == 0);
    assert(at_find(at, tad[2], NULL) == 0);
    at_destroy(at);
    for (int i = 0; i < 3; i++)
        free(tad[i]);
}

// 2 nodes cases
static void test_rcase_2(void) {
    avl_tree_t *at;
    test_at_data_t *tad[2];
    
    printf("test_rcase_2\n");
    
    assert(at = at_create(&test_at_ops));
    
    for (int i = 0; i < 2; i++) {
        assert(tad[i] = malloc(sizeof(test_at_data_t)));
        memset(tad[i], 0, sizeof(test_at_data_t));
        tad[i]->tad_key = i;
        tad[i]->tad_flags = TADF_NO_FREE_ON_DESTROY;
    }
    
    assert(at_insert(at, tad[0]) == 0);
    assert(at_insert(at, tad[1]) == 0);
    at_check(at);
    assert(at_remove(at, tad[1]) == 0);
    at_check(at);
    assert(at_find(at, tad[1], NULL) == ENOENT);
    at_destroy(at);
    for (int i = 0; i < 2; i++)
        free(tad[i]);
}

// remove root
static void test_rcase_2b(void) {
    avl_tree_t *at;
    test_at_data_t *tad[2];
    
    printf("test_rcase_2b\n");
    
    assert(at = at_create(&test_at_ops));
    
    for (int i = 0; i < 2; i++) {
        assert(tad[i] = malloc(sizeof(test_at_data_t)));
        memset(tad[i], 0, sizeof(test_at_data_t));
        tad[i]->tad_key = i;
        tad[i]->tad_flags = TADF_NO_FREE_ON_DESTROY;
    }
    
    assert(at_insert(at, tad[0]) == 0);
    assert(at_insert(at, tad[1]) == 0);
    at_check(at);
    assert(at_remove(at, tad[0]) == 0);
    at_check(at);
    assert(at_find(at, tad[0], NULL) == ENOENT);
    at_destroy(at);
    for (int i = 0; i < 2; i++)
        free(tad[i]);
}

// single root node case
static void test_rcase_1(void) {
    avl_tree_t *at;
    test_at_data_t *tad[1];
    
    printf("test_rcase_1\n");
    
    assert(at = at_create(&test_at_ops));
    
    for (int i = 0; i < 1; i++) {
        assert(tad[i] = malloc(sizeof(test_at_data_t)));
        memset(tad[i], 0, sizeof(test_at_data_t));
        tad[i]->tad_key = i;
        tad[i]->tad_flags = TADF_NO_FREE_ON_DESTROY;
    }
    
    assert(at_insert(at, tad[0]) == 0);
    at_check(at);
    assert(at_find(at, tad[0], NULL) == 0);
    assert(at_remove(at, tad[0]) == 0);
    at_check(at);
    assert(at_find(at, tad[0], NULL) == ENOENT);
    at_destroy(at);
    for (int i = 0; i < 1; i++)
        free(tad[i]);
}

static void test_specific_removal_cases(void) {
    test_rcase_1();
    test_rcase_2();
    test_rcase_2b();
    test_rcase_3();
    test_rcase_4();
    test_rcase_4b();
    test_rcase_4c();
    test_rcase_5();
    test_rcase_5b();
    test_rcase_5c();
    test_rcase_7();
    test_rcase_7b();
    test_rcase_12();
}

static void test_random_data_set(int num_elements) {
    avl_tree_t *at;
    test_at_data_t *tad = NULL, *_tad, *__tad;
    int err, remaining;
    
    printf("test_random_data_set\n");
    
    assert(at = at_create(&test_at_ops));
    
    // build a data set of num_elements
    assert(tad = malloc(sizeof(test_at_data_t) * num_elements));
    
    _tad = tad;
    for (int i = 0; i < num_elements; i++) {
        _tad->tad_key = rand();
        _tad++;
    }
    
#if 0
    for (int i = 0; i < num_elements; i++) {
        printf("tad[%d]: %d\n", i, tad[i].tad_key);
    }
#endif
    
    //printf("** doing insertions **\n");
    
    // insert all tad into the tree
    _tad = tad;
    for (int i = 0; i < num_elements; i++) {
        assert(__tad = malloc(sizeof(test_at_data_t)));
        memcpy(__tad, _tad, sizeof(*__tad));
        err = at_insert(at, __tad);
        while (err == EEXIST) {
            __tad->tad_key = rand();
            err = at_insert(at, __tad);
        }
        assert(err == 0);
        //printf("at_check %d\n", i);
        //at_check(at);
        memcpy(_tad, __tad, sizeof(*_tad));
        _tad++;
    }
    
    at_check(at);
    
    //printf("** doing finds **\n");
    
    for (int i = 0; i < num_elements; i++) {
        assert(at_find(at, &tad[i], (void *)&_tad) == 0);
        assert(_tad->tad_key == tad[i].tad_key);
    }
    
    //printf("** doing removals **\n");
    
    remaining = num_elements;
    for (int i = 0; i < num_elements; i++) {
        assert(at_remove(at, &tad[i]) == 0);
        remaining--;
        //printf("at_check %d\n", i);
        //at_check(at);
        assert(at_find(at, &tad[i], NULL) == ENOENT);
        if (remaining)
            assert(at_find(at, &tad[i+(rand()%remaining)+1], NULL) == 0);
    }
    
    at_check(at);
    
    at_destroy(at);
    free(tad);
}

int main(int argc, char **argv) {
    time_t seed = 0;
    int ch, num_elements = 0;
    
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
    
    test_specific_insertion_cases();
    test_specific_removal_cases();
    
    if (num_elements)
        test_random_data_set(num_elements);
    
	return 0;
}
