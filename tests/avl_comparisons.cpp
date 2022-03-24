#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <unordered_set>
#include <set>
#include <time.h>
#include <assert.h>

#include "avl_tree.h"
#include "binary_tree.h"
#include "hash_table.h"

static int test_compare(void *key1, void *key2) {
    int *k1, *k2;
    
    k1 = (int *)key1;
    k2 = (int *)key2;
    
    if (*k1 > *k2)
        return 1;
    else if (*k1 < *k2)
        return -1;
    else
        return 0;
}

static avl_tree_ops_t at_ops = {
    .ato_compare_fn = test_compare,
    .ato_destroy_data_fn = NULL,
    .ato_dump_data_fn = NULL
};

static double diff_timespec(struct timespec tend, struct timespec tstart) {
    return (tend.tv_sec - tstart.tv_sec) + ((tend.tv_nsec - tstart.tv_nsec) / 1e9);
}

int main(int argc, char **argv) {
    std::unordered_set<int> data, stlus;
    std::set<int> stls;
    binary_tree_t *bt;
    hash_table_t *ht;
    avl_tree_t *at;
    double bit = 0, bft = 0, ait = 0, aft = 0, hit = 0, hft = 0, sit = 0, sft = 0, usit = 0, usft = 0;
    struct timespec tstart, tend;
    int ch, num_elements = 0;
    
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
                printf("usage: %s --num <num-elements>\n", argv[0]);
                return -1;
        }
    }
    
    if (!num_elements) {
        printf("usage: %s --num <num-elements>\n", argv[0]);
        return -1;
    }
    
    srand(time(NULL));
    
    // build a test data set
    for (int i = 0; i < num_elements; i++) {
        auto [it, success] = data.insert(rand());
        while (!success) {
            auto [iit, nsuccess] = data.insert(rand());
            success = nsuccess;
        }
    }
    
    assert(bt = bt_create(sizeof(int), sizeof(int), test_compare));
    assert(at = at_create(&at_ops));
    assert(ht = ht_create(sizeof(int), sizeof(int), test_compare, num_elements * 2));
    
    // test inserts
    
    for (auto it = data.begin(); it != data.end(); it++) {
        // bst
        clock_gettime(CLOCK_MONOTONIC, &tstart);
        assert(bt_insert(bt, (void *)std::addressof(*it), (void *)std::addressof(*it)) == 0);
        clock_gettime(CLOCK_MONOTONIC, &tend);
        bit += diff_timespec(tend, tstart);
        
        // avl tree
        clock_gettime(CLOCK_MONOTONIC, &tstart);
        assert(at_insert(at, (void *)std::addressof(*it)) == 0);
        clock_gettime(CLOCK_MONOTONIC, &tend);
        ait += diff_timespec(tend, tstart);
        
        // hash table
        clock_gettime(CLOCK_MONOTONIC, &tstart);
        assert(ht_insert(ht, (void *)std::addressof(*it), (void *)std::addressof(*it)) == 0);
        clock_gettime(CLOCK_MONOTONIC, &tend);
        hit += diff_timespec(tend, tstart);
        
        // stl set
        clock_gettime(CLOCK_MONOTONIC, &tstart);
        {
            auto [_it, success] = stls.insert(*it);
            assert(success);
        }
        clock_gettime(CLOCK_MONOTONIC, &tend);
        sit += diff_timespec(tend, tstart);
        
        // stl unordered set
        clock_gettime(CLOCK_MONOTONIC, &tstart);
        {
            auto [_it, success] = stlus.insert(*it);
            assert(success);
        }
        clock_gettime(CLOCK_MONOTONIC, &tend);
        usit += diff_timespec(tend, tstart);
    }
    assert(at->at_nnodes == num_elements);
    
    printf("inserts: bt %.3fs (avg %.3fus) at %.3fs (avg %.3fus) ht %.3fs (avg %.3fus) "
           "set %.3fs (avg %.3fus) uset %.3fs (avg %.3fus)\n", bit, bit / num_elements * 1e6,
           ait, ait / num_elements * 1e6, hit, hit / num_elements * 1e6, sit,
           sit / num_elements * 1e6, usit, usit / num_elements * 1e6);
    
    // test lookups
    
    for (auto it = data.begin(); it != data.end(); it++) {
        // bst
        clock_gettime(CLOCK_MONOTONIC, &tstart);
        assert(bt_find(bt, (void *)std::addressof(*it), NULL) == 0);
        clock_gettime(CLOCK_MONOTONIC, &tend);
        bft += diff_timespec(tend, tstart);
        
        // avl tree
        clock_gettime(CLOCK_MONOTONIC, &tstart);
        assert(at_find(at, (void *)std::addressof(*it), NULL) == 0);
        clock_gettime(CLOCK_MONOTONIC, &tend);
        aft += diff_timespec(tend, tstart);
        
        // hash table
        clock_gettime(CLOCK_MONOTONIC, &tstart);
        assert(ht_find(ht, (void *)std::addressof(*it), NULL) == 0);
        clock_gettime(CLOCK_MONOTONIC, &tend);
        hft += diff_timespec(tend, tstart);
        
        // stl set
        clock_gettime(CLOCK_MONOTONIC, &tstart);
        {
            auto _it = stls.find(*it);
            assert(_it != stls.end());
        }
        clock_gettime(CLOCK_MONOTONIC, &tend);
        sft += diff_timespec(tend, tstart);
        
        // stl unordered set
        clock_gettime(CLOCK_MONOTONIC, &tstart);
        {
            auto _it = stlus.find(*it);
            assert(_it != stlus.end());
        }
        clock_gettime(CLOCK_MONOTONIC, &tend);
        usft += diff_timespec(tend, tstart);
    }
    
    printf("lookups: bt %.3fs (avg %.3fus) at %.3fs (avg %.3fus) ht %.3fs (avg %.3fus) "
           "set %.3fs (avg %.3fus) uset %.3fs (avg %.3fus)\n", bft, bft / num_elements * 1e6,
           aft, aft / num_elements * 1e6, hft, hft / num_elements * 1e6, sft,
           sft / num_elements * 1e6, usft, usft / num_elements * 1e6);
    
    bt_destroy(bt);
    at_destroy(at);
    ht_destroy(ht);
    
    return 0;
}
