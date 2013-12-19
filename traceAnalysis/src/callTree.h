#ifndef CALLTREE_H
#define CALLTREE_H

#include "instruction.h"
#include "codeMap.h"
#include "traceFragment.h"
#include "graph.h"

struct callTree_element{
	struct instruction* ins;
	struct codeMap*		cm;
};

/* This structure can be only define in the source - remove dependency to traceFragement.h */
struct callTree_node{
	struct traceFragment 	fragment;
	unsigned long			entry_address;
	char 					name[CODEMAP_DEFAULT_NAME_SIZE];
};

void* callTree_create_node(void* first_element);
int callTree_may_add_element(void* data, void* element);
int callTree_add_element(void* data, void* element);
int callTree_element_is_owned(void* data, void* element);
void callTree_node_printDot(void* data, FILE* file);
void callTree_delete_node(void* data);

#endif