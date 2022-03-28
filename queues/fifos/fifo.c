#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <assert.h>

#include "fifo.h"

fifo_t *fi_create(fifo_ops_t *fops) {
    fifo_t *fi;
    
    fi = malloc(sizeof(fifo_t));
    if (!fi)
        goto error_out;
    
    memset(fi, 0, sizeof(fifo_t));
    
    fi->fi_ops = malloc(sizeof(fifo_ops_t));
    if (!fi->fi_ops)
        goto error_out;
    
    if (fops)
        memcpy(fi->fi_ops, fops, sizeof(fifo_ops_t));
    else
        memset(fi->fi_ops, 0, sizeof(fifo_ops_t));
    
    fi->fi_queue = malloc(sizeof(void *) * FIFO_START_SIZE);
    if (!fi->fi_queue)
        goto error_out;
    
    memset(fi->fi_queue, 0, sizeof(void *) * FIFO_START_SIZE);
    fi->fi_size = FIFO_START_SIZE;
    
    return fi;
    
error_out:
    if (fi) {
        if (fi->fi_ops)
            free(fi->fi_ops);
        free(fi);
    }
    
    return NULL;
}

void fi_destroy(fifo_t *fi) {
    int curr;
    
    curr = fi->fi_head;
    if (fi->fi_ops->fo_destroy_data_fn) {
        for (int i = 0; i < fi->fi_nelements; i++) {
            fi->fi_ops->fo_destroy_data_fn(fi->fi_queue[curr]);
            curr = (curr + 1) % fi->fi_size;
        }
    }
    
    free(fi->fi_queue);
    free(fi->fi_ops);
    free(fi);
}

bool fi_full(fifo_t *fi) {
    return (fi->fi_nelements == fi->fi_size);
}

static int fi_grow(fifo_t *fi) {
    void **new_queue;
    size_t new_size;
    int curr, err;
    
    assert(fi->fi_nelements == fi->fi_size);
    
    new_size = fi->fi_size * 2;
    new_queue = malloc(new_size * sizeof(void *));
    if (!new_queue) {
        err = ENOMEM;
        goto error_out;
    }
    
    memset(new_queue, 0, new_size * sizeof(void *));
    
    curr = fi->fi_head;
    for (int i = 0; i < fi->fi_nelements; i++) {
        assert(fi->fi_queue[curr]);
        new_queue[i] = fi->fi_queue[curr];
        curr = (curr + 1) % fi->fi_size;
    }
    
    fi->fi_head = 0;
    fi->fi_tail = fi->fi_size - 1;
    
    free(fi->fi_queue);
    fi->fi_queue = new_queue;
    fi->fi_size = new_size;
    
    return 0;
    
error_out:
    return err;
}

int fi_enq(fifo_t *fi, void *data) {
    int err;
    
    if (fi_full(fi)) {
        err = fi_grow(fi);
        if (err)
            goto error_out;
    }
    
    if (!fi_empty(fi)) {
        assert(fi->fi_queue[fi->fi_tail]);
        fi->fi_tail = (fi->fi_tail + 1) % fi->fi_size;
    } else {
        assert(fi->fi_head == fi->fi_tail);
    }
    
    assert(fi->fi_queue[fi->fi_tail] == NULL);
    fi->fi_queue[fi->fi_tail] = data;
    
    fi->fi_nelements++;
    
    return 0;
    
error_out:
    return err;
}

int fi_deq(fifo_t *fi, void **data) {
    int err;
    
    if (fi_empty(fi)) {
        err = ENOENT;
        goto error_out;
    }
    
    *data = fi->fi_queue[fi->fi_head];
    assert(*data);
    fi->fi_queue[fi->fi_head] = NULL;
    
    fi->fi_nelements--;
    
    if (!fi_empty(fi)) {
        fi->fi_head = (fi->fi_head + 1) % fi->fi_size;
        assert(fi->fi_queue[fi->fi_head]);
    } else {
        assert(fi->fi_head == fi->fi_tail);
    }
    
    return 0;
    
error_out:
    return err;
}

bool fi_empty(fifo_t *fi) {
    return (fi->fi_nelements == 0);
}

void fi_dump(fifo_t *fi) {
    int curr;
    
    printf("fi_size: %u\n", fi->fi_size);
    printf("fi_head: %d\n", fi->fi_head);
    printf("fi_tail: %d\n", fi->fi_tail);
    printf("fi_nelements: %u\n", fi->fi_nelements);
    
    printf("fi_queue:\n");
    for (int i = 0; i < fi->fi_size; i++) {
        if (fi->fi_queue[i]) {
            printf("[%d]: ", i);
            if (fi->fi_ops->fo_dump_data_fn)
                fi->fi_ops->fo_dump_data_fn(fi->fi_queue[i]);
            printf("\n");
        }
    }
    
    return;
}

void fi_check(fifo_t *fi) {
    size_t nelements = 0;
    
    assert((fi->fi_head >= 0) && (fi->fi_head < fi->fi_size));
    assert((fi->fi_tail >= 0) && (fi->fi_tail < fi->fi_size));
    assert(!fi_empty(fi) || (fi->fi_head == fi->fi_tail));
    
    for (int i = 0; i < fi->fi_size; i++) {
        if (fi->fi_head <= fi->fi_tail) {
            if (fi->fi_queue[i]) {
                assert((i >= fi->fi_head) && (i <= fi->fi_tail));
                nelements++;
            } else {
                assert(fi_empty(fi) || (i < fi->fi_head) || (i > fi->fi_tail));
            }
        } else { // wrapped around case: fi_tail < fi_head
            if (fi->fi_queue[i]) {
                assert((i >= fi->fi_head) || (i <= fi->fi_tail));
                nelements++;
            } else {
                assert(fi_empty(fi) || ((i < fi->fi_head) && (i > fi->fi_tail)));
            }
        }
    }
    
    assert(fi->fi_nelements == nelements);
}
