#ifndef TRACE_H
#define TRACE_H

#include <stdint.h>

#include "assembly.h"
#include "assemblyScan.h"
#include "ir.h"
#include "codeMap.h"
#include "array.h"
#include "base.h"
#include "memTrace.h"
#include "synthesisGraph.h"

#define TRACE_TAG_LENGTH 32
#define TRACE_PATH_MAX_LENGTH 	256

enum traceType{
	EXECUTION_TRACE,
	ELF_TRACE,
	FRAGMENT_TRACE
};

struct trace{
	char 					tag[TRACE_TAG_LENGTH];
	struct assembly 		assembly;
	struct ir* 				ir;
	enum traceType 			type;
	char 					directory_path[TRACE_PATH_MAX_LENGTH];
	struct array 			result_array;
	struct memTrace* 		mem_trace;
	struct synthesisGraph* 	synthesis_graph;
};

struct trace* trace_load(const char* directory_path);

int32_t trace_change_thread(struct trace* trace, uint32_t thread_id);

struct trace* trace_load_elf(const char* file_path);

int32_t trace_extract_segment(struct trace* trace_src, struct trace* trace_dst, uint32_t offset, uint32_t length);

#define trace_print(trace, start, stop) assembly_print(&((trace)->assembly), start, stop)

int32_t trace_concat(struct trace** trace_src_buffer, uint32_t nb_trace_src, struct trace* trace_dst);

#define trace_get_nb_instruction(trace) assembly_get_nb_instruction(&((trace)->assembly))

void trace_create_ir(struct trace* trace);
void trace_normalize_ir(struct trace* trace);
void trace_normalize_concrete_ir(struct trace* trace);

static inline void trace_printDot_ir(struct trace* trace){
	if (trace->ir != NULL){
		ir_printDot(trace->ir);
	}
	else{
		log_err_m("IR is NULL for trace: \"%s\"", trace->tag);
	}
}

int32_t trace_register_code_signature_result(void* signature, struct array* assignement_array, void* arg);
void trace_push_code_signature_result(int32_t idx, void* arg);
void trace_pop_code_signature_result(int32_t idx, void* arg);

void trace_create_synthesis(struct trace* trace);

static inline void trace_printDot_synthesis(struct trace* trace){
	if (trace->synthesis_graph != NULL){
		synthesisGraph_printDot(trace->synthesis_graph);
	}
	else{
		log_err_m("synthesis graph is NULL for trace: \"%s\"", trace->tag);
	}
}

void trace_check(struct trace* trace);

void trace_print_location(struct trace* trace, struct codeMap* cm);
double trace_opcode_percent(struct trace* trace, uint32_t nb_opcode, uint32_t* opcode, uint32_t nb_excluded_opcode, uint32_t* excluded_opcode);

void trace_export_result(struct trace* trace, void** signature_buffer, uint32_t nb_signature);

void trace_reset(struct trace* trace);
void trace_clean(struct trace* trace);

#define trace_delete(trace) 	\
	trace_clean(trace); 		\
	free(trace)

#endif