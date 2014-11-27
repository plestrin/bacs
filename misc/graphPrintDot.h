#ifndef GRAPHPRINTDOT_H
#define GRAPHPRINTDOT_H

#include <stdint.h>

#define GRAPHPRINTDOT_DEFAULT_FILE_NAME "graph.dot"

#include "graph.h"

struct graphPrintDotFilter{
	int32_t(*node_filter)(struct node*,void*);
	int32_t(*edge_filter)(struct edge*,void*);
};

int32_t graphPrintDot_print(struct graph* graph, const char* name, struct graphPrintDotFilter* filters, void* arg);

#endif