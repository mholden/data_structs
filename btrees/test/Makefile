CC=gcc
CFLAGS=-g
DEVEL=/home/mholden/devel
DS=$(DEVEL)/data_structs
SY=$(DEVEL)/synch
INCLUDES=-I./include -I$(DS)/include -I$(SY)/include

all: test_btree

test_btree: test_btree.c ../btree.c $(DS)/include/btree.h tbr.c include/tbr.h tbr0.c include/tbr0.h tbr1.c \
				include/tbr1.h $(DS)/bcache/bcache.c
	$(CC) $(CFLAGS) $(INCLUDES) test_btree.c tbr.c tbr0.c tbr1.c ../btree.c $(SY)/synch.c \
		$(DS)/queues/fifos/fifo.c $(DS)/bcache/bcache.c -pthread -o test_btree

clean:
	rm *o test_btree

.PHONY: all clean
