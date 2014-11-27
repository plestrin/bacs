#ifndef TRACE_H
#define TRACE_H

#include <stdint.h>

#include "assembly.h"
#include "ir.h"
#include "codeMap.h"

#define TRACE_TAG_LENGTH 32

struct trace{
	char 				tag[TRACE_TAG_LENGTH];
	struct assembly 	assembly;
	struct ir* 			ir;
};

struct trace* trace_load(const char* directory_path);

struct trace* trace_load_elf(const char* file_path);

#define trace_init(trace) 																								\
	(trace)->tag[0]		= '\0'; 																						\
	(trace)->ir 		= NULL

#define trace_check(trace) 																								\
	if (assembly_check(&((trace)->assembly))){ 																			\
		printf("ERROR: in %s, assembly check failed\n", __func__); 														\
	}

void trace_print(struct trace* trace, uint32_t start, uint32_t stop);

#define trace_extract_segment(trace_src, trace_dst, offset, length) assembly_extract_segment(&((trace_src)->assembly), &((trace_dst)->assembly), offset, length)

int32_t trace_concat(struct trace** trace_src_buffer, uint32_t nb_trace_src, struct trace* trace_dst);

#define trace_get_nb_instruction(trace) assembly_get_nb_instruction(&((trace)->assembly))

static inline void trace_create_ir(struct trace* trace){
	if (trace->ir != NULL){
		printf("WARNING: in %s, an IR has already been built for the current trace - deleting\n", __func__);
		ir_delete(trace->ir);
	}
	trace->ir = ir_create(&(trace->assembly));
}

static inline void trace_printDot_ir(struct trace* trace, struct graphPrintDotFilter* filters){
	if (trace->ir != NULL){
		ir_printDot(trace->ir, filters);
	}
	else{
		printf("ERROR: in %s, the IR is NULL for the current trace\n", __func__);
	}
}

void trace_print_location(struct trace* trace, struct codeMap* cm);
double trace_opcode_percent(struct trace* trace, uint32_t nb_opcode, uint32_t* opcode, uint32_t nb_excluded_opcode, uint32_t* excluded_opcode);

void trace_clean(struct trace* trace);

#define trace_delete(trace) 																							\
	trace_clean(trace); 																								\
	free(trace)

#endif