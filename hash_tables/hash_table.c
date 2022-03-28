#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>

#include "hash_table.h"
#include "linked_list.h"

unsigned int ht_default_hash(void *key, size_t keysz) {
    unsigned int hash;
    char *p;

    p = (char *)key;
    hash = 2166136261;
    
    for (int i = 0; i < keysz; i++)
        hash = (hash ^ p[i]) * 16777619;

    return hash;
}

hash_table_t *ht_create(hash_table_ops_t *hops) {
    hash_table_t *ht;
    
    ht = (hash_table_t *)malloc(sizeof(hash_table_t));
    if (!ht) {
        printf("ht_create: failed to allocate memory for ht\n");
        goto error_out;
    }
    
    memset(ht, 0, sizeof(hash_table_t));
    
    ht->ht_table = (linked_list_t **)malloc(sizeof(linked_list_t *) * HT_START_SIZE);
    if (!ht->ht_table) {
        printf("ht_create: failed to allocate memory for ht_table\n");
        goto error_out;
    }
    memset(ht->ht_table, 0, sizeof(linked_list_t *) * HT_START_SIZE);
    ht->ht_size = HT_START_SIZE;
    
    ht->ht_ops = (hash_table_ops_t *)malloc(sizeof(hash_table_ops_t));
    if (!ht->ht_ops) {
        printf("ht_create: failed to allocate memory for ht_ops\n");
        goto error_out;
    }
    memcpy(ht->ht_ops, hops, sizeof(hash_table_ops_t));
    
    return ht;
    
error_out:
    if (ht) {
        if (ht->ht_table)
            free(ht->ht_table);
        
        free(ht);
    }
    
    return NULL;
}

#define HTDT_PRESERVE_DATA 0x01

static void ht_destroy_table(linked_list_t **table, size_t tsize, uint8_t flags) {
    for (int i = 0; i < tsize; i++) {
        if (table[i]) {
            if (flags & HTDT_PRESERVE_DATA)
                table[i]->ll_ops->llo_destroy_data_fn = NULL;
            ll_destroy(table[i]);
        }
    }
    free(table);
}

void ht_destroy(hash_table_t *ht) {
    ht_destroy_table(ht->ht_table, ht->ht_size, 0);
    free(ht->ht_ops);
    free(ht);
}

static int _ht_insert(hash_table_t *ht, linked_list_t **table, size_t tsize, void *data) {
    linked_list_t *ll;
    unsigned index;
    int err;
    
    index = ht->ht_ops->hto_hash_fn(data) % tsize;
    ll = table[index];
    if (!ll) { // create it
        ll = ll_create(&ht->ht_ops->hto_lops);
        if (!ll) {
            err = ENOMEM;
            goto error_out;
        }
        table[index] = ll;
    }
    
    err = ll_insert_head(ll, data);
    if (err)
        goto error_out;
    
    return 0;
    
error_out:
    return err;
}

int ht_insert(hash_table_t *ht, void *data) {
    int err;
    
    err = _ht_insert(ht, ht->ht_table, ht->ht_size, data);
    if (err)
        goto error_out;
    
    ht->ht_nelements++;
    
    // grow the table if it's no longer large enough
    if (ht_needs_resize(ht))
        ht_resize(ht);
    
    return 0;
    
error_out:
    return err;
}

int ht_find(hash_table_t *ht, void *to_find, void **data) {
    linked_list_t *ll;
    unsigned index;
    int err;
    
    index = ht->ht_ops->hto_hash_fn(to_find) % ht->ht_size;
    ll = ht->ht_table[index];
    if (!ll) {
        err = ENOENT;
        goto error_out;
    }
    
    err = ll_find(ll, to_find, data);
    if (err)
        goto error_out;
    
    return 0;
    
error_out:
    return err;
}

int ht_remove(hash_table_t *ht, void *to_remove) {
    linked_list_t *ll;
    unsigned index;
    int err;
    
    index = ht->ht_ops->hto_hash_fn(to_remove) % ht->ht_size;
    ll = ht->ht_table[index];
    if (!ll) {
        err = ENOENT;
        goto error_out;
    }
    
    err = ll_remove(ll, to_remove);
    if (err)
        goto error_out;
    
    ht->ht_nelements--;
    
    return 0;
    
error_out:
    return err;
}

bool ht_needs_resize(hash_table_t *ht) {
    return (ht->ht_nelements > (ht->ht_size / 2));
}

typedef struct ht_resize_migrate_cb_ctx {
    hash_table_t *ht;
    linked_list_t **new_table;
    size_t new_size;
} ht_resize_migrate_cb_ctx_t;

static int ht_resize_migrate_cb(void *data, void *ctx, bool *stop) {
    ht_resize_migrate_cb_ctx_t *hrc = (ht_resize_migrate_cb_ctx_t *)ctx;
    hash_table_t *ht = hrc->ht;
    linked_list_t **new_table = hrc->new_table;
    size_t new_size = hrc->new_size;
    int err;
    
    err = _ht_insert(ht, new_table, new_size, data);
    if (err)
        goto error_out;
    
    return 0;
    
error_out:
    return err;
}

int ht_resize(hash_table_t *ht) {
    linked_list_t **new_table;
    size_t new_size;
    ht_resize_migrate_cb_ctx_t hrc;
    int err;
    
    new_size = ht->ht_size * 2;
    new_table = (linked_list_t **)malloc(sizeof(linked_list_t *) * new_size);
    if (!new_table) {
        err = ENOMEM;
        goto error_out;
    }
    
    memset(&hrc, 0, sizeof(ht_resize_migrate_cb_ctx_t));
    hrc.ht = ht;
    hrc.new_table = new_table;
    hrc.new_size = new_size;
    
    err = ht_iterate(ht, ht_resize_migrate_cb, (void *)&hrc);
    if (err)
        goto error_out;
    
    // destroy the old table
    ht_destroy_table(ht->ht_table, ht->ht_size, HTDT_PRESERVE_DATA);
    
    ht->ht_table = new_table;
    ht->ht_size = new_size;
    
    return 0;
    
error_out:
    if (new_table)
        ht_destroy_table(new_table, new_size, HTDT_PRESERVE_DATA);
    
    return err;
}

typedef struct ht_iterate_cb_ctx {
    int (*callback)(void *, void *, bool *);
    void *ctx;
    bool *stop;
} ht_iterate_cb_ctx_t;

static int ht_iterate_cb(void *data, void *ctx, bool *stop) {
    ht_iterate_cb_ctx_t *hic = (ht_iterate_cb_ctx_t *)ctx;
    int err;
    
    err = hic->callback(data, hic->ctx, hic->stop);
    if (err)
        goto error_out;
    
    if (*hic->stop)
        *stop = true;
    
    return 0;
    
error_out:
    return err;
}

int ht_iterate(hash_table_t *ht, int (*callback)(void *, void *, bool *), void *ctx) {
    ht_iterate_cb_ctx_t hic;
    bool stop = false;
    int err;
    
    memset(&hic, 0, sizeof(ht_iterate_cb_ctx_t));
    hic.callback = callback;
    hic.ctx = ctx;
    hic.stop = &stop;
    
    for (int i = 0; i < ht->ht_size; i++) {
        if (ht->ht_table[i]) {
            err = ll_iterate(ht->ht_table[i], ht_iterate_cb, (void *)&hic);
            if (err)
                goto error_out;
            if (stop)
                goto out;
        }
    }
    
out:
    return 0;
    
error_out:
    return err;
}

void ht_check(hash_table_t *ht) {
    int nelements;
    
    nelements = 0;
    for (int i = 0; i < ht->ht_size; i++) {
        if (ht->ht_table[i])
            nelements += ht->ht_table[i]->ll_nnodes;
    }
    
    assert(nelements == ht->ht_nelements);
        
    return;
}

void ht_dump(hash_table_t *ht) {
    linked_list_t *ll;
    
    printf("ht_size: %d\n", ht->ht_size);
    printf("ht_nelements: %d\n", ht->ht_nelements);
    for (int i = 0; i < ht->ht_size; i++) {
        if (ht->ht_table[i]) {
            printf("ht_table[%d]: ", i);
            ll_dump(ht->ht_table[i]);
        }
    }
    
    return;
}
