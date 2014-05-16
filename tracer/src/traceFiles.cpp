#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef __linux__
#include <sys/stat.h>
#endif

#ifdef WIN32
#include <Windows.h>
#include "windowsComp.h"
#endif

#include "traceFiles.h"


struct traceFiles* traceFiles_create(const char* dir_name){
	char 				file_name[TRACEFILES_MAX_NAME_SIZE];
	struct traceFiles* 	result;

	result = (struct traceFiles*)malloc(sizeof(struct traceFiles));
	if (result != NULL){
		strncpy(result->dir_name, dir_name, TRACEFILES_MAX_NAME_SIZE);
		mkdir(dir_name, 0777);

		/* file instruction */
		snprintf(file_name, TRACEFILES_MAX_NAME_SIZE, "%s/%s", dir_name, TRACEFILES_INS_FILE_NAME);
		#ifdef __linux__
		result->ins_file = fopen(file_name, "wb");
		#endif
		#ifdef WIN32
		fopen_s(&(result->ins_file), file_name, "wb");
		#endif

		/* file operand */
		snprintf(file_name, TRACEFILES_MAX_NAME_SIZE, "%s/%s", dir_name, TRACEFILES_OP_FILE_NAME);
		#ifdef __linux__
		result->op_file = fopen(file_name, "wb");
		#endif
		#ifdef WIN32
		fopen_s(&(result->op_file), file_name, "wb");
		#endif

		/* file data */
		snprintf(file_name, TRACEFILES_MAX_NAME_SIZE, "%s/%s", dir_name, TRACEFILES_DATA_FILE_NAME);
		#ifdef __linux__
		result->data_file = fopen(file_name, "wb");
		#endif
		#ifdef WIN32
		fopen_s(&(result->data_file), file_name, "wb");
		#endif

		/* file blockId */
		snprintf(file_name, TRACEFILES_MAX_NAME_SIZE, "%s/%s", dir_name, TRACEFILES_BLOCKID_FILE_NAME);
		#ifdef __linux__
		result->blockId_file = fopen(file_name, "wb");
		#endif
		#ifdef WIN32
		fopen_s(&(result->blockId_file), file_name, "wb");
		#endif

		/* file block */
		snprintf(file_name, TRACEFILES_MAX_NAME_SIZE, "%s/%s", dir_name, TRACEFILES_BLOCK_FILE_NAME);
		#ifdef __linux__
		result->block_file = fopen(file_name, "wb");
		#endif
		#ifdef WIN32
		fopen_s(&(result->block_file), file_name, "wb");
		#endif

		if (result->ins_file == NULL || result->op_file == NULL || result->data_file == NULL || result->blockId_file == NULL || result->block_file == NULL){
			if (result->ins_file == NULL){
				printf("ERROR: in %s, unable to create file \"%s\"\n", __func__, TRACEFILES_INS_FILE_NAME);
			}
			if (result->op_file == NULL){
				printf("ERROR: in %s, unable to create file \"%s\"\n", __func__, TRACEFILES_OP_FILE_NAME);
			}
			if (result->data_file == NULL){
				printf("ERROR: in %s, unable to create file \"%s\"\n", __func__, TRACEFILES_DATA_FILE_NAME);
			}
			if (result->blockId_file == NULL){
				printf("ERROR: in %s, unable to create file \"%s\"\n", __func__, TRACEFILES_BLOCKID_FILE_NAME);
			}
			if (result->block_file == NULL){
				printf("ERROR: in %s, unable to create file \"%s\"\n", __func__, TRACEFILES_BLOCK_FILE_NAME);
			}
		}
	}
	else{
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
	}

	return result;
}

void traceFiles_print_codeMap(struct traceFiles* trace_file, struct codeMap* cm){
	char file_name[TRACEFILES_MAX_NAME_SIZE];

	if (cm != NULL){
		snprintf(file_name, TRACEFILES_MAX_NAME_SIZE, "%s/%s", trace_file->dir_name, TRACEFILES_CM_FILE_NAME);

		#ifdef __linux__
		trace_file->cm_file = fopen(file_name, "w");
		#endif

		#ifdef WIN32
		fopen_s(&(trace_file->cm_file), file_name, "w");
		#endif

		if (trace_file->cm_file != NULL){
			codeMap_print_JSON(cm, trace_file->cm_file);
			fclose(trace_file->cm_file);
		}
		else{
			printf("ERROR: in %s, unable to open file: \"%s\"\n", __func__, file_name);
		}
	}
}

void traceFiles_delete(struct traceFiles* trace_file){
	if (trace_file->ins_file != NULL){
		fclose(trace_file->ins_file);
	}
	if (trace_file->op_file != NULL){
		fclose(trace_file->op_file);
	}
	if (trace_file->data_file != NULL){
		fclose(trace_file->data_file);
	}
	if (trace_file->blockId_file != NULL){
		fclose(trace_file->blockId_file);
	}
	if (trace_file->block_file != NULL){
		fclose(trace_file->block_file);
	}
	free(trace_file);
}