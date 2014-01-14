#include <stdlib.h>
#include <stdio.h>

#include "graphPrintDot.h"

static void graphPrintDot_print_node(struct graph* graph, FILE* file, void* arg);
static void graphPrintDot_print_edge(struct graph* graph, FILE* file, void* arg);

int32_t graphPrintDot_print(struct graph* graph, const char* file_name, void* arg){
	FILE* 	file;
	
	if (graph != NULL){
		if (file_name == NULL){
			#ifdef VERBOSE
			printf("WARNING: in %s, no file name specified, using default file name: \"%s\"\n", __func__, GRAPHPRINTDOT_DEFAULT_FILE_NAME);
			#endif
			file = fopen(GRAPHPRINTDOT_DEFAULT_FILE_NAME, "w");
		}
		else{
			file = fopen(file_name, "w");
		}

		if (file != NULL){
			fprintf(file, "digraph G {\n");
			graphPrintDot_print_node(graph, file, arg);
			graphPrintDot_print_edge(graph, file, arg);
			fprintf(file, "}\n");

			fclose(file);
		}
		else{
			printf("ERROR: in %s, unable to open file\n", __func__);
			return -1;
		}
	}

	return 0;
}

static void graphPrintDot_print_node(struct graph* graph, FILE* file, void* arg){
	uint32_t i;

	for (i = 0; i < graph_get_nb_node(graph); i++){
		fprintf(file, "%u ", i);
		if (graph->callback.node_print_dot != NULL){
			graph->callback.node_print_dot(node_get_data(graph, i), file, arg);
		}
		fprintf(file, "\n");
	}
}

static void graphPrintDot_print_edge(struct graph* graph, FILE* file, void* arg){
	uint32_t 				i;
	
	for (i = 0; i < graph_get_nb_edge(graph); i++){
		fprintf(file, "%u -> %u ", edge_get_src(graph, i), edge_get_dst(graph, i));
		if (graph->callback.edge_print_dot != NULL){
			graph->callback.edge_print_dot(edge_get_data(graph, i), file, arg);
		}
		fprintf(file, "\n");
	}
}