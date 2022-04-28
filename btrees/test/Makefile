CC=gcc
CFLAGS=-g
DEVEL=/home/mholden/devel
DS=$(DEVEL)/data_structs
SY=$(DEVEL)/synch
INCLUDES=-I$(DS)/include -I$(SY)/include

all: test_btree

test_btree: test_btree.c ../btree.c $(DS)/include/btree.h $(DS)/bcache/bcache.c
	$(CC) $(CFLAGS) $(INCLUDES) test_btree.c ../btree.c $(SY)/synch.c $(DS)/queues/fifos/fifo.c $(DS)/bcache/bcache.c -pthread -o test_btree

clean:
	rm *o test_btree

.PHONY: all clean
