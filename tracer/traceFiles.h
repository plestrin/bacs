#ifndef TRACEFILE_H
#define TRACEFILE_H

#include <stdio.h>
#include "codeMap.h"

#define TRACEFILES_MAX_NAME_SIZE 256

#define TRACEFILES_INS_FILE_NAME	"ins.json"
#define TRACEFILES_CM_FILE_NAME		"cm.json"

struct traceFiles{
	char	dir_name[TRACEFILES_MAX_NAME_SIZE];

	FILE* 	ins_file;
	long	ins_header_position;

	FILE*	cm_file;
};


struct traceFiles* traceFiles_create(const char* dir_name);
void traceFiles_print_codeMap(struct traceFiles* trace, struct codeMap* cm);
void traceFiles_delete(struct traceFiles* trace);

#endif