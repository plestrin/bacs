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

		if (file == NULL){
			log_err("unable to open file");
			return -1;
		}

		fputs("digraph G {\n", file);

		graphPrintDot_print_prologue(graph, file, arg)

		for (node = graph_get_head_node(graph); node != NULL; node = node_get_next(node)){
			fprintf(file, "%u ", (uint32_t)node);
			graphPrintDot_print_node_data(graph, node, file, arg)
			fputs("\n", file);
		}

		for (node = graph_get_head_node(graph); node != NULL; node = node_get_next(node)){
			for (edge = node_get_head_edge_src(node); edge != NULL; edge = edge_get_next_src(edge)){
				fprintf(file, "%u -> %u ", (uint32_t)edge->src_node, (uint32_t)edge->dst_node);
				graphPrintDot_print_edge_data(graph, edge, file, arg)
				fputs("\n", file);
			}
		}

		graphPrintDot_print_epilogue(graph, file, arg)

		fputs("}\n", file);

		fclose(file);
	}

	return 0;
}

#define ugraphPrintDot_print_node_data(ugraph, unode, file, arg) 													\
	if ((ugraph)->dotPrint_node_data != NULL){ 																		\
		(ugraph)->dotPrint_node_data(unode_get_data(unode), (file), (arg)); 										\
	}

#define ugraphPrintDot_print_edge_data(ugraph, uedge, file, arg) 													\
	if ((ugraph)->dotPrint_edge_data != NULL){ 																		\
		(ugraph)->dotPrint_edge_data(uedge_get_data(uedge), (file), (arg)); 										\
	}

int32_t ugraphPrintDot_print(struct ugraph* ugraph, const char* name, void* arg){
	FILE* 			file;
	struct unode* 	unode;
	struct uedge* 	uedge;

	if (ugraph != NULL){
		if (name == NULL){
			#ifdef VERBOSE
			log_warn_m("no file name specified, using default file name: \"%s\"", GRAPHPRINTDOT_DEFAULT_FILE_NAME);
			#endif
			file = fopen(GRAPHPRINTDOT_DEFAULT_FILE_NAME, "w");
		}
		else{
			file = fopen(name, "w");
		}

		if (file == NULL){
			log_err("unable to open file");
			return -1;
		}

		fputs("graph G {\n", file);

		for (unode = ugraph_get_head_node(ugraph); unode != NULL; unode = unode_get_next(unode)){
			fprintf(file, "%u\n", (uint32_t)unode);
			ugraphPrintDot_print_node_data(ugraph, unode, file, arg)
			fputs("\n", file);
		}

		for (unode = ugraph_get_head_node(ugraph); unode != NULL; unode = unode_get_next(unode)){
			for (uedge = unode_get_head_edge(unode); uedge != NULL; uedge = uedge_get_next(uedge)){
				if ((void*)uedge->container == (void*)uedge){
					fprintf(file, "%u -- %u ", (uint32_t)unode, (uint32_t)uedge_get_endp(uedge));
					ugraphPrintDot_print_edge_data(ugraph, uedge, file, arg)
					fputs("\n", file);
				}
			}
		}

		fputs("}\n", file);

		fclose(file);
	}

	return 0;
}
