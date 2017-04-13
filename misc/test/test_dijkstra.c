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

		if (node_cursor != NULL && node_get_data(node_cursor) == data){
			fprintf(file, "[label=\"%c\",color=\"red\"]", *(char*)data);
		}
		else{
			fprintf(file, "[label=\"%c\"]", *(char*)data);
		}
	}
}

static void path_print(struct array* array){
	uint32_t 					i;
	struct dijkstraPathStep* 	step;

	for (i = array_get_length(array); i > 0; i--){
		step = (struct dijkstraPathStep*)array_get(array, i - 1);
		switch(step->dir){
			case PATH_SRC_TO_DST : {
				if (i == array_get_length(array)){
					printf("%c -> %c", *(char*)node_get_data(edge_get_src(step->edge)), *(char*)node_get_data(edge_get_dst(step->edge)));
				}
				else{
					printf(" -> %c", *(char*)node_get_data(edge_get_dst(step->edge)));
				}
				break;
			}
			case PATH_DST_TO_SRC : {
				if (i == array_get_length(array)){
					printf("%c <- %c", *(char*)node_get_data(edge_get_dst(step->edge)), *(char*)node_get_data(edge_get_src(step->edge)));
				}
				else{
					printf(" <- %c", *(char*)node_get_data(edge_get_src(step->edge)));
				}
				break;
			}
			case PATH_INVALID : {
				break;
			}
		}
	}
}

int main(void){
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
	struct array 	path_array;

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
	if (graphPrintDot_print(graph, "path.dot", NULL)){
		log_err("unable to print graph to dot format");
	}

	#define search_print_path(node1_, node2_) 																											\
	{ 																																					\
		uint32_t 				i_; 																													\
		struct dijkstraPath* 	path_; 																													\
																																						\
		if (dijkstra_min_path(graph, &(node1_), 1, &(node2_), 1, &path_array, NULL)){ 																	\
			log_err_m("error while searching a path between (%c, %c)", *(char*)node_get_data(node1_), *(char*)node_get_data(node2_)); 					\
		} 																																				\
		else if (!array_get_length(&path_array)){ 																										\
			log_info_m("there is not path between (%c, %c)", *(char*)node_get_data(node1_), *(char*)node_get_data(node2_)); 							\
		} 																																				\
		else{ 																																			\
			log_info_m("%u path(s) between (%c, %c)", array_get_length(&path_array), *(char*)node_get_data(node1_), *(char*)node_get_data(node2_)); 	\
			for (i_ = 0; i_ < array_get_length(&path_array); i_ ++){ 																					\
				path_ = (struct dijkstraPath*)array_get(&path_array, i_); 																				\
				putchar('\t'); path_print(path_->step_array); putchar('\n'); 																			\
				array_delete(path_->step_array); 																										\
			} 																																			\
			array_empty(&path_array); 																													\
		} 																																				\
	}

	if (array_init(&path_array, sizeof(struct dijkstraPath))){
		log_err("unable to init array");
		return EXIT_FAILURE;
	}

	search_print_path(node10, node2)
	search_print_path(node9 , node1)
	search_print_path(node1 , node5)
	search_print_path(node2 , node5)
	search_print_path(node3 , node5)
	search_print_path(node1 , node3)
	search_print_path(node2 , node1)

	array_clean(&path_array);

	graph_delete(graph);

	return EXIT_SUCCESS;
}
