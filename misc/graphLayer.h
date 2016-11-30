#ifndef GRAPHLAYER_H
#define GRAPHLAYER_H

#include "graph.h"

#define node_has_layer_counterpart(node_) ((node_)->ptr != NULL)
#define node_get_layer_counterpart(node_) ((struct node*)(node_)->ptr)

struct graphLayer{
	struct graph 	layer;
	struct graph* 	master;
};

struct graphLayer* graphLayer_create(struct graph* master, uint32_t node_data_size, uint32_t edge_data_size);
void graphLayer_init(struct graphLayer* graph_layer, struct graph* master, uint32_t node_data_size, uint32_t edge_data_size);

void graphLayer_remap_mas_lay(struct graphLayer* graph_layer);
void graphLayer_remap_lay_mas(struct graphLayer* graph_layer);

struct edge* graphLayer_add_edge_(struct graphLayer* graph_layer, struct node* node_src, struct node* node_dst);
struct edge* graphLayer_add_edge(struct graphLayer* graph_layer, struct node* node_src, struct node* node_dst, void* data);

static inline struct edge* graphLayer_node_get_head_edge_src(struct node* node){
	if (node->ptr == NULL){
		return NULL;
	}
	else{
		return node_get_head_edge_src((struct node*)(node->ptr));
	}
}

static inline struct edge* graphLayer_node_get_head_edge_dst(struct node* node){
	if (node->ptr == NULL){
		return NULL;
	}
	else{
		return node_get_head_edge_dst((struct node*)(node->ptr));
	}
}

#define graphLayer_edge_get_src(edge) ((struct node*)((edge)->src_node->ptr))
#define graphLayer_edge_get_dst(edge) ((struct node*)((edge)->dst_node->ptr))

int32_t graphLayer_copy_src_edge(struct graphLayer* graph_layer, struct node* node1, struct node* node2);
int32_t graphLayer_copy_dst_edge(struct graphLayer* graph_layer, struct node* node1, struct node* node2);

void graphLayer_remove_master_node(struct graphLayer* graph_layer, struct node* node);

#define graphLayer_clean(graph_layer) graph_clean(&((graph_layer)->layer))

#define graphLayer_delete(graph_layer) 																									\
	graphLayer_clean(graph_layer); 																										\
	free(graph_layer);

#endif
