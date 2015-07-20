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

static int32_t dijkstra_reset_path(struct array** path){
	if (*path == NULL){
		*path = array_create(sizeof(struct edge*));
		if (*path == NULL){
			log_err("unable to create array");
			return -1;
		}
	}
	else{
		array_empty(*path);
	}

	return 0;
}

int32_t dijkstra_min_path(struct graph* graph, struct node** src_node, uint32_t nb_src, struct node** dst_node, uint32_t nb_dst, struct array** path, uint32_t(*edge_get_distance)(void*)){
	struct dijkstraInternal{
		struct edge*				path;
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

	if (dijkstra_reset_path(path)){
		log_err("unable to reset path");
		return -1;
	}

	internals = (struct dijkstraInternal*)malloc(sizeof(struct dijkstraInternal) * graph->nb_node);
	if (internals == NULL){
		log_err("unable to allocate memory");
		return -1;
	}

	for (node_cursor = graph_get_head_node(graph), i = 0; i < graph->nb_node && node_cursor != NULL; node_cursor = node_get_next(node_cursor), i++){
		node_cursor->ptr = internals + i;

		internals[i].path = NULL;
		internals[i].node = node_cursor;
		internals[i].next = NULL;
	}

	for (i = 0; i < nb_src; i++){
		internal_cursor = (struct dijkstraInternal*)src_node[i]->ptr;
		internal_cursor->next = curr_orbital;
		curr_orbital = internal_cursor;
	}

	for (i = 0; i < nb_dst; i++){
		internal_cursor = (struct dijkstraInternal*)dst_node[i]->ptr;
		internal_cursor->node = NULL;
	}

	if (curr_orbital == NULL){
		log_err("unable to find the any of the src node(s)");
		free(internals);
		return -1;
	}

	for( ; curr_orbital != NULL; curr_orbital = next_orbital){
		for(next_orbital = NULL; curr_orbital != NULL; curr_orbital = curr_orbital->next){
			if (curr_orbital->node == NULL){
				goto return_path;
			}

			for (edge_cursor = node_get_head_edge_src(curr_orbital->node); edge_cursor != NULL; edge_cursor = edge_get_next_src(edge_cursor)){
				if (edge_get_distance != NULL && edge_get_distance(&(edge_cursor->data)) == DIJKSTRA_INVALID_DST){
					continue;
				}

				node_cursor = edge_get_dst(edge_cursor);
				internal_cursor = (struct dijkstraInternal*)node_cursor->ptr;

				if (internal_cursor->path == NULL){
					internal_cursor->path = edge_cursor;
					internal_cursor->next = next_orbital;
					next_orbital = internal_cursor;
				}
			}
		}
	}

	free(internals);

	return 1;

	return_path:
	for ( ; curr_orbital->path != NULL; curr_orbital = (struct dijkstraInternal*)(edge_get_src(curr_orbital->path)->ptr)){
		if (array_add(*path, &(curr_orbital->path)) < 0){
			log_err("unable to add element to array");
		}
	}

	free(internals);

	return 0;
}

struct node* dijkstra_lowest_common_ancestor(struct graph* graph, struct node** node_buffer1, uint32_t nb_node_buffer1, struct node** node_buffer2, uint32_t nb_node_buffer2, struct array** path1, struct array** path2, uint32_t(*edge_get_distance)(void*,uint32_t)){
	struct dijkstraInternal{
		struct edge*				path;
		struct node*				node;
		uint32_t 					tag;
		uint32_t 					dst;
		struct dijkstraInternal* 	next;
	};

	struct node* 				node_cursor;
	uint32_t 					i;
	struct dijkstraInternal* 	internals;
	struct dijkstraInternal* 	curr_orbital1 = NULL;
	struct dijkstraInternal* 	curr_orbital2 = NULL;
	struct dijkstraInternal* 	next_orbital;
	struct dijkstraInternal* 	internal_cursor;
	struct edge* 				edge_cursor;
	uint32_t 					dst = 0;

	if (dijkstra_reset_path(path1)){
		log_err("unable to reset path");
		return NULL;
	}
	if (dijkstra_reset_path(path2)){
		log_err("unable to reset path");
		return NULL;
	}

	internals = (struct dijkstraInternal*)malloc(sizeof(struct dijkstraInternal) * graph->nb_node);
	if (internals == NULL){
		log_err("unable to allocate memory");
		return NULL;
	}

	for (node_cursor = graph_get_head_node(graph), i = 0; i < graph->nb_node && node_cursor != NULL; node_cursor = node_get_next(node_cursor), i++){
		node_cursor->ptr = internals + i;

		internals[i].path 	= NULL;
		internals[i].node 	= node_cursor;
		internals[i].tag 	= 0;
		internals[i].dst 	= 0;
		internals[i].next 	= NULL;
	}

	for (i = 0; i < nb_node_buffer1; i++){
		internal_cursor = (struct dijkstraInternal*)node_buffer1[i]->ptr;
		internal_cursor->tag 	= 0x00000001;
		internal_cursor->next 	= curr_orbital1;
		curr_orbital1 = internal_cursor;
	}

	for (i = 0; i < nb_node_buffer2; i++){
		internal_cursor = (struct dijkstraInternal*)node_buffer2[i]->ptr;

		if (internal_cursor->tag == 0x00000001){
			free(internals);
			return node_buffer2[i];
		}

		internal_cursor->tag 	= 0x00000002;
		internal_cursor->next 	= curr_orbital2;
		curr_orbital2 = internal_cursor;
	}

	if (curr_orbital1 == NULL || curr_orbital2 == NULL){
		log_err("unable to find the any of the specified node(s)");
		free(internals);
		return NULL;
	}

	for( ; curr_orbital1 != NULL; curr_orbital1 = next_orbital){
		for(next_orbital = NULL; curr_orbital1 != NULL; curr_orbital1 = curr_orbital1->next){

			for (edge_cursor = node_get_head_edge_dst(curr_orbital1->node); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
				if (edge_get_distance != NULL && (dst = edge_get_distance(&(edge_cursor->data), curr_orbital1->dst)) == DIJKSTRA_INVALID_DST){
					continue;
				}

				node_cursor = edge_get_src(edge_cursor);
				internal_cursor = (struct dijkstraInternal*)node_cursor->ptr;

				if (internal_cursor->path == NULL && (internal_cursor->tag & 0x00000001) == 0){
					internal_cursor->path 	= edge_cursor;
					internal_cursor->tag 	= internal_cursor->tag | 0x00000001;
					internal_cursor->dst 	= dst;
					internal_cursor->next 	= next_orbital;
					next_orbital = internal_cursor;

					if (internal_cursor->tag == 0x00000003){
						goto return_path;
					}
				}
			}
		}
	}

	for( ; curr_orbital2 != NULL; curr_orbital2 = next_orbital){
		for(next_orbital = NULL; curr_orbital2 != NULL; curr_orbital2 = curr_orbital2->next){

			for (edge_cursor = node_get_head_edge_dst(curr_orbital2->node); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
				if (edge_get_distance != NULL && (dst = edge_get_distance(&(edge_cursor->data), curr_orbital2->dst)) == DIJKSTRA_INVALID_DST){
					continue;
				}

				node_cursor = edge_get_src(edge_cursor);
				internal_cursor = (struct dijkstraInternal*)node_cursor->ptr;

				if (internal_cursor->tag == 0x00000001){
					goto return_path;
				}

				if (internal_cursor->path == NULL && (internal_cursor->tag & 0x00000002) == 0){
					internal_cursor->path 	= edge_cursor;
					internal_cursor->dst 	= dst;
					internal_cursor->next 	= next_orbital;
					next_orbital = internal_cursor;
				}
			}
		}
	}

	free(internals);

	return NULL;

	return_path:
	for ( ; internal_cursor->path != NULL; internal_cursor = (struct dijkstraInternal*)(edge_get_dst(internal_cursor->path)->ptr)){
		if (array_add(*path1, &(internal_cursor->path)) < 0){
			log_err("unable to add element to array");
		}
	}

	if (curr_orbital1 == NULL){
		for ( ; edge_cursor != NULL; edge_cursor = ((struct dijkstraInternal*)(edge_get_dst(edge_cursor)->ptr))->path){
			if (array_add(*path2, &(edge_cursor)) < 0){
				log_err("unable to add element to array");
			}
		}
	}

	free(internals);

	return node_cursor;
}

struct node* dijkstra_highest_common_descendant(struct graph* graph, struct node** node_buffer1, uint32_t nb_node_buffer1, struct node** node_buffer2, uint32_t nb_node_buffer2, struct array** path1, struct array** path2, uint32_t(*edge_get_distance)(void*)){
	struct dijkstraInternal{
		struct edge*				path;
		struct node*				node;
		uint32_t 					tag;
		struct dijkstraInternal* 	next;
	};

	struct node* 				node_cursor;
	uint32_t 					i;
	struct dijkstraInternal* 	internals;
	struct dijkstraInternal* 	curr_orbital1 = NULL;
	struct dijkstraInternal* 	curr_orbital2 = NULL;
	struct dijkstraInternal* 	next_orbital;
	struct dijkstraInternal* 	internal_cursor;
	struct edge* 				edge_cursor;

	if (dijkstra_reset_path(path1)){
		log_err("unable to reset path");
		return NULL;
	}
	if (dijkstra_reset_path(path2)){
		log_err("unable to reset path");
		return NULL;
	}

	internals = (struct dijkstraInternal*)malloc(sizeof(struct dijkstraInternal) * graph->nb_node);
	if (internals == NULL){
		log_err("unable to allocate memory");
		return NULL;
	}

	for (node_cursor = graph_get_head_node(graph), i = 0; i < graph->nb_node && node_cursor != NULL; node_cursor = node_get_next(node_cursor), i++){
		node_cursor->ptr = internals + i;

		internals[i].path 	= NULL;
		internals[i].node 	= node_cursor;
		internals[i].tag 	= 0;
		internals[i].next 	= NULL;
	}

	for (i = 0; i < nb_node_buffer1; i++){
		internal_cursor = (struct dijkstraInternal*)node_buffer1[i]->ptr;
		internal_cursor->tag 	= 0x00000001;
		internal_cursor->next 	= curr_orbital1;
		curr_orbital1 = internal_cursor;
	}

	for (i = 0; i < nb_node_buffer2; i++){
		internal_cursor = (struct dijkstraInternal*)node_buffer2[i]->ptr;

		if (internal_cursor->tag == 0x00000001){
			free(internals);
			return node_buffer2[i];
		}

		internal_cursor->tag 	= 0x00000002;
		internal_cursor->next 	= curr_orbital2;
		curr_orbital2 = internal_cursor;
	}

	if (curr_orbital1 == NULL || curr_orbital2 == NULL){
		log_err("unable to find the any of the specified node(s)");
		free(internals);
		return NULL;
	}

	for( ; curr_orbital1 != NULL; curr_orbital1 = next_orbital){
		for(next_orbital = NULL; curr_orbital1 != NULL; curr_orbital1 = curr_orbital1->next){

			for (edge_cursor = node_get_head_edge_src(curr_orbital1->node); edge_cursor != NULL; edge_cursor = edge_get_next_src(edge_cursor)){
				if (edge_get_distance != NULL && edge_get_distance(&(edge_cursor->data)) == DIJKSTRA_INVALID_DST){
					continue;
				}

				node_cursor = edge_get_dst(edge_cursor);
				internal_cursor = (struct dijkstraInternal*)node_cursor->ptr;

				if (internal_cursor->path == NULL && (internal_cursor->tag & 0x00000001) == 0){
					internal_cursor->path 	= edge_cursor;
					internal_cursor->tag 	= internal_cursor->tag | 0x00000001;
					internal_cursor->next 	= next_orbital;
					next_orbital = internal_cursor;

					if (internal_cursor->tag == 0x00000003){
						goto return_path;
					}
				}
			}
		}
	}

	for( ; curr_orbital2 != NULL; curr_orbital2 = next_orbital){
		for(next_orbital = NULL; curr_orbital2 != NULL; curr_orbital2 = curr_orbital2->next){

			for (edge_cursor = node_get_head_edge_src(curr_orbital2->node); edge_cursor != NULL; edge_cursor = edge_get_next_src(edge_cursor)){
				if (edge_get_distance != NULL && edge_get_distance(&(edge_cursor->data)) == DIJKSTRA_INVALID_DST){
					continue;
				}

				node_cursor = edge_get_dst(edge_cursor);
				internal_cursor = (struct dijkstraInternal*)node_cursor->ptr;

				if (internal_cursor->tag == 0x00000001){
					goto return_path;
				}

				if (internal_cursor->path == NULL && (internal_cursor->tag & 0x00000002) == 0){
					internal_cursor->path 	= edge_cursor;
					internal_cursor->next 	= next_orbital;
					next_orbital = internal_cursor;
				}
			}
		}
	}

	free(internals);

	return NULL;


	return_path:
	if (internal_cursor->path != NULL){
		internal_cursor = (struct dijkstraInternal*)(edge_get_src(internal_cursor->path)->ptr);
	}
	for ( ; internal_cursor->path != NULL; internal_cursor = (struct dijkstraInternal*)(edge_get_src(internal_cursor->path)->ptr)){
		if (array_add(*path1, &(internal_cursor->path)) < 0){
			log_err("unable to add element to array");
		}
	}

	if (curr_orbital1 == NULL){
		for ( ; edge_cursor != NULL; edge_cursor = ((struct dijkstraInternal*)(edge_get_src(edge_cursor)->ptr))->path){
			if (array_add(*path2, &(edge_cursor)) < 0){
				log_err("unable to add element to array");
			}
		}
	}

	free(internals);

	return node_cursor;
}