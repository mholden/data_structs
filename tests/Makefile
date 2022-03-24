CC=g++
CFLAGS=-g -std=c++20
DS=/home/mholden/devel/data_structs
INCLUDES=-I$(DS)/include

all: avl_comparisons

avl_comparisons: avl_comparisons.cpp
	$(CC) $(CFLAGS) $(INCLUDES) avl_comparisons.cpp $(DS)/avl_trees/avl_tree.c \
	$(DS)/keyval/binary_tree.c $(DS)/keyval/hash_table.c \
	$(DS)/keyval/linked_list.c -o avl_comparisons

clean:
	rm *o avl_comparisons

.PHONY: all clean
