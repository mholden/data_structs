#ifndef _HASH_TABLE_H_
#define _HASH_TABLE_H_

/*
 * Hash Table definitions.
 */

struct hash_entry{
	int key;
	int value;
};

struct hash_table{
        int (*hash)(void *key);
        struct hash_entry **table;
	struct ht_ops *ops;
};

struct ht_ops{
	int (*insert)(struct hash_table *ht, int key, int value);
	int (*find)(struct hash_table *ht, int key, int *value);
	int (*remove)(struct hash_table *ht, int key);
};

struct hash_table *ht_create(int size);
void ht_destroy(struct hash_table *ht);

#endif /* _HASH_TABLE_H */
