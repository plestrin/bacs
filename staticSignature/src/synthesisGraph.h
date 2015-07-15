#ifndef SYNTHESISGRAPH_H
#define SYNTHESISGRAPH_H

#include "ir.h"
#include "graph.h"
#include "graphPrintDot.h"
#include "array.h"

struct synthesisGraph {
	struct graph 		graph;
	struct array 		cluster_array;
};

struct synthesisGraph* synthesisGraph_create(struct ir* ir);
int32_t synthesisGraph_init(struct synthesisGraph* synthesis_graph, struct ir* ir);

#define synthesisGraph_printDot(synthesis_graph) graphPrintDot_print(&(synthesis_graph->graph), NULL, NULL)

void synthesisGraph_clean(struct synthesisGraph* synthesis_graph);

#define synthesisGraph_delete(synthesis_graph) 								\
	synthesisGraph_clean(synthesis_graph); 									\
	free(synthesis_graph);

#endif