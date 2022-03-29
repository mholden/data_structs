#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#include "linked_list.h"

linked_list_t *ll_create(linked_list_ops_t *lops) {
    linked_list_t *ll;
    
    ll = (linked_list_t *)malloc(sizeof(linked_list_t));
    if (!ll) {
        printf("ll_create: failed to allocate memory for ll\n");
        goto error_out;
    }
    
    memset(ll, 0, sizeof(linked_list_t));
    
    ll->ll_ops = (linked_list_ops_t *)malloc(sizeof(linked_list_ops_t));
    if (!ll->ll_ops) {
        printf("ll_create: failed to allocate memory for ll_ops\n");
        goto error_out;
    }
    if (lops)
        memcpy(ll->ll_ops, lops, sizeof(linked_list_ops_t));
    else
        memset(ll->ll_ops, 0, sizeof(linked_list_ops_t));
    
    return ll;
    
error_out:
    return NULL;
}

static int ll_create_copy_cb(void *data, void *ctx, bool *stop) {
    linked_list_t *new_ll = (linked_list_t *)ctx;
    int err;
    
    err = ll_insert(new_ll, data);
    if (err)
        goto error_out;
    
    return 0;
    
error_out:
    return err;
}

linked_list_t *ll_create_copy(linked_list_t *ll) {
    linked_list_t *new_ll;
    int err;
    
    new_ll = (linked_list_t *)malloc(sizeof(linked_list_t));
    if (!new_ll) {
        printf("ll_create: failed to allocate memory for new_ll\n");
        goto error_out;
    }
    
    memset(new_ll, 0, sizeof(linked_list_t));
    
    new_ll->ll_ops = (linked_list_ops_t *)malloc(sizeof(linked_list_ops_t));
    if (!new_ll->ll_ops) {
        printf("ll_create: failed to allocate memory for ll_ops\n");
        goto error_out;
    }
    memcpy(new_ll->ll_ops, ll->ll_ops, sizeof(linked_list_ops_t));
    
    err = ll_iterate(ll, ll_create_copy_cb, (void *)new_ll);
    if (err)
        goto error_out;
    
    return new_ll;
    
error_out:
    if (new_ll)
        ll_destroy(new_ll);
    
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
    if (ll->ll_ops)
        free(ll->ll_ops);
    free(ll);
}

int ll_insert(linked_list_t *ll, void *data) {
    ll_node_t *new_node, *curr, *prev;
    int err;
    
    new_node = (ll_node_t *)malloc(sizeof(ll_node_t));
    if (!new_node) {
        err = ENOMEM;
        goto error_out;
    }
    memset(new_node, 0, sizeof(ll_node_t));
    new_node->ln_data = data;
    
    prev = NULL;
    curr = ll->ll_root;
    while (curr) {
        if (ll->ll_ops->llo_compare_fn(curr->ln_data, data) == 0) {
            err = EEXIST;
            goto error_out;
        }
        prev = curr;
        curr = curr->ln_next;
    }
    
    if (!prev) { // inserting at root
        assert(ll_empty(ll));
        ll->ll_root = new_node;
    } else {
        prev->ln_next = new_node;
    }
    
    ll->ll_nnodes++;
    
    return 0;
    
error_out:
    if (new_node)
        free(new_node);
    
    return err;
}

int ll_insert_head(linked_list_t *ll, void *data) {
    ll_node_t *new_node, *curr;
    int err;
    
    new_node = (ll_node_t *)malloc(sizeof(ll_node_t));
    if (!new_node) {
        err = ENOMEM;
        goto error_out;
    }
    memset(new_node, 0, sizeof(ll_node_t));
    new_node->ln_data = data;
    
    // first check if it already exists
    curr = ll->ll_root;
    while (curr) {
        if (ll->ll_ops->llo_compare_fn(curr->ln_data, data) == 0) {
            err = EEXIST;
            goto error_out;
        }
        curr = curr->ln_next;
    }
    
    new_node->ln_next = ll->ll_root;
    ll->ll_root = new_node;
    
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

size_t ll_length(linked_list_t *ll) {
    return ll->ll_nnodes;
}

bool ll_empty(linked_list_t *ll) {
    return (ll->ll_nnodes == 0);
}

int ll_iterate(linked_list_t *ll, int (*callback)(void *, void *, bool *), void *ctx) {
    ll_node_t *n;
    bool stop = false;
    int err;
    
    n = ll->ll_root;
    while (n) {
        err = callback(n->ln_data, ctx, &stop);
        if (err)
            goto error_out;
        if (stop)
            goto out;
        n = n->ln_next;
    }
    
out:
    return 0;
    
error_out:
    return err;
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
