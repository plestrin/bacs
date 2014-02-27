#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef __linux__

#include <sys/stat.h>

#endif

#ifdef WIN32

#include <Windows.h>

#endif

#include "traceFiles.h"

#ifdef WIN32

#ifndef __func__
#define __func__ __FUNCTION__
#endif

#define snprintf(str, size, format, ...) _snprintf_s((str), (size), _TRUNCATE, (format), __VA_ARGS__)
#define strncpy(dst, src, size) strncpy_s((dst), (size), (src), _TRUNCATE)

#endif

struct traceFiles* traceFiles_create(const char* dir_name){
	char file_name[TRACEFILES_MAX_NAME_SIZE];

	struct traceFiles* result = (struct traceFiles*)malloc(sizeof(struct traceFiles));
	if (result != NULL){
		strncpy(result->dir_name, dir_name, TRACEFILES_MAX_NAME_SIZE);

		#ifdef __linux__
		mkdir(dir_name, 0777);
		#endif

		#ifdef WIN32
		CreateDirectoryA(dir_name, NULL);
		#endif

		/* file instruction */
		snprintf(file_name, TRACEFILES_MAX_NAME_SIZE, "%s/%s", dir_name, TRACEFILES_INS_FILE_NAME);

		#ifdef __linux__
		result->ins_file = fopen(file_name, "w");
		#endif

		#ifdef WIN32
		fopen_s(&(result->ins_file), file_name, "w");
		#endif

		/* file operand */
		snprintf(file_name, TRACEFILES_MAX_NAME_SIZE, "%s/%s", dir_name, TRACEFILES_OP_FILE_NAME);

		#ifdef __linux__
		result->op_file = fopen(file_name, "w");
		#endif

		#ifdef WIN32
		fopen_s(&(result->op_file), file_name, "w");
		#endif

		/* file data */
		snprintf(file_name, TRACEFILES_MAX_NAME_SIZE, "%s/%s", dir_name, TRACEFILES_DATA_FILE_NAME);

		#ifdef __linux__
		result->data_file = fopen(file_name, "w");
		#endif

		#ifdef WIN32
		fopen_s(&(result->data_file), file_name, "w");
		#endif

		if (result->ins_file == NULL || result->op_file == NULL || result->data_file == NULL){
			if (result->ins_file == NULL){
				printf("ERROR: in %s, unable to create file \"%s\"\n", __func__, TRACEFILES_INS_FILE_NAME);
			}
			if (result->op_file == NULL){
				printf("ERROR: in %s, unable to create file \"%s\"\n", __func__, TRACEFILES_OP_FILE_NAME);
			}
			if (result->data_file == NULL){
				printf("ERROR: in %s, unable to create file \"%s\"\n", __func__, TRACEFILES_DATA_FILE_NAME);
			}
		}
	}
	else{
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
	}

	return result;
}

void traceFiles_print_codeMap(struct traceFiles* trace, struct codeMap* cm){
	char file_name[TRACEFILES_MAX_NAME_SIZE];

	if (cm != NULL){
		snprintf(file_name, TRACEFILES_MAX_NAME_SIZE, "%s/%s", trace->dir_name, TRACEFILES_CM_FILE_NAME);

		#ifdef __linux__
		trace->cm_file = fopen(file_name, "w");
		#endif

		#ifdef WIN32
		fopen_s(&(trace->cm_file), file_name, "w");
		#endif

		if (trace->cm_file != NULL){
			codeMap_print_JSON(cm, trace->cm_file);
			fclose(trace->cm_file);
		}
		else{
			printf("ERROR: in %s, unable to open file: \"%s\"\n", __func__, file_name);
		}
	}
}

void traceFiles_delete(struct traceFiles* trace){
	if (trace->ins_file != NULL){
		fclose(trace->ins_file);
	}
	if (trace->op_file != NULL){
		fclose(trace->op_file);
	}
	if (trace->data_file != NULL){
		fclose(trace->data_file);
	}
	free(trace);
}