#include "graph.h"
#include "../base.h"

void graph_add_node(struct graph* graph, struct node* node){
	graph->nb_node ++;

	node->nb_edge_src = 0;
	node->nb_edge_dst = 0;

	llh_init(&node->llh_edge_src);
	llh_init(&node->llh_edge_dst);

	llh_add_bot(&graph->llh_node, &node->ll_node);
}

void graph_del_node(struct graph* graph, struct node* node){
	#ifdef DEBUG
	if (!llh_is_empty(&node->llh_edge_src)){
		log_err("del node, but src edge linked list is not empty");
	}
	if (!llh_is_empty(&node->llh_edge_dst)){
		log_err("del node, but dst edge linked list is not empty");
	}
	#endif

	graph->nb_node --;
	llh_del(&graph->llh_node, &node->ll_node);

}

void graph_add_edge(struct graph* graph, struct node* src, struct edge* edge, struct node* dst){
	graph->nb_edge ++;

	edge->src_node = src;
	edge->dst_node = dst;

	src->nb_edge_src ++;
	dst->nb_edge_dst ++;

	llh_add_bot(&src->llh_edge_src, &edge->ll_src);
	llh_add_bot(&dst->llh_edge_dst, &edge->ll_dst);
}

void graph_del_edge(struct graph* graph, struct edge* edge){
	graph->nb_edge --;

	edge_get_src(edge)->nb_edge_src --;
	edge_get_dst(edge)->nb_edge_dst --;

	llh_del(&edge_get_src(edge)->llh_edge_src, &edge->ll_src);
	llh_del(&edge_get_dst(edge)->llh_edge_dst, &edge->ll_dst);
}
