CC=gcc
CFLAGS=-g
DS=/home/mholden/devel/data_structs
INCLUDES=-I$(DS)/include

all: heap

heap: heap.o test_heap.o
	$(CC) $(CFLAGS) $(INCLUDES) heap.o test_heap.o -o test_heap

heap.o: heap.c
	$(CC) $(CFLAGS) $(INCLUDES) -c heap.c

test_heap.o: test_heap.c
	$(CC) $(CFLAGS) $(INCLUDES) -c test_heap.c

clean:
	rm *o test_heap

.PHONY: all clean
