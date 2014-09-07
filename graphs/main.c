#include "include/ud_graph.h"

/*
 * Main.
 */

int main(int argc, char **argv){
	/* 
	 * Size of graph to generate in argv[0]
	 * Max edges = (size)(size-1)/2, so generate some 
	 * fraction of that randomly.
	 * Do dfs and bfs on a couple random vertexes
	 */
	struct ud_graph *graph;
	int ret, dummy_vert, i, max_edges, edges_to_add, v1, v2;

	graph = udg_create(6);

	/* Build a graph of maximum size */
	for(i=0; i<graph->max_size; i++){
		ret = graph->ops->add_vertex(graph, &dummy_vert);
		if(ret) return -1;
	}

	/* Add a bunch of edges */
	max_edges = graph->max_size * (graph->max_size - 1) / 2;
	edges_to_add = max_edges / 2;

	for(i=0; i<edges_to_add; i++){
		v1 = (i + 3) % graph->max_size;		
		v2 = (i + 1) % graph->max_size;

		ret = graph->ops->add_edge(graph, v1, v2);
		if(ret) return -1;
	}

	v1 = 0;
	v2 = 1;
	ret = graph->ops->add_edge(graph, v1, v2);
	if(ret) return -1;

	/* 
	 * Our graph looks like this:
	 *			
	 *	----0---1-------3
	 *	|   |	|	|
	 *	|   |	|	|
	 *	2---4	5-------|
	 */

	/* Test depth-first search */
	v1 = 0;
	v2 = 4;
	udg_dfs(graph, v1, v2);

	v1 = 3;
	v2 = 2;
	udg_dfs(graph, v1, v2);
	
	/* Test breadth-first search */
	v1 = 0;
        v2 = 4;
        udg_bfs(graph, v1, v2);

        v1 = 3;
        v2 = 2;
        udg_bfs(graph, v1, v2);
	
	udg_destroy(graph);	

	return 0;
}
