#ifndef GRAPH_H
#define GRAPH_H

#include <stdint.h>
#include <stdio.h>

#include "array.h"

enum graphMappingType{
	GRAPH_MAPPING_TYPE_NODE,
	GRAPH_MAPPING_TYPE_EDGE
};

struct graphMapping{
	struct graphMapping* 	next;
	struct graphMapping* 	prev;
	enum graphMappingType 	type;
	int32_t (*compare)(const void*, const void*, void*);
	int32_t (*search)(const void*, const void*, void*);
	uint32_t* 				map;
	uint32_t 				size;
};

struct graphCallback{
	void(*node_print_dot)(void*, FILE*);
	void(*edge_print_dot)(void*, FILE*);
	void(*node_clean)(void*);
	void(*edge_clean)(void*);
};

struct graph{
	struct array 			node_array;
	struct array 			edge_array;
	struct graphCallback	callback;
	struct graphMapping* 	mapping_head;
	struct graphMapping* 	mapping_tail;
	struct graphMapping* 	special_mapping_edge_src;
	struct graphMapping* 	special_mapping_edge_dst;
};

struct node{
	uint32_t 	id;
	uint32_t 	nb_src;
	uint32_t 	nb_dst;
	int32_t 	min_src_parent;
	int32_t 	min_dst_parent;
	void* 		data;
};

struct edge{
	uint32_t 	id_src;
	uint32_t 	id_dst;
	void* 		data;
};

struct graph* graph_create(struct graphCallback* callback);
int32_t graph_init(struct graph* graph, struct graphCallback* callback);
int32_t graph_add_node(struct graph* graph, void* data);
int32_t graph_add_edge(struct graph* graph, int32_t node_src, int32_t node_dst, void* data);

static inline uint32_t graph_get_nb_node(struct graph* graph){
	return array_get_length(&(graph->node_array));
}

static inline uint32_t graph_get_nb_edge(struct graph* graph){
	return array_get_length(&(graph->edge_array));
}

void graph_clean(struct graph* graph);
void graph_delete(struct graph* graph);

static inline void* node_get_data(struct graph* graph, int32_t node){
	return ((struct node*)array_get(&(graph->node_array), node))->data;
}

static inline uint32_t node_get_nb_src(struct graph* graph, int32_t node){
	return ((struct node*)array_get(&(graph->node_array), node))->nb_src;
}

static inline uint32_t node_get_nb_dst(struct graph* graph, int32_t node){
	return ((struct node*)array_get(&(graph->node_array), node))->nb_dst;
}

int32_t node_get_src_edge(struct graph* graph, int32_t node, uint32_t index);
int32_t node_get_dst_edge(struct graph* graph, int32_t node, uint32_t index);

static inline void* edge_get_data(struct graph* graph, int32_t edge){
	return ((struct edge*)array_get(&(graph->edge_array), edge))->data;
}

static inline uint32_t edge_get_src(struct graph* graph, int32_t edge){
	return ((struct edge*)array_get(&(graph->edge_array), edge))->id_src;
}

static inline uint32_t edge_get_dst(struct graph* graph, int32_t edge){
	return ((struct edge*)array_get(&(graph->edge_array), edge))->id_dst;
}

int32_t edge_sort_src_dst(uint32_t* edge1, uint32_t* edge2, struct graph* graph);
int32_t edge_sort_dst_src(uint32_t* edge1, uint32_t* edge2, struct graph* graph);

int32_t edge_search_src_dst(struct edge* key, uint32_t* edge, struct graph* graph);
int32_t edge_search_dst_src(struct edge* key, uint32_t* edge, struct graph* graph);

struct graphMapping* graphMapping_create(struct graph* graph, enum graphMappingType type, int32_t(*compare)(const void*,const void*,void*), int32_t(*search)(const void*,const void*,void*));
int32_t graphMapping_map(struct graph* graph, struct graphMapping* mapping);
int32_t graphMapping_search(struct graph* graph, struct graphMapping* mapping, const void* key);
uint32_t* graphMapping_search_(struct graph* graph, struct graphMapping* mapping, const void* key);
void graphMapping_print(struct graph* graph, struct graphMapping* mapping);

static inline int32_t graphMapping_is_valid(struct graph* graph, struct graphMapping* mapping){
	int32_t result = -1;

	switch(mapping->type){
	case GRAPH_MAPPING_TYPE_NODE : {result =  mapping->size - graph_get_nb_node(graph); break;}
	case GRAPH_MAPPING_TYPE_EDGE : {result =  mapping->size - graph_get_nb_edge(graph); break;}
	}

	return result;
}

void graphMapping_delete(struct graph* graph, struct graphMapping* mapping);


#endif