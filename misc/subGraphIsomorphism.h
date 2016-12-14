#ifndef SUBGRAPHISOMORPHISM_H
#define SUBGRAPHISOMORPHISM_H

#include <stdint.h>

#include "graph.h"
#include "array.h"

#define SUBGRAPHISOMORPHISM_JOKER_LABEL 0xffffffff

#define SUBGRAPHISOMORPHISM_OPTIM_SORT 			1
#define SUBGRAPHISOMORPHISM_OPTIM_COMPACT 		1
#define SUBGRAPHISOMORPHISM_OPTIM_GLOBAL 		1
#define SUBGRAPHISOMORPHISM_OPTIM_LAZY_DUP 		0 /* Not compatible with LAYER */
#define SUBGRAPHISOMORPHISM_OPTIM_FAST_STEP 	1
#define SUBGRAPHISOMORPHISM_OPTIM_MARK_ASSIGNED 1
#define SUBGRAPHISOMORPHISM_OPTIM_LAYER 		0 /* Not compatible with LAZY_DUP */
#define SUBGRAPHISOMORPHISM_OPTIM_COMPACT_LAYER 1 /* Only active if LAYER is active */

#if SUBGRAPHISOMORPHISM_OPTIM_LAYER == 1
#include "ugraph.h"
#endif

#define assignment_get_size(nb_node, nb_edge) ((nb_node) * sizeof(struct node*) + (nb_edge) * sizeof(struct edge*))
#define assignment_node(assignment, i) (*((struct node**)(assignment) + (i)))
#define assignment_edge(assignment, i, nb_node) (*((struct edge**)((struct node**)(assignment) + (nb_node)) + (i)))

enum layerNodeType{
	LAYERNODE_TYPE_PRIM,
	LAYERNODE_TYPE_MACRO
};

struct layerNodeData{
	uint32_t 			selected;
	uint32_t 			disabled;
	enum layerNodeType 	type;
	#if SUBGRAPHISOMORPHISM_OPTIM_COMPACT_LAYER == 1
	uint32_t 			nb_edge;
	struct edge** 		edge_buffer;
	#endif
};

#define unode_get_layerNodeData(unode) ((struct layerNodeData*)unode_get_data(unode))

#if SUBGRAPHISOMORPHISM_OPTIM_LAYER == 1 && SUBGRAPHISOMORPHISM_OPTIM_COMPACT_LAYER == 1
void layerNodeData_clean_unode(struct unode* unode);
#else
#define layerNodeData_clean_unode NULL
#endif

#if SUBGRAPHISOMORPHISM_OPTIM_LAYER == 1
void layerNodeData_printDot(void* data, FILE* file);
#else
#define layerNodeData_printDot NULL
#endif

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

#if SUBGRAPHISOMORPHISM_OPTIM_LAYER == 1
void graphIso_prepare_layer(struct graph* graph);
void graphIso_check_layer(struct ugraph* graph_layer);
#else
#define graphIso_prepare_layer(graph)
#define graphIso_check_layer(graph_layer)
#endif

#endif
