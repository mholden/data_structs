CC=gcc
CFLAGS=-g
DS=/home/mholden/devel/data_structs
INCLUDES=-I$(DS)/include

all: test_udw_graph

test_udw_graph: test_udw_graph.c udw_graph.c $(DS)/include/udw_graph.h $(DS)/queues/heaps/heap.c
	$(CC) $(CFLAGS) $(INCLUDES) test_udw_graph.c udw_graph.c $(DS)/avl_trees/avl_tree.c \
		$(DS)/linked_lists/linked_list.c $(DS)/queues/heaps/heap.c -o test_udw_graph

clean:
	rm *o test_udw_graph

.PHONY: all clean
