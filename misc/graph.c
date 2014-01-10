#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "graph.h"

void* bsearch_r(const void* key, const void* base, size_t num, size_t size, __compar_d_fn_t compare, void* arg);

/* ===================================================================== */
/* Graph function(s) 	                                                 */
/* ===================================================================== */

struct graph* graph_create(struct graphCallback* callback){
	struct graph* graph;

	graph = (struct graph*)malloc(sizeof(struct graph));
	if (graph != NULL){
		if (graph_init(graph, callback)){
			free(graph);
			graph = NULL;
		}
	}

	return graph;
}

int32_t graph_init(struct graph* graph, struct graphCallback* callback){
	if (array_init(&(graph->node_array), sizeof(struct node))){
		printf("ERROR: in %s, unable to init node array\n", __func__);
		return -1;
	}
	if (array_init(&(graph->edge_array), sizeof(struct edge))){
		printf("ERROR: in %s, unable to init edge array\n", __func__);
		array_clean(&(graph->node_array));
		return -1;
	}

	if (callback != NULL){
		memcpy(&(graph->callback), callback, sizeof(struct graphCallback));
	}
	else{
		memset(&(graph->callback), 0, sizeof(struct graphCallback));
	}

	graph->mapping_head = NULL;
	graph->mapping_tail = NULL;
	graph->special_mapping_edge_src = NULL;
	graph->special_mapping_edge_dst = NULL;

	return 0;
}

int32_t graph_add_node(struct graph* graph, void* data){
	struct node node;

	node.id = array_get_length(&(graph->node_array));
	node.nb_src = 0;
	node.nb_dst = 0;
	node.min_src_parent = -1;
	node.min_dst_parent = -1;
	node.data = data;

	return array_add(&(graph->node_array), &node);
}

int32_t graph_add_edge(struct graph* graph, int32_t node_src, int32_t node_dst, void* data){
	struct edge 	edge;
	int32_t 		result;
	struct node* 	src;
	struct node* 	dst;

	edge.id_src = node_src;
	edge.id_dst = node_dst;
	edge.data = data;

	result = array_add(&(graph->edge_array), &edge);
	if (result >= 0){
		src = (struct node*)array_get(&(graph->node_array), node_src);
		dst = (struct node*)array_get(&(graph->node_array), node_dst);

		src->nb_src ++;
		dst->nb_dst ++;

		if (src->min_dst_parent == -1){
			src->min_dst_parent = node_dst;
		}
		else{
			src->min_dst_parent = (node_dst < src->min_dst_parent) ? node_dst : src->min_dst_parent;
		}

		if (dst->min_src_parent == -1){
			dst->min_src_parent = node_src;
		}
		else{
			dst->min_src_parent = (node_src < dst->min_src_parent) ? node_src : dst->min_src_parent;
		}
	}
	return result;
}

int32_t graph_get_edge(struct graph* graph, int32_t node_src, int32_t node_dst){
	struct edge edge;

	if (graph->special_mapping_edge_src == NULL){
		graph->special_mapping_edge_src = graphMapping_create(graph, GRAPH_MAPPING_TYPE_EDGE, (int32_t(*)(const void*,const void*,void*))edge_sort_src_dst, (int32_t(*)(const void*,const void*,void*))edge_search_src_dst);
		if (graph->special_mapping_edge_src == NULL){
			printf("ERROR: in %s, unable to create mapping\n", __func__);
			return -1;
		}
	}

	edge.id_src = node_src;
	edge.id_dst = node_dst;

	return graphMapping_search(graph, graph->special_mapping_edge_src, &edge);
}

void graph_clean(struct graph* graph){
	uint32_t i;

	while(graph->mapping_head != NULL){
		graphMapping_delete(graph, graph->mapping_head);
	}

	if (graph->callback.node_clean != NULL){
		for (i = 0; i < graph_get_nb_node(graph); i++){
			graph->callback.node_clean(node_get_data(graph, i));
		}
	}
	if (graph->callback.edge_clean != NULL){
		for (i = 0; i < graph_get_nb_edge(graph); i++){
			graph->callback.edge_clean(edge_get_data(graph, i));
		}
	}
	array_clean(&(graph->node_array));
	array_clean(&(graph->edge_array));
}

void graph_delete(struct graph* graph){
	if (graph != NULL){
		graph_clean(graph);
		free(graph);
	}
}

/* ===================================================================== */
/* Node function(s) 	                                                 */
/* ===================================================================== */

int32_t node_get_src_edge(struct graph* graph, int32_t node, uint32_t index){
	struct edge 	edge;
	uint32_t* 		base;

	if (graph->special_mapping_edge_src == NULL){
		graph->special_mapping_edge_src = graphMapping_create(graph, GRAPH_MAPPING_TYPE_EDGE, (int32_t(*)(const void*,const void*,void*))edge_sort_src_dst, (int32_t(*)(const void*,const void*,void*))edge_search_src_dst);
		if (graph->special_mapping_edge_src == NULL){
			printf("ERROR: in %s, unable to create mapping\n", __func__);
			return -1;
		}
	}

	edge.id_src = node;
	edge.id_dst = ((struct node*)array_get(&(graph->node_array), node))->min_dst_parent;

	base = graphMapping_search_(graph, graph->special_mapping_edge_src, &edge);
	if (base == NULL){
		return -1;
	}
	else{
		return *(base + index);
	}
}

int32_t node_get_dst_edge(struct graph* graph, int32_t node, uint32_t index){
	struct edge 	edge;
	uint32_t* 		base;

	if (graph->special_mapping_edge_dst == NULL){
		graph->special_mapping_edge_dst = graphMapping_create(graph, GRAPH_MAPPING_TYPE_EDGE, (int32_t(*)(const void*,const void*,void*))edge_sort_dst_src, (int32_t(*)(const void*,const void*,void*))edge_search_dst_src);
		if (graph->special_mapping_edge_dst == NULL){
			printf("ERROR: in %s, unable to create mapping\n", __func__);
			return -1;
		}
	}
	
	edge.id_src = ((struct node*)array_get(&(graph->node_array), node))->min_src_parent;
	edge.id_dst = node;

	base = graphMapping_search_(graph, graph->special_mapping_edge_dst, &edge);
	if (base == NULL){
		return -1;
	}
	else{
		return *(base + index);
	}
}

/* ===================================================================== */
/* Edge function(s) 	                                                 */
/* ===================================================================== */


int32_t edge_sort_src_dst(uint32_t* edge1, uint32_t* edge2, struct graph* graph){
	uint32_t edge1_src;
	uint32_t edge2_src;

	edge1_src = edge_get_src(graph, *edge1);
	edge2_src = edge_get_src(graph, *edge2);

	if (edge1_src > edge2_src){
		return 1;
	}
	else if (edge1_src < edge2_src){
		return -1;
	}
	else{
		return (int32_t)edge_get_dst(graph, *edge1) - (int32_t)edge_get_dst(graph, *edge2);
	}
}

int32_t edge_sort_dst_src(uint32_t* edge1, uint32_t* edge2, struct graph* graph){
	uint32_t edge1_dst;
	uint32_t edge2_dst;

	edge1_dst = edge_get_dst(graph, *edge1);
	edge2_dst = edge_get_dst(graph, *edge2);

	if (edge1_dst > edge2_dst){
		return 1;
	}
	else if (edge1_dst < edge2_dst){
		return -1;
	}
	else{
		return (int32_t)edge_get_src(graph, *edge1) - (int32_t)edge_get_src(graph, *edge2);
	}
}

int32_t edge_search_src_dst(struct edge* key, uint32_t* edge, struct graph* graph){
	uint32_t edge_src;

	edge_src = edge_get_src(graph, *edge);

	if (key->id_src > edge_src){
		return 1;
	}
	else if (key->id_src < edge_src){
		return -1;
	}
	else{
		return (int32_t)key->id_dst - (int32_t)edge_get_dst(graph, *edge);
	}
}

int32_t edge_search_dst_src(struct edge* key, uint32_t* edge, struct graph* graph){
	uint32_t edge_dst;

	edge_dst = edge_get_dst(graph, *edge);

	if (key->id_dst > edge_dst){
		return 1;
	}
	else if (key->id_dst < edge_dst){
		return -1;
	}
	else{
		return (int32_t)key->id_src - (int32_t)edge_get_src(graph, *edge);
	}
}

/* ===================================================================== */
/* Mapping function(s) 	                                                 */
/* ===================================================================== */

struct graphMapping* graphMapping_create(struct graph* graph, enum graphMappingType type, int32_t(*compare)(const void*,const void*,void*), int32_t(*search)(const void*,const void*,void*)){
	struct graphMapping* mapping;

	mapping = (struct graphMapping*)malloc(sizeof(struct graphMapping));
	if (mapping == NULL){
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
	}
	else{
		mapping->type 		= type;
		mapping->compare 	= compare;
		mapping->search 	= search;
		mapping->map 		= NULL;
		mapping->size 		= 0;
		mapping->prev 		= graph->mapping_tail;
		mapping->next 		= NULL;

		if (graphMapping_map(graph, mapping)){
			printf("ERROR: in %s, unable to map\n", __func__);
			free(mapping);
			mapping = NULL;
		}
		else{
			if (graph->mapping_tail == NULL){
				graph->mapping_head = mapping;
			}
			else{
				graph->mapping_tail->next = mapping;
			}

			graph->mapping_tail = mapping;
		}
	}

	return mapping;
}

int32_t graphMapping_map(struct graph* graph, struct graphMapping* mapping){
	uint32_t 	new_size;
	uint32_t* 	new_map;
	uint32_t 	i;

	switch(mapping->type){
	case GRAPH_MAPPING_TYPE_NODE 		: {new_size = graph_get_nb_node(graph); break;}
	case GRAPH_MAPPING_TYPE_EDGE 		: {new_size = graph_get_nb_edge(graph); break;}
	}

	if (new_size != mapping->size){
		new_map = (uint32_t*)realloc(mapping->map, sizeof(uint32_t) * new_size);
		if (new_map == NULL){
			printf("ERROR: in %s, unable to realloc map\n", __func__);
			return -1;
		}

		for (i = mapping->size; i < new_size; i++){
			new_map[i] = i;
		}

		mapping->map = new_map;
		mapping->size = new_size;

		qsort_r(mapping->map, mapping->size, sizeof(uint32_t), (__compar_d_fn_t)mapping->compare, graph);
	}

	return 0;
}

int32_t graphMapping_search(struct graph* graph, struct graphMapping* mapping, const void* key){
	void*		ptr;
	uint32_t 	result = -1;

	if (graphMapping_is_valid(graph, mapping)){
		if (graphMapping_map(graph, mapping)){
			printf("ERROR: in %s, unable to recompute mapping\n", __func__);
			return -1;
		}
	}

	ptr = bsearch_r(key, mapping->map, mapping->size, sizeof(uint32_t), (__compar_d_fn_t)mapping->search, graph);
	if (ptr != NULL){
		result = *(uint32_t*)ptr;
	}
	return result;
}

uint32_t* graphMapping_search_(struct graph* graph, struct graphMapping* mapping, const void* key){
	if (graphMapping_is_valid(graph, mapping)){
		if (graphMapping_map(graph, mapping)){
			printf("ERROR: in %s, unable to recompute mapping\n", __func__);
			return NULL;
		}
	}

	return (uint32_t*)bsearch_r(key, mapping->map, mapping->size, sizeof(uint32_t), (__compar_d_fn_t)mapping->search, graph);
}

void graphMapping_print(struct graph* graph, struct graphMapping* mapping){
	uint32_t i;

	switch(mapping->type){
	case GRAPH_MAPPING_TYPE_NODE : {
		for (i = 0; i < mapping->size; i++){
			printf("Node %u\n", mapping->map[i]);
		}
		break;
	}
	case GRAPH_MAPPING_TYPE_EDGE : {
		for (i = 0; i < mapping->size; i++){
			printf("Edge %u -> %u\n", edge_get_src(graph, mapping->map[i]), edge_get_dst(graph, mapping->map[i]));
		}
		break;
	}
	default : {
		printf("ERROR: in %s, this case is suppose to happen\n", __func__);
	}
	}
}

void graphMapping_delete(struct graph* graph, struct graphMapping* mapping){
	if (mapping != NULL){
		if (mapping->prev == NULL){
			graph->mapping_head = mapping->next;
		}
		else{
			mapping->prev->next = mapping->next;
		}
		if (mapping->next == NULL){
			graph->mapping_tail = mapping->prev;
		}
		else{
			mapping->next->prev = mapping->prev;
		}

		free(mapping->map);
		free(mapping);
	}
}

void* bsearch_r(const void* key, const void* base, size_t num, size_t size, __compar_d_fn_t compare, void* arg){
	size_t 		low = 0;
	size_t 		up = num;
	size_t 		idx;
	const void 	*p;
	int 		comparison;

	while (low < up){
		idx = (low + up) / 2;
		p = (void *) (((const char *) base) + (idx * size));
		comparison = (*compare)(key, p, arg);
		if (comparison < 0){
			up = idx;
		}
		else if (comparison > 0){
			low = idx + 1;
		}
		else{
			return (void*)p;
		}
	}

	return NULL;
}