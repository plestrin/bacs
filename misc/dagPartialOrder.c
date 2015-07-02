#include <stdlib.h>
#include <stdio.h>

#include "dagPartialOrder.h"

#define DAGPARTIALORDER_NODE_STATE_UNSET 	(void*)0x00
#define DAGPARTIALORDER_NODE_STATE_SETTING 	(void*)0x01
#define DAGPARTIALORDER_NODE_STATE_SET 		(void*)0x02

static void dagPartialOrder_recursive_sort(struct node** node_buffer, struct node* node, uint32_t* generator);

int32_t dagPartialOrder_sort_src_dst(struct graph* graph){
	struct node** 	node_buffer;
	uint32_t 		i;
	struct node* 	node_cursor;
	uint32_t 		generator = 0;

	if (graph->nb_node == 0){
		return 0;
	}

	node_buffer = (struct node**)malloc(graph->nb_node * sizeof(struct node*));
	if (node_buffer == NULL){
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
		return -1;
	}

	for(node_cursor = graph_get_head_node(graph); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
		node_cursor->ptr = DAGPARTIALORDER_NODE_STATE_UNSET;
	}

	for(node_cursor = graph_get_head_node(graph); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
		if (node_cursor->ptr == DAGPARTIALORDER_NODE_STATE_UNSET){
			dagPartialOrder_recursive_sort(node_buffer, node_cursor, &generator);
		}
	}

	graph->node_linkedList_head = node_buffer[0];
	node_buffer[0]->prev = NULL;
	if (graph->nb_node > 1){
		node_buffer[0]->next = node_buffer[1];

		for (i = 1; i < graph->nb_node - 1; i++){
			node_buffer[i]->prev = node_buffer[i - 1];
			node_buffer[i]->next = node_buffer[i + 1];
		}
			
		node_buffer[i]->prev = node_buffer[i - 1];
		node_buffer[i]->next = NULL;
		graph->node_linkedList_tail = node_buffer[i];
	}
	else{
		node_buffer[0]->next = NULL;
		graph->node_linkedList_tail = node_buffer[0];
	}

	free(node_buffer);

	return 0;
}

int32_t dagPartialOrder_sort_dst_src(struct graph* graph){
	struct node** 	node_buffer;
	uint32_t 		i;
	struct node* 	node_cursor;
	uint32_t 		generator = 0;

	if (graph->nb_node == 0){
		return 0;
	}

	node_buffer = (struct node**)malloc(graph->nb_node * sizeof(struct node*));
	if (node_buffer == NULL){
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
		return -1;
	}

	for(node_cursor = graph_get_head_node(graph); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
		node_cursor->ptr = DAGPARTIALORDER_NODE_STATE_UNSET;
	}

	for(node_cursor = graph_get_head_node(graph); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
		if (node_cursor->ptr == DAGPARTIALORDER_NODE_STATE_UNSET){
			dagPartialOrder_recursive_sort(node_buffer, node_cursor, &generator);
		}
	}

	graph->node_linkedList_head = node_buffer[graph->nb_node - 1];
	node_buffer[graph->nb_node - 1]->prev = NULL;
	if (graph->nb_node > 1){
		node_buffer[graph->nb_node - 1]->next = node_buffer[graph->nb_node - 2];

		for (i = graph->nb_node - 2; i > 0; i--){
			node_buffer[i]->prev = node_buffer[i + 1];
			node_buffer[i]->next = node_buffer[i - 1];
		}
			
		node_buffer[0]->prev = node_buffer[1];
		node_buffer[0]->next = NULL;
		graph->node_linkedList_tail = node_buffer[0];
	}
	else{
		node_buffer[0]->next = NULL;
		graph->node_linkedList_tail = node_buffer[0];
	}

	free(node_buffer);

	return 0;
}

static void dagPartialOrder_recursive_sort(struct node** node_buffer, struct node* node, uint32_t* generator){
	struct edge* edge_cursor;
	struct node* parent_node;

	node->ptr = DAGPARTIALORDER_NODE_STATE_SETTING;
	for (edge_cursor = node_get_head_edge_dst(node); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
		parent_node = edge_get_src(edge_cursor);

		if (parent_node->ptr == DAGPARTIALORDER_NODE_STATE_UNSET){
			dagPartialOrder_recursive_sort(node_buffer, parent_node, generator);
		}
		else if (parent_node->ptr == DAGPARTIALORDER_NODE_STATE_SETTING){
			printf("ERROR: in %s, cycle detected in graph\n", __func__);
		}
	}

	node->ptr = DAGPARTIALORDER_NODE_STATE_SET;
	node_buffer[*generator] = node;
	*generator = *generator + 1;
}