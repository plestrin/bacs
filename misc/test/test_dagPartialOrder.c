#include <stdlib.h>
#include <stdio.h>

#include "../dagPartialOrder.h"
#include "../graphPrintDot.h"
#include "../base.h"

#pragma GCC diagnostic ignored "-Wunused-parameter"
void dotPrint_node(void* data, FILE* file, void* arg){
	fprintf(file, "[label=\"%u\"]", *(uint32_t*)data);
}

#pragma GCC diagnostic ignored "-Wunused-parameter"
void dotPrint_edge(void* data, FILE* file, void* arg){
}

struct graph* create_graph(){
	struct graph* 	graph;
	struct node* 	node_10;
	struct node* 	node_21;
	struct node* 	node_22;
	struct node* 	node_30;
	struct node* 	node_40;

	uint32_t value_10 = 1;
	uint32_t value_21 = 21;
	uint32_t value_22 = 22;
	uint32_t value_30 = 3;
	uint32_t value_40 = 4;

	graph = graph_create(sizeof(uint32_t), 0);
	if (graph != NULL){
		graph_register_dotPrint_callback(graph, NULL, dotPrint_node, dotPrint_edge, NULL)

		node_21 = graph_add_node(graph, &value_21);
		node_10 = graph_add_node(graph, &value_10);
		node_40 = graph_add_node(graph, &value_40);
		node_22 = graph_add_node(graph, &value_22);
		node_30 = graph_add_node(graph, &value_30);

		graph_add_edge_(graph, node_30, node_40);
		graph_add_edge_(graph, node_10, node_40);
		graph_add_edge_(graph, node_21, node_30);
		graph_add_edge_(graph, node_22, node_30);
		graph_add_edge_(graph, node_10, node_22);
		graph_add_edge_(graph, node_10, node_21);

		if (graphPrintDot_print(graph, "dag.dot", graph)){
			log_err("unable to print graph to dot format");
		}
	}

	return graph;
}

int main(){
	struct graph* 	graph;
	struct node* 	node_cursor;

	graph = create_graph();
	if (graph != NULL){
		printf("Printing node in intial order:\n");
		for(node_cursor = graph_get_head_node(graph); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
			printf("\t%u\n", *(uint32_t*)&(node_cursor->data));
		}

		if (dagPartialOrder_sort_dst_src(graph)){
			log_err("unable to sort DAG");
		}

		printf("Printing node in partial order:\n");
		for(node_cursor = graph_get_head_node(graph); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
			printf("\t%u\n", *(uint32_t*)&(node_cursor->data));
		}

		graph_delete(graph);
	}
	else{
		log_err("unable to create graph");
	}

	return 0;
}