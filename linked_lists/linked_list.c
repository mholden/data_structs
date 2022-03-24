#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#include "linked_list.h"

linked_list_t *ll_create(ll_ops_t *lops) {
    linked_list_t *ll;
    
    ll = malloc(sizeof(linked_list_t));
    if (!ll) {
        printf("ll_create: failed to allocate memory for ll\n");
        goto error_out;
    }
    
    memset(ll, 0, sizeof(linked_list_t));
    
    ll->ll_ops = malloc(sizeof(ll_ops_t));
    if (!ll->ll_ops) {
        printf("ll_create: failed to allocate memory for ll_ops\n");
        goto error_out;
    }
    memcpy(ll->ll_ops, lops, sizeof(ll_ops_t));
    
    return ll;
    
error_out:
    return NULL;
}

static void ll_destroy_node(linked_list_t *ll, ll_node_t *n) {
    if (ll->ll_ops->llo_destroy_data_fn)
        ll->ll_ops->llo_destroy_data_fn(n->ln_data);
    free(n);
}

void ll_destroy(linked_list_t *ll) {
    ll_node_t *curr, *prev;
    
    curr = ll->ll_root;
    while (curr) {
        prev = curr;
        curr = curr->ln_next;
        ll_destroy_node(ll, prev);
    }
    free(ll->ll_ops);
    free(ll);
}

int ll_insert(linked_list_t *ll, void *data) {
    ll_node_t *new, *curr, *prev;
    int err;
    
    new = malloc(sizeof(ll_node_t));
    if (!new) {
        err = ENOMEM;
        goto error_out;
    }
    memset(new, 0, sizeof(ll_node_t));
    new->ln_data = data;
    
    prev = NULL;
    curr = ll->ll_root;
    while (curr) {
        prev = curr;
        curr = curr->ln_next;
    }
    
    if (!prev) { // inserting at root
        assert(ll_empty(ll));
        ll->ll_root = new;
    } else {
        prev->ln_next = new;
    }
    
    ll->ll_nnodes++;
    
    return 0;
    
error_out:
    return err;
}

int ll_find(linked_list_t *ll, void *to_find, void **data) {
    ll_node_t *n;
    int err;
    
    n = ll->ll_root;
    while (n) {
        if (!ll->ll_ops->llo_compare_fn(to_find, n->ln_data))
            break;
        n = n->ln_next;
    }
    
    if (!n) {
        err = ENOENT;
        goto error_out;
    }
    
    if (data)
        *data = n->ln_data;
    
    return 0;
    
error_out:
    return err;
}

int ll_remove(linked_list_t *ll, void *to_remove) {
    ll_node_t *n, *prev;
    int err;
    
    prev = NULL;
    n = ll->ll_root;
    while (n) {
        if (!ll->ll_ops->llo_compare_fn(to_remove, n->ln_data))
            break;
        prev = n;
        n = n->ln_next;
    }
    
    if (!n) {
        err = ENOENT;
        goto error_out;
    }
    
    if (!prev) // root
        ll->ll_root = n->ln_next;
    else
        prev->ln_next = n->ln_next;
        
    ll_destroy_node(ll, n);
    ll->ll_nnodes--;
    
    return 0;
    
error_out:
    return err;
}

bool ll_empty(linked_list_t *ll) {
    return (ll->ll_nnodes == 0);
}

void ll_dump(linked_list_t *ll) {
    ll_node_t *n;
    
    printf("ll_nnodes: %lu ", ll->ll_nnodes);
    printf("nodes: ");
    n = ll->ll_root;
    while (n) {
        if (ll->ll_ops->llo_dump_data_fn)
            ll->ll_ops->llo_dump_data_fn(n->ln_data);
        n = n->ln_next;
    }
    printf("\n");
    
    return;
}

void ll_check(linked_list_t *ll) {
    ll_node_t *n;
    int nnodes;
    
    n = ll->ll_root;
    nnodes = 0;
    while (n) {
        nnodes++;
        n = n->ln_next;
    }
    assert(ll->ll_nnodes == nnodes);
    
    return;
}
