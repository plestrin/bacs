#ifndef TRACEFILE_H
#define TRACEFILE_H

#include <stdint.h>
#include <stdio.h>

#ifdef __linux__

#include "codeMap.h"
#include "instruction.h"

#endif

#ifdef WIN32

#include "../../shared/codeMap.h"
#include "../../shared/instruction.h"

#endif

#define TRACEFILES_MAX_NAME_SIZE 	256

#define TRACEFILES_INS_FILE_NAME	"ins.bin"
#define TRACEFILES_OP_FILE_NAME		"op.bin"
#define TRACEFILES_DATA_FILE_NAME	"data.bin"
#define TRACEFILES_CM_FILE_NAME		"cm.json"

struct traceFiles{
	char	dir_name[TRACEFILES_MAX_NAME_SIZE];

	FILE* 	ins_file;
	FILE* 	op_file;
	FILE* 	data_file;

	FILE*	cm_file;
};


struct traceFiles* traceFiles_create(const char* dir_name);
void traceFiles_print_codeMap(struct traceFiles* trace, struct codeMap* cm);

static inline void traceFiles_print_instruction(struct traceFiles* trace, struct instruction* buffer, uint32_t nb_instruction){
	/*instruction_flush_tracer_buffer(trace->ins_file, buffer, nb_instruction);*/
	_instruction_flush_tracer_buffer(trace->ins_file, trace->op_file, trace->data_file, buffer, nb_instruction);
}

void traceFiles_delete(struct traceFiles* trace);

#endif