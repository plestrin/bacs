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

#define TRACE_NB_MAX_PROCESS 	16
#define TRACE_NB_MAX_THREAD 	32

struct threadIdentifier{
	uint32_t 					id;
};

struct processIdentifier{
	uint32_t 					id;
	uint32_t 					nb_thread;
	struct threadIdentifier 	thread[TRACE_NB_MAX_THREAD];
};

struct traceIdentifier{
	uint32_t 					nb_process;
	struct processIdentifier 	process[TRACE_NB_MAX_PROCESS];
	uint32_t 					current_pid;
	uint32_t 					current_tid;
};

#define traceIdentifier_init(identifier) (identifier)->nb_process = 0

int32_t traceIdentifier_add(struct traceIdentifier* identifier, uint32_t pid, uint32_t tid);
void traceIdentifier_sort(struct traceIdentifier* identifier);
int32_t traceIdentifier_select(struct traceIdentifier* identifier, uint32_t p_index, uint32_t t_index);

struct trace{
	enum traceType 					type;
	struct memTrace* 				mem_trace;
	struct assembly 				assembly;
	union {
		struct {
			char 					tag[TRACE_TAG_LENGTH];
			struct ir* 				ir;
			struct array 			result_array;
			struct synthesisGraph* 	synthesis_graph;
		} 							frag;
		struct {
			char 					directory_path[TRACE_PATH_MAX_LENGTH];
			struct traceIdentifier 	identifier;
		} 							exe;
	} 								trace_type;
};

struct trace* trace_load_exe(const char* directory_path);

int32_t trace_change(struct trace* trace, uint32_t p_index, uint32_t t_index);

struct trace* trace_load_elf(const char* file_path);

int32_t trace_extract_segment(struct trace* trace_src, struct trace* trace_dst, uint32_t offset, uint32_t length);

#define trace_print(trace, start, stop) assembly_print_all(&((trace)->assembly), start, stop, (trace)->mem_trace)
#define trace_print_all(trace) 			assembly_print_all(&((trace)->assembly), 0, trace_get_nb_instruction(trace), (trace)->mem_trace)

int32_t trace_concat(struct trace** trace_src_buffer, uint32_t nb_trace_src, struct trace* trace_dst);

#define trace_get_nb_instruction(trace) assembly_get_nb_instruction(&((trace)->assembly))

void trace_create_ir(struct trace* trace);

void trace_search_irComponent(struct trace* trace_ext, struct trace* trace_inn, struct array* ir_component_array);
void trace_create_compound_ir(struct trace* trace, struct array* ir_component_array);

#define componentFrag_clean_array(array) array_clean(array)

void trace_normalize_ir(struct trace* trace);
void trace_normalize_concrete_ir(struct trace* trace);

static inline void trace_printDot_ir(struct trace* trace){
	if (trace->type == FRAGMENT_TRACE && trace->trace_type.frag.ir != NULL){
		ir_printDot(trace->trace_type.frag.ir);
	}
	else{
		log_err_m("IR is NULL for trace: \"%s\"", trace->trace_type.frag.tag);
	}
}

static inline void trace_print_aliasing_ir(struct trace* trace){
	if (trace->type == FRAGMENT_TRACE && trace->trace_type.frag.ir != NULL){
		irMemory_print_aliasing(trace->trace_type.frag.ir);
	}
	else{
		log_err_m("IR is NULL for trace: \"%s\"", trace->trace_type.frag.tag);
	}
}

void trace_search_buffer_signature(struct trace* trace);
int32_t trace_register_code_signature_result(void* signature, struct array* assignement_array, void* arg);
void trace_push_code_signature_result(int32_t idx, void* arg);
void trace_pop_code_signature_result(int32_t idx, void* arg);

void trace_create_synthesis(struct trace* trace);

static inline void trace_printDot_synthesis(struct trace* trace, const char* name){
	if (trace->type == FRAGMENT_TRACE && trace->trace_type.frag.synthesis_graph != NULL){
		synthesisGraph_printDot(trace->trace_type.frag.synthesis_graph, name);
	}
	else{
		log_err_m("synthesis graph is NULL for trace: \"%s\"", trace->trace_type.frag.tag);
	}
}

void trace_check(struct trace* trace);

void trace_print_location(const struct trace* trace, struct codeMap* cm);

void trace_export_result(struct trace* trace, void** signature_buffer, uint32_t nb_signature);
void trace_print_result(struct trace* trace, const char* result_desc);

int32_t trace_compare(const struct trace* trace1, const struct trace* trace2);

void trace_search_memory(struct trace* trace, uint32_t offset, ADDRESS addr);

void trace_reset_ir(struct trace* trace);
void trace_reset_result(struct trace* trace);
void trace_reset_synthesis(struct trace* trace);
void trace_reset(struct trace* trace);

void trace_clean(struct trace* trace);

#define trace_delete(trace) 	\
	trace_clean(trace); 		\
	free(trace)

#endif