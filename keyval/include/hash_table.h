#ifndef _HASH_TABLE_H_
#define _HASH_TABLE_H_

/*
 * Hash Table.
 */
#include <stdint.h>

struct linked_list;

struct hash_table{
	unsigned int (*hash)(void *key, size_t bytes);
	struct linked_list **table;
    int (*key_compare)(void *key1, void *key2);
    size_t keysz;
    size_t valsz;
	size_t size;
};

struct hash_table *ht_create(size_t keysz, size_t valsz, int (*key_compare)(void *key1, void *key2), int size);
int ht_insert(struct hash_table *ht, void *key, void *value);
int ht_find(struct hash_table *ht, void *key, void *value);
void ht_destroy(struct hash_table *ht);

#endif /* _HASH_TABLE_H */
