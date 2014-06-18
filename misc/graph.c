#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "graph.h"

/* One can implement special callback mecanisms here */
#define graph_node_set_data(graph, node, data_) 	memcpy(&((node)->data), (data_), (graph)->node_data_size);
#define graph_edge_set_data(graph, edge, data_) 	memcpy(&((edge)->data), (data_), (graph)->edge_data_size);
#define graph_node_clean_data(graph, node) 			
#define graph_edge_clean_data(graph, edge) 			

struct graph* graph_create(uint32_t node_data_size, uint32_t edge_data_size){
	struct graph* graph;

	graph = (struct graph*)malloc(sizeof(struct graph));
	if (graph != NULL){
		graph_init(graph, node_data_size, edge_data_size);
	}
	else{
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
	}

	return graph;
}

struct node* graph_add_node_(struct graph* graph){
	struct node* node;

	node = graph_allocate_node(graph);
	if (node == NULL){
		printf("ERROR: in %s, unable to allocate node\n", __func__);
	}
	else{
		node->nb_edge_src 			= 0;
		node->nb_edge_dst 			= 0;
		node->next 					= graph->node_linkedList;
		node->prev 					= NULL;
		node->src_edge_linkedList 	= NULL;
		node->dst_edge_linkedList 	= NULL;
		node->ptr 					= NULL;

		if (node->next != NULL){
			node->next->prev = node;
		}

		graph->node_linkedList = node;
		graph->nb_node ++;
	}
	
	return node;
}

struct node* graph_add_node(struct graph* graph, void* data){
	struct node* node;

	node = graph_add_node_(graph);
	if (node != NULL){
		graph_node_set_data(graph, node, data)
	}
	else{
		printf("ERROR: in %s, node is NULL\n", __func__);
	}
	
	return node;
}

void graph_remove_node(struct graph* graph, struct node* node){
	graph->nb_node --;

	while(node->src_edge_linkedList != NULL){
		graph_remove_edge(graph, node->src_edge_linkedList);
	}

	while(node->dst_edge_linkedList != NULL){
		graph_remove_edge(graph, node->dst_edge_linkedList);
	}

	if (node->prev == NULL){
		graph->node_linkedList = node->next;
	}
	else{
		node->prev->next = node->next;
	}

	if (node->next != NULL){
		node->next->prev = node->prev;
	}

	graph_node_clean_data(graph, node)

	graph_free_node(graph, node);
}

struct edge* graph_add_edge_(struct graph* graph, struct node* node_src, struct node* node_dst){
	struct edge* edge;

	edge = graph_allocate_edge(graph);
	if (edge == NULL){
		printf("ERROR: in %s, unable to allocate edge\n", __func__);
	}
	else{
		edge->src_node = node_src;
		edge->dst_node = node_dst;
		edge->src_prev = NULL;
		edge->src_next = node_src->src_edge_linkedList;
		edge->dst_prev = NULL;
		edge->dst_next = node_dst->dst_edge_linkedList;

		if (edge->src_next != NULL){
			edge->src_next->src_prev = edge;
		}

		if (edge->dst_next != NULL){
			edge->dst_next->dst_prev = edge;
		}

		node_src->src_edge_linkedList = edge;
		node_src->nb_edge_src ++;

		node_dst->dst_edge_linkedList = edge;
		node_dst->nb_edge_dst ++;

		graph->nb_edge ++;
	}

	return edge;
}

struct edge* graph_add_edge(struct graph* graph, struct node* node_src, struct node* node_dst, void* data){
	struct edge* edge;

	edge = graph_add_edge_(graph, node_src, node_dst);
	if (edge != NULL){
		graph_edge_set_data(graph, edge, data)
	}
	else{
		printf("ERROR: in %s, edge is NULL\n", __func__);
	}
	
	return edge;
}

void graph_remove_edge(struct graph* graph, struct edge* edge){
	graph->nb_edge --;

	if (edge->src_prev == NULL){
		edge->src_node->src_edge_linkedList = edge->src_next;
	}
	else{
		edge->src_prev->src_next = edge->src_next;
	}
	if (edge->src_next != NULL){
		edge->src_next->src_prev = edge->src_prev;
	}
	edge->src_node->nb_edge_src --;

	if (edge->dst_prev == NULL){
		edge->dst_node->dst_edge_linkedList = edge->dst_next;
	}
	else{
		edge->dst_prev->dst_next = edge->dst_next;
	}
	if (edge->dst_next != NULL){
		edge->dst_next->dst_prev = edge->dst_prev;
	}
	edge->dst_node->nb_edge_dst --;

	graph_edge_clean_data(graph, edge)

	graph_free_edge(graph, edge);
}