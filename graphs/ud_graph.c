#include "include/ud_graph.h"
#include "../key_val/include/linked_list.h"
#include <stdlib.h>
#include <stdio.h>

static int udg_add_vertex(struct ud_graph *graph, int *new_vertex){
	/* IMPLEMENT */
	return 0;
}

static int udg_add_edge(struct ud_graph *graph, int v1, int v2){
        /* IMPLEMENT */
        return 0;
}

static struct ud_graph_ops udg_standard_ops = {
	udg_add_vertex,
	udg_add_edge
};

struct ud_graph *udg_create(int size){
	struct ud_graph *graph;
	int i;

	graph = malloc(sizeof(struct ud_graph));
	if(graph == NULL){
		printf("udg_create(): Out of memory?\n");
		return NULL;
	}

	graph->num_vertices = 0;
	graph->max_size = size;
	graph->adj_list = malloc(size * sizeof(struct linked_list *));
	if(graph->adj_list == NULL){
		printf("udg_create(): Out of memory?\n");
                return NULL;
	}

	for(i=0; i<size; i++) graph->adj_list[i] = NULL;
	
	graph->ops = &udg_standard_ops;

	return graph;
}

void udg_destroy(struct ud_graph *graph){
	int i;

	for(i=0; i<graph->max_size; i++){
		if(graph->adj_list[i] != NULL) 
			ll_destroy(graph->adj_list[i]);
	}

	free(graph);

	return;
}
