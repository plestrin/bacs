#ifndef ANALYSIS_H
#define ANALYSIS_H

#include "array.h"
#include "codeMap.h"
#include "trace.h"
#include "codeSignature.h"
#include "callGraph.h"

struct analysis{
	struct trace* 					trace;
	struct codeMap* 				code_map;
	struct codeSignatureCollection 	code_signature_collection;
	struct callGraph* 				call_graph;
	struct array					frag_array;
};

struct analysis* analysis_create();

void analysis_trace_load(struct analysis* analysis, char* arg);
void analysis_trace_change_thread(struct analysis* analysis, char* arg);
void analysis_trace_load_elf(struct analysis* analysis, char* arg);
void analysis_trace_print(struct analysis* analysis, char* arg);
void analysis_trace_check(struct analysis* analysis);
void analysis_trace_check_codeMap(struct analysis* analysis);
void analysis_trace_print_codeMap(struct analysis* analysis, char* arg);
void analysis_trace_export(struct analysis* analysis, char* arg);
void analysis_trace_locate_pc(struct analysis* analysis, char* arg);
void analysis_trace_delete(struct analysis* analysis);

void analysis_frag_print(struct analysis* analysis, char* arg);
void analysis_frag_set_tag(struct analysis* analysis, char* arg);
void analysis_frag_locate(struct analysis* analysis, char* arg);
void analysis_frag_concat(struct analysis* analysis, char* arg);
void analysis_frag_check(struct analysis* analysis, char* arg);
void analysis_frag_print_result(struct analysis* analysis, char* arg);
void analysis_frag_export_result(struct analysis* analysis, char* arg);
void analysis_frag_clean(struct analysis* analysis);

void analysis_frag_create_ir(struct analysis* analysis, char* arg);
void analysis_frag_printDot_ir(struct analysis* analysis, char* arg);
void analysis_frag_normalize_ir(struct analysis* analysis, char* arg);
void analysis_frag_check_ir(struct analysis* analysis, char* arg);
void analysis_frag_print_aliasing_ir(struct analysis* analysis, char* arg);
void analysis_frag_simplify_concrete_ir(struct analysis* analysis, char* arg);

void analysis_code_signature_search(struct analysis* analysis, char* arg);
void analysis_code_signature_clean(struct analysis* analysis);

void analysis_call_create(struct analysis* analysis, char* arg);
void analysis_call_printDot(struct analysis* analysis);
void analysis_call_check(struct analysis* analysis);
void analysis_call_export(struct analysis* analysis, char* arg);
void analysis_call_print_stack(struct analysis* analysis, char* arg);

void analysis_synthesis_create(struct analysis* analysis, char* arg);
void analysis_synthesis_print(struct analysis* analysis, char* arg);

void analysis_delete(struct analysis* analysis);

#endif