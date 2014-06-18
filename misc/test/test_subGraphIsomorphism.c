#include <stdlib.h>
#include <stdio.h>

#include "../graph.h"
#include "../subGraphIsomorphism.h"
#include "../graphPrintDot.h"

struct standaloneAssignement{
	uint32_t 		nb_node;
	struct node** 	assignement;
};

void dotPrint_node(void* data, FILE* file, void* arg){
	struct standaloneAssignement* 	sta_assignement;
	uint32_t 		i;
	char* 			color = "black";

	if (arg == NULL){
		fprintf(file, "[label=\"%c\"]", *(char*)data);
		/*fprintf(file, "[label=\"%c - %p\"]", *(char*)data, (void*)data);*/
	}
	else{
		sta_assignement = (struct standaloneAssignement*)arg;
		for (i = 0; i < sta_assignement->nb_node; i++){
			if (data == sta_assignement->assignement[i]->data){
				color= "red";
				break;
			}
		}
		fprintf(file, "[label=\"%c\",color=\"%s\"]", *(char*)data, color);
	}
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

struct graph* create_graph(){
	struct graph* 	graph;
	struct node* 	node_a1;
	struct node* 	node_a2;
	struct node* 	node_a3;
	struct node* 	node_a4;
	struct node* 	node_a5;
	struct node* 	node_a6;

	struct node* 	node_b1;
	struct node* 	node_b2;
	struct node* 	node_b3;
	struct node* 	node_b4;
	struct node* 	node_b5;
	struct node* 	node_b6;

	struct node* 	node_i1;
	struct node* 	node_i2;
	struct node* 	node_i3;
	struct node* 	node_i4;

	struct node* 	node_n1;
	struct node* 	node_n2;
	struct node* 	node_n3;

	struct node* 	node_o1;
	struct node* 	node_o2;
	struct node* 	node_o3;

	struct node* 	node_r1;
	struct node* 	node_r2;
	struct node* 	node_r3;

	graph = graph_create(1, 0);
	graph_register_dotPrint_callback(graph, NULL, dotPrint_node, dotPrint_edge, NULL)

	/* add nodes */
	node_a1 = graph_add_node(graph, "a");
	node_a2 = graph_add_node(graph, "a");
	node_a3 = graph_add_node(graph, "a");
	node_a4 = graph_add_node(graph, "a");
	node_a5 = graph_add_node(graph, "a");
	node_a6 = graph_add_node(graph, "a");
	node_b1 = graph_add_node(graph, "b");
	node_b2 = graph_add_node(graph, "b");
	node_b3 = graph_add_node(graph, "b");
	node_b4 = graph_add_node(graph, "b");
	node_b5 = graph_add_node(graph, "b");
	node_b6 = graph_add_node(graph, "b");
	node_i1 = graph_add_node(graph, "i");
	node_i2 = graph_add_node(graph, "i");
	node_i3 = graph_add_node(graph, "i");
	node_i4 = graph_add_node(graph, "i");
	node_n1 = graph_add_node(graph, "n");
	node_n2 = graph_add_node(graph, "n");
	node_n3 = graph_add_node(graph, "n");
	node_o1 = graph_add_node(graph, "o");
	node_o2 = graph_add_node(graph, "o");
	node_o3 = graph_add_node(graph, "o");
	node_r1 = graph_add_node(graph, "r");
	node_r2 = graph_add_node(graph, "r");
	node_r3 = graph_add_node(graph, "r");

	/* add edges */
	graph_add_edge_(graph, node_i1, node_a1);
	graph_add_edge_(graph, node_i2, node_a1);
	graph_add_edge_(graph, node_a1, node_o1);
	graph_add_edge_(graph, node_i1, node_n1);
	graph_add_edge_(graph, node_n1, node_a2);
	graph_add_edge_(graph, node_i3, node_a2);
	graph_add_edge_(graph, node_a2, node_o1);
	graph_add_edge_(graph, node_o1, node_b1);
	graph_add_edge_(graph, node_i4, node_b1);
	graph_add_edge_(graph, node_b1, node_r1);
	graph_add_edge_(graph, node_r1, node_b2);
	graph_add_edge_(graph, node_i1, node_b2);
	graph_add_edge_(graph, node_b2, node_n2);
	graph_add_edge_(graph, node_b2, node_a3);
	graph_add_edge_(graph, node_i1, node_a3);
	graph_add_edge_(graph, node_n2, node_a4);
	graph_add_edge_(graph, node_i2, node_a4);
	graph_add_edge_(graph, node_a3, node_o2);
	graph_add_edge_(graph, node_a4, node_o2);
	graph_add_edge_(graph, node_o2, node_b3);
	graph_add_edge_(graph, node_i3, node_b3);
	graph_add_edge_(graph, node_b3, node_r2);
	graph_add_edge_(graph, node_r2, node_b4);
	graph_add_edge_(graph, node_b2, node_b4);
	graph_add_edge_(graph, node_b4, node_n3);
	graph_add_edge_(graph, node_n3, node_a5);
	graph_add_edge_(graph, node_i1, node_a5);
	graph_add_edge_(graph, node_b4, node_a6);
	graph_add_edge_(graph, node_b2, node_a6);
	graph_add_edge_(graph, node_a6, node_o3);
	graph_add_edge_(graph, node_a5, node_o3);
	graph_add_edge_(graph, node_o3, node_b5);
	graph_add_edge_(graph, node_i2, node_b5);
	graph_add_edge_(graph, node_b5, node_r3);
	graph_add_edge_(graph, node_r3, node_b6);
	graph_add_edge_(graph, node_b4, node_b6);

	/* print graph */
	if (graphPrintDot_print(graph, "graph.dot", NULL)){
		printf("ERROR: in %s, unable to print graph to dot format\n", __func__);
	}

	return graph;
}

struct graph* create_subGraph(){
	struct graph* 	sub_graph;
	struct node* 	node_a1;
	struct node* 	node_a2;

	struct node* 	node_b1;
	struct node* 	node_b2;

	struct node* 	node_n1;

	struct node* 	node_o1;

	struct node* 	node_r1;

	sub_graph = graph_create(1, 0);
	graph_register_dotPrint_callback(sub_graph, NULL, dotPrint_node, dotPrint_edge, NULL)

	/* add nodes */
	node_a1 = graph_add_node(sub_graph, "a");
	node_a2 = graph_add_node(sub_graph, "a");
	node_b1 = graph_add_node(sub_graph, "b");
	node_b2 = graph_add_node(sub_graph, "b");
	node_n1 = graph_add_node(sub_graph, "n");
	node_o1 = graph_add_node(sub_graph, "o");
	node_r1 = graph_add_node(sub_graph, "r");

	/* add edges */
	graph_add_edge_(sub_graph, node_a1, node_o1);
	graph_add_edge_(sub_graph, node_n1, node_a2);
	graph_add_edge_(sub_graph, node_a2, node_o1);
	graph_add_edge_(sub_graph, node_o1, node_b1);
	graph_add_edge_(sub_graph, node_b1, node_r1);
	graph_add_edge_(sub_graph, node_r1, node_b2);

	/* print graph */
	if (graphPrintDot_print(sub_graph, "subGraph.dot", NULL)){
		printf("ERROR: in %s, unable to print graph to dot format\n", __func__);
	}

	return sub_graph;
}

int main(){
	struct graph* 					graph;
	struct graph* 					sub_graph;
	struct graphIsoHandle* 			graph_handle;
	struct subGraphIsoHandle* 		sub_graph_handle;
	struct array* 					assignement_array;
	uint32_t 						i;
	uint32_t 						j;
	struct node** 					assignement;
	struct standaloneAssignement 	sta_assignement;
	char 							iso_file_name[32];

	graph = create_graph();
	sub_graph = create_subGraph();

	graph_handle = graphIso_create_graph_handle(graph, node_get_label);
	if (graph_handle == NULL){
		printf("ERROR: in %s, unable to create graphHandle\n", __func__);
		return -1;
	}

	sub_graph_handle = graphIso_create_sub_graph_handle(sub_graph, node_get_label);
	if (sub_graph_handle == NULL){
		printf("ERROR: in %s, unable to create subGraphHandle\n", __func__);
		return -1;
	}
	
	assignement_array = graphIso_search(graph_handle, sub_graph_handle);
	if (assignement_array == NULL){
		printf("ERROR: in %s, the subgraph isomorphism routine\n", __func__);
	}
	else{
		printf("Found %u subgraph instance -> printing\n", array_get_length(assignement_array));
	
		for (i = 0; i < array_get_length(assignement_array); i++){
			assignement = (struct node**)array_get(assignement_array, i);
			printf("Assignement %u:\n", i + 1);

			for (j = 0; j < sub_graph->nb_node; j++){
				printf("\t %u - %p -> %p (%c)\n", j, (void*)sub_graph_handle->label_tab[j].node->data, (void*)assignement[j]->data, (char)(sub_graph_handle->label_tab[j].label));
			}

			sta_assignement.nb_node = sub_graph->nb_node;
			sta_assignement.assignement = assignement;

			snprintf(iso_file_name, 32, "iso_%u.dot", i + 1);
			if (graphPrintDot_print(graph, iso_file_name, &sta_assignement)){
				printf("ERROR: in %s, unable to print graph to dot format\n", __func__);
			}
		}

		array_delete(assignement_array);
	}

	graphIso_delete_graph_handle(graph_handle);
	graphIso_delete_subGraph_handle(sub_graph_handle);
	
	graph_delete(sub_graph);
	graph_delete(graph);
	
	return 0;
}