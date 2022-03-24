CC=gcc
CFLAGS=-g
DS=/home/mholden/devel/data_structs
INCLUDES=-I$(DS)/include

all: test_linked_list

test_linked_list: test_linked_list.c linked_list.c $(DS)/include/linked_list.h
	$(CC) $(CFLAGS) $(INCLUDES) test_linked_list.c linked_list.c -o test_linked_list

clean:
	rm *o test_linked_list

.PHONY: all clean
