#include <stdlib.h>
#include <stdio.h>

#include "graphPrintDot.h"

static void graphPrintDot_print_node(struct graph* graph, FILE* file);
static void graphPrintDot_print_edge(struct graph* graph, FILE* file);

int graphPrintDot_compare_exit_point(const void* data1, const void* data2);

int graphPrintDot_print(struct graph* graph, const char* name){
	int 	result = -1;
	FILE* 	file;

	
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
			graphPrintDot_print_node(graph, file);
			graphPrintDot_print_edge(graph, file);
			fprintf(file, "}\n");

			fclose(file);
			result = 0;
		}
		else{

			printf("ERROR: in %s, unable to open file\n", __func__);
		}
	}

	return result;
}

static void graphPrintDot_print_node(struct graph* graph, FILE* file){
	int n;

	for (n = 0; n < graph->nb_node; n++){
		fprintf(file, "%lu ", graph->nodes[n].entry_point);
		graphNode_print_dot(graph->nodes + n, &(graph->callback_node), file);
		fprintf(file, "\n");
	}
}

static void graphPrintDot_print_edge(struct graph* graph, FILE* file){
	int 					e;
	unsigned long 			src_id;
	unsigned long 			dst_id;
	struct graphMapping* 	mapping;
	struct graphNode 		node;
	struct graphNode* 		node_pointer = &node;
	struct graphNode** 		src_node;

	mapping = graph_create_mapping(graph, GRAPHMAPPING_NODE, graphPrintDot_compare_exit_point);
	if (mapping != NULL){
		for (e = 0; e < graph->nb_edge; e++){
			dst_id = graph->edges[e].dst_id;
			node.exit_point = graph->edges[e].src_id;

			src_node = (struct graphNode**)graph_search(graph, mapping, &node_pointer);
			if (src_node != NULL){
				src_id = (*src_node)->entry_point;

				fprintf(file, "%lu -> %lu [label=\"%d\"]\n", src_id, dst_id, graph->edges[e].nb_execution);
			}
			else{
				printf("ERROR: in %s, graph serach return NULL pointer\n", __func__);
				break;
			}

		}
		graph_delete_mapping(graph, mapping);
	}
	else{
		printf("ERROR: in %s, unable to create mapping\n", __func__);
	}
}

int graphPrintDot_compare_exit_point(const void* data1, const void* data2){
	struct graphNode* node1 = *(struct graphNode**)data1;
	struct graphNode* node2 = *(struct graphNode**)data2;

	return node1->exit_point - node2->exit_point;
}