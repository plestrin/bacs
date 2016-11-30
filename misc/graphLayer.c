#include <stdlib.h>
#include <stdio.h>

#include "graphLayer.h"
#include "base.h"

static inline struct node* graphLayer_get_node(struct graphLayer* graph_layer, struct node* node){
	if (node->ptr == NULL){
		if ((node->ptr = (void*)graph_add_node_(&(graph_layer->layer))) == NULL){
			log_err("unable to add node to graph");
		}
		else{
			((struct node*)(node->ptr))->ptr = (void*)node;
		}
	}

	return node->ptr;
}

struct graphLayer* graphLayer_create(struct graph* master, uint32_t node_data_size, uint32_t edge_data_size){
	struct graphLayer* graph_layer;

	if ((graph_layer = (struct graphLayer*)malloc(sizeof(struct graphLayer))) != NULL){
		graphLayer_init(graph_layer, master, node_data_size, edge_data_size);
	}
	else{
		log_err("unable to allocate memory");
	}

	return graph_layer;
}

void graphLayer_init(struct graphLayer* graph_layer, struct graph* master, uint32_t node_data_size, uint32_t edge_data_size){
	struct node* node_cursor;

	graph_init(&(graph_layer->layer), node_data_size, edge_data_size)
	graph_layer->master = master;

	for (node_cursor = graph_get_head_node(master); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
		node_cursor->ptr = NULL;
	}
}

void graphLayer_remap_mas_lay(struct graphLayer* graph_layer){
	struct node* node_cursor;

	for (node_cursor = graph_get_head_node(graph_layer->master); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
		((struct node*)(node_cursor->ptr))->ptr = node_cursor;
	}
}

void graphLayer_remap_lay_mas(struct graphLayer* graph_layer){
	struct node* node_cursor;

	for (node_cursor = graph_get_head_node(&(graph_layer->layer)); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
		((struct node*)(node_cursor->ptr))->ptr = node_cursor;
	}
}

struct edge* graphLayer_add_edge_(struct graphLayer* graph_layer, struct node* node_src, struct node* node_dst){
	struct node* node_layer_src;
	struct node* node_layer_dst;

	node_layer_src = graphLayer_get_node(graph_layer, node_src);
	node_layer_dst = graphLayer_get_node(graph_layer, node_dst);

	if (node_layer_src == NULL || node_layer_dst == NULL){
		log_err("unable to get node");
		return NULL;
	}

	return graph_add_edge_(&(graph_layer->layer), node_layer_src, node_layer_dst);
}

struct edge* graphLayer_add_edge(struct graphLayer* graph_layer, struct node* node_src, struct node* node_dst, void* data){
	struct node* node_layer_src;
	struct node* node_layer_dst;

	node_layer_src = graphLayer_get_node(graph_layer, node_src);
	node_layer_dst = graphLayer_get_node(graph_layer, node_dst);

	if (node_layer_src == NULL || node_layer_dst == NULL){
		log_err("unable to get node");
		return NULL;
	}

	return graph_add_edge(&(graph_layer->layer), node_layer_src, node_layer_dst, data);
}

int32_t graphLayer_copy_src_edge(struct graphLayer* graph_layer, struct node* node1, struct node* node2){
	struct node* node_layer_1;
	struct node* node_layer_2;

	node_layer_1 = graphLayer_get_node(graph_layer, node1);
	node_layer_2 = graphLayer_get_node(graph_layer, node2);

	if (node_layer_1 == NULL || node_layer_2 == NULL){
		log_err("unable to get node");
		return -1;
	}

	return graph_copy_src_edge(&(graph_layer->layer), node_layer_1, node_layer_2);
}

int32_t graphLayer_copy_dst_edge(struct graphLayer* graph_layer, struct node* node1, struct node* node2){
	struct node* node_layer_1;
	struct node* node_layer_2;

	node_layer_1 = graphLayer_get_node(graph_layer, node1);
	node_layer_2 = graphLayer_get_node(graph_layer, node2);

	if (node_layer_1 == NULL || node_layer_2 == NULL){
		log_err("unable to get node");
		return -1;
	}

	return graph_copy_dst_edge(&(graph_layer->layer), node_layer_1, node_layer_2);
}

void graphLayer_remove_master_node(struct graphLayer* graph_layer, struct node* node){
	if (node->ptr != NULL){
		graph_remove_node(&(graph_layer->layer), node->ptr);
	}
	graph_remove_node(graph_layer->master, node);
}
