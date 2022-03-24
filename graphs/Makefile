CC=gcc
CFLAGS=-g
DS=/home/mholden/devel/data_structs
INCLUDES=-I$(DS)/include

all: test_ud_graph

test_ud_graph: test_ud_graph.c ud_graph.c $(DS)/include/ud_graph.h
	$(CC) $(CFLAGS) $(INCLUDES) test_ud_graph.c ud_graph.c $(DS)/avl_trees/avl_tree.c \
		$(DS)/linked_lists/linked_list.c $(DS)/queues/queue.c -o test_ud_graph

clean:
	rm *o test_ud_graph

.PHONY: all clean
