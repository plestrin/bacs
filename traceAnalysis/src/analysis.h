#ifndef ANALYSIS_H
#define ANALYSIS_H

#include "array.h"
#include "codeMap.h"
#include "traceReaderJSON.h"
#include "cmReaderJSON.h"
#include "ioChecker.h"
#include "loop.h"

#define ANALYSIS_DIRECTORY_NAME_MAX_LENGTH 256
#define ANALYSIS_INS_FILE_NAME "ins.json"
#define ANALYSIS_CM_FILE_NAME "cm.json"

struct analysis{

	/*a supprimer par la suite - pas besoin de garder de référence */
	char directory_name[ANALYSIS_DIRECTORY_NAME_MAX_LENGTH];

	/*a supprimer par la suite - pas besoin de garder de référence */
	union {
		struct traceReaderJSON 	json;
	} 							ins_reader;

	struct ioChecker*			checker;
	struct codeMap* 			code_map;

	struct loopEngine*			loop_engine;

	struct array				frag_array;
	struct array 				arg_array;
};

struct analysis* analysis_create(const char* dir_name);

void analysis_instruction_print(struct analysis* analysis);
void analysis_instruction_export(struct analysis* analysis);

void analysis_loop_create(struct analysis* analysis);
void analysis_loop_remove_redundant(struct analysis* analysis);
void analysis_loop_pack_epilogue(struct analysis* analysis);
void analysis_loop_print(struct analysis* analysis);
void analysis_loop_export(struct analysis* analysis, char* arg);
void analysis_loop_delete(struct analysis* analysis);

void analysis_frag_clean(struct analysis* analysis);
void analysis_frag_print_stat(struct analysis* analysis, char* arg);
void analysis_frag_print_ins(struct analysis* analysis, char* arg);
void analysis_frag_print_percent(struct analysis* analysis);
void analysis_frag_print_register(struct analysis* analysis, char* arg);
void analysis_frag_print_memory(struct analysis* analysis, char* arg);
void analysis_frag_set_tag(struct analysis* analysis, char* arg);
void analysis_frag_locate(struct analysis* analysis, char* arg);
void analysis_frag_extract_arg(struct analysis* analysis, char* arg);

void analysis_arg_clean(struct analysis* analysis);
void analysis_arg_print(struct analysis* analysis, char* arg);
void analysis_arg_set_tag(struct analysis* analysis, char* arg);
void analysis_arg_search(struct analysis* analysis, char* arg);
void analysis_arg_seek(struct analysis* analysis, char* arg);


void analysis_delete(struct analysis* analysis);

#endif