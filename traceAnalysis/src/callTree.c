#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "codeSegment.h"
#include "callTree.h"

struct callTree_node{
	struct codeSegment 	segment;
	unsigned long		entry_address;
	char 				name[CODEMAP_DEFAULT_NAME_SIZE];
	int 				nb_execution;
};

void* callTree_create_node(void* first_element){
	struct callTree_node* 		node;
	struct cm_routine* 			routine;
	struct callTree_element* 	element = (struct callTree_element*)first_element;

	node = (struct callTree_node*)malloc(sizeof(struct callTree_node));
	if (node != NULL){
		codeSegment_init(&(node->segment));

		if (codeSegment_add_instruction(&(node->segment), element->ins)){
			printf("ERROR: in %s, unable to add instruction to code segment\n", __func__);
			codeSegment_clean(&(node->segment));
			free(node);
			node = NULL;
		}
		else{
			node->nb_execution = 1;
			
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

int callTree_contain_element(void* data, void* element){
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
		if (el->ins->pc == node->entry_address){
			result = 0;
		}
	}

	return result;
}

int callTree_add_element(void* data, void* element){
	struct callTree_node* 		node = (struct callTree_node*)data;
	struct callTree_element*	el = (struct callTree_element*)element;
	int 						ins_offset;
	int 						result = 0;

	ins_offset = codeSegment_search_instruction(&(node->segment), el->ins);
	if (ins_offset < 0){
		if (codeSegment_add_instruction(&(node->segment), el->ins) < 0){
			printf("ERROR: in %s, unable to add instruction to code segment\n", __func__);
			result = -1;
		}
		else{
			if (el->ins->pc == node->entry_address){
				node->nb_execution ++;
			}
		}
	}

	return result;
}

void callTree_printDot_node(FILE* file, void* data){
	struct callTree_node* node = (struct callTree_node*)data;

	fprintf(file, "[label=\"%s\"]", node->name);
}

void callTree_delete_node(void* data){
	struct callTree_node* node = (struct callTree_node*)data;

	codeSegment_clean(&(node->segment));
	free(node);
}