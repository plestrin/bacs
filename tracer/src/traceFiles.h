#ifndef TRACEFILE_H
#define TRACEFILE_H

#include <stdint.h>
#include <stdio.h>

#include "codeMap.h"
#include "instruction.h"
#include "assembly.h"

#define TRACEFILES_MAX_NAME_SIZE 		256

#define TRACEFILES_INS_FILE_NAME		"ins.bin"
#define TRACEFILES_OP_FILE_NAME			"op.bin"
#define TRACEFILES_DATA_FILE_NAME		"data.bin"
#define TRACEFILES_CM_FILE_NAME			"cm.json"
#define TRACEFILES_BLOCKID_FILE_NAME 	"blockId.bin"
#define TRACEFILES_BLOCK_FILE_NAME 		"block.bin"

struct traceFiles{
	char	dir_name[TRACEFILES_MAX_NAME_SIZE];

	FILE* 	ins_file;
	FILE* 	op_file;
	FILE* 	data_file;

	FILE*	cm_file;

	FILE*	blockId_file;
	FILE* 	block_file;
};


struct traceFiles* traceFiles_create(const char* dir_name);
void traceFiles_print_codeMap(struct traceFiles* trace_file, struct codeMap* cm);

#define traceFiles_flush_instruction(trace_file, buffer, nb_instruction) 	fwrite((buffer), sizeof(struct instruction), (nb_instruction), (trace_file)->ins_file)
#define traceFiles_flush_operand(trace_file, buffer, nb_operand) 			fwrite((buffer), sizeof(struct operand), (nb_operand), (trace_file)->op_file)
#define traceFiles_flush_data(trace_file, buffer, nb_data) 					fwrite((buffer), 1, (nb_data), (trace_file)->data_file)

#define traceFiles_write_blockId(trace_file, id) 							fwrite((id), sizeof(uint32_t), 1, (trace_file)->blockId_file)
#define traceFiles_write_block(trace_file, header) 																														\
	fwrite((header), sizeof(struct asmBlockHeader), 1, (trace_file)->block_file); 																						\
	fwrite((void*)((header)->address), 1, (header)->size, (trace_file)->block_file);

void traceFiles_delete(struct traceFiles* trace_file);

#endif