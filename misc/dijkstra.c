#include <stdlib.h>
#include <stdio.h>

#include "dijkstra.h"

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
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
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
		printf("ERROR: in %s, unable to find the given node\n", __func__);
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
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
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
		printf("ERROR: in %s, unable to find the given node\n", __func__);
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
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
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
		printf("ERROR: in %s, unable to find the given node\n", __func__);
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
			printf("ERROR: in %s, unable to create array\n", __func__);
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
	uint32_t 					j;
	struct node* 				node_cursor;
	struct edge*				edge_cursor;
	struct dijkstraInternal* 	internal_cursor;

	if (dijkstra_reset_path(path)){
		printf("ERROR: in %s, unable to reset path\n", __func__);
		return -1;
	}

	internals = (struct dijkstraInternal*)malloc(sizeof(struct dijkstraInternal) * graph->nb_node);
	if (internals == NULL){
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
		return -1;
	}

	for (node_cursor = graph_get_head_node(graph), i = 0; i < graph->nb_node && node_cursor != NULL; node_cursor = node_get_next(node_cursor), i++){
		node_cursor->ptr = internals + i;

		internals[i].path = NULL;
		internals[i].node = node_cursor;
		internals[i].next = NULL;

		for (j = 0; j < nb_src; j++){
			if (src_node[j] == node_cursor){
				internals[i].next = curr_orbital;
				curr_orbital = internals + i;
				break;
			}
		}

		for (j = 0; j < nb_dst; j++){
			if (dst_node[j] == node_cursor){
				internals[i].node = NULL;
				break;
			}
		}
	}

	if (curr_orbital == NULL){
		printf("ERROR: in %s, unable to find the any of the src node(s)\n", __func__);
		free(internals);
		return -1;
	}

	for(i = 0; curr_orbital != NULL; curr_orbital = next_orbital, i++){
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
	if (array_add(*path, &(edge_get_dst(curr_orbital->path))) < 0){
		printf("ERROR: in %s, unable to add element to array\n", __func__);
	}
	else{
		for ( ; curr_orbital->path != NULL; curr_orbital = (struct dijkstraInternal*)(edge_get_src(curr_orbital->path)->ptr)){
			if (array_add(*path, &(curr_orbital->path)) < 0){
				printf("ERROR: in %s, unable to add element to array\n", __func__);
			}
		}
	}

	free(internals);

	return 0;
}

struct node* dijkstra_lowest_common_ancestor(struct graph* graph, struct node** node_buffer1, uint32_t nb_node_buffer1, struct node** node_buffer2, uint32_t nb_node_buffer2, struct array** path1, struct array** path2, uint32_t(*edge_get_distance)(void*)){
	struct dijkstraInternal{
		struct edge*				path;
		struct node*				node;
		uint32_t 					tag;
		struct dijkstraInternal* 	next1;
		struct dijkstraInternal* 	next2;
	};

	struct node* 				node_cursor;
	uint32_t 					i;
	uint32_t 					j;
	struct dijkstraInternal* 	internals;
	struct dijkstraInternal* 	curr_orbital1 = NULL;
	struct dijkstraInternal* 	curr_orbital2 = NULL;
	struct dijkstraInternal* 	next_orbital;
	struct dijkstraInternal* 	internal_cursor;
	struct edge* 				edge_cursor;
	struct node* 				result;

	if (dijkstra_reset_path(path1)){
		printf("ERROR: in %s, unable to reset path\n", __func__);
		return NULL;
	}
	if (dijkstra_reset_path(path2)){
		printf("ERROR: in %s, unable to reset path\n", __func__);
		return NULL;
	}

	internals = (struct dijkstraInternal*)malloc(sizeof(struct dijkstraInternal) * graph->nb_node);
	if (internals == NULL){
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
		return NULL;
	}

	for (node_cursor = graph_get_head_node(graph), i = 0; i < graph->nb_node && node_cursor != NULL; node_cursor = node_get_next(node_cursor), i++){
		node_cursor->ptr = internals + i;

		internals[i].path 	= NULL;
		internals[i].node 	= node_cursor;
		internals[i].tag 	= 0;
		internals[i].next1 	= NULL;
		internals[i].next2 	= NULL;

		for (j = 0; j < nb_node_buffer1; j++){
			if (node_buffer1[j] == node_cursor){
				internals[i].tag |= 0x00000001;
				internals[i].next1 = curr_orbital1;
				curr_orbital1 = internals + i;
				break;
			}
		}

		for (j = 0; j < nb_node_buffer2; j++){
			if (node_buffer2[j] == node_cursor){
				internals[i].tag |= 0x00000002;
				internals[i].next2 = curr_orbital2;
				curr_orbital2 = internals + i;
				break;
			}
		}

		if (internals[i].tag == 0x00000003){
			free(internals);
			return node_cursor;
		}
	}

	if (curr_orbital1 == NULL || curr_orbital2 == NULL){
		printf("ERROR: in %s, unable to find the any of the specified node(s)\n", __func__);
		free(internals);
		return NULL;
	}

	for(i = 0; curr_orbital1 != NULL; curr_orbital1 = next_orbital, i++){
		for(next_orbital = NULL; curr_orbital1 != NULL; curr_orbital1 = curr_orbital1->next1){

			for (edge_cursor = node_get_head_edge_dst(curr_orbital1->node); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
				if (edge_get_distance != NULL && edge_get_distance(&(edge_cursor->data)) == DIJKSTRA_INVALID_DST){
					continue;
				}

				node_cursor = edge_get_src(edge_cursor);
				internal_cursor = (struct dijkstraInternal*)node_cursor->ptr;

				if (internal_cursor->path == NULL){
					internal_cursor->path = edge_cursor;
					internal_cursor->tag |= 0x00000001;
					internal_cursor->next1 = next_orbital;
					next_orbital = internal_cursor;

					if (internal_cursor->tag == 0x00000003){
						goto return_path;
					}
				}
			}
		}
	}

	for(i = 0; curr_orbital2 != NULL; curr_orbital2 = next_orbital, i++){
		for(next_orbital = NULL; curr_orbital2 != NULL; curr_orbital2 = curr_orbital2->next2){

			for (edge_cursor = node_get_head_edge_dst(curr_orbital2->node); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
				if (edge_get_distance != NULL && edge_get_distance(&(edge_cursor->data)) == DIJKSTRA_INVALID_DST){
					continue;
				}

				node_cursor = edge_get_src(edge_cursor);
				internal_cursor = (struct dijkstraInternal*)node_cursor->ptr;

				if (internal_cursor->path == NULL){
					internal_cursor->path = edge_cursor;
					internal_cursor->next2 = next_orbital;
					next_orbital = internal_cursor;
				}

				if (internal_cursor->tag == 0x00000002){
					goto return_path;
				}
			}
		}
	}

	free(internals);

	return NULL;

	return_path:
	for (result = internal_cursor->node; internal_cursor->path != NULL; internal_cursor = (struct dijkstraInternal*)(edge_get_dst(internal_cursor->path)->ptr)){
		if (array_add(*path1, &(internal_cursor->path)) < 0){
			printf("ERROR: in %s, unable to add element to array\n", __func__);
		}
	}

	if (curr_orbital1 == NULL){
		for ( ; edge_cursor != NULL; edge_cursor = ((struct dijkstraInternal*)(edge_get_dst(edge_cursor)->ptr))->path){
			if (array_add(*path2, &(edge_cursor)) < 0){
				printf("ERROR: in %s, unable to add element to array\n", __func__);
			}
		}
	}

	free(internals);

	return result;
}