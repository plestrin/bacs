#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#include "trace.h"
#include "instruction.h"
#include "simpleTraceStat.h"


struct trace* trace_create(const char* dir_name){
	struct trace* 	ptrace;
	char			ins_file_name[TRACE_DIRECTORY_NAME_MAX_LENGTH];

	ptrace = (struct trace*)malloc(sizeof(struct trace));
	if (ptrace == NULL){
		printf("ERROR: in %s, unable to alloacte memory\n", __func__);
		return NULL;
	}

	snprintf(ins_file_name, TRACE_DIRECTORY_NAME_MAX_LENGTH, "%s/%s", dir_name, TRACE_INS_FILE_NAME);
	if (traceReaderJSON_init(&(ptrace->ins_reader.json), ins_file_name)){
		printf("ERROR: in %s, unable to init trace JSON reader\n", __func__);
		free(ptrace);
		return NULL;
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

void trace_delete(struct trace* ptrace){
	if (ptrace != NULL){
		traceReaderJSON_clean(&(ptrace->ins_reader.json));
		free(ptrace);
	}
}