#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "callGraph.h"
#include "traceFragment.h"

#define CALLGRAPH_MAX_DEPTH 	512
#define CALLGRAPH_START_DEPTH 	256

static int32_t callGraphNode_add_snippet(struct callGraph* call_graph, struct callGraphNode* call_graph_node, uint32_t start, uint32_t stop, ADDRESS address_start);
static int32_t callGraphNode_get_first_snippet(struct callGraph* call_graph, struct callGraphNode* call_graph_node);

#pragma GCC diagnostic ignored "-Wunused-parameter"
void callGraph_dotPrint_node(void* data, FILE* file, void* arg){
	struct callGraphNode* node = (struct callGraphNode*)data;

	if (node->routine != NULL){
		fprintf(file, "[label=\"%s\"]", node->routine->name);
	}
	else{
		fprintf(file, "[label=\"NULL\"]");
	}
}

#pragma GCC diagnostic ignored "-Wunused-parameter"
void callGraph_dotPrint_edge(void* data, FILE* file, void* arg){
	struct callGraphEdge* edge = (struct callGraphEdge*)data;

	switch(edge->type){
		case CALLGRAPH_EDGE_CALL 	: {fprintf(file, "[label=\"call\"]"); break;}
		case CALLGRAPH_EDGE_RET 	: {fprintf(file, "[label=\"ret\"]"); break;}
	}
}

struct callGraph* callGraph_create(struct trace* trace){
	struct callGraph* call_graph;

	call_graph = (struct callGraph*)malloc(sizeof(struct callGraph));
	if (call_graph != NULL){
		if (callGraph_init(call_graph, trace)){
			free(call_graph);
			call_graph = NULL;
		}
	}

	return call_graph;
}

#define callGraph_node_init(node) 																\
	(node)->last_snippet_offset = -1; 															\
	(node)->nb_instruction = 0; 																\
	(node)->routine = NULL;

int32_t callGraph_init(struct callGraph* call_graph, struct trace* trace){
	struct instructionIterator 	it;
	struct node* 				call_graph_stack[CALLGRAPH_MAX_DEPTH];
	uint32_t 					call_graph_ptr;
	struct callGraphNode* 		call_graph_node;
	uint32_t 					snippet_start;
	uint32_t 					snippet_stop;
	ADDRESS 					address_start;
	struct callGraphEdge 		call_graph_edge;


	memset(call_graph_stack, 0, sizeof(struct node*) * CALLGRAPH_START_DEPTH);

	if (array_init(&(call_graph->snippet_array), sizeof(struct assemblySnippet))){
		printf("ERROR: in %s, unable to init array\n", __func__);
		return -1;
	}

	graph_init(&(call_graph->graph), sizeof(struct callGraphNode), sizeof(struct callGraphEdge))
	graph_register_dotPrint_callback(&(call_graph->graph), NULL, callGraph_dotPrint_node, callGraph_dotPrint_edge, NULL) 		

	if (assembly_get_instruction(&(trace->assembly), &it, 0)){
		printf("ERROR: in %s, unable to fetch first instruction from the assembly\n", __func__);
		return -1;
	}

	call_graph_ptr = CALLGRAPH_START_DEPTH;
	call_graph_stack[call_graph_ptr] = graph_add_node_(&(call_graph->graph));
	if (call_graph_stack[call_graph_ptr] == NULL){
		printf("ERROR: in %s, unable to create node\n", __func__);
	}

	call_graph_node = callGraph_node_get_data(call_graph_stack[call_graph_ptr]);
	callGraph_node_init(call_graph_node)

	address_start = it.instruction_address;
	snippet_start = 0;
	snippet_stop = 0;

	for (;;){
		if (snippet_start == snippet_stop){
			address_start = it.instruction_address;
		}

		snippet_stop ++;
		switch(xed_decoded_inst_get_iclass(&(it.xedd))){
			case XED_ICLASS_CALL_FAR 	:
			case XED_ICLASS_CALL_NEAR 	: {
				if (callGraphNode_add_snippet(call_graph, call_graph_node, snippet_start, snippet_stop, address_start)){
					printf("ERROR: in %s, unable to add snippet to current graph node\n", __func__);
				}
				
				call_graph_ptr ++;
				if (call_graph_ptr == CALLGRAPH_MAX_DEPTH){
					printf("ERROR: in %s, the call stack is too small (%u)\n", __func__, CALLGRAPH_MAX_DEPTH);
					return -1;
				}

				call_graph_stack[call_graph_ptr] = graph_add_node_(&(call_graph->graph));
				if (call_graph_stack[call_graph_ptr] == NULL){
					printf("ERROR: in %s, unable to create node\n", __func__);
					return -1;
				}

				call_graph_node = callGraph_node_get_data(call_graph_stack[call_graph_ptr]);
				callGraph_node_init(call_graph_node)

				call_graph_edge.type 		= CALLGRAPH_EDGE_CALL;
				call_graph_edge.ins_offset 	= snippet_stop;

				if (graph_add_edge(&(call_graph->graph), call_graph_stack[call_graph_ptr - 1], call_graph_stack[call_graph_ptr], &call_graph_edge) == NULL){
					printf("ERROR: in %s, unable to add edge to callGraph\n", __func__);
				}

				snippet_start = snippet_stop;
				break;
			}
			case XED_ICLASS_RET_FAR 		:
			case XED_ICLASS_RET_NEAR 		: {
				if (callGraphNode_add_snippet(call_graph, call_graph_node, snippet_start, snippet_stop, address_start)){
					printf("ERROR: in %s, unable to add snippet to current graph node\n", __func__);
				}
				
				if (call_graph_ptr == 0){
					printf("ERROR: in %s, the call stack is empty, illegal RET instruction @ %u\n", __func__, snippet_stop - 1);
					break;
				}
				call_graph_ptr --;
				if (call_graph_stack[call_graph_ptr] == NULL){
					call_graph_stack[call_graph_ptr] = graph_add_node_(&(call_graph->graph));
					if (call_graph_stack[call_graph_ptr] == NULL){
						printf("ERROR: in %s, unable to create node\n", __func__);
						return -1;
					}

					call_graph_node = callGraph_node_get_data(call_graph_stack[call_graph_ptr]);
					callGraph_node_init(call_graph_node)
				}
				else{
					call_graph_node = callGraph_node_get_data(call_graph_stack[call_graph_ptr]);
				}

				call_graph_edge.type 		= CALLGRAPH_EDGE_RET;
				call_graph_edge.ins_offset 	= call_graph_node->nb_instruction;

				if (graph_add_edge(&(call_graph->graph), call_graph_stack[call_graph_ptr + 1], call_graph_stack[call_graph_ptr], &call_graph_edge) == NULL){
					printf("ERROR: in %s, unable to add edge to callGraph\n", __func__);
				}

				snippet_start = snippet_stop;
				break;
			}
			default 					: {
				break;
			}
		}

		if (instructionIterator_get_instruction_index(&it) ==  assembly_get_nb_instruction(&(trace->assembly)) - 1){
			break;
		}
		else{
			if (assembly_get_next_instruction(&(trace->assembly), &it)){
				printf("ERROR: in %s, unable to fetch next instruction from the assembly\n", __func__);
				return -1;
			}
		}
	}

	return 0;
}

static int32_t callGraphNode_add_snippet(struct callGraph* call_graph, struct callGraphNode* call_graph_node, uint32_t start, uint32_t stop, ADDRESS address_start){
	int32_t 				new_snippet_index;
	struct assemblySnippet 	new_snippet;

	new_snippet.address_start 			= address_start;
	new_snippet.offset 					= start;
	new_snippet.length 					= stop - start;
	new_snippet.next_snippet_offset 	= -1;
	new_snippet.prev_snippet_offset 	= call_graph_node->last_snippet_offset;

	new_snippet_index = array_add(&(call_graph->snippet_array), &new_snippet);
	if (new_snippet_index < 0){
		printf("ERROR: in %s, unable to add snippet to array\n", __func__);
		return -1;
	}

	call_graph_node->last_snippet_offset = new_snippet_index;
	call_graph_node->nb_instruction += new_snippet.length;

	return 0;
}

static int32_t callGraphNode_get_first_snippet(struct callGraph* call_graph, struct callGraphNode* call_graph_node){
	int32_t 					index;
	struct assemblySnippet* 	snippet;

	index = call_graph_node->last_snippet_offset;
	while (index  >= 0){
		snippet = (struct assemblySnippet*)array_get(&(call_graph->snippet_array), index);
		if (snippet->prev_snippet_offset >= 0){
			index = snippet->prev_snippet_offset;
		}
		else{
			break;
		}
	}

	return index;
}

void callGraph_locate_in_codeMap(struct callGraph* call_graph, struct codeMap* code_map){
	struct node* 				node;
	struct callGraphNode* 		call_graph_node;
	int32_t 					first_snippet_index;
	struct assemblySnippet* 	first_snippet;

	for (node = graph_get_head_node(&(call_graph->graph)); node != NULL; node = node_get_next(node)){
		call_graph_node = callGraph_node_get_data(node);
		first_snippet_index = callGraphNode_get_first_snippet(call_graph, call_graph_node);
		if (first_snippet_index < 0){
			printf("ERROR: in %s, unable to get first snippet for a callGraph node\n", __func__);
			continue;
		}

		first_snippet = (struct assemblySnippet*)array_get(&(call_graph->snippet_array), first_snippet_index);

		call_graph_node->routine = codeMap_search_routine(code_map, first_snippet->address_start);
	}
}

int32_t callGraph_export_inclusive(struct callGraph* call_graph, struct trace* trace, struct array* frag_array, char* name_filter){
	struct node* 				node;
	struct callGraphNode* 		call_graph_node;
	int32_t 					first_snippet_index;
	struct assemblySnippet* 	snippet;
	uint32_t 					start_index;
	uint32_t 					stop_index;
	struct traceFragment 		fragment;

	for (node = graph_get_head_node(&(call_graph->graph)); node != NULL; node = node_get_next(node)){
		call_graph_node = callGraph_node_get_data(node);
		if (name_filter == NULL || (name_filter != NULL && call_graph_node->routine != NULL && !strncmp(name_filter, call_graph_node->routine->name, CODEMAP_DEFAULT_NAME_SIZE))){
			if (call_graph_node->last_snippet_offset < 0){
				printf("ERROR: in %s, no code snippet for the current callGraph node\n", __func__);
				continue;
			}
			
			snippet = (struct assemblySnippet*)array_get(&(call_graph->snippet_array), call_graph_node->last_snippet_offset);
			stop_index = snippet->offset + snippet->length;

			first_snippet_index = callGraphNode_get_first_snippet(call_graph, call_graph_node);
			if (first_snippet_index < 0){
				printf("ERROR: in %s, unable to get first snippet for a callGraph node\n", __func__);
				continue;
			}

			snippet = (struct assemblySnippet*)array_get(&(call_graph->snippet_array), first_snippet_index);
			start_index = snippet->offset;

			traceFragment_init(&fragment, TRACEFRAGMENT_TYPE_NONE, NULL);
			if (trace_extract_segment(trace, &(fragment.trace), start_index, stop_index - start_index)){
				printf("ERROR: in %s, unable to extract traceFragment\n", __func__);
			}
			else{
				if (call_graph_node->routine != NULL){
					snprintf(fragment.tag, TRACEFRAGMENT_TAG_LENGTH, "rtn_inc:%s", call_graph_node->routine->name);
				}

				if (array_add(frag_array, &fragment) < 0){
					printf("ERROR: in %s, unable to add traceFragment to array\n", __func__);
					traceFragment_clean(&fragment);
				}
			}
		}
	}

	return 0;
}