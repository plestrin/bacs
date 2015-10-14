#ifndef GRAPH_H
#define GRAPH_H

#include <stdint.h>
#include <stdio.h>

/* One can use a better allocator */
#define graph_allocate_edge(graph) 		(struct edge*)malloc(sizeof(struct edge) + (graph)->edge_data_size)
#define graph_allocate_node(graph)		(struct node*)malloc(sizeof(struct node) + (graph)->node_data_size)
#define graph_free_edge(graph, edge) 	free(edge)
#define graph_free_node(graph, node) 	free(node)

struct edge{
	struct node* 	src_node;
	struct node* 	dst_node;
	struct edge* 	src_prev;
	struct edge* 	src_next;
	struct edge* 	dst_prev;
	struct edge* 	dst_next;
};

#define edge_get_data(edge_) ((void*)((struct edge*)(edge_) + 1))

#define edge_get_src(edge) ((edge)->src_node)
#define edge_get_dst(edge) ((edge)->dst_node)

struct node{
	struct node* 	prev;
	struct node* 	next;
	uint32_t 		nb_edge_src;
	uint32_t 		nb_edge_dst;
	struct edge*	src_edge_linkedList;
	struct edge*	dst_edge_linkedList;
	void* 			ptr; 									/* Used in various algorithm: dijktra, isomosphism, dag partial ordering, graph copy */
};

#define node_get_data(node_) ((void*)((struct node*)(node_) + 1))

struct graph{
	struct node* 	node_linkedList_head;
	struct node* 	node_linkedList_tail;
	uint32_t 		nb_node;
	uint32_t 		nb_edge;
	uint32_t 		node_data_size;
	uint32_t 		edge_data_size;

	void(*dotPrint_prologue)(FILE*,void*);
	void(*dotPrint_node_data)(void*,FILE*,void*);
	void(*dotPrint_edge_data)(void*,FILE*,void*);
	void(*dotPrint_epilogue)(FILE*,void*);

	void(*node_clean_data)(struct node* edge);
	void(*edge_clean_data)(struct edge* edge);
};

struct graph* graph_create(uint32_t node_data_size, uint32_t edge_data_size);

#define graph_init(graph, node_data_size_, edge_data_size_)																				\
	(graph)->node_linkedList_head 	= NULL; 																							\
	(graph)->node_linkedList_tail 	= NULL; 																							\
	(graph)->nb_node 				= 0; 																								\
	(graph)->nb_edge 				= 0; 																								\
	(graph)->node_data_size 		= node_data_size_; 																					\
	(graph)->edge_data_size 		= edge_data_size_; 																					\
	(graph)->dotPrint_prologue 		= NULL; 																							\
	(graph)->dotPrint_node_data 	= NULL; 																							\
	(graph)->dotPrint_edge_data 	= NULL; 																							\
	(graph)->dotPrint_epilogue 		= NULL; 																							\
	(graph)->node_clean_data 		= NULL; 																							\
	(graph)->edge_clean_data 		= NULL;

#define graph_register_dotPrint_callback(graph, prologue, node_data, edge_data, epilogue) 												\
	(graph)->dotPrint_prologue 	= (prologue); 																							\
	(graph)->dotPrint_node_data = (node_data); 																							\
	(graph)->dotPrint_edge_data = (edge_data); 																							\
	(graph)->dotPrint_epilogue 	= (epilogue);

#define graph_register_node_clean_call_back(graph, callback) ((graph)->node_clean_data = callback)
#define graph_register_edge_clean_call_back(graph, callback) ((graph)->edge_clean_data = callback)

struct node* graph_add_node_(struct graph* graph);
struct node* graph_add_node(struct graph* graph, void* data);

struct node* graph_insert_node_(struct graph* graph, struct node* root);
struct node* graph_insert_node(struct graph* graph, struct node* root, void* data);

void graph_transfert_src_edge(struct graph* graph, struct node* node1, struct node* node2);
void graph_transfert_dst_edge(struct graph* graph, struct node* node1, struct node* node2);

#define graph_merge_node(graph, node1, node2) 																							\
	graph_transfert_src_edge(graph, node1, node2); 																						\
	graph_transfert_dst_edge(graph, node1, node2);

int32_t graph_copy_src_edge(struct graph* graph, struct node* node1, struct node* node2);
int32_t graph_copy_dst_edge(struct graph* graph, struct node* node1, struct node* node2);

void graph_remove_node(struct graph* graph, struct node* node);

#define graph_get_head_node(graph) 		((graph)->node_linkedList_head)
#define node_get_next(node) 			((node)->next)
#define node_get_prev(node) 			((node)->prev)
#define graph_get_tail_node(graph) 		((graph)->node_linkedList_tail)

struct edge* graph_add_edge_(struct graph* graph, struct node* node_src, struct node* node_dst);
struct edge* graph_add_edge(struct graph* graph, struct node* node_src, struct node* node_dst, void* data);
struct edge* graph_get_edge(struct node* node_src, struct node* node_dst);
void graph_remove_edge(struct graph* graph, struct edge* edge);

#define node_get_head_edge_src(node) 	((node)->src_edge_linkedList)
#define node_get_head_edge_dst(node) 	((node)->dst_edge_linkedList)
#define edge_get_next_src(edge) 		((edge)->src_next)
#define edge_get_next_dst(edge) 		((edge)->dst_next)
#define edge_get_prev_src(edge) 		((edge)->src_prev)
#define edge_get_prev_dst(edge) 		((edge)->dst_prev)

int32_t graph_copy(struct graph* graph_dst, struct graph* graph_src, int32_t(*node_copy)(void*,const void*,void*), int32_t(*edge_copy)(void*,const void*,void*), void* arg);

struct graph* graph_clone(struct graph* graph_src, int32_t(*node_copy)(void*,const void*,void*), int32_t(*edge_copy)(void*,const void*,void*), void* arg);
int32_t graph_concat(struct graph* graph_dst, struct graph* graph_src, int32_t(*node_copy)(void*,const void*,void*), int32_t(*edge_copy)(void*,const void*,void*), void* arg);

#define graph_clean(graph) 																												\
	while((graph)->node_linkedList_head != NULL){ 																						\
		graph_remove_node((graph), (graph)->node_linkedList_head); 																		\
	}

#define graph_delete(graph) 																											\
	graph_clean(graph); 																												\
	free(graph);

#endif