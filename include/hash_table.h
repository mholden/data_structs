#ifndef _HASH_TABLE_H_
#define _HASH_TABLE_H_

#include <stddef.h>
#include <stdbool.h>

#include "linked_list.h"

typedef struct hash_table_ops {
    unsigned int (*hto_hash_fn)(void *);
    linked_list_ops_t hto_lops;
} hash_table_ops_t;

typedef struct hash_table {
    linked_list_t **ht_table;
    hash_table_ops_t *ht_ops;
    size_t ht_size;
	size_t ht_nelements;
} hash_table_t;

#define HT_START_SIZE 1024

hash_table_t *ht_create(hash_table_ops_t *hops);
void ht_destroy(hash_table_t *ht);

int ht_insert(hash_table_t *ht, void *data);
int ht_find(hash_table_t *ht, void *to_find, void **data);
int ht_remove(hash_table_t *ht, void *to_remove);

unsigned int ht_default_hash(void *key, size_t keysz);

bool ht_empty(hash_table_t *ht);
int ht_get_random(hash_table_t *ht, void **data);

bool ht_needs_resize(hash_table_t *ht);
int ht_resize(hash_table_t *ht);

int ht_iterate(hash_table_t *ht, int (*callback)(void *data, void *ctx, bool *stop), void *ctx);

void ht_check(hash_table_t *ht);
void ht_dump(hash_table_t *ht);

#endif /* _HASH_TABLE_H */
