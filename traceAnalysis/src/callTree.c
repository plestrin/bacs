#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "traceFragment.h"
#include "callTree.h"

struct callTree_node{
	struct traceFragment 	fragment;
	unsigned long			entry_address;
	char 					name[CODEMAP_DEFAULT_NAME_SIZE];
};

void* callTree_create_node(void* first_element){
	struct callTree_node* 		node;
	struct cm_routine* 			routine;
	struct callTree_element* 	element = (struct callTree_element*)first_element;

	node = (struct callTree_node*)malloc(sizeof(struct callTree_node));
	if (node != NULL){
		traceFragment_init(&(node->fragment));

		if (traceFragment_add_instruction(&(node->fragment), element->ins)){
			printf("ERROR: in %s, unable to add instruction to code fragment\n", __func__);
			traceFragment_clean(&(node->fragment));
			free(node);
			node = NULL;
		}
		else{
			routine = codeMap_search_routine(element->cm, element->ins->pc);
			if (routine != NULL){
				node->entry_address = routine->address_start;
				memcpy(node->name, routine->name, CODEMAP_DEFAULT_NAME_SIZE);
			}
			else{
				printf("WARNING: in %s, instruction @ 0x%lx, does not be belong to any routine\n", __func__, element->ins->pc);

				node->entry_address = element->ins->pc;
				memset(node->name, '\0', CODEMAP_DEFAULT_NAME_SIZE);
			}
		}
	}

	return (void*)node;
}

int callTree_may_add_element(void* data, void* element){
	struct callTree_node* 		node = (struct callTree_node*)data;
	struct cm_routine* 			routine;
	struct callTree_element* 	el = (struct callTree_element*)element;
	int 						result = -1;
	struct instruction* 		previous_instruction;

	routine = codeMap_search_routine(el->cm, el->ins->pc);
	if (routine != NULL){
		if (routine->address_start == node->entry_address){
			result = 0;
		}
	}
	else{
		if (traceFragment_search_pc(&(node->fragment), el->ins) >= 0){
			result = 0;
		}
		else{
			previous_instruction = traceFragment_get_last_instruction(&(node->fragment));
			if (previous_instruction != NULL){
				/* Idealy in case of a CALL check if the dst is whitelisted */
				if (previous_instruction->opcode != XED_ICLASS_RET_FAR && previous_instruction->opcode != XED_ICLASS_RET_NEAR && previous_instruction->opcode != XED_ICLASS_CALL_FAR && previous_instruction->opcode != XED_ICLASS_CALL_NEAR){
					result = 0;
				}
			}
		}
	}

	return result;
}

int callTree_element_is_owned(void* data, void* element){
	struct callTree_node* 		node = (struct callTree_node*)data;
	struct cm_routine* 			routine;
	struct callTree_element* 	el = (struct callTree_element*)element;
	int 						result = -1;

	routine = codeMap_search_routine(el->cm, el->ins->pc);
	if (routine != NULL){
		if (routine->address_start == node->entry_address){
			result = 0;
		}
	}
	else{
		if (traceFragment_search_pc(&(node->fragment), el->ins) >= 0){
			result = 0;
		}
	}

	return result;
}

int callTree_add_element(void* data, void* element){
	struct callTree_node* 		node = (struct callTree_node*)data;
	struct callTree_element*	el = (struct callTree_element*)element;
	int 						result = 0;

	if (traceFragment_add_instruction(&(node->fragment), el->ins) < 0){
		printf("ERROR: in %s, unable to add instruction to code fragment\n", __func__);
		result = -1;
	}

	return result;
}

void callTree_printDot_node(FILE* file, void* data){
	struct callTree_node* node = (struct callTree_node*)data;

	fprintf(file, "[label=\"%s\"]", node->name);
}

void callTree_delete_node(void* data){
	struct callTree_node* node = (struct callTree_node*)data;

	traceFragment_clean(&(node->fragment));
	free(node);
}