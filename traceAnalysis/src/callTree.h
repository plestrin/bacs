#ifndef CALLTREE_H
#define CALLTREE_H

#include "instruction.h"
#include "codeMap.h"

struct callTree_element{
	struct instruction* ins;
	struct codeMap*		cm;
};

void* callTree_create_codeSegment(void* first_element);
int callTree_contain_element(void* data, void* element);
int callTree_add_element(void* data, void* element);
void callTree_delete_node(void* data);

#endif