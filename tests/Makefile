CC=g++
CFLAGS=-g -std=c++20
DEVEL=/home/mholden/devel
INCLUDES=-I$(DEVEL)/data_structs/avl-trees/include -I$(DEVEL)/data_structs/keyval/include

all: avl-comparisons

avl-comparisons: avl-comparisons.cpp
	$(CC) $(CFLAGS) $(INCLUDES) avl-comparisons.cpp $(DEVEL)/data_structs/avl-trees/avl-tree.c \
	$(DEVEL)/data_structs/keyval/binary_tree.c $(DEVEL)/data_structs/keyval/hash_table.c \
	$(DEVEL)/data_structs/keyval/linked_list.c -o avl-comparisons

clean:
	rm *o avl-comparisons

.PHONY: all clean
