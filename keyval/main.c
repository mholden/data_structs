#include "include/linked_list.h"
#include "include/binary_tree.h"
#include "include/hash_table.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>

#define NAME_LEN 30

/* key compare function */
static int key_compare(void *key1, void *key2){
    int _key1, _key2;
    
    _key1 = *(int *)key1;
    _key2 = *(int *)key2;
    
    if (_key1 == _key2)
        return 0;
    else
        return (_key2 - _key1);
}

/*
 * Main.
 *
 * Looking to analyze the peformance of some of the
 * most common key-value based data structures:
 * linked lists, binary search trees, and hash tables.
 */

int main(int argc, char **argv){
	/*
	 * Usage: ./key_val size
	 * 
	 * Creates each data structure, does size insertions
	 * followed by size lookups, measuring and reporting
	 * performance for each.
	 */

	/*
	 * 1. Create the data structure
	 * 2. Generate an array of size random integers
	 * 3. Insert all the integers, report performance
	 * 4. Do size random lookups (pick array index
	 *    at random and lookup the integer at that
	 *    index), report performance
	 */

	int *array;
	int next=0, unique, n, i, j;
	char data_struct[NAME_LEN];
	clock_t start, end;
	double tot, avg;

	/* Error check inputs */
	if(argc != 2){
		printf("Usage: ./key_val num\n");
		return -1;
	}

	/* Generate array of random integers */
	n = atoi(argv[1]);
	array = malloc(n * sizeof(int));
	if(array == NULL){
		printf("Out of memory?\n");
		return -1;
	}
	
	for(i=0; i<n; i++){
		unique = 0;
		while(!unique){
			next = rand() % RAND_MAX;
			for(j=0; j<i; j++){
				if(next == array[j]) break;
			}
			if(j == i) unique = 1;
		}
		array[i] = next;
	}

	printf("Data Structure\t\tNumber of Inserts/Lookups\tTime\n");

	/* 1. Linked List */
	struct linked_list *ll;
	int val, ret;

	strcpy(data_struct, "linked list");
	
	ll = ll_create(sizeof(int), sizeof(int), key_compare);
	if(ll == NULL) return -1;

	/* n inserts */
	start = clock();
	for(i=0; i<n; i++){
		ret = ll_insert(ll, &array[i], &i);
		if(ret) return -1;
	}
	end = clock();

	tot = (double) (end - start) / CLOCKS_PER_SEC;
	avg = tot / (double) n;
	printf("%s\t\t%d inserts\t\t\t%.9fs\n", data_struct, n, avg);

	/* n lookups */
	tot = 0;
	for(i=0; i<n; i++){
		j = rand() % n;
		start = clock();
		ret = ll_find(ll, &array[j], &val);
		if(ret) return -1;
		end = clock();
		tot += (double) (end - start) / CLOCKS_PER_SEC;
	}

	avg = tot / (double) n;
	printf("%s\t\t%d lookups\t\t\t%.9fs\n", data_struct, n, avg);

	ll_destroy(ll);
	ll = NULL;

	/* 2. Binary Search Tree */
	struct binary_tree *bt;
	
	strcpy(data_struct, "binary tree");
	
	bt = bt_create(sizeof(int), sizeof(int), key_compare);

	start = clock();
	for(i=0; i<n; i++){
		ret = bt_insert(bt, &array[i], &i);
		if(ret) return -1;
	}
	end = clock();

	tot = (double) (end - start) / CLOCKS_PER_SEC;
        avg = tot / (double) n;
        printf("%s\t\t%d inserts\t\t\t%.9fs\n", data_struct, n, avg);

	tot=0;
	for(i=0; i<n; i++){
		j = rand() % n;
		start = clock();
		ret = bt_find(bt, &array[j], &val);
		if(ret) return -1;
		end = clock();
		tot += (double) (end - start) / CLOCKS_PER_SEC;
	}

	avg = tot / (double) n;
        printf("%s\t\t%d lookups\t\t\t%.9fs\n", data_struct, n, avg);

	bt_destroy(bt);
	bt = NULL;

	/* 3. Hash Table */
	struct hash_table *ht;

	strcpy(data_struct, "hash table");

	ht = ht_create(sizeof(int), sizeof(int), key_compare, 2*n);
	if(ht == NULL) return -1;

	start = clock();
    for(i=0; i<n; i++){
        ret = ht_insert(ht, &array[i], &i);
        if(ret) return -1;
    }
    end = clock();
    
    tot = (double) (end - start) / CLOCKS_PER_SEC;
    avg = tot / (double) n;
    printf("%s\t\t%d inserts\t\t\t%.9fs\n", data_struct, n, avg);
    
    tot=0;
    for(i=0; i<n; i++){
        j = rand() % n;
        start = clock();
        ret = ht_find(ht, &array[j], &val);
        if(ret) return -1;
        end = clock();
        tot += (double) (end - start) / CLOCKS_PER_SEC;
    }
    
    avg = tot / (double) n;
    printf("%s\t\t%d lookups\t\t\t%.9fs\n", data_struct, n, avg);

	ht_destroy(ht);
	
	return 0;
}
