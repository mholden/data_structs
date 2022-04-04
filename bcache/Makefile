CC=gcc
CFLAGS=-g
DEVEL=/home/mholden/devel
DS=$(DEVEL)/data_structs
SY=$(DEVEL)/synch
INCLUDES=-I$(DS)/include -I$(SY)/include

all: bcache

bcache: bcache.c test_bcache.c ../include/bcache.h
	$(CC) $(CFLAGS) $(INCLUDES) bcache.c test_bcache.c $(SY)/synch.c  ../linked_lists/linked_list.c \
	../hash_tables/hash_table.c -pthread -o test_bcache

clean:
	rm *o test_bcache

.PHONY: all clean
