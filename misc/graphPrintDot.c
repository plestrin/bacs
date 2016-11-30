#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "graphPrintDot.h"
#include "base.h"

#define GRAPHPRINTDOT_DEFAULT_FILE_NAME "graph.dot"

#define graphPrintDot_print_prologue(graph, file, arg) 																\
	if ((graph)->dotPrint_prologue != NULL){  																		\
		(graph)->dotPrint_prologue((file), (arg)); 																	\
	}

#define graphPrintDot_print_node_data(graph, node, file, arg) 														\
	if ((graph)->dotPrint_node_data != NULL){ 																		\
		(graph)->dotPrint_node_data(node_get_data(node), (file), (arg)); 											\
	}

#define graphPrintDot_print_edge_data(graph, edge, file, arg) 														\
	if ((graph)->dotPrint_edge_data != NULL){ 																		\
		(graph)->dotPrint_edge_data(edge_get_data(edge), (file), (arg)); 											\
	}

#define graphPrintDot_print_epilogue(graph, file, arg) 																\
	if ((graph)->dotPrint_epilogue != NULL){  																		\
		(graph)->dotPrint_epilogue((file), (arg)); 																	\
	}

int32_t graphPrintDot_print(struct graph* graph, const char* name, void* arg){
	FILE* 			file;
	struct node* 	node;
	struct edge* 	edge;

	if (graph != NULL){
		if (name == NULL){
			#ifdef VERBOSE
			log_warn_m("no file name specified, using default file name: \"%s\"", GRAPHPRINTDOT_DEFAULT_FILE_NAME);
			#endif
			file = fopen(GRAPHPRINTDOT_DEFAULT_FILE_NAME, "w");
		}
		else{
			file = fopen(name, "w");
		}

		if (file != NULL){
			fprintf(file, "digraph G {\n");

			graphPrintDot_print_prologue(graph, file, arg)

			for(node = graph_get_head_node(graph); node != NULL; node = node_get_next(node)){
				fprintf(file, "%u ", (uint32_t)node);
				graphPrintDot_print_node_data(graph, node, file, arg)
				fprintf(file, "\n");
			}

			for(node = graph_get_head_node(graph); node != NULL; node = node_get_next(node)){
				for (edge = node_get_head_edge_src(node); edge != NULL; edge = edge_get_next_src(edge)){
					fprintf(file, "%u -> %u ", (uint32_t)edge->src_node, (uint32_t)edge->dst_node);
					graphPrintDot_print_edge_data(graph, edge, file, arg)
					fprintf(file, "\n");
				}
			}

			graphPrintDot_print_epilogue(graph, file, arg)

			fprintf(file, "}\n");

			fclose(file);
		}
		else{
			log_err("unable to open file");
			return -1;
		}
	}

	return 0;
}

int32_t graphLayerPrintDot_print(struct graphLayer* graph_layer, const char* name, void* arg){
	FILE* 			file;
	struct node* 	node_layer;
	struct node* 	node_master;
	struct edge* 	edge;

	if (name == NULL){
		#ifdef VERBOSE
		log_warn_m("no file name specified, using default file name: \"%s\"", GRAPHPRINTDOT_DEFAULT_FILE_NAME);
		#endif
		file = fopen(GRAPHPRINTDOT_DEFAULT_FILE_NAME, "w");
	}
	else{
		file = fopen(name, "w");
	}

	if (file != NULL){
		fprintf(file, "digraph G {\n");

		graphPrintDot_print_prologue(&(graph_layer->layer), file, arg)

		for(node_layer = graph_get_head_node(&(graph_layer->layer)); node_layer != NULL; node_layer = node_get_next(node_layer)){
			node_master = node_layer->ptr;
			fprintf(file, "%u ", (uint32_t)node_master);
			graphPrintDot_print_node_data(graph_layer->master, node_master, file, arg)
			fprintf(file, "\n");
		}

		for(node_layer = graph_get_head_node(&(graph_layer->layer)); node_layer != NULL; node_layer = node_get_next(node_layer)){
			for (edge = node_get_head_edge_src(node_layer); edge != NULL; edge = edge_get_next_src(edge)){
				fprintf(file, "%u -> %u ", (uint32_t)edge->src_node->ptr, (uint32_t)edge->dst_node->ptr);
				graphPrintDot_print_edge_data(&(graph_layer->layer), edge, file, arg)
				fprintf(file, "\n");
			}
		}

		graphPrintDot_print_epilogue(&(graph_layer->layer), file, arg)

		fprintf(file, "}\n");

		fclose(file);
	}
	else{
		log_err("unable to open file");
		return -1;
	}

	return 0;
}
