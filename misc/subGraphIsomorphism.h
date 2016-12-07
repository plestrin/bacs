#ifndef SUBGRAPHISOMORPHISM_H
#define SUBGRAPHISOMORPHISM_H

#include <stdint.h>

#include "graph.h"
#include "array.h"

#define SUBGRAPHISOMORPHISM_JOKER_LABEL 0xffffffff


#define SUBGRAPHISOMORPHISM_OPTIM_SORT 			1
#define SUBGRAPHISOMORPHISM_OPTIM_COMPACT 		1
#define SUBGRAPHISOMORPHISM_OPTIM_GLOBAL 		1
#define SUBGRAPHISOMORPHISM_OPTIM_LAZY_DUP 		1
// #define SUBGRAPHISOMORPHISM_OPTIM_FAST_STEP 	1
// #define SUBGRAPHISOMORPHISM_OPTIM_LAYER 		1

#define assignment_get_size(nb_node, nb_edge) ((nb_node) * sizeof(struct node*) + (nb_edge) * sizeof(struct edge*))
#define assignment_node(assignment, i) (*((struct node**)(assignment) + (i)))
#define assignment_edge(assignment, i, nb_node) (*((struct edge**)((struct node**)(assignment) + (nb_node)) + (i)))

struct nodeTab{
	uint32_t 		label;
	struct node* 	node;
};

struct subNodeTab{
	uint32_t 		label;
	struct node* 	node;
	uint32_t 		off_src;
	uint32_t 		nb_src;
	uint32_t 		off_dst;
	uint32_t 		nb_dst;
	uint32_t 		restart_ctr;
};

struct edgeTab{
	uint32_t 		label;
	struct edge* 	edge;
	uint32_t 		src;
	uint32_t 		dst;
};

struct edgeFastAccess{
	uint32_t 		label;
	uint32_t 		offset;
	uint32_t 		size;
};

struct graphIsoHandle{
	struct graph* 	graph;
	struct nodeTab*	node_tab;
	struct edgeTab* edge_tab;
	uint32_t* 		src_edge_mapping;
	uint32_t* 		dst_edge_mapping;
};

struct subGraphIsoHandle{
	struct graph* 		graph;
	struct subNodeTab* 	sub_node_tab;
	struct edgeTab* 	edge_tab;
	uint32_t* 			src_fast;
	uint32_t* 			dst_fast;
};

struct graphIsoHandle* graphIso_create_graph_handle(struct graph* graph, uint32_t(*node_get_label)(struct node*), uint32_t(*edge_get_label)(struct edge*));
struct subGraphIsoHandle* graphIso_create_sub_graph_handle(struct graph* graph, uint32_t(*node_get_label)(struct node*), uint32_t(*edge_get_label)(struct edge*));

struct array* graphIso_search(struct graphIsoHandle* graph_handle, struct subGraphIsoHandle* sub_graph_handle);

void graphIso_delete_graph_handle(struct graphIsoHandle* handle);
void graphIso_delete_subGraph_handle(struct subGraphIsoHandle* handle);

#endif
