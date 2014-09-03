#include "include/linked_list.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>

#define NAME_LEN 30

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

	char *array;
	int n, i, j;
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
	
	for(i=0; i<n; i++) array[i] = rand() % 2*n;

	printf("Data Structure\t\tNumber of Inserts/Lookups\tTime\n");

	/* 1. Linked List */
	struct linked_list *ll;
	int val, ret;

	strcpy(data_struct, "linked list");
	
	ll = ll_create();
	if(ll == NULL) return -1;

	/* n inserts */
	start = clock();
	for(i=0; i<n; i++){
		ret = ll->ops->insert(&ll->root, i, array[i]);
		if(ret) return -1;
	}
	end = clock();

	tot = (double) (end - start) / CLOCKS_PER_SEC;
	avg = tot / (double) n;
	printf("%s\t\t%d inserts\t\t\t%fs\n", data_struct, n, avg);

	/* n lookups */
	tot = 0;
	for(i=0; i<n; i++){
		j = rand() % n;
		start = clock();
		ret = ll->ops->find(ll->root, j, &val);
		if(ret) return -1;
		end = clock();
		tot += (double) (end - start) / CLOCKS_PER_SEC;
	}

	avg = tot / (double) n;
	printf("%s\t\t%d lookups\t\t\t%fs\n", data_struct, n, avg);

	ll_destroy(ll);

	/* 2. Binary Search Tree */

	/* 3. Hash Table */
	
	return 0;
}
