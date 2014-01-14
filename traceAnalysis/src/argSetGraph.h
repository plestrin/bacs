#ifndef ARGSETGRAPH_H
#define ARGSETGRAPH_H

#include "array.h"
#include "graph.h"
#include "graphPrintDot.h"
#include "argBuffer.h"
#include "argSet.h"

struct argSetGraph{
	struct graph 	graph;
	struct array* 	arg_array;
};



struct argSetGraph* argSetGraph_create(struct array* arg_array);
int32_t argSetGraph_init(struct argSetGraph* argSetGraph, struct array* arg_array);

int32_t argSetGraph_pack_simple(struct argSetGraph* arg_set_graph, uint32_t dst_node);

static inline int32_t argSetGraph_print_dot(struct argSetGraph* arg_set_graph, const char* file_name){
	return graphPrintDot_print(&(arg_set_graph->graph), file_name, arg_set_graph->arg_array);
}

void argSetGraph_clean(struct argSetGraph* arg_set_graph);
void argSetGraph_delete(struct argSetGraph* arg_set_graph);

#endif