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

struct dijkstraPathMultiStep{
	struct dijkstraPathStep 		step;
	struct dijkstraPathMultiStep* 	next;
};

struct dijkstraInternal{
	struct dijkstraPathMultiStep 	multi_step;
	uint32_t 						dst;
	struct node*					node;
	struct dijkstraInternal* 		next;
};

static void dijkstraPath_add_recursive(struct array* path_array, struct dijkstraPath* dijkstra_path, struct dijkstraPathStep* step){
	struct dijkstraPath 			fork;
	struct dijkstraInternal* 		node_meta_data;
	struct dijkstraPathMultiStep* 	multi_step;

	for ( ; step->edge != NULL; step = &(node_meta_data->multi_step.step)){
		if (array_add(dijkstra_path->step_array, step) < 0){
			log_err("unable to add element to array");
		}

		if (step->dir == PATH_SRC_TO_DST){
			node_meta_data = (struct dijkstraInternal*)(edge_get_src(step->edge)->ptr);
		}
		else{
			node_meta_data = (struct dijkstraInternal*)(edge_get_dst(step->edge)->ptr);
		}

		for (multi_step = node_meta_data->multi_step.next; multi_step != NULL; multi_step = multi_step->next){
			fork.step_array = (struct array*)malloc(sizeof(struct array));
			fork.reached_node = dijkstra_path->reached_node;

			if (fork.step_array == NULL){
				log_err("unable to allocate memory");
				continue;
			}

			if (array_clone(dijkstra_path->step_array, fork.step_array)){
				log_err("unable to clone array");
				free(fork.step_array);
				continue;
			}

			dijkstraPath_add_recursive(path_array, &fork, &(multi_step->step));
		}
	}

	if (array_add(path_array, dijkstra_path) < 0){
		log_err("unable to add element to array");
		array_delete(dijkstra_path->step_array);
	}
}

static void dijkstraPath_add(struct array* path_array, struct dijkstraInternal* node_meta_data, struct node* reached_node){
	struct dijkstraPath dijkstra_path;

	dijkstra_path.step_array = array_create(sizeof(struct dijkstraPathStep));
	if (dijkstra_path.step_array == NULL){
		log_err("unable to create array");
		return;
	}

	dijkstra_path.reached_node = reached_node;

	dijkstraPath_add_recursive(path_array, &dijkstra_path, &(node_meta_data->multi_step.step));
}

int32_t dijkstra_min_path(struct graph* graph, struct node** buffer_src, uint32_t nb_src, struct node** buffer_dst, uint32_t nb_dst, struct array* path_array, uint32_t(*edge_get_distance)(void*)){
	struct dijkstraInternal* 		internals;
	struct dijkstraInternal* 		curr_orbital;
	struct dijkstraInternal* 		next_orbital;
	uint32_t 						i;
	struct node* 					node_cursor;
	struct edge*					edge_cursor;
	struct dijkstraInternal* 		internal_cursor;
	uint32_t 						nb_min_path 		= 0;
	uint32_t 						dst;
	struct dijkstraPathMultiStep* 	multi_step;
	struct dijkstraPathMultiStep* 	tmp;

	internals = (struct dijkstraInternal*)malloc(sizeof(struct dijkstraInternal) * graph->nb_node);
	if (internals == NULL){
		log_err("unable to allocate memory");
		return -1;
	}

	for (node_cursor = graph_get_head_node(graph), i = 0; i < graph->nb_node && node_cursor != NULL; node_cursor = node_get_next(node_cursor), i++){
		node_cursor->ptr = internals + i;

		internals[i].multi_step.step.edge 	= NULL;
		internals[i].multi_step.step.dir 	= PATH_INVALID;
		internals[i].multi_step.next 		= NULL;
		internals[i].dst 					= 0xffffffff;
		internals[i].node 					= node_cursor;
		internals[i].next 					= NULL;
	}

	for (i = 0; i < nb_dst; i++){
		internal_cursor = (struct dijkstraInternal*)buffer_dst[i]->ptr;
		internal_cursor->node = NULL;
	}

	for (i = 0, curr_orbital = NULL; i < nb_src; i++){
		internal_cursor = (struct dijkstraInternal*)buffer_src[i]->ptr;
		if (internal_cursor->next != NULL){
			log_warn_m("several instances of node %p in buffer_src", (void*)buffer_src[i]);
			continue;
		}

		internal_cursor->dst = 0;
		internal_cursor->next = curr_orbital;
		curr_orbital = internal_cursor;

		if (internal_cursor->node == NULL){
			dijkstraPath_add(path_array, curr_orbital, buffer_src[i]);
			nb_min_path ++;
		}
	}

	for(dst = 0; curr_orbital != NULL && nb_min_path == 0; curr_orbital = next_orbital, dst ++){
		for(next_orbital = NULL; curr_orbital != NULL; curr_orbital = curr_orbital->next){

			for (edge_cursor = node_get_head_edge_src(curr_orbital->node); edge_cursor != NULL; edge_cursor = edge_get_next_src(edge_cursor)){
				if (edge_get_distance != NULL && edge_get_distance(edge_get_data(edge_cursor)) == DIJKSTRA_INVALID_DST){
					continue;
				}

				node_cursor = edge_get_dst(edge_cursor);
				internal_cursor = (struct dijkstraInternal*)node_cursor->ptr;

				if (internal_cursor->dst == 0xffffffff){
					internal_cursor->multi_step.step.edge = edge_cursor;
					internal_cursor->multi_step.step.dir = PATH_SRC_TO_DST;

					if (internal_cursor->node == NULL){
						dijkstraPath_add(path_array, internal_cursor, node_cursor);
						nb_min_path ++;
					}
					else{
						internal_cursor->dst = dst;
						internal_cursor->next = next_orbital;
						next_orbital = internal_cursor;
					}
				}
				else if (internal_cursor->dst == dst){
					for (multi_step = &(internal_cursor->multi_step); multi_step->next != NULL; ){
						multi_step = multi_step->next;
					}
					multi_step->next = (struct dijkstraPathMultiStep*)malloc(sizeof(struct dijkstraPathMultiStep));
					if (multi_step->next == NULL){
						log_err("unable to allocate memory");
					}

					multi_step->next->step.edge = edge_cursor;
					multi_step->next->step.dir = PATH_SRC_TO_DST;
					multi_step->next->next = NULL;
				}
			}

			for (edge_cursor = node_get_head_edge_dst(curr_orbital->node); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
				if (edge_get_distance != NULL && edge_get_distance(edge_get_data(edge_cursor)) == DIJKSTRA_INVALID_DST){
					continue;
				}

				node_cursor = edge_get_src(edge_cursor);
				internal_cursor = (struct dijkstraInternal*)node_cursor->ptr;

				if (internal_cursor->dst == 0xffffffff){
					internal_cursor->multi_step.step.edge = edge_cursor;
					internal_cursor->multi_step.step.dir = PATH_DST_TO_SRC;

					if (internal_cursor->node == NULL){
						dijkstraPath_add(path_array, internal_cursor, node_cursor);
						nb_min_path ++;
					}
					else{
						internal_cursor->dst = dst;
						internal_cursor->next = next_orbital;
						next_orbital = internal_cursor;
					}
				}
				else if (internal_cursor->dst == dst){
					for (multi_step = &(internal_cursor->multi_step); multi_step->next != NULL; ){
						multi_step = multi_step->next;
					}
					multi_step->next = (struct dijkstraPathMultiStep*)malloc(sizeof(struct dijkstraPathMultiStep));
					if (multi_step->next == NULL){
						log_err("unable to allocate memory");
					}

					multi_step->next->step.edge = edge_cursor;
					multi_step->next->step.dir = PATH_DST_TO_SRC;
					multi_step->next->next = NULL;
				}
			}
		}
	}

	for (i = 0; i < graph->nb_node; i++){
		for (multi_step = internals[i].multi_step.next; multi_step != NULL; ){
			tmp = multi_step->next;
			free(multi_step);
			multi_step = tmp;
		}
	}

	free(internals);

	return 0;
}
