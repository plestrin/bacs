#ifndef TRACEFILE_H
#define TRACEFILE_H

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "codeMap.h"
#include "assembly.h"

#define TRACEFILE_NAME_MAX_LENGTH 		256

#define TRACEFILE_CM_FILE_NAME			"cm.json"
#define TRACEFILE_BLOCK_FILE_NAME 		"block.bin"

struct traceFile{
	char				dir_name[TRACEFILE_NAME_MAX_LENGTH];
	FILE* 				block_file;
	struct asmWriter 	asm_writer;
};

struct traceFile* traceFile_create(const char* dir_name);
int32_t traceFile_init(struct traceFile* trace_file, const char* dir_name);

void traceFile_print_codeMap(struct traceFile* trace_file, struct codeMap* cm);

#define traceFile_get_dir_name(trace_file) (trace_file)->dir_name

#define traceFile_write_block(trace_file, header) 										\
	(header).id = asmWrite_get_BlockId(&(trace_file->asm_writer)); 						\
	fwrite(&(header), sizeof(struct asmBlockHeader), 1, (trace_file)->block_file); 		\
	fwrite((void*)((header).address), 1, (header).size, (trace_file)->block_file);

void traceFile_clean(struct traceFile* trace_file);

#define traceFile_delete(trace_file) 													\
	traceFile_clean(trace_file); 														\
	free(trace_file);

#endif