#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <errno.h>

#include "linked_list.h"

typedef struct test_ll_data {
    int tll_key;
} test_ll_data_t;

static int test_ll_compare(void *data1, void *data2) {
    test_ll_data_t *tll1, *tll2;
    
    tll1 = (test_ll_data_t *)data1;
    tll2 = (test_ll_data_t *)data2;
    
    return !(tll1->tll_key == tll2->tll_key);
}

static void test_ll_dump_data(void *data) {
    test_ll_data_t *tll = (test_ll_data_t *)data;
    printf("%d ", tll->tll_key);
}

static linked_list_ops_t test_ll_ops = {
    .llo_compare_fn = test_ll_compare,
    .llo_dump_data_fn = test_ll_dump_data,
    .llo_destroy_data_fn = NULL
};

// add three, remove middle
void test_specific_case3(void) {
    linked_list_t *ll;
    test_ll_data_t tll[3], *_tll;
    
    printf("test_specific_case3\n");
    
    assert(ll = ll_create(&test_ll_ops));
    
    for (int i = 0; i < 3; i++) {
        tll[i].tll_key = rand();
        assert(ll_insert(ll, (void *)&tll[i]) == 0);
    }
    ll_check(ll);
    
    for (int i = 0; i < 3; i++) {
        assert(ll_find(ll, (void *)&tll[i], (void **)&_tll) == 0);
        assert(_tll->tll_key == tll[i].tll_key);
    }
    
    assert(ll_remove(ll, (void *)&tll[1]) == 0);
    ll_check(ll);
    
    assert(ll_find(ll, (void *)&tll[1], NULL) == ENOENT);
    assert(ll_find(ll, (void *)&tll[0], (void **)&_tll) == 0);
    assert(_tll->tll_key == tll[0].tll_key);
    assert(ll_find(ll, (void *)&tll[2], (void **)&_tll) == 0);
    assert(_tll->tll_key == tll[2].tll_key);
    assert(!ll_empty(ll));
    
    ll_destroy(ll);
}

// add two, remove root
void test_specific_case2b(void) {
    linked_list_t *ll;
    test_ll_data_t tll[2], *_tll;
    
    printf("test_specific_case2b\n");
    
    assert(ll = ll_create(&test_ll_ops));
    
    tll[0].tll_key = rand();
    assert(ll_insert(ll, (void *)&tll[0]) == 0);
    tll[1].tll_key = rand();
    assert(ll_insert(ll, (void *)&tll[1]) == 0);
    ll_check(ll);
    assert(ll_find(ll, (void *)&tll[0], (void **)&_tll) == 0);
    assert(_tll->tll_key == tll[0].tll_key);
    assert(ll_find(ll, (void *)&tll[1], (void **)&_tll) == 0);
    assert(_tll->tll_key == tll[1].tll_key);
    assert(ll_remove(ll, (void *)&tll[0]) == 0);
    ll_check(ll);
    assert(ll_find(ll, (void *)&tll[0], NULL) == ENOENT);
    assert(ll_find(ll, (void *)&tll[1], (void **)&_tll) == 0);
    assert(_tll->tll_key == tll[1].tll_key);
    assert(!ll_empty(ll));
    
    ll_destroy(ll);
}

// add two, remove tail
void test_specific_case2(void) {
    linked_list_t *ll;
    test_ll_data_t tll[2], *_tll;
    
    printf("test_specific_case2\n");
    
    assert(ll = ll_create(&test_ll_ops));
    
    tll[0].tll_key = rand();
    assert(ll_insert(ll, (void *)&tll[0]) == 0);
    tll[1].tll_key = rand();
    assert(ll_insert(ll, (void *)&tll[1]) == 0);
    ll_check(ll);
    assert(ll_find(ll, (void *)&tll[0], (void **)&_tll) == 0);
    assert(_tll->tll_key == tll[0].tll_key);
    assert(ll_find(ll, (void *)&tll[1], (void **)&_tll) == 0);
    assert(_tll->tll_key == tll[1].tll_key);
    assert(ll_remove(ll, (void *)&tll[1]) == 0);
    ll_check(ll);
    assert(ll_find(ll, (void *)&tll[1], NULL) == ENOENT);
    assert(ll_find(ll, (void *)&tll[0], (void **)&_tll) == 0);
    assert(_tll->tll_key == tll[0].tll_key);
    assert(!ll_empty(ll));
    
    ll_destroy(ll);
}

// add and remove single element
void test_specific_case1(void) {
    linked_list_t *ll;
    test_ll_data_t tll, *_tll;
    
    printf("test_specific_case1\n");
    
    assert(ll = ll_create(&test_ll_ops));
    
    tll.tll_key = rand();
    assert(ll_insert(ll, (void *)&tll) == 0);
    ll_check(ll);
    assert(ll_find(ll, (void *)&tll, (void **)&_tll) == 0);
    assert(_tll->tll_key == tll.tll_key);
    assert(ll_remove(ll, (void *)&tll) == 0);
    ll_check(ll);
    assert(ll_find(ll, (void *)&tll, NULL) == ENOENT);
    assert(ll_empty(ll));
    
    ll_destroy(ll);
}

// same as a above but with ll_insert_head()

// add three, remove middle
void test_specific_case3h(void) {
    linked_list_t *ll;
    test_ll_data_t tll[3], *_tll;
    
    printf("test_specific_case3h\n");
    
    assert(ll = ll_create(&test_ll_ops));
    
    for (int i = 0; i < 3; i++) {
        tll[i].tll_key = rand();
        assert(ll_insert_head(ll, (void *)&tll[i]) == 0);
    }
    ll_check(ll);
    
    for (int i = 0; i < 3; i++) {
        assert(ll_find(ll, (void *)&tll[i], (void **)&_tll) == 0);
        assert(_tll->tll_key == tll[i].tll_key);
    }
    
    assert(ll_remove(ll, (void *)&tll[1]) == 0);
    ll_check(ll);
    
    assert(ll_find(ll, (void *)&tll[1], NULL) == ENOENT);
    assert(ll_find(ll, (void *)&tll[0], (void **)&_tll) == 0);
    assert(_tll->tll_key == tll[0].tll_key);
    assert(ll_find(ll, (void *)&tll[2], (void **)&_tll) == 0);
    assert(_tll->tll_key == tll[2].tll_key);
    assert(!ll_empty(ll));
    
    ll_destroy(ll);
}

// add two, remove root
void test_specific_case2hb(void) {
    linked_list_t *ll;
    test_ll_data_t tll[2], *_tll;
    
    printf("test_specific_case2hb\n");
    
    assert(ll = ll_create(&test_ll_ops));
    
    tll[0].tll_key = rand();
    assert(ll_insert_head(ll, (void *)&tll[0]) == 0);
    tll[1].tll_key = rand();
    assert(ll_insert_head(ll, (void *)&tll[1]) == 0);
    ll_check(ll);
    assert(ll_find(ll, (void *)&tll[0], (void **)&_tll) == 0);
    assert(_tll->tll_key == tll[0].tll_key);
    assert(ll_find(ll, (void *)&tll[1], (void **)&_tll) == 0);
    assert(_tll->tll_key == tll[1].tll_key);
    assert(ll_remove(ll, (void *)&tll[0]) == 0);
    ll_check(ll);
    assert(ll_find(ll, (void *)&tll[0], NULL) == ENOENT);
    assert(ll_find(ll, (void *)&tll[1], (void **)&_tll) == 0);
    assert(_tll->tll_key == tll[1].tll_key);
    assert(!ll_empty(ll));
    
    ll_destroy(ll);
}

// add two, remove tail
void test_specific_case2h(void) {
    linked_list_t *ll;
    test_ll_data_t tll[2], *_tll;
    
    printf("test_specific_case2h\n");
    
    assert(ll = ll_create(&test_ll_ops));
    
    tll[0].tll_key = rand();
    assert(ll_insert_head(ll, (void *)&tll[0]) == 0);
    tll[1].tll_key = rand();
    assert(ll_insert_head(ll, (void *)&tll[1]) == 0);
    ll_check(ll);
    assert(ll_find(ll, (void *)&tll[0], (void **)&_tll) == 0);
    assert(_tll->tll_key == tll[0].tll_key);
    assert(ll_find(ll, (void *)&tll[1], (void **)&_tll) == 0);
    assert(_tll->tll_key == tll[1].tll_key);
    assert(ll_remove(ll, (void *)&tll[1]) == 0);
    ll_check(ll);
    assert(ll_find(ll, (void *)&tll[1], NULL) == ENOENT);
    assert(ll_find(ll, (void *)&tll[0], (void **)&_tll) == 0);
    assert(_tll->tll_key == tll[0].tll_key);
    assert(!ll_empty(ll));
    
    ll_destroy(ll);
}

// add and remove single element
void test_specific_case1h(void) {
    linked_list_t *ll;
    test_ll_data_t tll, *_tll;
    
    printf("test_specific_case1h\n");
    
    assert(ll = ll_create(&test_ll_ops));
    
    tll.tll_key = rand();
    assert(ll_insert_head(ll, (void *)&tll) == 0);
    ll_check(ll);
    assert(ll_find(ll, (void *)&tll, (void **)&_tll) == 0);
    assert(_tll->tll_key == tll.tll_key);
    assert(ll_remove(ll, (void *)&tll) == 0);
    ll_check(ll);
    assert(ll_find(ll, (void *)&tll, NULL) == ENOENT);
    assert(ll_empty(ll));
    
    ll_destroy(ll);
}

void test_specific_cases(void) {
    test_specific_case1();
    test_specific_case2();
    test_specific_case2b();
    test_specific_case3();
    test_specific_case1h();
    test_specific_case2h();
    test_specific_case2hb();
    test_specific_case3h();
}

static int test_iterate_cb(void *data, void *ctx, bool *stop) {
    test_ll_data_t *tll = (test_ll_data_t *)data;
    printf("  test_iterate_cb: %d\n", tll->tll_key);
    return 0;
}

void test_iterate(void) {
    linked_list_t *ll;
    test_ll_data_t *tll;
    int n = 8;
    
    printf("test_iterate\n");
    
    assert(ll = ll_create(&test_ll_ops));
    assert(tll = malloc(sizeof(test_ll_data_t) * n));
    
    memset(tll, 0, sizeof(test_ll_data_t) * n);
    
    for (int i = 0; i < n; i++) {
        tll[i].tll_key = i + 1;
        assert(ll_insert(ll, (void *)&tll[i]) == 0);
        //ll_check();
    }
    ll_check(ll);
    
    assert(ll_iterate(ll, test_iterate_cb, NULL) == 0);
    
    ll_destroy(ll);
}

void test_random_case(int n) {
    linked_list_t *ll;
    test_ll_data_t *tll, *_tll;
    int j, k, hfreq;
    
    printf("test_random_case (n %d)\n", n);
    
    assert(ll = ll_create(&test_ll_ops));
    assert(tll = malloc(sizeof(test_ll_data_t) * n));
    
    memset(tll, 0, sizeof(test_ll_data_t) * n);
    
    printf("  inserting %d...\n", n);
    
    hfreq = (rand() % 10) + 1; // how often to insert elements at head
    for (int i = 0; i < n; i++) {
        tll[i].tll_key = i + 1;
        if (n % hfreq)
            assert(ll_insert(ll, (void *)&tll[i]) == 0);
        else
            assert(ll_insert_head(ll, (void *)&tll[i]) == 0);
        //ll_check();
    }
    ll_check(ll);
    
    // delete a random number of elements
    j = rand() % n;
    
    printf("  deleting %d...\n", j);

    for (int i = 0; i < j; i++) {
        //printf("deleting[%d]\n", i);
        k = rand() % n;
        if (!tll[k].tll_key) { // must have already deleted this one
            assert(ll_find(ll, (void *)&tll[k], NULL) == ENOENT);
        } else {
            assert(ll_remove(ll, (void *)&tll[k]) == 0);
            tll[k].tll_key = 0;
            //ll_check();
        }
    }
    ll_check(ll);
    
    printf("  verifying...\n");
    
    // make sure we can still find all non-zero (non-deleted) keys
    for (int i = 0; i < n; i++) {
        if (!tll[i].tll_key) { // we deleted this one
            assert(ll_find(ll, (void *)&tll[i], NULL) == ENOENT);
        } else { // didn't delete this one
            assert(ll_find(ll, (void *)&tll[i], (void **)&_tll) == 0);
            assert(_tll->tll_key == tll[i].tll_key);
        }
    }
    
    ll_destroy(ll);
}

#define DEFAULT_NUM_ELEMENTS (1 << 10)

int main(int argc, char **argv) {
    unsigned int seed = 0;
    int ch, num_elements = DEFAULT_NUM_ELEMENTS;
    
    struct option lonllopts[] = {
        { "num",    required_argument,   NULL,   'n' },
        { "seed",   required_argument,   NULL,   's' },
        { NULL,                0,        NULL,    0 }
    };

    while ((ch = getopt_long(argc, argv, "", lonllopts, NULL)) != -1) {
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
