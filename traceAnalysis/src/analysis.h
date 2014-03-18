#ifndef ANALYSIS_H
#define ANALYSIS_H

#include "array.h"
#include "codeMap.h"
#include "cmReaderJSON.h"
#include "ioChecker.h"
#include "cstChecker.h"
#include "loop.h"
#include "trace.h"


struct analysis{
	struct trace* 				trace;
	struct codeMap* 			code_map;

	struct ioChecker			io_checker;
	struct cstChecker 			cst_checker;

	struct loopEngine*			loop_engine;

	struct array				frag_array;
	struct array 				arg_array;
};

struct analysis* analysis_create();

void analysis_trace_load(struct analysis* analysis, char* arg);
void analysis_trace_print(struct analysis* analysis, char* arg);
void analysis_trace_check(struct analysis* analysis);
void analysis_trace_check_codeMap(struct analysis* analysis);
void analysis_trace_print_codeMap(struct analysis* analysis, char* arg);
void analysis_trace_search_constant(struct analysis* analysis);
void analysis_trace_export(struct analysis* analysis, char* arg);
void analysis_trace_delete(struct analysis* analysis);

void analysis_loop_create(struct analysis* analysis);
void analysis_loop_remove_redundant(struct analysis* analysis);
void analysis_loop_pack_epilogue(struct analysis* analysis);
void analysis_loop_print(struct analysis* analysis);
void analysis_loop_export(struct analysis* analysis, char* arg);
void analysis_loop_delete(struct analysis* analysis);

void analysis_frag_print_stat(struct analysis* analysis, char* arg);
void analysis_frag_print_ins(struct analysis* analysis, char* arg);
void analysis_frag_print_percent(struct analysis* analysis);
void analysis_frag_print_register(struct analysis* analysis, char* arg);
void analysis_frag_print_memory(struct analysis* analysis, char* arg);
void analysis_frag_set_tag(struct analysis* analysis, char* arg);
void analysis_frag_locate(struct analysis* analysis, char* arg);
void analysis_frag_extract_arg(struct analysis* analysis, char* arg);
void analysis_frag_analyse_operand(struct analysis* analysis, char* arg);
void analysis_frag_clean(struct analysis* analysis);

void analysis_arg_print(struct analysis* analysis, char* arg);
void analysis_arg_set_tag(struct analysis* analysis, char* arg);
void analysis_arg_search(struct analysis* analysis, char* arg);
void analysis_arg_seek(struct analysis* analysis, char* arg);
void analysis_arg_clean(struct analysis* analysis);

void analysis_delete(struct analysis* analysis);

#endif