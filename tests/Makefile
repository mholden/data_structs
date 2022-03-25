CC=g++
CFLAGS=-g -std=c++20
DS=/home/mholden/devel/data_structs
INCLUDES=-I$(DS)/include

all: avl_comparisons

avl_comparisons: avl_comparisons.cpp
	$(CC) $(CFLAGS) $(INCLUDES) avl_comparisons.cpp $(DS)/avl_trees/avl_tree.c \
	$(DS)/binary_trees/binary_tree.c $(DS)/hash_tables/hash_table.c \
	$(DS)/linked_lists/linked_list.c -o avl_comparisons

clean:
	rm *o avl_comparisons

.PHONY: all clean
