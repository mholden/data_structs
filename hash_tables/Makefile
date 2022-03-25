CC=gcc
CFLAGS=-g
DS=/home/mholden/devel/data_structs
INCLUDES=-I$(DS)/include

all: test_hash_table

test_hash_table: test_hash_table.c hash_table.c $(DS)/include/hash_table.h
	$(CC) $(CFLAGS) $(INCLUDES) test_hash_table.c hash_table.c $(DS)/linked_lists/linked_list.c -o test_hash_table

clean:
	rm *o test_hash_table

.PHONY: all clean
