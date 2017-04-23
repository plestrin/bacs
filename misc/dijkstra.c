#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "dijkstra.h"
#include "base.h"

struct dijkstraPathMultiStep{
	struct dijkstraPathStep 		step;
	struct dijkstraPathMultiStep* 	next;
};

struct dijkstraInternal{
	struct dijkstraPathMultiStep 	multi_step;
	uint32_t 						dst;
	uint64_t 						mask;
	uint32_t 						sink;
	struct node*					node;
	struct dijkstraInternal* 		next;
};

static inline void dijkstraInternal_init(struct dijkstraInternal* internal, struct node* node){
	internal->multi_step.step.edge 	= NULL;
	internal->multi_step.step.dir 	= PATH_INVALID;
	internal->multi_step.next 		= NULL;
	internal->dst 					= DIJKSTRA_INVALID_DST;
	internal->mask 					= 0;
	internal->sink 					= 0;
	internal->node 					= node;
	internal->next 					= NULL;
}

static inline void dijkstraInternal_add_step(struct dijkstraInternal* internal, struct edge* edge, enum dijkstraPathDirection dir){
	struct dijkstraPathMultiStep* step;

	if ((step = (struct dijkstraPathMultiStep*)malloc(sizeof(struct dijkstraPathMultiStep))) == NULL){
		log_err("unable to allocate memory");
		return;
	}

	memcpy(step, &(internal->multi_step), sizeof(struct dijkstraPathMultiStep));

	internal->multi_step.step.edge 	= edge;
	internal->multi_step.step.dir 	= dir;
	internal->multi_step.next 		= step;
}

static inline void dijkstraInternal_clean(struct dijkstraInternal* internal){
	struct dijkstraPathMultiStep* step;

	for (step = internal->multi_step.next; step != NULL; step = internal->multi_step.next){
		internal->multi_step.next = step->next;
		free(step);
	}
}

#define DIJKSTRA_MAX_MIN_PATH 1024

static uint32_t dijkstraPath_add_recursive(struct array* path_array, struct dijkstraPath* dijkstra_path, struct dijkstraPathStep* step, uint32_t nb_path){
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

			nb_path = dijkstraPath_add_recursive(path_array, &fork, &(multi_step->step), nb_path);
			if (nb_path == DIJKSTRA_MAX_MIN_PATH){
				array_delete(dijkstra_path->step_array);
				return DIJKSTRA_MAX_MIN_PATH;
			}
		}
	}

	if (array_add(path_array, dijkstra_path) < 0){
		log_err("unable to add element to array");
		array_delete(dijkstra_path->step_array);
	}
	else{
		nb_path ++;
	}

	return nb_path;
}

static uint32_t dijkstraPath_add(struct array* path_array, struct dijkstraInternal* internal, struct node* reached_node, uint32_t nb_path){
	struct dijkstraPath dijkstra_path;

	if (nb_path < DIJKSTRA_MAX_MIN_PATH){
		if (dijkstraPath_init(&dijkstra_path) == NULL){
			log_err("unable to init dijkstraPath");
			return nb_path;
		}

		dijkstra_path.reached_node = reached_node;

		return dijkstraPath_add_recursive(path_array, &dijkstra_path, &(internal->multi_step.step), nb_path);
	}
	else{
		return DIJKSTRA_MAX_MIN_PATH;
	}
}

int32_t dijkstra_min_path(struct graph* graph, struct node** buffer_src, uint32_t nb_src, struct node** buffer_dst, uint32_t nb_dst, struct array* path_array, uint64_t(*get_mask)(uint64_t,struct node*,struct edge*,enum dijkstraPathDirection)){
	struct dijkstraInternal* 	internals;
	struct dijkstraInternal* 	curr_orbital;
	struct dijkstraInternal* 	next_orbital;
	uint32_t 					i;
	struct node* 				node_cursor;
	struct edge*				edge_cursor;
	struct dijkstraInternal* 	internal_cursor;
	uint32_t 					nb_min_path 		= 0;
	uint32_t 					dst;
	uint64_t 					mask;

	if ((internals = (struct dijkstraInternal*)malloc(sizeof(struct dijkstraInternal) * graph->nb_node)) == NULL){
		log_err("unable to allocate memory");
		return -1;
	}

	for (node_cursor = graph_get_head_node(graph), i = 0; i < graph->nb_node && node_cursor != NULL; node_cursor = node_get_next(node_cursor), i++){
		node_cursor->ptr = internals + i;
		dijkstraInternal_init(internals + i, node_cursor);
	}

	for (i = 0; i < nb_dst; i++){
		((struct dijkstraInternal*)buffer_dst[i]->ptr)->sink = 1;
	}

	for (i = 0, curr_orbital = NULL; i < nb_src; i++){
		internal_cursor = (struct dijkstraInternal*)buffer_src[i]->ptr;
		if (internal_cursor->dst == 0){
			continue;
		}

		internal_cursor->dst 	= 0;
		internal_cursor->mask 	= 0xffffffffffffffffULL;
		internal_cursor->next 	= curr_orbital;
		curr_orbital = internal_cursor;

		if (internal_cursor->sink){
			nb_min_path = dijkstraPath_add(path_array, curr_orbital, buffer_src[i], nb_min_path);
		}
	}

	for(dst = 1; curr_orbital != NULL && nb_min_path == 0; curr_orbital = next_orbital, dst ++){
		for(next_orbital = NULL; curr_orbital != NULL; curr_orbital = curr_orbital->next){

			for (edge_cursor = node_get_head_edge_src(curr_orbital->node); edge_cursor != NULL; edge_cursor = edge_get_next_src(edge_cursor)){
				if (get_mask != NULL){
					mask = get_mask(curr_orbital->mask, curr_orbital->node, edge_cursor, PATH_SRC_TO_DST);
					if (mask == 0){
						continue;
					}
				}
				else{
					mask = 0xffffffffffffffffULL;
				}

				node_cursor = edge_get_dst(edge_cursor);
				internal_cursor = (struct dijkstraInternal*)node_cursor->ptr;

				if (internal_cursor->dst == DIJKSTRA_INVALID_DST){
					internal_cursor->multi_step.step.edge = edge_cursor;
					internal_cursor->multi_step.step.dir = PATH_SRC_TO_DST;

					if (internal_cursor->sink){
						nb_min_path = dijkstraPath_add(path_array, internal_cursor, node_cursor, nb_min_path);
					}

					internal_cursor->dst 	= dst;
					internal_cursor->mask 	= mask;
					internal_cursor->next 	= next_orbital;
					next_orbital = internal_cursor;
				}
				else if (internal_cursor->dst == dst){
					internal_cursor->mask |= mask;
					dijkstraInternal_add_step(internal_cursor, edge_cursor, PATH_SRC_TO_DST);

					if (internal_cursor->sink){
						nb_min_path = dijkstraPath_add(path_array, internal_cursor, node_cursor, nb_min_path);
					}
				}
			}

			for (edge_cursor = node_get_head_edge_dst(curr_orbital->node); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
				if (get_mask != NULL){
					mask = get_mask(curr_orbital->mask, curr_orbital->node, edge_cursor, PATH_DST_TO_SRC);
					if (mask == 0){
						continue;
					}
				}
				else{
					mask = 0xffffffffffffffffULL;
				}

				node_cursor = edge_get_src(edge_cursor);
				internal_cursor = (struct dijkstraInternal*)node_cursor->ptr;

				if (internal_cursor->dst == DIJKSTRA_INVALID_DST){
					internal_cursor->multi_step.step.edge = edge_cursor;
					internal_cursor->multi_step.step.dir = PATH_DST_TO_SRC;

					if (internal_cursor->sink){
						nb_min_path = dijkstraPath_add(path_array, internal_cursor, node_cursor, nb_min_path);
					}

					internal_cursor->dst 	= dst;
					internal_cursor->mask 	= mask;
					internal_cursor->next 	= next_orbital;
					next_orbital = internal_cursor;
				}
				else if (internal_cursor->dst == dst){
					internal_cursor->mask |= mask;
					dijkstraInternal_add_step(internal_cursor, edge_cursor, PATH_DST_TO_SRC);

					if (internal_cursor->sink){
						nb_min_path = dijkstraPath_add(path_array, internal_cursor, node_cursor, nb_min_path);
					}
				}
			}
		}
	}

	for (i = 0; i < graph->nb_node; i++){
		dijkstraInternal_clean(internals + i);
	}

	if (nb_min_path == DIJKSTRA_MAX_MIN_PATH){
		log_warn("min path limit has been reached");
	}

	free(internals);

	return 0;
}

int32_t dijkstraPath_check(struct dijkstraPath* path){
	uint32_t 					i;
	struct node* 				next_node;
	struct dijkstraPathStep* 	step;

	for (i = array_get_length(path->step_array), next_node = NULL; i > 0; i--){
		step = (struct dijkstraPathStep*)array_get(path->step_array, i - 1);
		switch (step->dir){
			case PATH_SRC_TO_DST 	: {
				if (next_node != NULL){
					if (edge_get_src(step->edge) != next_node){
						log_err_m("found incoherence in path @ step %u/%u", i, array_get_length(path->step_array));
					}
				}

				next_node = edge_get_dst(step->edge);

				break;
			}
			case PATH_DST_TO_SRC 	: {
				if (next_node != NULL){
					if (edge_get_dst(step->edge) != next_node){
						log_err_m("found incoherence in path @ step %u/%u", i, array_get_length(path->step_array));
					}
				}

				next_node = edge_get_src(step->edge);

				break;
			}
			case PATH_INVALID 		: {
				log_err("step direction is invalid");
				return -1;
			}
		}
	}

	return 0;
}

uint32_t dijkstraPath_get_nb_dir_change(struct dijkstraPath* path){
	uint32_t i;
	uint32_t nb_dir_change;

	for (i = 1, nb_dir_change = 0; i < array_get_length(path->step_array); i++){
		if (((struct dijkstraPathStep*)array_get(path->step_array, i - 1))->dir != ((struct dijkstraPathStep*)array_get(path->step_array, i))->dir){
			nb_dir_change ++;
		}
	}

	return nb_dir_change;
}
