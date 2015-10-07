#include <stdlib.h>
#include <stdio.h>

#include "dijkstra.h"
#include "base.h"

int32_t dijkstra_dst_to(struct graph* graph, struct node* node, uint32_t* dst_buffer){
	struct dijkstraInternal{
		uint32_t 					dst;
		struct node*				node;
		struct dijkstraInternal* 	next;
	};

	struct dijkstraInternal* 	internals;
	struct dijkstraInternal* 	curr_orbital = NULL;
	struct dijkstraInternal* 	next_orbital;
	uint32_t 					i;
	struct node* 				node_cursor;
	struct edge*				edge_cursor;
	struct dijkstraInternal* 	internal_cursor;

	internals = (struct dijkstraInternal*)malloc(sizeof(struct dijkstraInternal) * graph->nb_node);
	if (internals == NULL){
		log_err("unable to allocate memory");
		return -1;
	}

	for (node_cursor = graph_get_head_node(graph), i = 0; i < graph->nb_node && node_cursor != NULL; node_cursor = node_get_next(node_cursor), i++){
		node_cursor->ptr = internals + i;

		if (node_cursor != node){
			internals[i].dst 	= DIJKSTRA_INVALID_DST;
			internals[i].node 	= node_cursor;
			internals[i].next 	= NULL;
		}
		else{
			internals[i].dst 	= 0;
			internals[i].node 	= node_cursor;
			internals[i].next 	= NULL;

			curr_orbital 		= internals + i;
		}
	}

	if (curr_orbital == NULL){
		log_err("unable to find the given node");
		free(internals);
		return -1;
	}

	for(i = 0; curr_orbital != NULL; curr_orbital = next_orbital, i++){
		for(next_orbital = NULL; curr_orbital != NULL; curr_orbital = curr_orbital->next){
			for (edge_cursor = node_get_head_edge_dst(curr_orbital->node); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
				node_cursor = edge_get_src(edge_cursor);
				internal_cursor = (struct dijkstraInternal*)node_cursor->ptr;

				if (internal_cursor->dst > i + 1){
					internal_cursor->dst = i + 1;
					internal_cursor->next = next_orbital;
					next_orbital = internal_cursor;
				}
			}
		}
	}

	for (node_cursor = graph_get_head_node(graph), i = 0; i < graph->nb_node && node_cursor != NULL; node_cursor = node_get_next(node_cursor), i++){
		internal_cursor = (struct dijkstraInternal*)node_cursor->ptr;
		dst_buffer[i] = internal_cursor->dst;
	}

	free(internals);

	return 0;
}

int32_t dijkstra_dst_from(struct graph* graph, struct node* node, uint32_t* dst_buffer){
	struct dijkstraInternal{
		uint32_t 					dst;
		struct node*				node;
		struct dijkstraInternal* 	next;
	};

	struct dijkstraInternal* 	internals;
	struct dijkstraInternal* 	curr_orbital = NULL;
	struct dijkstraInternal* 	next_orbital;
	uint32_t 					i;
	struct node* 				node_cursor;
	struct edge*				edge_cursor;
	struct dijkstraInternal* 	internal_cursor;

	internals = (struct dijkstraInternal*)malloc(sizeof(struct dijkstraInternal) * graph->nb_node);
	if (internals == NULL){
		log_err("unable to allocate memory");
		return -1;
	}

	for (node_cursor = graph_get_head_node(graph), i = 0; i < graph->nb_node && node_cursor != NULL; node_cursor = node_get_next(node_cursor), i++){
		node_cursor->ptr = internals + i;

		if (node_cursor != node){
			internals[i].dst 	= DIJKSTRA_INVALID_DST;
			internals[i].node 	= node_cursor;
			internals[i].next 	= NULL;
		}
		else{
			internals[i].dst 	= 0;
			internals[i].node 	= node_cursor;
			internals[i].next 	= NULL;

			curr_orbital 		= internals + i;
		}
	}

	if (curr_orbital == NULL){
		log_err("unable to find the given node");
		free(internals);
		return -1;
	}

	for(i = 0; curr_orbital != NULL; curr_orbital = next_orbital, i++){
		for(next_orbital = NULL; curr_orbital != NULL; curr_orbital = curr_orbital->next){
			for (edge_cursor = node_get_head_edge_src(curr_orbital->node); edge_cursor != NULL; edge_cursor = edge_get_next_src(edge_cursor)){
				node_cursor = edge_get_dst(edge_cursor);
				internal_cursor = (struct dijkstraInternal*)node_cursor->ptr;

				if (internal_cursor->dst > i + 1){
					internal_cursor->dst = i + 1;
					internal_cursor->next = next_orbital;
					next_orbital = internal_cursor;
				}
			}
		}
	}

	for (node_cursor = graph_get_head_node(graph), i = 0; i < graph->nb_node && node_cursor != NULL; node_cursor = node_get_next(node_cursor), i++){
		internal_cursor = (struct dijkstraInternal*)node_cursor->ptr;
		dst_buffer[i] = internal_cursor->dst;
	}

	free(internals);

	return 0;
}

int32_t dijkstra_dst(struct graph* graph, struct node* node, uint32_t* dst_buffer){
	struct dijkstraInternal{
		uint32_t 					dst;
		struct node*				node;
		struct dijkstraInternal* 	next;
	};

	struct dijkstraInternal* 	internals;
	struct dijkstraInternal* 	curr_orbital = NULL;
	struct dijkstraInternal* 	next_orbital;
	uint32_t 					i;
	struct node* 				node_cursor;
	struct edge*				edge_cursor;
	struct dijkstraInternal* 	internal_cursor;

	internals = (struct dijkstraInternal*)malloc(sizeof(struct dijkstraInternal) * graph->nb_node);
	if (internals == NULL){
		log_err("unable to allocate memory");
		return -1;
	}

	for (node_cursor = graph_get_head_node(graph), i = 0; i < graph->nb_node && node_cursor != NULL; node_cursor = node_get_next(node_cursor), i++){
		node_cursor->ptr = internals + i;

		if (node_cursor != node){
			internals[i].dst 	= DIJKSTRA_INVALID_DST;
			internals[i].node 	= node_cursor;
			internals[i].next 	= NULL;
		}
		else{
			internals[i].dst 	= 0;
			internals[i].node 	= node_cursor;
			internals[i].next 	= NULL;

			curr_orbital 		= internals + i;
		}
	}

	if (curr_orbital == NULL){
		log_err("unable to find the given node");
		free(internals);
		return -1;
	}

	for(i = 0; curr_orbital != NULL; curr_orbital = next_orbital, i++){
		for(next_orbital = NULL; curr_orbital != NULL; curr_orbital = curr_orbital->next){
			for (edge_cursor = node_get_head_edge_src(curr_orbital->node); edge_cursor != NULL; edge_cursor = edge_get_next_src(edge_cursor)){
				node_cursor = edge_get_dst(edge_cursor);
				internal_cursor = (struct dijkstraInternal*)node_cursor->ptr;

				if (internal_cursor->dst > i + 1){
					internal_cursor->dst = i + 1;
					internal_cursor->next = next_orbital;
					next_orbital = internal_cursor;
				}
			}

			for (edge_cursor = node_get_head_edge_dst(curr_orbital->node); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
				node_cursor = edge_get_src(edge_cursor);
				internal_cursor = (struct dijkstraInternal*)node_cursor->ptr;

				if (internal_cursor->dst > i + 1){
					internal_cursor->dst = i + 1;
					internal_cursor->next = next_orbital;
					next_orbital = internal_cursor;
				}
			}
		}
	}

	for (node_cursor = graph_get_head_node(graph), i = 0; i < graph->nb_node && node_cursor != NULL; node_cursor = node_get_next(node_cursor), i++){
		internal_cursor = (struct dijkstraInternal*)node_cursor->ptr;
		dst_buffer[i] = internal_cursor->dst;
	}


	free(internals);

	return 0;
}

int32_t dijkstra_min_path(struct graph* graph, struct node** buffer_src, uint32_t nb_src, struct node** buffer_dst, uint32_t nb_dst, struct dijkstraPath* path, uint32_t(*edge_get_distance)(void*)){
	struct dijkstraInternal{
		struct dijkstraPathStep 	step;
		struct node*				node;
		struct dijkstraInternal* 	next;
	};

	struct dijkstraInternal* 	internals;
	struct dijkstraInternal* 	curr_orbital;
	struct dijkstraInternal* 	next_orbital;
	uint32_t 					i;
	struct node* 				node_cursor;
	struct edge*				edge_cursor;
	struct dijkstraInternal* 	internal_cursor;

	#define END_ORBITAL (void*)(-1)

	if (path->step_array == NULL){
		path->step_array = array_create(sizeof(struct dijkstraPathStep));
		if (path->step_array == NULL){
			log_err("unable to create array");
			return -1;
		}
	}
	else{
		array_empty(path->step_array);
	}

	internals = (struct dijkstraInternal*)malloc(sizeof(struct dijkstraInternal) * graph->nb_node);
	if (internals == NULL){
		log_err("unable to allocate memory");
		return -1;
	}

	for (node_cursor = graph_get_head_node(graph), i = 0; i < graph->nb_node && node_cursor != NULL; node_cursor = node_get_next(node_cursor), i++){
		node_cursor->ptr = internals + i;

		internals[i].step.edge 	= NULL;
		internals[i].step.dir 	= PATH_INVALID;
		internals[i].node 		= node_cursor;
		internals[i].next 		= NULL;
	}

	for (i = 0; i < nb_dst; i++){
		internal_cursor = (struct dijkstraInternal*)buffer_dst[i]->ptr;
		internal_cursor->node = NULL;
	}

	for (i = 0, curr_orbital = END_ORBITAL; i < nb_src; i++){
		internal_cursor = (struct dijkstraInternal*)buffer_src[i]->ptr;
		if (internal_cursor->next != NULL){
			log_warn_m("several instances of node %p in buffer_src", (void*)buffer_src[i]);
			continue;
		}

		internal_cursor->next = curr_orbital;
		curr_orbital = internal_cursor;

		if (internal_cursor->node == NULL){
			path->reached_node = buffer_src[i];
			goto return_path;
		}
	}

	for( ; curr_orbital != END_ORBITAL; curr_orbital = next_orbital){
		for(next_orbital = END_ORBITAL; curr_orbital != END_ORBITAL; curr_orbital = curr_orbital->next){

			for (edge_cursor = node_get_head_edge_src(curr_orbital->node); edge_cursor != NULL; edge_cursor = edge_get_next_src(edge_cursor)){
				if (edge_get_distance != NULL && edge_get_distance(edge_get_data(edge_cursor)) == DIJKSTRA_INVALID_DST){
					continue;
				}

				node_cursor = edge_get_dst(edge_cursor);
				internal_cursor = (struct dijkstraInternal*)node_cursor->ptr;

				if (internal_cursor->next == NULL){
					internal_cursor->step.edge = edge_cursor;
					internal_cursor->step.dir = PATH_SRC_TO_DST;

					if (internal_cursor->node == NULL){
						curr_orbital = internal_cursor;
						path->reached_node = node_cursor;
						goto return_path;
					}
					else{
						internal_cursor->next = next_orbital;
						next_orbital = internal_cursor;
					}
				}
			}

			for (edge_cursor = node_get_head_edge_dst(curr_orbital->node); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
				if (edge_get_distance != NULL && edge_get_distance(edge_get_data(edge_cursor)) == DIJKSTRA_INVALID_DST){
					continue;
				}

				node_cursor = edge_get_src(edge_cursor);
				internal_cursor = (struct dijkstraInternal*)node_cursor->ptr;

				if (internal_cursor->next == NULL){
					internal_cursor->step.edge = edge_cursor;
					internal_cursor->step.dir = PATH_DST_TO_SRC;

					if (internal_cursor->node == NULL){
						curr_orbital = internal_cursor;
						path->reached_node = node_cursor;
						goto return_path;
					}
					else{
						internal_cursor->next = next_orbital;
						next_orbital = internal_cursor;
					}
				}
			}
		}
	}

	free(internals);

	return 1;

	return_path:
	for ( ; curr_orbital->step.edge != NULL; ){
		if (array_add(path->step_array, &(curr_orbital->step)) < 0){
			log_err("unable to add element to array");
		}

		if (curr_orbital->step.dir == PATH_SRC_TO_DST){
			curr_orbital = (struct dijkstraInternal*)(edge_get_src(curr_orbital->step.edge)->ptr);
		}
		else{
			curr_orbital = (struct dijkstraInternal*)(edge_get_dst(curr_orbital->step.edge)->ptr);
		}
	}

	free(internals);

	return 0;
}
