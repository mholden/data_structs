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

	graph = udg_create(10);

	udg_destroy(graph);	

	return 0;
}
