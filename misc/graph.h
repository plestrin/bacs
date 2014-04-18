#ifndef GRAPH_H
#define GRAPH_H

#include <stdint.h>
#include <stdio.h>

#define GRAPH_DATA_PADDING_ALIGNEMENT 4

/* One can use a better allocator */
#define graph_allocate_edge(graph) 		(struct edge*)malloc(sizeof(struct edge) + ((graph)->edge_data_size - GRAPH_DATA_PADDING_ALIGNEMENT))
#define graph_allocate_node(graph)		(struct node*)malloc(sizeof(struct node) + ((graph)->node_data_size - GRAPH_DATA_PADDING_ALIGNEMENT))
#define graph_free_edge(graph, edge) 	free(edge)
#define graph_free_node(graph, node) 	free(node)

struct edge{
	struct node* 	src_node;
	struct node* 	dst_node;
	struct edge* 	src_prev;
	struct edge* 	src_next;
	struct edge* 	dst_prev;
	struct edge* 	dst_next;
	char 			data[GRAPH_DATA_PADDING_ALIGNEMENT];
};

struct node{
	struct node* 	prev;
	struct node* 	next;
	uint32_t 		nb_edge_src;
	uint32_t 		nb_edge_dst;
	struct edge*	src_edge_linkedList;
	struct edge*	dst_edge_linkedList;
	char 			data[GRAPH_DATA_PADDING_ALIGNEMENT];
};

struct graph{
	struct node* 	node_linkedList;
	uint32_t 		nb_node;
	uint32_t 		nb_edge;
	uint32_t 		node_data_size;
	uint32_t 		edge_data_size;

	void(*dotPrint_node_data)(void*,FILE*);
	void(*dotPrint_edge_data)(void*,FILE*);
};

struct graph* graph_create(uint32_t node_data_size, uint32_t edge_data_size);

#define graph_init(graph, node_data_size_, edge_data_size_)																						\
	(graph)->node_linkedList 	= NULL; 																										\
	(graph)->nb_node 			= 0; 																											\
	(graph)->nb_edge 			= 0; 																											\
	(graph)->node_data_size 	= ((node_data_size_) > GRAPH_DATA_PADDING_ALIGNEMENT) ? (node_data_size_) : GRAPH_DATA_PADDING_ALIGNEMENT; 		\
	(graph)->edge_data_size 	= ((edge_data_size_) > GRAPH_DATA_PADDING_ALIGNEMENT) ? (edge_data_size_) : GRAPH_DATA_PADDING_ALIGNEMENT; 		\
	(graph)->dotPrint_node_data = NULL; 																										\
	(graph)->dotPrint_edge_data = NULL;

#define graph_register_dotPrint_callback(graph, node_data, edge_data) 																			\
	(graph)->dotPrint_node_data = node_data; 																									\
	(graph)->dotPrint_edge_data = edge_data;

struct node* graph_add_node_(struct graph* graph);
struct node* graph_add_node(struct graph* graph, void* data);
void graph_remove_node(struct graph* graph, struct node* node);

#define graph_get_head_node(graph) 		((graph)->node_linkedList)
#define node_get_next(node) 			((node)->next)

struct edge* graph_add_edge_(struct graph* graph, struct node* node_src, struct node* node_dst);
struct edge* graph_add_edge(struct graph* graph, struct node* node_src, struct node* node_dst, void* data);
void graph_remove_edge(struct graph* graph, struct edge* edge);

#define node_get_head_edge_src(node) 	((node)->src_edge_linkedList)
#define node_get_head_edge_dst(node) 	((node)->dst_edge_linkedList)
#define edge_get_next_src(edge) 		((edge)->src_next)
#define edge_get_next_dst(edge) 		((edge)->dst_next)

#define graph_clean(graph) 																														\
	while((graph)->node_linkedList != NULL){ 																									\
		graph_remove_node((graph), (graph)->node_linkedList); 																					\
	}

#define graph_delete(graph) 																													\
	graph_clean(graph); 																														\
	free(graph);

#endif