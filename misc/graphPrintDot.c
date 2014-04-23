#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "graphPrintDot.h"

#define graphPrintDot_print_node_data(graph, node, file) 															\
	if ((graph)->dotPrint_node_data != NULL){ 																		\
		(graph)->dotPrint_node_data(&((node)->data), (file)); 														\
	}

#define graphPrintDot_print_edge_data(graph, edge, file) 															\
	if ((graph)->dotPrint_edge_data != NULL){ 																		\
		(graph)->dotPrint_edge_data(&((edge)->data), (file)); 														\
	}

int32_t graphPrintDot_print(struct graph* graph, const char* name){
	FILE* 			file;
	struct node* 	node;
	struct edge* 	edge;
	
	if (graph != NULL){
		if (name == NULL){
			#ifdef VERBOSE
			printf("WARNING: in %s, no file name specified, using default file name: \"%s\"\n", __func__, GRAPHPRINTDOT_DEFAULT_FILE_NAME);
			#endif
			file = fopen(GRAPHPRINTDOT_DEFAULT_FILE_NAME, "w");
		}
		else{
			file = fopen(name, "w");
		}

		if (file != NULL){
			fprintf(file, "digraph G {\n");
			
			/* Write every node at the beginning of file. Else strange dot bug (missing parts) */
			for(node = graph_get_head_node(graph); node != NULL; node = node_get_next(node)){
				fprintf(file, "%u ", (uint32_t)node);
				graphPrintDot_print_node_data(graph, node, file)
				fprintf(file, "\n");
			}

			for(node = graph_get_head_node(graph); node != NULL; node = node_get_next(node)){
				for (edge = node_get_head_edge_src(node); edge != NULL; edge = edge_get_next_src(edge)){
					fprintf(file, "%u -> %u ", (uint32_t)edge->src_node, (uint32_t)edge->dst_node);
					graphPrintDot_print_edge_data(graph, edge, file)
					fprintf(file, "\n");
				}
			}

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