#ifndef ANALYSIS_H
#define ANALYSIS_H

#include "array.h"
#include "codeMap.h"
#include "loop.h"
#include "trace.h"
#include "codeSignature.h"


struct analysis{
	struct trace* 					trace;
	struct codeMap* 				code_map;

	struct loopEngine*				loop_engine;

	struct codeSignatureCollection code_signature_collection;

	struct array					frag_array;
};

struct analysis* analysis_create();

void analysis_trace_load(struct analysis* analysis, char* arg);
void analysis_trace_print(struct analysis* analysis, char* arg);
void analysis_trace_print_asm(struct analysis* analysis, char* arg);
void analysis_trace_check(struct analysis* analysis);
void analysis_trace_check_codeMap(struct analysis* analysis);
void analysis_trace_print_codeMap(struct analysis* analysis, char* arg);
void analysis_trace_export(struct analysis* analysis, char* arg);
void analysis_trace_locate_pc(struct analysis* analysis, char* arg);
void analysis_trace_delete(struct analysis* analysis);

void analysis_loop_create(struct analysis* analysis, char* arg);
void analysis_loop_remove_redundant(struct analysis* analysis, char* arg);
void analysis_loop_print(struct analysis* analysis);
void analysis_loop_export(struct analysis* analysis, char* arg);
void analysis_loop_delete(struct analysis* analysis);

void analysis_frag_print_stat(struct analysis* analysis, char* arg);
void analysis_frag_print_ins(struct analysis* analysis, char* arg);
void analysis_frag_print_asm(struct analysis* analysis, char* arg);
void analysis_frag_print_percent(struct analysis* analysis);
void analysis_frag_set_tag(struct analysis* analysis, char* arg);
void analysis_frag_locate(struct analysis* analysis, char* arg);
void analysis_frag_clean(struct analysis* analysis);

void analysis_frag_create_ir(struct analysis* analysis, char* arg);
void analysis_frag_printDot_ir(struct analysis* analysis, char* arg);
void analysis_frag_normalize_ir(struct analysis* analysis, char* arg);

void analysis_code_signature_search(struct analysis* analysis, char* arg);

void analysis_delete(struct analysis* analysis);

#endif