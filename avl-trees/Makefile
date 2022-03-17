CC=gcc
CFLAGS=-g
#INCLUDES=-I./include

all: avl-tree

avl-tree: avl-tree.o test-avl-tree.o
	$(CC) $(CFLAGS) avl-tree.o test-avl-tree.o -o test-avl-tree

avl-tree.o: avl-tree.c avl-tree.h
	$(CC) $(CFLAGS) -c avl-tree.c

test-avl-tree.o: test-avl-tree.c
	$(CC) $(CFLAGS) -c test-avl-tree.c

clean:
	rm *o test-avl-tree

.PHONY: all clean
