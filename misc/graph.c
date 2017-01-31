#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "graph.h"
#include "base.h"

/* One can implement special callback mecanisms here */
#define graph_node_set_data(graph, node, data_) 	memcpy(node_get_data(node), (data_), (graph)->node_data_size);
#define graph_edge_set_data(graph, edge, data_) 	memcpy(edge_get_data(edge), (data_), (graph)->edge_data_size);
#define graph_node_clean_data(graph, node) 			if ((graph)->node_clean_data != NULL){(graph)->node_clean_data(node);}
#define graph_edge_clean_data(graph, edge) 			if ((graph)->edge_clean_data != NULL){(graph)->edge_clean_data(edge);}

#define graph_connect_node(graph, node) 								\
	(node)->nb_edge_src 			= 0; 								\
	(node)->nb_edge_dst 			= 0; 								\
	(node)->next 					= (graph)->node_linkedList_head; 	\
	(node)->prev 					= NULL; 							\
	(node)->src_edge_linkedList 	= NULL; 							\
	(node)->dst_edge_linkedList 	= NULL; 							\
	(node)->ptr 					= NULL; 							\
																		\
	if ((node)->next != NULL){ 											\
		(node)->next->prev = (node); 									\
	} 																	\
	else{ 																\
		(graph)->node_linkedList_tail = (node); 						\
	} 																	\
																		\
	(graph)->node_linkedList_head = (node); 							\
	(graph)->nb_node ++;

#define graph_connect_edge(graph, node_src, node_dst, edge) 			\
	(edge)->src_node 	= (node_src); 									\
	(edge)->dst_node 	= (node_dst); 									\
	(edge)->src_prev 	= NULL; 										\
	(edge)->src_next 	= (node_src)->src_edge_linkedList; 				\
	(edge)->dst_prev 	= NULL; 										\
	(edge)->dst_next 	= (node_dst)->dst_edge_linkedList; 				\
	(edge)->ptr 		= NULL; 										\
																		\
	if ((edge)->src_next != NULL){ 										\
		(edge)->src_next->src_prev = (edge); 							\
	} 																	\
																		\
	if ((edge)->dst_next != NULL){ 										\
		(edge)->dst_next->dst_prev = (edge); 							\
	} 																	\
																		\
	(node_src)->src_edge_linkedList = (edge); 							\
	(node_src)->nb_edge_src ++; 										\
																		\
	(node_dst)->dst_edge_linkedList = (edge); 							\
	(node_dst)->nb_edge_dst ++; 										\
																		\
	(graph)->nb_edge ++;


struct graph* graph_create(uint32_t node_data_size, uint32_t edge_data_size){
	struct graph* graph;

	graph = (struct graph*)malloc(sizeof(struct graph));
	if (graph != NULL){
		graph_init(graph, node_data_size, edge_data_size);
	}
	else{
		log_err("unable to allocate memory");
	}

	return graph;
}

struct node* graph_add_node_(struct graph* graph){
	struct node* node;

	node = graph_allocate_node(graph);
	if (node == NULL){
		log_err("unable to allocate node");
	}
	else{
		graph_connect_node(graph, node)
	}

	return node;
}

struct node* graph_add_node(struct graph* graph, void* data){
	struct node* node;

	node = graph_allocate_node(graph);
	if (node == NULL){
		log_err("unable to allocate node");
	}
	else{
		graph_node_set_data(graph, node, data)
		graph_connect_node(graph, node)
	}

	return node;
}

struct node* graph_insert_node_(struct graph* graph, struct node* root){
	struct node* node;

	node = graph_allocate_node(graph);
	if (node == NULL){
		log_err("unable to allocate node");
	}
	else{
		node->nb_edge_src 			= 0;
		node->nb_edge_dst 			= 0;
		node->next 					= root->next;
		node->prev 					= root;
		node->src_edge_linkedList 	= NULL;
		node->dst_edge_linkedList 	= NULL;
		node->ptr 					= NULL;

		if (node->next != NULL){
			node->next->prev = node;
		}
		else{
			graph->node_linkedList_tail = node;
		}

		root->next = node;
		graph->nb_node ++;
	}

	return node;
}

struct node* graph_insert_node(struct graph* graph, struct node* root, void* data){
	struct node* node;

	node = graph_insert_node_(graph, root);
	if (node != NULL){
		graph_node_set_data(graph, node, data)
	}
	else{
		log_err("node is NULL");
	}

	return node;
}

void graph_transfert_src_edge(struct graph* graph, struct node* node1, struct node* node2){
	struct edge* edge_cursor;
	struct edge* edge_current;

	edge_cursor = node_get_head_edge_src(node2);
	while (edge_cursor != NULL){
		edge_current = edge_cursor;
		edge_cursor = edge_get_next_src(edge_cursor);

		if (edge_get_dst(edge_current) == node1){
			graph_remove_edge(graph, edge_current);
		}
		else{
			if (edge_current->src_prev == NULL){
				node2->src_edge_linkedList = edge_current->src_next;
			}
			else{
				edge_current->src_prev->src_next = edge_current->src_next;
			}
			if (edge_current->src_next != NULL){
				edge_current->src_next->src_prev = edge_current->src_prev;
			}

			edge_current->src_node = node1;
			edge_current->src_prev = NULL;
			edge_current->src_next = node1->src_edge_linkedList;

			if (edge_current->src_next != NULL){
				edge_current->src_next->src_prev = edge_current;
			}

			node2->nb_edge_src --;
			node1->nb_edge_src ++;
			node1->src_edge_linkedList = edge_current;
		}
	}
}

void graph_transfert_dst_edge(struct graph* graph, struct node* node1, struct node* node2){
	struct edge* edge_cursor;
	struct edge* edge_current;

	edge_cursor = node_get_head_edge_dst(node2);
	while (edge_cursor != NULL){
		edge_current = edge_cursor;
		edge_cursor = edge_get_next_dst(edge_cursor);

		if (edge_get_src(edge_current) == node1){
			graph_remove_edge(graph, edge_current);
		}
		else{
			if (edge_current->dst_prev == NULL){
				node2->dst_edge_linkedList = edge_current->dst_next;
			}
			else{
				edge_current->dst_prev->dst_next = edge_current->dst_next;
			}
			if (edge_current->dst_next != NULL){
				edge_current->dst_next->dst_prev = edge_current->dst_prev;
			}

			edge_current->dst_node = node1;
			edge_current->dst_prev = NULL;
			edge_current->dst_next = node1->dst_edge_linkedList;

			if (edge_current->dst_next != NULL){
				edge_current->dst_next->dst_prev = edge_current;
			}

			node2->nb_edge_dst --;
			node1->nb_edge_dst ++;
			node1->dst_edge_linkedList = edge_current;
		}
	}
}

int32_t graph_copy_src_edge(struct graph* graph, struct node* node1, struct node* node2){
	struct edge* edge_cursor;

	for (edge_cursor = node_get_head_edge_src(node2); edge_cursor != NULL; edge_cursor = edge_get_next_src(edge_cursor)){
		if (graph_add_edge(graph, node1, edge_get_dst(edge_cursor), edge_get_data(edge_cursor)) == NULL){
			return -1;
		}
	}

	return 0;
}

int32_t graph_copy_dst_edge(struct graph* graph, struct node* node1, struct node* node2){
	struct edge* edge_cursor;

	for (edge_cursor = node_get_head_edge_dst(node2); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
		if (graph_add_edge(graph, edge_get_src(edge_cursor), node1, edge_get_data(edge_cursor)) == NULL){
			return -1;
		}
	}

	return 0;
}

void graph_remove_node(struct graph* graph, struct node* node){
	graph->nb_node --;

	graph_remove_src_edge(graph, node);
	graph_remove_dst_edge(graph, node);

	if (node->prev == NULL){
		graph->node_linkedList_head = node->next;
	}
	else{
		node->prev->next = node->next;
	}

	if (node->next == NULL){
		graph->node_linkedList_tail = node->prev;
	}
	else{
		node->next->prev = node->prev;
	}

	graph_node_clean_data(graph, node)

	graph_free_node(graph, node);
}

struct edge* graph_add_edge_(struct graph* graph, struct node* node_src, struct node* node_dst){
	struct edge* edge;

	edge = graph_allocate_edge(graph);
	if (edge == NULL){
		log_err("unable to allocate edge");
	}
	else{
		graph_connect_edge(graph, node_src, node_dst, edge)
	}

	return edge;
}

struct edge* graph_add_edge(struct graph* graph, struct node* node_src, struct node* node_dst, void* data){
	struct edge* edge;

	edge = graph_allocate_edge(graph);
	if (edge == NULL){
		log_err("unable to allocate edge");
	}
	else{
		graph_edge_set_data(graph, edge, data)
		graph_connect_edge(graph, node_src, node_dst, edge)
	}

	return edge;
}

struct edge* graph_get_edge(struct node* node_src, struct node* node_dst){
	struct edge* edge;

	edge = node_get_head_edge_src(node_src);
	while(edge != NULL){
		if (edge_get_dst(edge) == node_dst){
			break;
		}
		edge = edge_get_next_src(edge);
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

void graph_remove_src_edge(struct graph* graph, struct node* node){
	struct edge* edge_cursor;
	struct edge* edge_current;

	for (edge_current = node->src_edge_linkedList; edge_current != NULL; edge_current = edge_cursor){
		edge_cursor = edge_current->src_next;

		if (edge_current->dst_prev == NULL){
			edge_current->dst_node->dst_edge_linkedList = edge_current->dst_next;
		}
		else{
			edge_current->dst_prev->dst_next = edge_current->dst_next;
		}
		if (edge_current->dst_next != NULL){
			edge_current->dst_next->dst_prev = edge_current->dst_prev;
		}
		edge_current->dst_node->nb_edge_dst --;
		graph->nb_edge --;

		graph_edge_clean_data(graph, edge_current)

		graph_free_edge(graph, edge_current);
	}

	node->src_edge_linkedList = NULL;
	node->nb_edge_src = 0;
}

void graph_remove_dst_edge(struct graph* graph, struct node* node){
	struct edge* edge_cursor;
	struct edge* edge_current;

	for (edge_current = node->dst_edge_linkedList; edge_current != NULL; edge_current = edge_cursor){
		edge_cursor = edge_current->dst_next;

		if (edge_current->src_prev == NULL){
			edge_current->src_node->src_edge_linkedList = edge_current->src_next;
		}
		else{
			edge_current->src_prev->src_next = edge_current->src_next;
		}
		if (edge_current->src_next != NULL){
			edge_current->src_next->src_prev = edge_current->src_prev;
		}
		edge_current->src_node->nb_edge_src --;
		graph->nb_edge --;

		graph_edge_clean_data(graph, edge_current)

		graph_free_edge(graph, edge_current);
	}

	node->dst_edge_linkedList = NULL;
	node->nb_edge_dst = 0;
}

int32_t graph_copy(struct graph* graph_dst, const struct graph* graph_src, int32_t(*node_copy)(void*,const void*,void*), int32_t(*edge_copy)(void*,const void*,void*), void* arg){
	struct node* node_cursor;
	struct edge* edge_cursor;
	struct node* new_node;
	struct edge* new_edge;

	for (node_cursor = graph_get_head_node(graph_src); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
		node_cursor->ptr = NULL;
	}

	for (node_cursor = graph_get_head_node(graph_src); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
		new_node = graph_allocate_node(graph_dst);
		if (new_node == NULL){
			log_err("unable to allocate node");
			return -1;
		}

		if (node_copy(node_get_data(new_node), node_get_data(node_cursor), arg)){
			log_err("node_copy returned an error code");
			free(new_node);
			return -1;
		}

		graph_connect_node(graph_dst, new_node)
		node_cursor->ptr = new_node;

		for (edge_cursor = node_get_head_edge_src(node_cursor); edge_cursor != NULL; edge_cursor = edge_get_next_src(edge_cursor)){
			if (edge_get_dst(edge_cursor)->ptr != NULL){
				new_edge = graph_allocate_edge(graph_dst);
				if (new_edge == NULL){
					log_err("unable to allocate memory");
					return -1;
				}

				if (edge_copy(edge_get_data(new_edge), edge_get_data(edge_cursor), arg)){
					log_err("edge_copy returned an error code");
					free(new_edge);
					return -1;
				}

				graph_connect_edge(graph_dst, new_node, (struct node*)(edge_get_dst(edge_cursor)->ptr), new_edge)
			}
		}

		for (edge_cursor = node_get_head_edge_dst(node_cursor); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
			if (edge_get_src(edge_cursor)->ptr != NULL){
				new_edge = graph_allocate_edge(graph_dst);
				if (new_edge == NULL){
					log_err("unable to allocate memory");
					return -1;
				}

				if (edge_copy(edge_get_data(new_edge), edge_get_data(edge_cursor), arg)){
					log_err("edge_copy returned an error code");
					free(new_edge);
					return -1;
				}

				graph_connect_edge(graph_dst, (struct node*)(edge_get_src(edge_cursor)->ptr), new_node, new_edge)
			}
		}
	}

	return 0;
}

struct graph* graph_clone(const struct graph* graph_src, int32_t(*node_copy)(void*,const void*,void*), int32_t(*edge_copy)(void*,const void*,void*), void* arg){
	struct graph* graph;

	graph = (struct graph*)malloc(sizeof(struct graph));
	if (graph != NULL){
		memcpy(graph, graph_src, sizeof(struct graph));
		graph->node_linkedList_head 	= NULL;
		graph->node_linkedList_tail 	= NULL;
		graph->nb_node 					= 0;
		graph->nb_edge 					= 0;

		if (graph_copy(graph, graph_src, node_copy, edge_copy, arg)){
			log_err("unable to copy graph");
			graph_delete(graph);
			graph = NULL;
		}
	}
	else{
		log_err("unable to allocate memory");
	}

	return graph;
}

int32_t graph_concat(struct graph* graph_dst, const struct graph* graph_src, int32_t(*node_copy)(void*,const void*,void*), int32_t(*edge_copy)(void*,const void*,void*), void* arg){
	if (graph_dst->node_data_size != graph_src->node_data_size){
		log_err("unable to concat graphs with different node size");
		return -1;
	}
	if (graph_dst->edge_data_size != graph_src->edge_data_size){
		log_err("unable to concat graphs with different edge size");
		return -1;
	}
	if (graph_copy(graph_dst, graph_src, node_copy, edge_copy, arg)){
		log_err("unable to copy graph");
		return -1;
	}

	return 0;
}
