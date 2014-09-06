#ifndef _UD_GRAPH_H_
#define _UD_GRAPH_H_

/*
 * A simple undirected graph.
 */

struct linked_list;

struct ud_graph{
	int num_vertices;
	int max_size;
	struct linked_list **adj_list;
	struct ud_graph_ops *ops;
};

struct ud_graph_ops{
        int (*add_vertex)(struct ud_graph *graph, int *new_vertex);
        int (*add_edge)(struct ud_graph *graph, int v1, int v2);
};

struct ud_graph *udg_create(int size);
void udg_destroy(struct ud_graph *graph);

/* 
 * Depth first search - searches for any path in graph from start_vertex
 * to search_vertex, and prints the result. 
 */
void udg_dfs(struct ud_graph *graph, int start_vertex, int search_vertex);

/*
 * Breadth first search - finds the shortest path in graph from start_vertex
 * to search_vertex if one exists, and prints the result.
 */
void udg_bfs(struct ud_graph *graph, int start_vertex, int search_vertex);

#endif /* _UD_GRAPH_H_ */
