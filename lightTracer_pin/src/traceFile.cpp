#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <iostream>

#ifdef __linux__
#include <sys/stat.h>
#endif

#ifdef WIN32
#include <Windows.h>
#include "windowsComp.h"
#endif

#include "traceFile.h"

struct traceFile* traceFile_create(const char* dir_name, uint32_t pid){
	struct traceFile* trace_file;

	trace_file = (struct traceFile*)malloc(sizeof(struct traceFile));
	if (trace_file != NULL){
		if (traceFile_init(trace_file, dir_name, pid)){
			std::cerr << "ERROR: in " << __func__ << ", unable to initialize traceFile" << std::endl;
			free(trace_file);
			trace_file = NULL;
		}
	}
	else{
		std::cerr << "ERROR: in " << __func__ << ", unable to allocate memory" << std::endl;
	}

	return trace_file;
}

int32_t traceFile_init(struct traceFile* trace_file, const char* dir_name, uint32_t pid){
	char file_name[TRACEFILE_NAME_MAX_LENGTH];

	asmWriter_init(&(trace_file->asm_writer));

	strncpy(trace_file->dir_name, dir_name, TRACEFILE_NAME_MAX_LENGTH);
	mkdir(dir_name, 0777);

	snprintf(file_name, TRACEFILE_NAME_MAX_LENGTH, "%s/block%u.bin", dir_name, pid);
	#ifdef __linux__
	trace_file->block_file = fopen(file_name, "wb");
	#endif
	#ifdef WIN32
	fopen_s(&(trace_file->block_file), file_name, "wb");
	#endif

	if (trace_file->block_file == NULL){
		std::cerr << "ERROR: in " << __func__ << ", unable to create file \"" << file_name << "\"" << std::endl;
		return -1;
	}

	trace_file->pid = pid;

	return 0;
}

void traceFile_print_codeMap(struct traceFile* trace_file, struct codeMap* code_map){
	char 	file_name[TRACEFILE_NAME_MAX_LENGTH];
	FILE* 	codeMap_file;

	snprintf(file_name, TRACEFILE_NAME_MAX_LENGTH, "%s/cm%u.json", trace_file->dir_name, trace_file->pid);

	#ifdef __linux__
	codeMap_file = fopen(file_name, "w");
	#endif

	#ifdef WIN32
	fopen_s(&(codeMap_file), file_name, "w");
	#endif

	if (codeMap_file != NULL){
		codeMap_print_JSON(code_map, codeMap_file);
		fclose(codeMap_file);
	}
	else{
		std::cerr << "ERROR: in " << __func__ << ", unable to open file: \"" << file_name << "\"" << std::endl;
	}
}

void traceFile_clean(struct traceFile* trace_file){
	if (trace_file->block_file != NULL){
		fclose(trace_file->block_file);
	}
}