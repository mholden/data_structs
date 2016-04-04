#include "include/btree.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
	btree_t *bt = NULL;
	int error = 0;
	
	bt = btree_create(sizeof(int), sizeof(int));
	if (bt == NULL)
		return -1;
	
	int key, val = 0;
	
	key = 50;
	error = btree_insert(&bt, &key, &val);
	
	key = 75;
	error = btree_insert(&bt, &key, &val);
	
	key = 25;
	error = btree_insert(&bt, &key, &val);
	
	key = 30;
	error = btree_insert(&bt, &key, &val);
	
	return error;
}