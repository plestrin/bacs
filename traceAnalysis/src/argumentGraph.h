#ifndef ARGUMENTGRAPH_H
#define ARGUMENTGRAPH_H

#include "graph.h"
#include "argBuffer.h"

void argumentGraph_node_print_dot(void* node_data, FILE* file);
void argumentGraph_edge_print_dot(void* edge_data, FILE* file);

static inline struct graph* argumentGraph_create(){
	struct graphCallback callback;

	callback.node_print_dot = argumentGraph_node_print_dot;
	callback.edge_print_dot = argumentGraph_edge_print_dot;
	callback.node_clean 	= NULL;
	callback.edge_clean 	= (void(*)(void*))argBuffer_delete;

	return graph_create(&callback);
}

int32_t argumentGraph_add_argument(struct graph* graph, struct argument* argument);


/* liste des choses qu'il reste à faire pourque le graph soir a peu près fonctionnel
	- 1 - faire des méthodes print pour les nodes et les edges
	- 2 - faire une méthode pour regrouper suivant différents niveaux
*/

#endif