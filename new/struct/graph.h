#ifndef GRAPH_H
#define GRAPH_H

#include <stdlib.h>

#include "llist.h"

struct edge{
	struct llist ll_src;
	struct llist ll_dst;
	struct node* src_node;
	struct node* dst_node;
};

#define edge_get_src(edge) ((edge)->src_node)
#define edge_get_dst(edge) ((edge)->dst_node)

#define edge_get_next_src(edge_) ((struct edge*)((edge_)->ll_src.next))
#define edge_get_prev_src(edge_) ((struct edge*)((edge_)->ll_src.prev))
#define edge_get_next_dst(edge_) ((struct edge*)((edge_)->ll_dst.next - 1))
#define edge_get_prev_dst(edge_) ((struct edge*)((edge_)->ll_dst.prev - 1))

#define NULL_SRC NULL
#define NULL_DST ((struct edge*)((struct llist*)NULL - 1))

struct node{
	struct llist 	ll_node;
	size_t 			nb_edge_src;
	size_t 			nb_edge_dst;
	struct ll_head	llh_edge_src;
	struct ll_head	llh_edge_dst;
};

#define node_get_next(node_) ((struct node*)((node_)->ll_node.next))
#define node_get_prev(node_) ((struct node*)((node_)->ll_node.prev))

#define node_get_top_src_edge(node) ((struct edge*)(llh_get_top(&(node)->llh_edge_src)))
#define node_get_bot_src_edge(node) ((struct edge*)(llh_get_bot(&(node)->llh_edge_src)))
#define node_get_top_dst_edge(node) ((struct edge*)(llh_get_top(&(node)->llh_edge_dst) - 1))
#define node_get_bot_dst_edge(node) ((struct edge*)(llh_get_bot(&(node)->llh_edge_dst) - 1))

struct graph{
	struct ll_head 	llh_node;
	size_t 			nb_node;
	size_t 			nb_edge;
};

#define GRAPH_INIT {.llh_node = LLH_INIT, .nb_node = 0, .nb_edge = 0}

static inline void graph_init(struct graph* graph){
	graph->nb_node = 0;
	graph->nb_edge = 0;
	llh_init(&graph->llh_node);
}

#define graph_get_top_node(graph) ((struct node*)llh_get_top(&(graph)->llh_node))
#define graph_get_bot_node(graph) ((struct node*)llh_get_bot(&(graph)->llh_node))

void graph_add_node(struct graph* graph, struct node* node);
void graph_del_node(struct graph* graph, struct node* node);
void graph_add_edge(struct graph* graph, struct node* src, struct edge* edge, struct node* dst);
void graph_del_edge(struct graph* graph, struct edge* edge);

#define graph_is_empty(graph) llh_is_empty(&(graph)->llh_node)

#endif
