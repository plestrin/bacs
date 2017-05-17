#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "graphMinPath.h"
#include "base.h"

#define GRAPHMINPATH_MAX_MIN_PATH 1024

static int32_t minPath_add_step(struct minPath* path, struct minPathStep* step);

struct minPathMultiStep{
	struct minPathStep 			step;
	struct minPathMultiStep* 	next;
};

struct minPathIntItem{
	struct minPathMultiStep* 	multi_step_ll;
	uint64_t 					mask;
	struct minPathIntRoot* 		root;
	struct minPathIntItem* 		next;
};

struct minPathIntRoot{
	uint32_t 				dst;
	uint32_t 				sink;
	struct node* 			node;
	struct minPathIntItem 	item_buffer[1];
};

static inline void minPathIntRoot_init(struct minPathIntRoot* root, struct node* node, uint32_t alpha){
	uint32_t i;

	root->dst 	= GRAPHMINPATH_INVALID_DST;
	root->sink 	= 0;
	root->node 	= node;

	for (i = 0; i < alpha + 1; i++){
		root->item_buffer[i].multi_step_ll 	= NULL;
		root->item_buffer[i].mask 			= 0;
		root->item_buffer[i].root 			= root;
		root->item_buffer[i].next 			= NULL;
	}
}

static inline void minPathIntRoot_clean(struct minPathIntRoot* root, uint32_t alpha){
	uint32_t 					i;
	struct minPathMultiStep* 	multi_step_curr;
	struct minPathMultiStep* 	multi_step_next;

	for (i = 0; i < alpha + 1; i++){
		for (multi_step_curr = root->item_buffer[i].multi_step_ll; multi_step_curr != NULL; multi_step_curr = multi_step_next){
			multi_step_next = multi_step_curr->next;
			free(multi_step_curr);
		}
	}
}

static inline void minPathIntItem_add_step(struct minPathIntItem* int_item, struct edge* edge, enum minPathDirection dir, uint64_t mask){
	struct minPathMultiStep* multi_step;

	if ((multi_step = malloc(sizeof(struct minPathMultiStep))) == NULL){
		log_err("unable to allocate memory");
		return;
	}

	multi_step->step.edge 	= edge;
	multi_step->step.dir 	= dir;
	multi_step->next 		= int_item->multi_step_ll;

	int_item->multi_step_ll = multi_step;
	int_item->mask |= mask;
}

#ifdef EXTRA_CHECK
static uint32_t minPath_add_recursive(struct array* path_array, struct minPath* min_path, struct minPathStep* step, uint32_t dst, uint32_t nb_path, uint32_t alpha)
#else
static uint32_t minPath_add_recursive(struct array* path_array, struct minPath* min_path, struct minPathStep* step, uint32_t dst, uint32_t nb_path)
#endif
{
	struct minPath 				fork;
	struct minPathIntRoot* 		int_root;
	struct minPathMultiStep* 	multi_step_cursor;
	int32_t 					return_code;

	for (; step->edge != NULL; step = &(int_root->item_buffer[dst - int_root->dst].multi_step_ll->step)){
		#ifdef EXTRA_CHECK
		if (!dst){
			log_err("inconsistency between dst and edge pointer");
			break;
		}
		#endif

		return_code = minPath_add_step(min_path, step);
		if (return_code < 0){
			log_err("unable to add set to minPath");
			break;
		}
		else if (return_code > 0){
			minPath_clean(min_path);
			return nb_path;
		}

		if (step->dir == PATH_SRC_TO_DST){
			int_root = (struct minPathIntRoot*)(edge_get_src(step->edge)->ptr);
		}
		else if (step->dir == PATH_DST_TO_SRC){
			int_root = (struct minPathIntRoot*)(edge_get_dst(step->edge)->ptr);
		}
		else{
			log_err("incorrect path direction");
			break;
		}

		dst --;

		#ifdef EXTRA_CHECK
		if (dst - int_root->dst> alpha){
			log_err_m("incorrect dst value (root: %u, dst: %u, alpha: %u)", int_root->dst, dst, alpha);
			break;
		}
		#endif

		if (!dst){
			break;
		}

		#ifdef EXTRA_CHECK
		if (int_root->item_buffer[dst - int_root->dst].multi_step_ll == NULL){
			log_err("unable to reconstruct path");
			break;
		}
		#endif

		for (multi_step_cursor = int_root->item_buffer[dst - int_root->dst].multi_step_ll->next; multi_step_cursor != NULL; multi_step_cursor = multi_step_cursor->next){
			fork.step_array = malloc(sizeof(struct array));
			fork.reached_node = min_path->reached_node;

			if (fork.step_array == NULL){
				log_err("unable to allocate memory");
				continue;
			}

			if (array_clone(min_path->step_array, fork.step_array)){
				log_err("unable to clone array");
				free(fork.step_array);
				continue;
			}

			#ifdef EXTRA_CHECK
			nb_path = minPath_add_recursive(path_array, &fork, &(multi_step_cursor->step), dst, nb_path, alpha);
			#else
			nb_path = minPath_add_recursive(path_array, &fork, &(multi_step_cursor->step), dst, nb_path);
			#endif
			if (nb_path == GRAPHMINPATH_MAX_MIN_PATH){
				minPath_clean(min_path);
				return GRAPHMINPATH_MAX_MIN_PATH;
			}
		}
	}

	#ifdef EXTRA_CHECK
	if (dst){
		log_err("inconsistency between dst and edge pointer");
		return nb_path;
	}
	#endif

	if (array_add(path_array, min_path) < 0){
		log_err("unable to add element to array");
		array_delete(min_path->step_array);
	}
	else{
		nb_path ++;
	}

	return nb_path;
}

#ifdef EXTRA_CHECK
#define minPath_add_(path_array, node, edge, dir, dst, nb_path, alpha) minPath_add(path_array, node, edge, dir, dst, nb_path, alpha)
static uint32_t minPath_add(struct array* path_array, struct node* node, struct edge* edge, enum minPathDirection dir, uint32_t dst, uint32_t nb_path, uint32_t alpha)
#else
#define minPath_add_(path_array, node, edge, dir, dst, nb_path, alpha) minPath_add(path_array, node, edge, dir, dst, nb_path)
static uint32_t minPath_add(struct array* path_array, struct node* node, struct edge* edge, enum minPathDirection dir, uint32_t dst, uint32_t nb_path)
#endif
{
	struct minPath 		min_path;
	struct minPathStep 	step;

	if (nb_path < GRAPHMINPATH_MAX_MIN_PATH){
		if (minPath_init(&min_path) == NULL){
			log_err("unable to init minPath");
			return nb_path;
		}

		min_path.reached_node = node;

		step.edge 	= edge;
		step.dir 	= dir;

		#ifdef EXTRA_CHECK
		return minPath_add_recursive(path_array, &min_path, &step, dst, nb_path, alpha);
		#else
		return minPath_add_recursive(path_array, &min_path, &step, dst, nb_path);
		#endif
	}
	else{
		return GRAPHMINPATH_MAX_MIN_PATH;
	}
}

int32_t graphMinPath_bfs(struct graph* graph, struct node** buffer_src, uint32_t nb_src, struct node** buffer_dst, uint32_t nb_dst, struct array* path_array, uint32_t alpha, uint64_t(*get_mask)(uint64_t,struct node*,struct edge*,enum minPathDirection)){
	uint8_t* 				internals;
	struct minPathIntItem* 	curr_orbital;
	struct minPathIntItem* 	next_orbital;
	uint32_t 				i;
	struct node* 			node_cursor;
	struct edge*			edge_cursor;
	struct minPathIntRoot* 	int_root_cursor;
	uint32_t 				nb_min_path 		= 0;
	uint32_t 				min_dst 			= GRAPHMINPATH_INVALID_DST;
	uint32_t 				dst;
	uint64_t 				mask;

	if ((internals = malloc((sizeof(struct minPathIntRoot) + alpha * sizeof(struct minPathIntItem)) * graph->nb_node)) == NULL){
		log_err("unable to allocate memory");
		return -1;
	}

	for (node_cursor = graph_get_head_node(graph), i = 0; i < graph->nb_node && node_cursor != NULL; node_cursor = node_get_next(node_cursor), i++){
		node_cursor->ptr = internals + i * (sizeof(struct minPathIntRoot) + alpha * sizeof(struct minPathIntItem));
		minPathIntRoot_init(node_cursor->ptr, node_cursor, alpha);
	}

	for (i = 0; i < nb_dst; i++){
		((struct minPathIntRoot*)buffer_dst[i]->ptr)->sink = 1;
	}

	for (i = 0, curr_orbital = NULL; i < nb_src; i++){
		int_root_cursor = buffer_src[i]->ptr;
		if (!int_root_cursor->dst){
			continue;
		}

		int_root_cursor->dst = 0;

		if (int_root_cursor->sink){
			nb_min_path = minPath_add_(path_array, buffer_src[i], NULL, PATH_INVALID, 0, nb_min_path, alpha);
			min_dst = 0;
		}
		else{
			int_root_cursor->item_buffer[0].mask = 0xffffffffffffffff;
			int_root_cursor->item_buffer[0].next = curr_orbital;
			curr_orbital = int_root_cursor->item_buffer;
		}
	}

	for (dst = 1; curr_orbital != NULL && (min_dst == GRAPHMINPATH_INVALID_DST || dst <= min_dst + alpha); curr_orbital = next_orbital, dst ++){
		for (next_orbital = NULL; curr_orbital != NULL; curr_orbital = curr_orbital->next){

			for (edge_cursor = node_get_head_edge_src(curr_orbital->root->node); edge_cursor != NULL; edge_cursor = edge_get_next_src(edge_cursor)){
				if (get_mask != NULL){
					mask = get_mask(curr_orbital->mask, curr_orbital->root->node, edge_cursor, PATH_SRC_TO_DST);
					if (!mask){
						continue;
					}
				}
				else{
					mask = 0xffffffffffffffff;
				}

				node_cursor = edge_get_dst(edge_cursor);
				int_root_cursor = (struct minPathIntRoot*)node_cursor->ptr;

				if (int_root_cursor->sink){
					nb_min_path = minPath_add_(path_array, node_cursor, edge_cursor, PATH_SRC_TO_DST, dst, nb_min_path, alpha);
					if (min_dst == GRAPHMINPATH_INVALID_DST){
						min_dst = dst;
					}
					continue;
				}

				if (int_root_cursor->dst == GRAPHMINPATH_INVALID_DST){
					minPathIntItem_add_step(int_root_cursor->item_buffer, edge_cursor, PATH_SRC_TO_DST, mask);

					int_root_cursor->dst = dst;
					int_root_cursor->item_buffer[0].next = next_orbital;
					next_orbital = int_root_cursor->item_buffer;
				}
				else if (dst - int_root_cursor->dst < alpha + 1){
					if (int_root_cursor->item_buffer[dst - int_root_cursor->dst].multi_step_ll == NULL){
						int_root_cursor->item_buffer[dst - int_root_cursor->dst].next = next_orbital;
						next_orbital = int_root_cursor->item_buffer + dst - int_root_cursor->dst;
					}

					minPathIntItem_add_step(int_root_cursor->item_buffer + dst - int_root_cursor->dst, edge_cursor, PATH_SRC_TO_DST, mask);
				}
			}

			for (edge_cursor = node_get_head_edge_dst(curr_orbital->root->node); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
				if (get_mask != NULL){
					mask = get_mask(curr_orbital->mask, curr_orbital->root->node, edge_cursor, PATH_DST_TO_SRC);
					if (!mask){
						continue;
					}
				}
				else{
					mask = 0xffffffffffffffff;
				}

				node_cursor = edge_get_src(edge_cursor);
				int_root_cursor = (struct minPathIntRoot*)node_cursor->ptr;

				if (int_root_cursor->sink){
					nb_min_path = minPath_add_(path_array, node_cursor, edge_cursor, PATH_DST_TO_SRC, dst, nb_min_path, alpha);
					if (min_dst == GRAPHMINPATH_INVALID_DST){
						min_dst = dst;
					}
					continue;
				}

				if (int_root_cursor->dst == GRAPHMINPATH_INVALID_DST){
					minPathIntItem_add_step(int_root_cursor->item_buffer, edge_cursor, PATH_DST_TO_SRC, mask);

					int_root_cursor->dst = dst;
					int_root_cursor->item_buffer[0].next = next_orbital;
					next_orbital = int_root_cursor->item_buffer;
				}
				else if (dst - int_root_cursor->dst < alpha + 1){
					if (int_root_cursor->item_buffer[dst - int_root_cursor->dst].multi_step_ll == NULL){
						int_root_cursor->item_buffer[dst - int_root_cursor->dst].next = next_orbital;
						next_orbital = int_root_cursor->item_buffer + dst - int_root_cursor->dst;
					}

					minPathIntItem_add_step(int_root_cursor->item_buffer + dst - int_root_cursor->dst, edge_cursor, PATH_DST_TO_SRC, mask);
				}
			}
		}
	}

	for (i = 0; i < graph->nb_node; i++){
		minPathIntRoot_clean((struct minPathIntRoot*)(internals + i * (sizeof(struct minPathIntRoot) + alpha * sizeof(struct minPathIntItem))), alpha);
	}

	if (nb_min_path == GRAPHMINPATH_MAX_MIN_PATH){
		log_warn("min path limit has been reached");
	}

	free(internals);

	return 0;
}

static int32_t minPath_add_step(struct minPath* path, struct minPathStep* step){
	uint32_t i;

	for (i = 0; i < array_get_length(path->step_array); i++){
		if (((struct minPathStep*)array_get(path->step_array, i))->edge == step->edge){
			return 1;
		}
	}

	if (array_add(path->step_array, step) < 0){
		log_err("unable to add element to array");
		return -1;
	}

	return 0;
}

int32_t minPath_check(struct minPath* path){
	uint32_t 			i;
	struct node* 		next_node;
	struct minPathStep* step;

	for (i = array_get_length(path->step_array), next_node = NULL; i > 0; i--){
		step = array_get(path->step_array, i - 1);
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

uint32_t minPath_get_nb_dir_change(struct minPath* path){
	uint32_t i;
	uint32_t nb_dir_change;

	for (i = 1, nb_dir_change = 0; i < array_get_length(path->step_array); i++){
		if (((struct minPathStep*)array_get(path->step_array, i - 1))->dir != ((struct minPathStep*)array_get(path->step_array, i))->dir){
			nb_dir_change ++;
		}
	}

	return nb_dir_change;
}
