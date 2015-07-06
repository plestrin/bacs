#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/mman.h>
#include <string.h>

#include "../mapFile.h"
#include "../graph.h"
#include "../graphReader.h"
#include "../graphPrintDot.h"
#include "../base.h"

#define NODE_MAX_LABEL_SIZE 32
#define GRAPH_NAME_SIZE		64

char graph_name[GRAPH_NAME_SIZE] = "default";

/* Example file: tintinGraph.txt */

#pragma GCC diagnostic ignored "-Wunused-parameter"
static void handle_graph_name(const char* str, size_t str_len, void* arg){
	memset(graph_name, 0, GRAPH_NAME_SIZE);
	memcpy(graph_name, str, min(GRAPH_NAME_SIZE - 1, str_len));
}

static void handle_graph_end(void* arg){
	struct graph* graph = (struct graph*)arg;
	char file_name[GRAPH_NAME_SIZE + 4];

	sprintf(file_name, "%s.dot", graph_name);
	if (graphPrintDot_print(graph, file_name, NULL)){
		log_err("unable to print graph in dot format");
	}
	else{
		log_info_m("the graph has been saved in dot format in: \"%s\"", file_name);
	}

	graph_clean(graph);
	graph_init(graph, NODE_MAX_LABEL_SIZE, 0);
}

static void handle_node_label(const char* str, size_t str_len, void* ptr, void* arg){
	struct node* node = (struct node*)ptr;

	memset(node->data, 0, NODE_MAX_LABEL_SIZE);
	memcpy(node->data, str, min(NODE_MAX_LABEL_SIZE - 1, str_len));
}

static void* handle_create_edge(void* src, void* dst, void* arg){
	return graph_add_edge_((struct graph*)arg, (struct node*)src, (struct node*)dst);
}

static void dotPrint_node(void* data, FILE* file, void* arg){
	fprintf(file, "[label=\"%s\"]", (char*)data);
}

int32_t main(int32_t argc, char** argv){
	void* 						file_map;
	size_t 						file_size;
	struct graph 				graph;
	struct graphReaderCallback 	callback = {
		.callback_graph_name 	= handle_graph_name,
		.callback_graph_label 	= NULL,
		.callback_graph_end 	= handle_graph_end,
		.callback_create_node 	= (void*(*)(void*))graph_add_node_,
		.callback_node_label 	= handle_node_label,
		.callback_node_io 		= NULL,
		.callback_create_edge 	= handle_create_edge,
		.callback_edge_label 	= NULL,
		.arg 					= (void*)&graph,
	};

	if (argc != 2){
		log_err("please specify one argument");
		return 0;
	}

	file_map = mapFile_map(argv[1], &file_size);
	if (file_map == NULL){
		log_err_m("unable to map file: \"%s\"", argv[1]);
	}
	else{
		graph_init(&graph, NODE_MAX_LABEL_SIZE, 0);
		graph_register_dotPrint_callback(&graph, NULL, dotPrint_node, NULL, NULL)
		graphReader_parse(file_map, file_size, &callback);
		munmap(file_map, file_size);
	}
}