#include <stdlib.h>
#include <stdio.h>

#include "../graph.h"
#include "../graphPrintDot.h"

#pragma GCC diagnostic ignored "-Wunused-parameter"
void node_print_dot(void* data, FILE* file, void* arg){
	fprintf(file, "[label=\"%s\"]", (char*)data);
}

#pragma GCC diagnostic ignored "-Wunused-parameter"
void edge_print_dot(void* data, FILE* file, void* arg){
	fprintf(file, "[label=\"%s\"]", (char*)data);
}

int main(){
	struct graph* 			graph;
	struct graphCallback 	callback;
	int32_t 				node_asterix;
	int32_t 				node_obelix;
	int32_t 				node_panoramix;
	int32_t 				node_idefix;
	int32_t 				node_abraracourcix;
	int32_t 				node_petibonum;
	int32_t 				edge;
	uint32_t 				i;

	callback.node_print_dot = node_print_dot;
	callback.edge_print_dot = edge_print_dot;
	callback.node_clean = NULL;
	callback.edge_clean = NULL;

	graph = graph_create(&callback);
	if (graph == NULL){
		printf("ERROR: in %s, unable to create graph\n", __func__);
		return 0;
	}

	node_asterix 		= graph_add_node(graph, "Asterix");
	node_obelix 		= graph_add_node(graph, "Obelix");
	node_panoramix 		= graph_add_node(graph, "Panoramix");
	node_idefix 		= graph_add_node(graph, "Idefix");
	node_abraracourcix 	= graph_add_node(graph, "Abraracourcix");
	node_petibonum 		= graph_add_node(graph, "Petibonum");

	graph_add_edge(graph, node_asterix, node_obelix, "Partage aventure");
	graph_add_edge(graph, node_panoramix, node_asterix, "Fournit potion");
	graph_add_edge(graph, node_idefix, node_obelix, "Obéit");
	graph_add_edge(graph, node_abraracourcix, node_asterix, "Dirige");
	graph_add_edge(graph, node_panoramix, node_obelix, "Interdit potion");
	graph_add_edge(graph, node_obelix, node_asterix, "Amitié");
	graph_add_edge(graph, node_obelix, node_petibonum, "Attaque");
	graph_add_edge(graph, node_abraracourcix, node_obelix, "Dirige");
	graph_add_edge(graph, node_asterix, node_petibonum, "Attaque");
	graph_add_edge(graph, node_panoramix, node_abraracourcix, "Conseil");
	graph_add_edge(graph, node_idefix, node_petibonum, "Attaque");
	graph_add_edge(graph, node_asterix, node_obelix, "Amitié");

	/* Testing edge fast access capability */
	for (i = 0; i < node_get_nb_src(graph, node_asterix); i++){
		edge = node_get_src_edge(graph, node_asterix, i);
		printf("%s -> %s: %s\n", (char*)node_get_data(graph, edge_get_src(graph, edge)), (char*)node_get_data(graph, edge_get_dst(graph, edge)), (char*)edge_get_data(graph, edge));
	}

	for (i = 0; i < node_get_nb_dst(graph, node_asterix); i++){
		edge = node_get_dst_edge(graph, node_asterix, i);
		printf("%s <- %s: %s\n", (char*)node_get_data(graph, edge_get_dst(graph, edge)), (char*)node_get_data(graph, edge_get_src(graph, edge)), (char*)edge_get_data(graph, edge));
	}


	/* Testing print dot capability*/
	if (graphPrintDot_print(graph, "asterix.dot", NULL)){
		printf("ERROR: in %s, unable to print graph in dot format\n", __func__);
	}



	graph_delete(graph);

	return 0;
}