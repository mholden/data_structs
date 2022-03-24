CC=gcc
CFLAGS=-g
DS=/home/mholden/devel/data_structs
INCLUDES=-I$(DS)/include

all: avl_tree

avl_tree: avl_tree.o test_avl_tree.o
	$(CC) $(CFLAGS) $(INCLUDES) avl_tree.o test_avl_tree.o -o test_avl_tree

avl_tree.o: avl_tree.c $(DS)/include/avl_tree.h
	$(CC) $(CFLAGS) $(INCLUDES) -c avl_tree.c

test_avl_tree.o: test_avl_tree.c
	$(CC) $(CFLAGS) $(INCLUDES) -c test_avl_tree.c

clean:
	rm *o test_avl_tree

.PHONY: all clean
