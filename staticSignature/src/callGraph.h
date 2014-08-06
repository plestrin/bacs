#ifndef CALLGRAPH_H
#define CALLGRAPH_H

#include "trace.h"
#include "graph.h"
#include "array.h"
#include "codeMap.h"

struct assemblySnippet{
	uint32_t 				offset;
	uint32_t 				length;
	ADDRESS 				expected_next_address;
	int32_t 				next_snippet_offset;
	int32_t 				prev_snippet_offset;
};

struct callGraphNode{
	int32_t 				last_snippet_offset;
	uint32_t 				nb_instruction;
	struct cm_routine* 		routine;
};

#define callGraph_node_get_data(node) 	((struct callGraphNode*)&((node)->data))

enum callGraphEdgeType{
	CALLGRAPH_EDGE_CALL,
	CALLGRAPH_EDGE_RET
};

struct callGraphEdge{
	enum callGraphEdgeType 	type;
	uint32_t 				ins_offset;
};

#define callGraph_edge_get_data(edge) 	((struct callGraphEdge*)&((edge)->data))

struct callGraph{
	struct graph 			graph;
	struct array 			snippet_array;
};

struct callGraph* callGraph_create(struct trace* trace);
int32_t callGraph_init(struct callGraph* call_graph, struct trace* trace);

void callGraph_locate_in_codeMap_linux(struct callGraph* call_graph, struct trace* trace, struct codeMap* code_map);

#define callGraph_printDot(call_graph) graphPrintDot_print(&(call_graph->graph), "callGraph.dot", call_graph)

int32_t callGraph_export_inclusive(struct callGraph* call_graph, struct trace* trace, struct array* frag_array, char* name_filter);

#define callGraph_clean(call_graph) 													\
	graph_clean(&((call_graph)->graph)); 												\
	array_clean(&((call_graph)->snippet_array))

#define callGraph_delete(call_graph) 													\
	callGraph_clean(call_graph); 														\
	free(call_graph)

#endif