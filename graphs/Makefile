CC=gcc
CFLAGS=-g
DEVEL=/home/mholden/devel
INCLUDES=-I./include -I$(DEVEL)/data_structs/avl-trees/include -I$(DEVEL)/data_structs/keyval/include

all: test_ud_graph

test_ud_graph: test_ud_graph.c ud_graph.c include/ud_graph.h
	$(CC) $(CFLAGS) $(INCLUDES) test_ud_graph.c ud_graph.c $(DEVEL)/data_structs/avl-trees/avl-tree.c \
		$(DEVEL)/data_structs/keyval/linked_list.c -o test_ud_graph

clean:
	rm *o test_ud_graph

.PHONY: all clean
