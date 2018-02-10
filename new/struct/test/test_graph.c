#include <stdlib.h>
#include <stdint.h>

#include "../../base.h"
#include "../graph.h"

static void cirle(struct node* node){
	struct node* cursor_node;
	struct edge* cursor_edge;

	cursor_node = node;

	/* src -> dst */
	do{
		for (cursor_edge = node_get_top_src_edge(cursor_node); ; cursor_edge = edge_get_next_src(cursor_edge)){
			if (edge_get_next_src(cursor_edge) == NULL_SRC){
				break;
			}
		}
		cursor_node = edge_get_dst(cursor_edge);
	} while (cursor_node != node);

	/* dst -> src */
	do{
		for (cursor_edge = node_get_top_dst_edge(cursor_node); ; cursor_edge = edge_get_next_dst(cursor_edge)){
			if (edge_get_next_dst(cursor_edge) == NULL_DST){
				break;
			}
		}
		cursor_node = edge_get_src(cursor_edge);
	} while (cursor_node != node);
}

int main(void){
	struct graph 	graph = GRAPH_INIT;
	struct node 	node_a;
	struct node 	node_b;
	struct node 	node_c;
	struct node 	node_d;
	struct edge 	edge_ab;
	struct edge 	edge_bc;
	struct edge 	edge_cd;
	struct edge 	edge_da;
	struct edge 	edge_bd;
	struct edge 	edge_dc;
	struct edge 	edge_ca;


	graph_add_node(&graph, &node_a);
	graph_add_node(&graph, &node_b);
	graph_add_node(&graph, &node_c);
	graph_add_node(&graph, &node_d);

	graph_add_edge(&graph, &node_a, &edge_ab, &node_b);
	graph_add_edge(&graph, &node_b, &edge_bc, &node_c);
	graph_add_edge(&graph, &node_c, &edge_cd, &node_d);
	graph_add_edge(&graph, &node_d, &edge_da, &node_a);

	/* a -> b -> c -> d -> a -> ... */
	cirle(graph_get_top_node(&graph));
	cirle(graph_get_bot_node(&graph));

	graph_del_edge(&graph, &edge_bc);

	graph_add_edge(&graph, &node_b, &edge_bd, &node_d);
	graph_add_edge(&graph, &node_d, &edge_dc, &node_c);
	graph_add_edge(&graph, &node_c, &edge_ca, &node_a);

	graph_del_edge(&graph, &edge_cd);
	graph_del_edge(&graph, &edge_da);

	/* a -> b -> d -> c -> a -> ... */
	cirle(&node_d);

	graph_del_edge(&graph, &edge_ab);
	graph_del_edge(&graph, &edge_bd);
	graph_del_edge(&graph, &edge_dc);
	graph_del_edge(&graph, &edge_ca);

	graph_del_node(&graph, &node_b);
	graph_del_node(&graph, &node_d);
	graph_del_node(&graph, &node_a);
	graph_del_node(&graph, &node_c);

	if (graph.nb_node){
		log_err_m("%u node(s) at the end (expected 0)", graph.nb_node);
		return EXIT_FAILURE;
	}

	if (graph.nb_edge){
		log_err_m("%u edge(s) at the end (expected 0)", graph.nb_edge);
		return EXIT_FAILURE;
	}

	if (!graph_is_empty(&graph)){
		log_err("graph is not empty at the end");
		return EXIT_FAILURE;
	}

	log_info("test_graph completed!");

	return EXIT_SUCCESS;
}
