#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <errno.h>

#include "hash_table.h"

typedef struct test_ht_data {
    int thd_key;
} test_ht_data_t;

static unsigned int test_ht_hash(void *data) {
    test_ht_data_t *thd = (test_ht_data_t *)data;
    return ht_default_hash(&thd->thd_key, sizeof(thd->thd_key));
}

static int test_ht_compare(void *data1, void *data2) {
    test_ht_data_t *thd1, *thd2;
    
    thd1 = (test_ht_data_t *)data1;
    thd2 = (test_ht_data_t *)data2;
    
    return !(thd1->thd_key == thd2->thd_key);
}

static void test_ht_dump_data(void *data) {
    test_ht_data_t *thd = (test_ht_data_t *)data;
    printf("%d ", thd->thd_key);
}

static hash_table_ops_t test_ht_ops = {
    .hto_hash_fn = test_ht_hash,
    .hto_lops = {
        .llo_compare_fn = test_ht_compare,
        .llo_destroy_data_fn = NULL,
        .llo_dump_data_fn = test_ht_dump_data
    }
};

// add and remove single element
void test_specific_case1(void) {
    hash_table_t *ht;
    test_ht_data_t thd, *_thd;
    
    printf("test_specific_case1\n");
    
    assert(ht = ht_create(&test_ht_ops));
    
    thd.thd_key = rand();
    assert(ht_insert(ht, (void *)&thd) == 0);
    ht_check(ht);
    assert(ht_find(ht, (void *)&thd, (void **)&_thd) == 0);
    assert(_thd->thd_key == thd.thd_key);
    assert(ht_remove(ht, (void *)&thd) == 0);
    ht_check(ht);
    assert(ht_find(ht, (void *)&thd, NULL) == ENOENT);
    
    ht_destroy(ht);
}

void test_specific_cases(void) {
    test_specific_case1();
}

static int test_iterate_cb(void *data, void *ctx, bool *stop) {
    test_ht_data_t *thd = (test_ht_data_t *)data;
    printf("  test_iterate_cb: %d\n", thd->thd_key);
    return 0;
}

void test_iterate(void) {
    hash_table_t *ht;
    test_ht_data_t *thd;
    int n = 8;
    
    printf("test_iterate\n");
    
    assert(ht = ht_create(&test_ht_ops));
    
    assert(thd = malloc(sizeof(test_ht_data_t) * n));
    memset(thd, 0, sizeof(test_ht_data_t) * n);
    
    for (int i = 0; i < n; i++) {
        thd[i].thd_key = i + 1;
        assert(ht_insert(ht, (void *)&thd[i]) == 0);
        //ht_check(ht);
    }
    ht_check(ht);
    
    assert(ht_iterate(ht, test_iterate_cb, NULL) == 0);
    
    ht_destroy(ht);
}

void test_random_case(int n) {
    hash_table_t *ht;
    test_ht_data_t *thd, *_thd;
    int j, k;
    
    printf("test_random_case (n %d)\n", n);
    
    assert(ht = ht_create(&test_ht_ops));
    
    assert(thd = malloc(sizeof(test_ht_data_t) * n));
    memset(thd, 0, sizeof(test_ht_data_t) * n);
    
    printf("  inserting %d...\n", n);
    
    for (int i = 0; i < n; i++) {
        thd[i].thd_key = i + 1;
        assert(ht_insert(ht, (void *)&thd[i]) == 0);
        //ht_check(ht);
    }
    ht_check(ht);
    
    // delete a random number of elements
    j = rand() % n;
    
    printf("  deleting %d...\n", j);

    for (int i = 0; i < j; i++) {
        k = rand() % n;
        if (!thd[k].thd_key) { // must have already deleted this one
            assert(ht_find(ht, (void *)&thd[k], NULL) == ENOENT);
        } else {
            assert(ht_remove(ht, (void *)&thd[k]) == 0);
            thd[k].thd_key = 0;
            //ht_check();
        }
    }
    ht_check(ht);
    
    printf("  verifying...\n");
    
    // make sure we can still find all non-zero (non-deleted) keys
    for (int i = 0; i < n; i++) {
        if (!thd[i].thd_key) { // we deleted this one
            assert(ht_find(ht, (void *)&thd[i], NULL) == ENOENT);
        } else { // didn't delete this one
            assert(ht_find(ht, (void *)&thd[i], (void **)&_thd) == 0);
            assert(_thd->thd_key == thd[i].thd_key);
        }
    }
    
    ht_destroy(ht);
}

#define DEFAULT_NUM_ELEMENTS (1 << 10)

int main(int argc, char **argv) {
    unsigned int seed = 0;
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
                seed = (unsigned int)strtol(optarg, NULL, 10);
                break;
            default:
                printf("usage: %s [--num <num-elements>] [--seed <seed>]\n", argv[0]);
                return -1;
        }
    }
    
    if (!seed) {
        seed = (unsigned int)time(NULL);
        srand(seed);
    } else
        srand(seed);
    
    printf("seed %u\n", seed);
    
    test_specific_cases();
    test_iterate();
    test_random_case(num_elements);
    
    return 0;
}
