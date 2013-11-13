#ifndef CALLTREE_H
#define CALLTREE_H

#include "instruction.h"
#include "codeMap.h"

struct callTree_element{
	struct instruction* ins;
	struct codeMap*		cm;
};

void* callTree_create_node(void* first_element);
int callTree_may_add_element(void* data, void* element);
int callTree_add_element(void* data, void* element);
int callTree_element_is_owned(void* data, void* element);
void callTree_printDot_node(FILE* file, void* data);
void callTree_delete_node(void* data);

#endif