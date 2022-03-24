#include "hash_table.h"
#include "linked_list.h"
#include <stdio.h>
#include <stdlib.h>

/*
 * Hash Table.
 */

int ht_insert(struct hash_table *ht, void *key, void *value){
	unsigned int hash;
	int ret;

	hash = ht->hash(key, ht->keysz) % ht->size;

	if(ht->table[hash] == NULL){ /* Nothing at this index */
		ht->table[hash] = ll_create(ht->keysz, ht->valsz, ht->key_compare);
		if(ht->table[hash] == NULL)
            return -1;
	}

	ret = ll_insert(ht->table[hash], key, value, 0);
	if(ret)
        return -1;

	return 0;
}

int ht_find(struct hash_table *ht, void *key, void *value){
	unsigned int hash;
    int ret;
    
    hash = ht->hash(key, ht->keysz) % ht->size;
    
    if(ht->table[hash] == NULL){ /* Nothing at this index */
        return -1;
    }
    
    ret = ll_find(ht->table[hash], key, value);
    if(ret)
        return -1;

	return 0;
}

int ht_remove(struct hash_table *ht, uint64_t key){
	/* IMPELEMENT */
	return 0;
}

static unsigned int ht_standard_hash(void *key, size_t bytes){
	unsigned int hash;
	char *p;
	int i;

	p = (char *)key;
	hash = 2166136261;
	
	i = 0;
	for(i=0; i<bytes; i++)
		hash = (hash ^ p[i]) * 16777619;

	return hash;
}

struct hash_table *ht_create(size_t keysz, size_t valsz, int (*key_compare)(void *key1, void *key2), int size){
	struct hash_table *ht;
	int i;

	ht = (struct hash_table *)malloc(sizeof(struct hash_table));
	if(ht == NULL){
		printf("ht_create(): Out of memory?\n");
		return NULL;
	}

	ht->hash = ht_standard_hash;
	
	ht->table = (struct linked_list **)malloc(size * sizeof(struct linked_list *));
	if(ht->table == NULL){
		printf("ht_create(): Out of memory?\n");
		free(ht);
		return NULL;
	}
	/* Initialize all to NULL */
	for(i=0; i<size; i++)
        ht->table[i] = NULL;

    ht->key_compare = key_compare;
    ht->keysz = keysz;
    ht->valsz = valsz;
	ht->size = size;

	return ht;
}

void ht_destroy(struct hash_table *ht){
	int i;

	for(i=0; i<ht->size; i++){
		if(ht->table[i])
            ll_destroy(ht->table[i]);
	}

	free(ht);
	return;
}
