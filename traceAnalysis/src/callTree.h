#ifndef CALLTREE_H
#define CALLTREE_H

#include "instruction.h"
#include "codeMap.h"
#include "graph.h"

struct callTree_element{
	struct instruction* ins;
	struct codeMap*		cm;
};

/* Callbacks for the graph structure */
void* callTree_create_node(void* first_element);
int callTree_may_add_element(void* data, void* element);
int callTree_add_element(void* data, void* element);
int callTree_element_is_owned(void* data, void* element);
void callTree_node_printDot(void* data, FILE* file);
void callTree_delete_node(void* data);


void callTree_print_opcode_percent(struct graph* callTree);

#endif