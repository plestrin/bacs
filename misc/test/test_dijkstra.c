#include <stdlib.h>
#include <stdio.h>

#include "../dijkstra.h"
#include "../graphPrintDot.h"
#include "../base.h"

#define INDEX_OF_NODE 3

static void dotPrint_node(void* data, FILE* file, void* arg){
	struct graph* 	graph = (struct graph*)arg;
	struct node* 	node_cursor;
	uint32_t 		i;

	if (graph == NULL){
		fprintf(file, "[label=\"%c\"]", *(char*)data);
	}
	else{
		for (node_cursor = graph_get_head_node(graph), i = 0; node_cursor != NULL; node_cursor = node_get_next(node_cursor), i++){
			if (i == INDEX_OF_NODE){
				break;
			}
		}

		if (node_cursor != NULL && node_cursor->data == data){
			fprintf(file, "[label=\"%c\",color=\"red\"]", *(char*)data);
		}
		else{
			fprintf(file, "[label=\"%c\"]", *(char*)data);
		}
	}
}

static struct graph* create_graph(){
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
	graph_register_dotPrint_callback(graph, NULL, dotPrint_node, NULL, NULL)

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
	if (graphPrintDot_print(graph, "dijkstra.dot", graph)){
		log_err("unable to print graph to dot format");
	}

	return graph;
}

static void zzPath_print(struct zzPath* path){
	uint32_t 		i;
	struct edge* 	edge;
	struct node*	desc;
	struct node* 	ancs;

	desc = zzPath_get_descendant(path);
	ancs = zzPath_get_ancestor(path);
	if (desc != NULL){
		printf("1 -> desc=%c (%u): ", *(char*)(desc->data), array_get_length(path->path_1_descendant) - 1);
		for (i = array_get_length(path->path_1_descendant); i > 1; i--){
			edge = *(struct edge**)array_get(path->path_1_descendant, i - 1);
			printf("%c ", *(char*)(edge_get_dst(edge)->data));
		}
		printf("\n");
	}
	if (ancs != NULL){
		printf("Ancs=%c -> 2 (%u): ", *(char*)(ancs->data), array_get_length(path->path_ancestor_2) - 1);
		for (i = 1; i < array_get_length(path->path_ancestor_2); i++){
			edge = *(struct edge**)array_get(path->path_ancestor_2, i);
			printf("%c ", *(char*)(edge_get_src(edge)->data));
		}
		printf("\n");
	}
	if (desc != NULL && ancs != NULL){
		printf("Ancs=%c -> desc=%c (%u): ", *(char*)(ancs->data), *(char*)(desc->data), array_get_length(path->path_ancestor_descendant) - 1);
		for (i = array_get_length(path->path_ancestor_descendant); i > 1; i--){
			edge = *(struct edge**)array_get(path->path_ancestor_descendant, i - 1);
			printf("%c ", *(char*)(edge_get_dst(edge)->data));
		}
	}
	else if (desc != NULL){
		printf("2 -> desc=%c (%u): ", *(char*)(desc->data), array_get_length(path->path_ancestor_descendant) - 1);
		for (i = array_get_length(path->path_ancestor_descendant); i > 1; i--){
			edge = *(struct edge**)array_get(path->path_ancestor_descendant, i - 1);
			printf("%c ", *(char*)(edge_get_dst(edge)->data));
		}
	}
	else if (ancs != NULL){
		printf("Ancs=%c -> 1 (%u): ", *(char*)(ancs->data), array_get_length(path->path_ancestor_descendant) - 1);
		for (i = array_get_length(path->path_ancestor_descendant); i > 1; i--){
			edge = *(struct edge**)array_get(path->path_ancestor_descendant, i - 1);
			printf("%c ", *(char*)(edge_get_dst(edge)->data));
		}
	}
	else if (array_get_length(path->path_1_descendant)){
		printf("1 -> 2 (%u): ", array_get_length(path->path_1_descendant) - 1);
		for (i = array_get_length(path->path_1_descendant); i > 1; i--){
			edge = *(struct edge**)array_get(path->path_1_descendant, i - 1);
			printf("%c ", *(char*)(edge_get_dst(edge)->data));
		}
	}
	else if (array_get_length(path->path_ancestor_descendant)){
		printf("2 -> 1 (%u): ", array_get_length(path->path_ancestor_descendant) - 1);
		for (i = array_get_length(path->path_ancestor_descendant); i > 1; i--){
			edge = *(struct edge**)array_get(path->path_ancestor_descendant, i - 1);
			printf("%c ", *(char*)(edge_get_dst(edge)->data));
		}
	}
	printf("\n");
}

static void zzPath_test(void){
	struct graph* 	graph;
	struct node* 	node1;
	struct node* 	node2;
	struct node* 	node3;
	struct node* 	node4;
	struct node* 	node5;
	struct node* 	node6;
	struct node* 	node7;
	struct node* 	node8;
	struct node* 	node9;
	struct zzPath 	path;
	int32_t 		return_code;

	graph = graph_create(1, 0);
	graph_register_dotPrint_callback(graph, NULL, dotPrint_node, NULL, NULL)

	/* add nodes */
	node1 = graph_add_node(graph, "a");
	node2 = graph_add_node(graph, "b");
	node3 = graph_add_node(graph, "c");
	node4 = graph_add_node(graph, "d");
	node5 = graph_add_node(graph, "e");
	node6 = graph_add_node(graph, "f");
	node7 = graph_add_node(graph, "g");
	node8 = graph_add_node(graph, "h");
	node9 = graph_add_node(graph, "i");

	/* add edges */
	graph_add_edge_(graph, node1, node6);
	graph_add_edge_(graph, node6, node7);
	graph_add_edge_(graph, node7, node2);
	graph_add_edge_(graph, node3, node4);
	graph_add_edge_(graph, node4, node2);
	graph_add_edge_(graph, node3, node8);
	graph_add_edge_(graph, node8, node5);
	graph_add_edge_(graph, node9, node3);
	graph_add_edge_(graph, node9, node5);

	/* print graph */
	if (graphPrintDot_print(graph, "zzPath.dot", NULL)){
		log_err("unable to print graph to dot format");
	}

	zzPath_init(&path);

	log_info_m("normal form {node_1=%c; node_2=%c}", *(char*)(node1->data), *(char*)(node5->data));
	return_code = dijkstra_min_zzPath(graph, &node1, 1, &node5, 1, &path, NULL);
	if (return_code < 0){
		log_err("error code");
	}
	else if (return_code == 0){
		zzPath_print(&path);
	}

	log_info_m("degenerate form 1 {node_1=%c; node_2=%c}", *(char*)(node2->data), *(char*)(node5->data));
	return_code = dijkstra_min_zzPath(graph, &node2, 1, &node5, 1, &path, NULL);
	if (return_code < 0){
		log_err("error code");
	}
	else if (return_code == 0){
		zzPath_print(&path);
	}

	log_info_m("degenerate form 2 {node_1=%c; node_2=%c}", *(char*)(node1->data), *(char*)(node3->data));
	return_code = dijkstra_min_zzPath(graph, &node1, 1, &node3, 1, &path, NULL);
	if (return_code < 0){
		log_err("error code");
	}
	else if (return_code == 0){
		zzPath_print(&path);
	}

	log_info_m("degenerate form 3 {node_1=%c; node_2=%c}", *(char*)(node2->data), *(char*)(node1->data));
	return_code = dijkstra_min_zzPath(graph, &node2, 1, &node1, 1, &path, NULL);
	if (return_code < 0){
		log_err("error code");
	}
	else if (return_code == 0){
		zzPath_print(&path);
	}

	log_info_m("degenerate form 4 {node_1=%c; node_2=%c}", *(char*)(node3->data), *(char*)(node5->data));
	return_code = dijkstra_min_zzPath(graph, &node3, 1, &node5, 1, &path, NULL);
	if (return_code < 0){
		log_err("error code");
	}
	else if (return_code == 0){
		zzPath_print(&path);
	}

	zzPath_clean(&path);
	graph_delete(graph);
}

#pragma GCC diagnostic ignored "-Wunused-parameter"
static uint32_t get_distance(void* arg){
	return 0;
}

static void backwardPath_print(struct array* array){
	uint32_t 		i;
	struct edge* 	edge;

	for (i = 1; i < array_get_length(array); i++){
		edge = *(struct edge**)array_get(array, i);
		printf("%c ", *(char*)(edge_get_src(edge)->data));
	}
}

static void forwardPath_print(struct array* array){
	uint32_t 		i;
	struct edge* 	edge;

	for (i = array_get_length(array); i > 1; i--){
		edge = *(struct edge**)array_get(array, i - 1);
		printf("%c ", *(char*)(edge_get_dst(edge)->data));
	}
}

static void common_test(void){
	struct graph* 	graph;
	struct node* 	node1;
	struct node* 	node2;
	struct node* 	node3;
	struct node* 	node4;
	struct node* 	node5;
	struct node* 	node6;
	struct node* 	node7;
	struct node* 	node8;
	struct node* 	node9;
	struct node* 	node10;
	struct array* 	path1 = NULL;
	struct array* 	path2 = NULL;
	struct node* 	result;

	graph = graph_create(1, 0);
	graph_register_dotPrint_callback(graph, NULL, dotPrint_node, NULL, NULL)

	/* add nodes */
	node1  = graph_add_node(graph, "a");
	node2  = graph_add_node(graph, "b");
	node3  = graph_add_node(graph, "c");
	node4  = graph_add_node(graph, "d");
	node5  = graph_add_node(graph, "e");
	node6  = graph_add_node(graph, "f");
	node7  = graph_add_node(graph, "g");
	node8  = graph_add_node(graph, "h");
	node9  = graph_add_node(graph, "i");
	node10 = graph_add_node(graph, "j");

	/* add edges */
	graph_add_edge_(graph, node1, node6);
	graph_add_edge_(graph, node6, node7);
	graph_add_edge_(graph, node7, node2);
	graph_add_edge_(graph, node3, node4);
	graph_add_edge_(graph, node4, node2);
	graph_add_edge_(graph, node3, node8);
	graph_add_edge_(graph, node8, node5);
	graph_add_edge_(graph, node9, node3);
	graph_add_edge_(graph, node9, node5);
	graph_add_edge_(graph, node5, node10);

	/* print graph */
	if (graphPrintDot_print(graph, "common.dot", NULL)){
		log_err("unable to print graph to dot format");
	}

	#define ANCESTOR_1 node10
	#define ANCESTOR_2 node2

	result = dijkstra_lowest_common_ancestor(graph, &ANCESTOR_1, 1, &ANCESTOR_2, 1, &path1, &path2, get_distance);
	if (result == NULL){
		log_err_m("unable to find common ancestor for (%c, %c)", *(char*)(ANCESTOR_1->data), *(char*)(ANCESTOR_2->data));
	}
	else{
		log_info_m("%c is lowest common ancestor for (%c, %c)", *(char*)(result->data), *(char*)(ANCESTOR_1->data), *(char*)(ANCESTOR_2->data));
		printf("\t-Path1: "); backwardPath_print(path1); printf("\n");
		printf("\t-Path2: "); backwardPath_print(path2); printf("\n");
	}

	#define DESCENDANT_1 node9
	#define DESCENDANT_2 node1

	result = dijkstra_highest_common_descendant(graph, &DESCENDANT_1, 1, &DESCENDANT_2, 1, &path1, &path2, get_distance);
	if (result == NULL){
		log_err_m("unable to find common descendant for (%c, %c)", *(char*)(DESCENDANT_1->data), *(char*)(DESCENDANT_2->data));
	}
	else{
		log_info_m("%c is highest common descendant for (%c, %c)", *(char*)(result->data), *(char*)(DESCENDANT_1->data), *(char*)(DESCENDANT_2->data));
		printf("\t-Path1: "); forwardPath_print(path1); printf("\n");
		printf("\t-Path2: "); forwardPath_print(path2); printf("\n");
	}
	
	if (path1 != NULL){
		array_delete(path1);
	}
	if (path2 != NULL){
		array_delete(path2);
	}

	graph_delete(graph);
}

int main(){
	struct graph* 	graph;
	struct node* 	node_cursor1;
	struct node* 	node_cursor2;
	uint32_t 		i;
	uint32_t* 		dst_buffer;

	graph = create_graph();
	if (graph != NULL){
		dst_buffer = (uint32_t*)malloc(sizeof(uint32_t) * graph->nb_node);
		if (dst_buffer != NULL){
			for (node_cursor1 = graph_get_head_node(graph), i = 0; node_cursor1 != NULL; node_cursor1 = node_get_next(node_cursor1), i++){
				if (i == INDEX_OF_NODE){
					printf("Dijkstra dst to INDEX:\n");
					if (dijkstra_dst_to(graph, node_cursor1, dst_buffer)){
						log_err("unable to compute graph dst (Dijkstra)");
					}
					else{
						for (node_cursor2 = graph_get_head_node(graph), i = 0; node_cursor2 != NULL && i < graph->nb_node; node_cursor2 = node_get_next(node_cursor2), i++){
							if (dst_buffer[i] != DIJKSTRA_INVALID_DST){
								printf("Dst [%u (%c) -> %u (%c)] = %u\n", i, *(char*)node_cursor2->data, INDEX_OF_NODE, *(char*)node_cursor1->data, dst_buffer[i]);
							}
						}
					}

					printf("Dijkstra dst from INDEX:\n");
					if (dijkstra_dst_from(graph, node_cursor1, dst_buffer)){
						log_err("unable to compute graph dst (Dijkstra)");
					}
					else{
						for (node_cursor2 = graph_get_head_node(graph), i = 0; node_cursor2 != NULL && i < graph->nb_node; node_cursor2 = node_get_next(node_cursor2), i++){
							if (dst_buffer[i] != DIJKSTRA_INVALID_DST){
								printf("Dst [%u (%c) -> %u (%c)] = %u\n", INDEX_OF_NODE, *(char*)node_cursor1->data, i, *(char*)node_cursor2->data, dst_buffer[i]);
							}
						}
					}
					break;
				}
			}
			free(dst_buffer);
		}
		else{
			log_err("unable to allocate memory");
		}

		graph_delete(graph);
	}
	else{
		log_err("unable to create graph");
	}

	zzPath_test();

	common_test();

	return 0;
}