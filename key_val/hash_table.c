#include "include/hash_table.h"
#include "include/linked_list.h"
#include <stdio.h>
#include <stdlib.h>

/*
 * Hash Table.
 */

static int ht_insert(struct hash_table *ht, int key, int value){
	unsigned int hash;
	int ret;

	hash = ht->hash(key) % ht->size;

	if(ht->table[hash] == NULL){ /* Nothing at this index */
		ht->table[hash] = ll_create();
		if(ht->table[hash] == NULL) return -1;
	}

	ret = ht->table[hash]->ops->insert(&ht->table[hash]->root, key, value);
	if(ret) return -1;

	return 0;
}

static int ht_find(struct hash_table *ht, int key, int *value){
	unsigned int hash;
        int ret;

        hash = ht->hash(key) % ht->size;

        if(ht->table[hash] == NULL){ /* Nothing at this index */
                //printf("ht_find(): key doesn't exist.\n");
                return -1;
        }

        ret = ht->table[hash]->ops->find(ht->table[hash]->root, key, value);
        if(ret) return -1;

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

static unsigned int ht_standard_hash(unsigned long key){
	unsigned int hash;
	char *p;
	int i;

	p = (char *) &key;
	hash = 2166136261;
	
	i = 0;
	for(i=0; i<sizeof(unsigned long); i++)
		hash = (hash ^ p[i]) * 16777619;

	return hash;
}

struct hash_table *ht_create(int size){
	struct hash_table *ht;
	int i;

	ht = malloc(sizeof(struct hash_table));
	if(ht == NULL){
		printf("ht_create(): Out of memory?\n");
		return NULL;
	}

	ht->hash = ht_standard_hash;
	
	ht->table = malloc(size * sizeof(struct linked_list *));
	if(ht->table == NULL){
		printf("ht_create(): Out of memory?\n");
		free(ht);
		return NULL;
	}
	/* Initialize all to NULL */
	for(i=0; i<size; i++) ht->table[i] = NULL;
	
	ht->ops = &ht_standard_ops;

	ht->size = size;

	return ht;
}

void ht_destroy(struct hash_table *ht){
	int i;

	for(i=0; i<ht->size; i++){
		if(ht->table[i]) ll_destroy(ht->table[i]);
	}

	free(ht);
	return;
}
