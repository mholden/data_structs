#ifndef _HASH_TABLE_H_
#define _HASH_TABLE_H_

/*
 * Hash Table definitions.
 */

struct linked_list;

struct hash_table{
        unsigned int (*hash)(void *key);
        struct linked_list **table;
	struct ht_ops *ops;
	int size;
};

struct ht_ops{
	int (*insert)(struct hash_table *ht, int key, int value);
	int (*find)(struct hash_table *ht, int key, int *value);
	int (*remove)(struct hash_table *ht, int key);
};

struct hash_table *ht_create(int size);
void ht_destroy(struct hash_table *ht);

#endif /* _HASH_TABLE_H */
