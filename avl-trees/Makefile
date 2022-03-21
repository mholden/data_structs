CC=gcc
CFLAGS=-g
INCLUDES=-I./include

all: avl-tree

avl-tree: avl-tree.o test-avl-tree.o
	$(CC) $(CFLAGS) $(INCLUDES) avl-tree.o test-avl-tree.o -o test-avl-tree

avl-tree.o: avl-tree.c include/avl-tree.h
	$(CC) $(CFLAGS) $(INCLUDES) -c avl-tree.c

test-avl-tree.o: test-avl-tree.c
	$(CC) $(CFLAGS) $(INCLUDES) -c test-avl-tree.c

clean:
	rm *o test-avl-tree

.PHONY: all clean
