#ifndef CALLGRAPH_H
#define CALLGRAPH_H

#include "trace.h"
#include "graph.h"
#include "array.h"
#include "list.h"
#include "codeMap.h"

struct assemblySnippet{
	uint32_t 	offset;
	uint32_t 	length;
	ADDRESS 	expected_next_address;
	int32_t 	next_snippet_offset;
	int32_t 	prev_snippet_offset;
};

enum functionType{
	FUNCTION_VALID,
	FUNCTION_INVALID
};

struct function{
	enum functionType 	type;
	int32_t 			last_snippet_offset;
	struct cm_routine* 	routine;
};

#define callGraph_node_get_function(node) ((struct function*)node_get_data(node))

#define function_init_valid(func) 														\
	(func)->type = FUNCTION_VALID; 														\
	(func)->last_snippet_offset = -1; 													\
	(func)->routine = NULL;

#define function_init_invalid(func) 													\
	(func)->type = FUNCTION_INVALID; 													\
	(func)->last_snippet_offset = -1; 													\
	(func)->routine = NULL;

#define function_is_valid(func) 	((func)->type == FUNCTION_VALID)
#define function_is_invalid(func) 	((func)->type == FUNCTION_INVALID)
#define function_set_invalid(func) 	(func)->type = FUNCTION_INVALID

enum callGraphEdgeType{
	CALLGRAPH_EDGE_CALL,
	CALLGRAPH_EDGE_RET
};

struct callGraphEdge{
	enum callGraphEdgeType type;
};

#define callGraph_edge_get_data(edge) 	((struct callGraphEdge*)edge_get_data(edge))

struct callGraph{
	struct graph graph;
	struct array snippet_array;
};

int32_t function_get_first_snippet(struct callGraph* call_graph, struct function* func);

struct callGraph* callGraph_create(struct assembly* assembly, uint32_t index_start, uint32_t index_stop);
int32_t callGraph_init(struct callGraph* call_graph, struct assembly* assembly, uint32_t start, uint32_t stop);

enum blockLabel{
	BLOCK_LABEL_UNKOWN 	= 0,
	BLOCK_LABEL_CALL 	= 1,
	BLOCK_LABEL_RET 	= 2,
	BLOCK_LABEL_NONE 	= 3
};

enum blockLabel callGraph_label_block(struct asmBlock* block);
enum blockLabel* callGraph_alloc_label_buffer(struct assembly* assembly);
enum blockLabel* callGraph_fill_label_buffer(struct assembly* assembly);

struct frameEstimation{
	uint32_t index_start;
	uint32_t index_stop;
	uint32_t is_not_leaf;
};

int32_t callGraph_get_frameEstimation(struct assembly* assembly, uint32_t index, struct frameEstimation* frame_est, uint64_t max_size, uint32_t verbose, enum blockLabel* label_buffer);
void callGraph_print_frame(struct callGraph* call_graph, struct assembly* assembly, uint32_t index, struct codeMap* code_map);

void callGraph_locate_in_codeMap_linux(struct callGraph* call_graph, const struct trace* trace, struct codeMap* code_map);
void callGraph_locate_in_codeMap_windows(struct callGraph* call_graph, const struct trace* trace, struct codeMap* code_map);

#define callGraph_printDot(call_graph) graphPrintDot_print(&(call_graph->graph), "callGraph.dot", call_graph)

void callGraph_check(struct callGraph* call_graph, const struct assembly* assembly, struct codeMap* code_map);

struct node* callGraph_get_index(struct callGraph* call_graph, uint32_t index);
void callGraph_fprint_node(struct callGraph* call_graph, const struct node* node, FILE* file);
void callGraph_fprint_stack(struct callGraph* call_graph, struct node* node, FILE* file);

int32_t callGraphNode_is_leaf(struct callGraph* call_graph, struct node* node, const struct assembly* assembly);

int32_t callGraph_export_node_inclusive(struct callGraph* call_graph, struct node* node, struct trace* trace, struct list* frag_list);

#define callGraph_clean(call_graph) 													\
	graph_clean(&((call_graph)->graph)); 												\
	array_clean(&((call_graph)->snippet_array))

#define callGraph_delete(call_graph) 													\
	callGraph_clean(call_graph); 														\
	free(call_graph)

#endif
