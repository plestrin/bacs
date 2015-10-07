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

static void path_test(void){
	struct graph* 			graph;
	struct node* 			node1;
	struct node* 			node2;
	struct node* 			node3;
	struct node* 			node4;
	struct node* 			node5;
	struct node* 			node6;
	struct node* 			node7;
	struct node* 			node8;
	struct node* 			node9;
	struct node* 			node10;
	struct dijkstraPath 	path;

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

	#define search_print_path(node1_, node2_) 																							\
	{ 																																	\
		int32_t result; 																												\
																																		\
		result = dijkstra_min_path_(graph, &(node1_), 1, &(node2_), 1, &path, NULL); 													\
		if (result < 0){ 																												\
			log_err_m("error while searching a path between (%c, %c)", *(char*)node_get_data(node1_), *(char*)node_get_data(node2_)); 	\
		} 																																\
		else if (result){ 																												\
			log_info_m("there is not path between (%c, %c)", *(char*)node_get_data(node1_), *(char*)node_get_data(node2_)); 			\
		} 																																\
		else{ 																															\
			log_info_m("a path has been found between (%c, %c)", *(char*)node_get_data(node1_), *(char*)node_get_data(node2_)); 		\
			putchar('\t'); path_print(path.step_array); putchar('\n'); 																	\
		} 																																\
	}

	dijkstraPath_init(path)

	search_print_path(node10, node2)
	search_print_path(node9 , node1)
	search_print_path(node1 , node5)
	search_print_path(node2 , node5)
	search_print_path(node3 , node5)
	search_print_path(node1 , node3)
	search_print_path(node2 , node1)
	
	dijkstraPath_clean(path)

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
								printf("Dst [%u (%c) -> %u (%c)] = %u\n", i, *(char*)node_get_data(node_cursor2), INDEX_OF_NODE, *(char*)node_get_data(node_cursor1), dst_buffer[i]);
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
								printf("Dst [%u (%c) -> %u (%c)] = %u\n", INDEX_OF_NODE, *(char*)node_get_data(node_cursor1), i, *(char*)node_get_data(node_cursor2), dst_buffer[i]);
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

	path_test();

	return 0;
}