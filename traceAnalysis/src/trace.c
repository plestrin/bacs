#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#include "trace.h"
#include "instruction.h"
#include "simpleTraceStat.h"


struct trace* trace_create(const char* dir_name){
	struct trace* 	ptrace;
	char			file_name[TRACE_DIRECTORY_NAME_MAX_LENGTH];

	ptrace = (struct trace*)malloc(sizeof(struct trace));
	if (ptrace == NULL){
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
		return NULL;
	}

	snprintf(file_name, TRACE_DIRECTORY_NAME_MAX_LENGTH, "%s/%s", dir_name, TRACE_INS_FILE_NAME);
	if (traceReaderJSON_init(&(ptrace->ins_reader.json), file_name)){
		printf("ERROR: in %s, unable to init trace JSON reader\n", __func__);
		free(ptrace);
		return NULL;
	}

	snprintf(file_name, TRACE_DIRECTORY_NAME_MAX_LENGTH, "%s/%s", dir_name, TRACE_CM_FILE_NAME);
	ptrace->cm = cmReaderJSON_parse_trace(file_name);
	if (ptrace->cm == NULL){
		printf("WARNING: in %s, continue without code map information\n", __func__);
	}

	return ptrace;
}

void trace_simple_traversal(struct trace* ptrace){
	struct instruction* ins;

	do{
		ins = traceReaderJSON_get_next_instruction(&(ptrace->ins_reader.json));
	} while (ins != NULL);
}

void trace_print_instructions(struct trace* ptrace){
	struct instruction* ins;

	do{
		ins = traceReaderJSON_get_next_instruction(&(ptrace->ins_reader.json));
		if (ins != NULL){
			instruction_print(ins);
		}
	} while (ins != NULL);
}

void trace_print_simpleTraceStat(struct trace* ptrace){
	struct instruction* 	ins;
	struct simpleTraceStat* stat;

	stat = simpleTraceStat_create();
	do{
		ins = traceReaderJSON_get_next_instruction(&(ptrace->ins_reader.json));
		if (ins != NULL){
			if (simpleTraceStat_add_instruction(stat, ins)){
				printf("ERROR: in %s, unable to add instruction to simpleTraceStat\n", __func__);
			}
		}
	} while (ins != NULL);

	simpleTraceStat_print(stat);
	simpleTraceStat_delete(stat);
}

void trace_print_codeMap(struct trace* ptrace){
	if (ptrace->cm != NULL){
		codeMap_check_address(ptrace->cm);
		/* We need to find a way to set the filter from the command line */
		codeMap_print(ptrace->cm, CODEMAP_FILTER_EXECUTED);
	}
	else{
		printf("ERROR: in %s, unable to print code map cause it's NULL\n", __func__);
	}
}

struct controlFlowGraph* trace_construct_flow_graph(struct trace* ptrace){
	struct controlFlowGraph* 	cfg;
	struct instruction*			ins;

	cfg = controlFlowGraph_create();
	if (cfg == NULL){
		printf("ERROR: in %s, unable to create CFG\n", __func__);
	}
	else{
		do{
			ins = traceReaderJSON_get_next_instruction(&(ptrace->ins_reader.json));
			if (ins != NULL){
				if (controlFlowGraph_add(cfg, ins)){
					printf("ERROR: in %s, unable to add instruction to CFG\n", __func__);
					break;
				}
			}
		} while (ins != NULL);
	}

	return cfg;
}

struct graph* trace_construct_call_tree(struct trace* ptrace){
	struct graph* 				callTree;
	struct instruction*			ins;
	struct callTree_element		element;

	callTree = graph_create();
	if (callTree != NULL){
		callTree->callback_node.create_data 		= callTree_create_node;
		callTree->callback_node.may_add_element 	= callTree_contain_element;
		callTree->callback_node.add_element 		= callTree_add_element;
		callTree->callback_node.element_is_owned 	= callTree_contain_element;
		callTree->callback_node.print_dot 			= callTree_printDot_node;
		callTree->callback_node.delete_data 		= callTree_delete_node;

		do{
			ins = traceReaderJSON_get_next_instruction(&(ptrace->ins_reader.json));
			if (ins != NULL){
				element.ins = ins;
				element.cm = ptrace->cm;
				if (graph_add_element(callTree, &element)){
					printf("ERROR: in %s, unable to add instruction to call tree\n", __func__);
					break;
				}
			}
		} while (ins != NULL);

	}
	else{
		printf("ERROR: in %s, unable to create graph\n", __func__);
	}

	return callTree;
}

void trace_delete(struct trace* ptrace){
	if (ptrace != NULL){
		traceReaderJSON_clean(&(ptrace->ins_reader.json));

		if (ptrace->cm != NULL){
			codeMap_delete(ptrace->cm);
		}

		free(ptrace);
	}
}