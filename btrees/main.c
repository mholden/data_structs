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
	
	key = 0x30;
	error = btree_insert(&bt, &key, &val);
	
	key = 0x50;
	error = btree_insert(&bt, &key, &val);
	
	key = 0x75;
	error = btree_insert(&bt, &key, &val);
	
	key = 0x65;
	error = btree_insert(&bt, &key, &val);
	
	key = 0x63;
	error = btree_insert(&bt, &key, &val);
	
	key = 0x90;
	error = btree_insert(&bt, &key, &val);
	
	key = 0x10;
	error = btree_insert(&bt, &key, &val);
	
	key = 0x00;
	error = btree_insert(&bt, &key, &val);
	
	key = 0x03;
	error = btree_insert(&bt, &key, &val);
	
	// after the split caused by 0x03, the 0x30 disappears??
	// but if I do the 'print index 1 node' it doesn't happen.. definitely a bug somewhere..
	
	key = 0x23;
	error = btree_insert(&bt, &key, &val);
	
	key = 0x38;
	error = btree_insert(&bt, &key, &val);
	
	key = 0x42;
	error = btree_insert(&bt, &key, &val);
	
	return error;
}
