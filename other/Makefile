CC=gcc
CFLAGS=-g
#INCLUDES=-I./include

all: heap

heap: heap.o test-heap.o
	$(CC) $(CFLAGS) heap.o test-heap.o -o test-heap

heap.o: heap.c
	$(CC) $(CFLAGS) -c heap.c

test-heap.o: test-heap.c
	$(CC) $(CFLAGS) -c test-heap.c

clean:
	rm *o test-heap

.PHONY: all clean
