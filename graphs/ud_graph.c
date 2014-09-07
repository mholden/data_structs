#include "include/ud_graph.h"
#include "../key_val/include/linked_list.h"
#include "../other/include/queue.h"
#include <stdlib.h>
#include <stdio.h>

static int udg_add_vertex(struct ud_graph *graph, int *new_vertex){
	int i;

	if(graph->num_vertices == graph->max_size){
		printf("udg_add_vertex(): graph already at max size.\n");
		return -1;
	}

	for(i=0; i<graph->max_size; i++){
		if(graph->adj_list[i] == NULL) break;
	}

	graph->adj_list[i] = ll_create();
	if(graph->adj_list[i] == NULL) return -1;

	graph->num_vertices++;
	
	*new_vertex = i;
	return 0;
}

static int udg_add_edge(struct ud_graph *graph, int v1, int v2){
	struct linked_list *v1_ll, *v2_ll;
	int dummy_val, ret;

	if(v1 >= graph->max_size || v2 >= graph->max_size || v1 < 0 || v2 < 0){
		printf("udg_add_edge(): one of the vertices is invalid.\n");
		return -1;
	}

	v1_ll = graph->adj_list[v1];
	v2_ll = graph->adj_list[v2];
	if(v1_ll == NULL || v2_ll == NULL){
		printf("udg_add_edge(): one of the vertices does not exist.\n");
		return -1;
	}

	if(!v1_ll->ops->find(v1_ll->root, v2, &dummy_val)){
		/* Edge already exists.. do nothing */
		return 0;
	}

	/* 
	 * Edge doesn't exist, so add it. 
	 * Note: 'value' in linked list is unused.
	 */
	ret = v1_ll->ops->insert(&v1_ll->root, v2, -1);
	if(ret) return -1;

	ret = v2_ll->ops->insert(&v2_ll->root, v1, -1);
	if(ret) return -1;

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

static void _udg_dfs(struct ud_graph *graph, int start_vertex, int *marked, int *path_to){
	struct ll_node *curr_node;
	int vertex;

	curr_node = graph->adj_list[start_vertex]->root;
        while(curr_node != NULL){
                vertex = curr_node->key;
                if(!marked[vertex]){
                        marked[vertex] = 1;
                        path_to[vertex] = start_vertex;
                        _udg_dfs(graph, vertex, marked, path_to);
                }
                curr_node = curr_node->next;
        }

	return;
}

void udg_dfs(struct ud_graph *graph, int start_vertex, int search_vertex){
	int *marked, *path_to;
	int i;

	marked = malloc(graph->num_vertices * sizeof(int));
	if(marked == NULL){
		printf("udg_dfs(): Out of memory?\n");
		return;
	}

	for(i=0; i<graph->num_vertices; i++) marked[i] = 0;
	marked[start_vertex] = 1;

	path_to = malloc(graph->num_vertices * sizeof(int));
	if(path_to == NULL){
		printf("udg_dfs(): Out of memory?\n");
                return;
	}

	for(i=0; i<graph->num_vertices; i++) path_to[i] = -1;

	_udg_dfs(graph, start_vertex, marked, path_to);

	if(marked[search_vertex]){
		printf("DFS - path from %d to %d: ", start_vertex, search_vertex);
		i = search_vertex;
		while(i != start_vertex){
			printf("%d-", i);
			i = path_to[i];
		}
		printf("%d\n", start_vertex);
	}
	else printf("DFS - no path from vertex %d to vertex %d.\n", start_vertex, search_vertex);

	free(marked);
	free(path_to);

	return;
}

static void _udg_bfs(struct ud_graph *graph, struct queue *q, int *marked, int *path_to){
	int *vertex;
	int adj, ret;
	struct ll_node *curr_node;

	if(q_empty(q)) return;

	ret = q_get(q, (void **)&vertex);
	if(ret) return;

	curr_node = graph->adj_list[*vertex]->root;
	while(curr_node != NULL){
		adj = curr_node->key;
		if(!marked[adj]){
			marked[adj] = 1;
			path_to[adj] = *vertex;
			
			ret = q_add(q, &curr_node->key);
			if(ret) return;
		}
		curr_node = curr_node->next;
	}
	
	_udg_bfs(graph, q, marked, path_to);

	return;
}

void udg_bfs(struct ud_graph *graph, int start_vertex, int search_vertex){
	int *marked, *path_to;
        int i, ret;
	struct queue *q;

        marked = malloc(graph->num_vertices * sizeof(int));
        if(marked == NULL){
                printf("udg_bfs(): Out of memory?\n");
                return;
        }

        for(i=0; i<graph->num_vertices; i++) marked[i] = 0;
        marked[start_vertex] = 1;

        path_to = malloc(graph->num_vertices * sizeof(int));
        if(path_to == NULL){
                printf("udg_bfs(): Out of memory?\n");
                free(marked);
		return;
        }

        for(i=0; i<graph->num_vertices; i++) path_to[i] = -1;

	q = q_create(graph->num_vertices);
	if(q == NULL){
		free(marked);
		free(path_to);
		return;
	}

	ret = q_add(q, &start_vertex);
	if(ret){
		free(marked);
		free(path_to);
		q_destroy(q);
		return;
	}

        _udg_bfs(graph, q, marked, path_to);

        if(marked[search_vertex]){
                printf("BFS - path from %d to %d: ", start_vertex, search_vertex);
                i = search_vertex;
                while(i != start_vertex){
                        printf("%d-", i);
                        i = path_to[i];
                }
                printf("%d\n", start_vertex);
        }
        else printf("BFS - no path from vertex %d to vertex %d.\n", start_vertex, search_vertex);

        free(marked);
        free(path_to);

        return;
}
