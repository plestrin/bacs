#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef __linux__
#include <sys/stat.h>
#endif

#include "traceFiles.h"

struct traceFiles* traceFiles_create(const char* dir_name){
	char file_name[TRACEFILES_MAX_NAME_SIZE];

	struct traceFiles* result = (struct traceFiles*)malloc(sizeof(struct traceFiles));
	if (result != NULL){
		strncpy(result->dir_name, dir_name, TRACEFILES_MAX_NAME_SIZE);

		#ifdef __linux__
		mkdir(dir_name, 0777);
		#endif

		snprintf(file_name, TRACEFILES_MAX_NAME_SIZE, "%s/%s", dir_name, TRACEFILES_INS_FILE_NAME);
		result->ins_file = fopen(file_name, "w");
		if (result->ins_file == NULL){
			printf("ERROR: unable to create file \"%s\"\n", file_name);
		}
		else{
			fprintf(result->ins_file, "{\"trace\":[");
			result->ins_header_position = ftell(result->ins_file);
		}
	}
	else{
		printf("ERROR: unable to allocate memory\n");
	}
	return result;
}

void traceFiles_print_codeMap(struct traceFiles* trace, struct codeMap* cm){
	char file_name[TRACEFILES_MAX_NAME_SIZE];

	if (trace != NULL){
		if (cm != NULL){
			snprintf(file_name, TRACEFILES_MAX_NAME_SIZE, "%s/%s", trace->dir_name, TRACEFILES_CM_FILE_NAME);
			trace->cm_file = fopen(file_name, "w");
			if (trace->cm_file != NULL){
				codeMap_print_JSON(cm, trace->cm_file);
				fclose(trace->cm_file);
			}
			else{
				printf("ERROR: in %s, unable to open file: \"%s\"\n", __func__, file_name);
			}
		}
	}
}

void traceFiles_delete(struct traceFiles* trace){

	if (trace != NULL){
		if (trace->ins_file != NULL){
			if (ftell(trace->ins_file) != trace->ins_header_position){
				if (fseek(trace->ins_file, -1, SEEK_CUR)){
					printf("ERROR: unable to set cursor position, trace file might be incorrect\n");
				}
			}
			fprintf(trace->ins_file, "]}");
			fclose(trace->ins_file);
		}
		free(trace);
	}
}