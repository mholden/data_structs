#include "include/hash_table.h"
#include <stdio.h>
#include <stdlib.h>

/*
 * Hash Table.
 */

static int ht_insert(struct hash_table *ht, int key, int value){
	/* IMPLEMENT */
	return 0;
}

static int ht_find(struct hash_table *ht, int key, int *value){
	/* IMPLEMENT */
	return 0;
}

static int ht_remove(struct hash_table *ht, int key){
	/* IMPELEMENT */
	return 0;
}

static struct ht_ops ht_standard_ops = {
	ht_insert,
	ht_find,
	ht_remove
};

static int ht_standard_hash(void *key){
	unsigned long ul_key;
	int hash;

	ul_key = (unsigned long) key;
	/* Call the hash function */

	return hash;
}

struct hash_table *ht_create(int size){
	struct hash_table *ht;

	ht = malloc(sizeof(struct hash_table));
	if(ht == NULL){
		printf("ht_create(): Out of memory?\n");
		return NULL;
	}

	ht->hash = ht_standard_hash;
	ht->table = malloc(size * sizeof(struct hash_entry *));
	if(ht->table == NULL){
		printf("ht_create(): Out of memory?\n");
		free(ht);
		return NULL;
	}
	ht->ops = &ht_standard_ops;

	return ht;
}

void ht_destroy(struct hash_table *ht){
	free(ht->table);
	free(ht);
	return;
}
