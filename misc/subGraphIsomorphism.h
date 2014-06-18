#ifndef SUBGRAPHISOMORPHISM_H
#define SUBGRAPHISOMORPHISM_H

#include <stdint.h>

#include "graph.h"
#include "array.h"

struct labelTab{
	uint32_t 				label;
	struct node* 			node;
	uint32_t 				fastAccess_index;
};

struct labelFastAccess{
	uint32_t 				label;
	uint32_t 				offset;
	uint32_t 				size;
};

struct graphIsoHandle{
	struct graph* 			graph;
	uint32_t 				nb_label;
	struct labelFastAccess* label_fast;
	struct labelTab*		label_tab;
};

struct subGraphIsoHandle{
	struct graph* 			graph;
	struct labelTab* 		label_tab;
};

struct graphIsoHandle* graphIso_create_graph_handle(struct graph* graph, uint32_t(*node_get_label)(struct node*));
struct subGraphIsoHandle* graphIso_create_sub_graph_handle(struct graph* graph, uint32_t(*node_get_label)(struct node*));

struct array* graphIso_search(struct graphIsoHandle* graph_handle, struct subGraphIsoHandle* sub_graph_handle);

void graphIso_delete_graph_handle(struct graphIsoHandle* handle);
void graphIso_delete_subGraph_handle(struct subGraphIsoHandle* handle);

#endif
