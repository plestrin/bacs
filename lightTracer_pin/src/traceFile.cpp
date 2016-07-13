#include "pin.H"

#ifdef _WIN32
#include "windowsComp.h"
#endif

#include "traceFile.h"

struct traceFile* traceFile_create(const char* dir_name, uint32_t pid){
	struct traceFile* trace_file;

	trace_file = (struct traceFile*)malloc(sizeof(struct traceFile));
	if (trace_file != NULL){
		if (traceFile_init(trace_file, dir_name, pid)){
			LOG("ERROR: unable to initialize traceFile\n");
			free(trace_file);
			trace_file = NULL;
		}
	}
	else{
		LOG("ERROR: unable to allocate memory\n");
	}

	return trace_file;
}

int32_t traceFile_init(struct traceFile* trace_file, const char* dir_name, uint32_t pid){
	char file_name[TRACEFILE_NAME_MAX_LENGTH];

	asmWriter_init(&(trace_file->asm_writer));

	strncpy(trace_file->dir_name, dir_name, TRACEFILE_NAME_MAX_LENGTH);
	#ifdef __linux__
	if (OS_MkDir(dir_name, 0777).generic_err != OS_RETURN_CODE_NO_ERROR){
		LOG("ERROR: unable to create directory \"");
		LOG(dir_name);
		LOG("\"\n");
		return -1;
	}
	#endif

	snprintf(file_name, TRACEFILE_NAME_MAX_LENGTH, "%s/block%u.bin", dir_name, pid);
	if ((trace_file->block_file = fopen(file_name, "wb")) == NULL){
		LOG("ERROR: unable to create file \"");
		LOG(file_name);
		LOG("\"\n");
		return -1;
	}

	trace_file->pid = pid;

	return 0;
}

void traceFile_print_codeMap(struct traceFile* trace_file, struct codeMap* code_map){
	char 	file_name[TRACEFILE_NAME_MAX_LENGTH];
	FILE* 	codeMap_file;

	snprintf(file_name, TRACEFILE_NAME_MAX_LENGTH, "%s/cm%u.json", trace_file->dir_name, trace_file->pid);

	if ((codeMap_file = fopen(file_name, "w")) != NULL){
		codeMap_print_JSON(code_map, codeMap_file);
		fclose(codeMap_file);
	}
	else{
		LOG("ERROR: unable to open file: \"");
		LOG(file_name);
		LOG("\"\n");
	}
}

void traceFile_clean(struct traceFile* trace_file){
	if (trace_file->block_file != NULL){
		fclose(trace_file->block_file);
	}
}