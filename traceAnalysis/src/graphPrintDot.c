#include <stdlib.h>
#include <stdio.h>

#include "graphPrintDot.h"

static void graphPrintDot_print_node(struct graph* graph, FILE* file);
static void graphPrintDot_print_edge(struct graph* graph, FILE* file);

int graphPrintDot_print(struct graph* graph, const char* name){
	int 	result = -1;
	FILE* 	file;

	
	if (graph != NULL){
		if (name == NULL){
			file = fopen(GRAPHPRINTDOT_DEFAULT_FILE_NAME, "w");
		}
		else{
			file = fopen(name, "w");
		}

		if (file != NULL){
			fprintf(file, "digraph G {\n");
			graphPrintDot_print_node(graph, file);
			graphPrintDot_print_edge(graph, file);
			fprintf(file, "}\n");

		}
		else{

			printf("ERROR: in %s, unable to open file\n", __func__);
		}
	}

	return result;
}

static void graphPrintDot_print_node(struct graph* graph, FILE* file){

}

static void graphPrintDot_print_edge(struct graph* graph, FILE* file){
	int 			e;
	unsigned long 	id_src;
	unsigned long 	id_dst;

	for (e = 0; e < graph->nb_edge; e++){
		
	}
}