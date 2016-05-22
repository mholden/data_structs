#ifndef _HASH_TABLE_H_
#define _HASH_TABLE_H_

/*
 * Hash Table definitions.
 */
#include <stdint.h>

struct linked_list;

struct hash_table{
	unsigned int (*hash)(unsigned long key);
	struct linked_list **table;
	struct ht_ops *ops;
	int size;
};

struct ht_ops{
	int (*insert)(struct hash_table *ht, uint64_t key, uint64_t value);
	int (*find)(struct hash_table *ht, uint64_t key, uint64_t *value);
	int (*remove)(struct hash_table *ht, uint64_t key);
};

struct hash_table *ht_create(int size);
void ht_destroy(struct hash_table *ht);

#endif /* _HASH_TABLE_H */
