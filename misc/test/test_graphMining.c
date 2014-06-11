#include <stdlib.h>
#include <stdio.h>

#include "../graph.h"
#include "../graphMining.h"
#include "../graphPrintDot.h"

#define NODE_DESCRIPTION_LENGTH 32
#define EDGE_DESCRIPTION_LENGTH 16

#pragma GCC diagnostic ignored "-Wunused-parameter"
void dotPrint_node(void* data, FILE* file, void* arg){
	fprintf(file, "[label=\"%c\"]", *(char*)data);
}

#pragma GCC diagnostic ignored "-Wunused-parameter"
void dotPrint_edge(void* data, FILE* file, void* arg){
}

uint32_t node_get_label(struct node* node){
	switch(*(char*)node->data){
		case 'a' : {return 97 ;}
		case 'b' : {return 98 ;}
		case 'i' : {return 105;}
		case 'n' : {return 110;}
		case 'o' : {return 111;}
		case 'r' : {return 114;}
		default : {
			printf("ERROR: in %s, unknown label: %c\n", __func__, *(char*)node->data);
			return 0;
		}
	}
}

int main(){
	struct graph* 	graph;
	struct node* 	node_a1;
	struct node* 	node_a2;
	struct node* 	node_a3;
	struct node* 	node_a4;

	struct node* 	node_b1;
	struct node* 	node_b2;

	struct node* 	node_i1;
	struct node* 	node_i2;
	struct node* 	node_i3;
	struct node* 	node_i4;

	struct node* 	node_n1;
	struct node* 	node_n2;

	struct node* 	node_o1;
	struct node* 	node_o2;

	struct node* 	node_r1;

	graph = graph_create(1, 0);
	graph_register_dotPrint_callback(graph, NULL, dotPrint_node, dotPrint_edge, NULL)

	/* add nodes */
	node_a1 = graph_add_node(graph, "a");
	if (node_a1 == NULL){
		printf("ERROR: in %s, unable to add node to graph\n", __func__);
	}
	node_a2 = graph_add_node(graph, "a");
	if (node_a2 == NULL){
		printf("ERROR: in %s, unable to add node to graph\n", __func__);
	}
	node_a3 = graph_add_node(graph, "a");
	if (node_a3 == NULL){
		printf("ERROR: in %s, unable to add node to graph\n", __func__);
	}
	node_a4 = graph_add_node(graph, "a");
	if (node_a4 == NULL){
		printf("ERROR: in %s, unable to add node to graph\n", __func__);
	}
	node_b1 = graph_add_node(graph, "b");
	if (node_b1 == NULL){
		printf("ERROR: in %s, unable to add node to graph\n", __func__);
	}
	node_b2 = graph_add_node(graph, "b");
	if (node_b2 == NULL){
		printf("ERROR: in %s, unable to add node to graph\n", __func__);
	}
	node_i1 = graph_add_node(graph, "i");
	if (node_i1 == NULL){
		printf("ERROR: in %s, unable to add node to graph\n", __func__);
	}
	node_i2 = graph_add_node(graph, "i");
	if (node_i2 == NULL){
		printf("ERROR: in %s, unable to add node to graph\n", __func__);
	}
	node_i3 = graph_add_node(graph, "i");
	if (node_i3 == NULL){
		printf("ERROR: in %s, unable to add node to graph\n", __func__);
	}
	node_i4 = graph_add_node(graph, "i");
	if (node_i4 == NULL){
		printf("ERROR: in %s, unable to add node to graph\n", __func__);
	}
	node_n1 = graph_add_node(graph, "n");
	if (node_n1 == NULL){
		printf("ERROR: in %s, unable to add node to graph\n", __func__);
	}
	node_n2 = graph_add_node(graph, "n");
	if (node_n2 == NULL){
		printf("ERROR: in %s, unable to add node to graph\n", __func__);
	}
	node_o1 = graph_add_node(graph, "o");
	if (node_o1 == NULL){
		printf("ERROR: in %s, unable to add node to graph\n", __func__);
	}
	node_o2 = graph_add_node(graph, "o");
	if (node_o2 == NULL){
		printf("ERROR: in %s, unable to add node to graph\n", __func__);
	}
	node_r1 = graph_add_node(graph, "r");
	if (node_r1 == NULL){
		printf("ERROR: in %s, unable to add node to graph\n", __func__);
	}

	/* add edges */
	if (graph_add_edge_(graph, node_i1, node_a1) == NULL){
		printf("ERROR: in %s, unable to add edge to graph\n", __func__);
	}
	if (graph_add_edge_(graph, node_i2, node_a1) == NULL){
		printf("ERROR: in %s, unable to add edge to graph\n", __func__);
	}
	if (graph_add_edge_(graph, node_a1, node_o1) == NULL){
		printf("ERROR: in %s, unable to add edge to graph\n", __func__);
	}
	if (graph_add_edge_(graph, node_i1, node_n1) == NULL){
		printf("ERROR: in %s, unable to add edge to graph\n", __func__);
	}
	if (graph_add_edge_(graph, node_n1, node_a2) == NULL){
		printf("ERROR: in %s, unable to add edge to graph\n", __func__);
	}
	if (graph_add_edge_(graph, node_i3, node_a2) == NULL){
		printf("ERROR: in %s, unable to add edge to graph\n", __func__);
	}
	if (graph_add_edge_(graph, node_a2, node_o1) == NULL){
		printf("ERROR: in %s, unable to add edge to graph\n", __func__);
	}
	if (graph_add_edge_(graph, node_o1, node_b1) == NULL){
		printf("ERROR: in %s, unable to add edge to graph\n", __func__);
	}
	if (graph_add_edge_(graph, node_i4, node_b1) == NULL){
		printf("ERROR: in %s, unable to add edge to graph\n", __func__);
	}
	if (graph_add_edge_(graph, node_b1, node_r1) == NULL){
		printf("ERROR: in %s, unable to add edge to graph\n", __func__);
	}
	if (graph_add_edge_(graph, node_r1, node_b2) == NULL){
		printf("ERROR: in %s, unable to add edge to graph\n", __func__);
	}
	if (graph_add_edge_(graph, node_i1, node_b2) == NULL){
		printf("ERROR: in %s, unable to add edge to graph\n", __func__);
	}
	if (graph_add_edge_(graph, node_b2, node_n2) == NULL){
		printf("ERROR: in %s, unable to add edge to graph\n", __func__);
	}
	if (graph_add_edge_(graph, node_b2, node_a3) == NULL){
		printf("ERROR: in %s, unable to add edge to graph\n", __func__);
	}
	if (graph_add_edge_(graph, node_i1, node_a3) == NULL){
		printf("ERROR: in %s, unable to add edge to graph\n", __func__);
	}
	if (graph_add_edge_(graph, node_n2, node_a4) == NULL){
		printf("ERROR: in %s, unable to add edge to graph\n", __func__);
	}
	if (graph_add_edge_(graph, node_i2, node_a4) == NULL){
		printf("ERROR: in %s, unable to add edge to graph\n", __func__);
	}
	if (graph_add_edge_(graph, node_a3, node_o2) == NULL){
		printf("ERROR: in %s, unable to add edge to graph\n", __func__);
	}
	if (graph_add_edge_(graph, node_a4, node_o2) == NULL){
		printf("ERROR: in %s, unable to add edge to graph\n", __func__);
	}

	/* print graph */
	if (graphPrintDot_print(graph, "test.dot", NULL)){
		printf("ERROR: in %s, unable to print graph to dot format\n", __func__);
	}

	/* mine graph */
	if (graphMining_mine(graph, node_get_label)){
		printf("ERROR: in %s, unable to mine graph\n", __func__);
	}

	graph_delete(graph);
	
	return 0;
}