#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "traceFragment.h"
#include "callTree.h"
#include "multiColumn.h"

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

void callTree_delete_node(void* data){
	struct callTree_node* node = (struct callTree_node*)data;

	traceFragment_clean(&(node->fragment));
	free(node);
}

void callTree_node_printDot(void* data, FILE* file){
	struct callTree_node* node = (struct callTree_node*)data;

	fprintf(file, "[label=\"%s\"]", node->name);
}

void callTree_print_opcode_percent(struct graph* callTree){
	struct multiColumnPrinter* 	printer;
	int 						i;
	struct callTree_node* 		node;
	float 						percent;
	char 						percent_string[32];
	int 						nb_opcode = 3;
	uint32_t 					opcode[3] = {XED_ICLASS_XOR, XED_ICLASS_SHL, XED_ICLASS_SHR};
	int 						nb_excluded_opcode = 3;
	uint32_t 					excluded_opcode[3] = {XED_ICLASS_MOV, XED_ICLASS_PUSH, XED_ICLASS_POP};


	if (callTree != NULL){
		printer = multiColumnPrinter_create(stdout, 2, NULL, NULL);

		if (printer != NULL){
			multiColumnPrinter_set_column_size(printer, 0, 24);
			multiColumnPrinter_set_column_size(printer, 1, 16);

			multiColumnPrinter_set_title(printer, 0, (char*)"FRAGMENT");
			multiColumnPrinter_set_title(printer, 1, (char*)"PERCENT");
			multiColumnPrinter_print_header(printer);

			for (i = 0; i < callTree->nb_node; i++){
				node = (struct callTree_node*)callTree->nodes[i].data;
				percent = traceFragment_opcode_percent(&(node->fragment), nb_opcode, opcode, nb_excluded_opcode, excluded_opcode);
				snprintf(percent_string, 32, "%f", percent*100);
				multiColumnPrinter_print(printer, node->name, percent_string, NULL);
			}

			multiColumnPrinter_delete(printer);
		}
		else{
			printf("ERROR: in %s, unable to create multi column printer\n", __func__);
		}
	}
}