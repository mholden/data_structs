#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <getopt.h>

#include "fifo.h"

typedef struct test_fifo_data {
    int tfd_key;
} test_fifo_data_t;

static void test_fifo_dump_data(void *data) {
    test_fifo_data_t *tfd = (test_fifo_data_t *)data;
    printf("%d ", tfd->tfd_key);
}

static fifo_ops_t test_fifo_ops = {
    .fo_destroy_data_fn = NULL,
    .fo_dump_data_fn = test_fifo_dump_data
};

//
// repeatedly enq FIFO_START_SIZE * (3/4), deq FIFO_START_SIZE / 2
// this will wrap around and will also trigger a fifo_grow() eventually
//
static void test_specific_case1d(void) {
    fifo_t *fi;
    test_fifo_data_t *tfd, *_tfd;
    int n = FIFO_START_SIZE * 4, nelements = 0, head = 0, tail = 0;
    
    printf("test_specific_case1d\n");
    
    assert(fi = fi_create(&test_fifo_ops));
    
    assert(tfd = malloc(sizeof(test_fifo_data_t) * n));
    memset(tfd, 0, sizeof(test_fifo_data_t) * n);
    
    for (int i = 0; i < n; i++)
        tfd[i].tfd_key = i;
    
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < FIFO_START_SIZE * 3 / 4; j++) {
            assert(fi_enq(fi, (void *)&tfd[tail]) == 0);
            tail = (tail + 1) % n;
            nelements++;
            assert(nelements <= n);
        }
        fi_check(fi);
        for (int j = 0; j < FIFO_START_SIZE / 2; j++) {
            assert(fi_deq(fi, (void *)&_tfd) == 0);
            assert(_tfd->tfd_key == tfd[head].tfd_key);
            head = (head + 1) % n;
            nelements--;
            assert(nelements > 0);
        }
        fi_check(fi);
    }
    
    assert(!fi_empty(fi));
    assert(fi->fi_size == (FIFO_START_SIZE * 2 * 2)); // should fifo_grow() twice
    
    fi_destroy(fi);
    free(tfd);
}

//
// enq FIFO_START_SIZE / 2, deq FIFO_START_SIZE / 2
// then enq FIFO_START_SIZE * (3/4), deq FIFO_START_SIZE * (3/4)
// this will wrap around, but shouldn't trigger a fifo_grow()
//
static void test_specific_case1c(void) {
    fifo_t *fi;
    test_fifo_data_t *tfd, *_tfd;
    int n = FIFO_START_SIZE, head = 0, tail = 0;
    
    printf("test_specific_case1c\n");
    
    assert(fi = fi_create(&test_fifo_ops));
    
    assert(tfd = malloc(sizeof(test_fifo_data_t) * n));
    memset(tfd, 0, sizeof(test_fifo_data_t) * n);
    
    for (int i = 0; i < n; i++)
        tfd[i].tfd_key = i;
    
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < n / 2; j++) {
            assert(fi_enq(fi, (void *)&tfd[tail]) == 0);
            tail = (tail + 1) % n;
        }
        fi_check(fi);
        for (int j = 0; j < n / 2; j++) {
            assert(fi_deq(fi, (void *)&_tfd) == 0);
            assert(_tfd->tfd_key == tfd[head].tfd_key);
            head = (head + 1) % n;
        }
        fi_check(fi);
        for (int j = 0; j < n * 3 / 4; j++) {
            assert(fi_enq(fi, (void *)&tfd[tail]) == 0);
            tail = (tail + 1) % n;
        }
        fi_check(fi);
        for (int j = 0; j < n * 3 / 4; j++) {
            assert(fi_deq(fi, (void *)&_tfd) == 0);
            assert(_tfd->tfd_key == tfd[head].tfd_key);
            head = (head + 1) % n;
        }
        fi_check(fi);
    }
    
    assert(fi_empty(fi));
    assert(fi->fi_size == FIFO_START_SIZE);
    
    fi_destroy(fi);
    free(tfd);
}

//
// enq 2, then enq 1 deq 1 a bunch
// this will wrap around, but shouldn't trigger a
// fifo_grow()
//
static void test_specific_case1b(void) {
    fifo_t *fi;
    test_fifo_data_t *tfd, *_tfd;
    int n = FIFO_START_SIZE * 2;
    
    printf("test_specific_case1b\n");
    
    assert(fi = fi_create(&test_fifo_ops));
    
    assert(tfd = malloc(sizeof(test_fifo_data_t) * n));
    memset(tfd, 0, sizeof(test_fifo_data_t) * n);
    
    tfd[0].tfd_key = 0;
    assert(fi_enq(fi, (void *)&tfd[0]) == 0);
    //fi_dump(fi);
    
    for (int i = 1; i < n; i++) {
        tfd[i].tfd_key = 1 - tfd[i-1].tfd_key;
        assert(fi_enq(fi, (void *)&tfd[i]) == 0);
        //fi_dump(fi);
        fi_check(fi);
        assert(fi_deq(fi, (void **)&_tfd) == 0);
        //fi_dump(fi);
        assert(_tfd->tfd_key == tfd[i-1].tfd_key);
        fi_check(fi);
        assert(!fi_empty(fi));
        assert(fi->fi_size == FIFO_START_SIZE);
    }
    
    fi_destroy(fi);
    free(tfd);
}

// enq deq single element
static void test_specific_case1(void) {
    fifo_t *fi;
    test_fifo_data_t tfd, *_tfd;
    
    printf("test_specific_case1\n");
    
    assert(fi = fi_create(&test_fifo_ops));
    
    for (int i = 0; i < FIFO_START_SIZE / 2; i++) {
        tfd.tfd_key = i;
        assert(fi_enq(fi, (void *)&tfd) == 0);
        fi_check(fi);
        assert(fi_deq(fi, (void **)&_tfd) == 0);
        assert(_tfd->tfd_key == tfd.tfd_key);
        fi_check(fi);
        assert(fi_empty(fi));
        assert(fi->fi_head == 0);
        assert(fi->fi_tail == 0);
        assert(fi->fi_size == FIFO_START_SIZE);
    }
    
    fi_destroy(fi);
}

static void test_specific_cases(void) {
    test_specific_case1();
    test_specific_case1b();
    test_specific_case1c();
    test_specific_case1d();
}

static void test_random_case(int n) {
    fifo_t *fi;
    test_fifo_data_t *tfd, *_tfd;
    int j, k, l, nelements = 0, head = 0, tail = 0;
    
    printf("test_random_case (n %d)\n", n);
    
    assert(fi = fi_create(&test_fifo_ops));
    
    assert(tfd = malloc(sizeof(test_fifo_data_t) * n));
    memset(tfd, 0, sizeof(test_fifo_data_t) * n);
    
    // init data set
    for (int i = 0; i < n; i++)
        tfd[i].tfd_key = i;
    
    //
    // do a random number of enq's followed by a random
    // number of deq's a random number of times
    //
    l = rand() % n;
    printf("  running enqueue-dequeue loop %d times...\n", l);
    
    for (int c = 0; c < l; c++) {
        j = (rand() % (n - nelements)) + 1;
        //printf("  enq'ing %d\n", j);
        for (int i = 0; i < j; i++) {
            assert(fi_enq(fi, (void *)&tfd[tail]) == 0);
            tail = (tail + 1) % n;
            nelements++;
            assert(nelements <= n);
        }
        fi_check(fi);
        //fi_dump(fi);
        
        k = (rand() % nelements) + 1;
        //printf("  deq'ing %d\n", k);
        for (int i = 0; i < k; i++) {
            assert(fi_deq(fi, (void **)&_tfd) == 0);
            assert(tfd[head].tfd_key == _tfd->tfd_key);
            head = (head + 1) % n;
            nelements--;
            assert(nelements >= 0);
        }
        //if (fi_empty(fi))
        //    printf("  de-queued to empty\n");
        //fi_dump(fi);
        fi_check(fi);
    }
    
    fi_destroy(fi);
    free(tfd);
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
    
    return 0;
}
