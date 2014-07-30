#ifndef TRACEFRAGMENT_H
#define TRACEFRAGMENT_H

#include <stdint.h>
#include <string.h>

#include "codeMap.h"
#include "instruction.h"
#include "ir.h"
#include "array.h"
#include "trace.h"

#define TRACEFRAGMENT_TAG_LENGTH 32

struct traceFragment{
	char 			tag[TRACEFRAGMENT_TAG_LENGTH];
	struct trace 	trace;
	struct ir* 		ir;
};

#define traceFragment_init(frag) 														\
	(frag)->tag[0]	= '\0'; 															\
	(frag)->ir 		= NULL;

#define traceFragment_get_nb_instruction(frag) assembly_get_nb_instruction(&((frag)->trace.assembly))

double traceFragment_opcode_percent(struct traceFragment* frag, uint32_t nb_opcode, uint32_t* opcode, uint32_t nb_excluded_opcode, uint32_t* excluded_opcode);

void traceFragment_print_location(struct traceFragment* frag, struct codeMap* cm);

static inline void traceFragment_print_assembly(struct traceFragment* frag){
	trace_print_asm(&(frag->trace), 0, frag->trace.nb_instruction);
}

static inline void traceFragment_create_ir(struct traceFragment* frag){
	if (frag->ir != NULL){
		printf("WARNING: in %s, an IR has already been built for the current fragment - deleting\n", __func__);
		ir_delete(frag->ir);
	}
	frag->ir = ir_create(&(frag->trace));
}

static inline void traceFragment_printDot_ir(struct traceFragment* frag){
	if (frag->ir != NULL){
		ir_printDot(frag->ir);
	}
	else{
		printf("ERROR: in %s, the IR is NULL for the current fragment\n", __func__);
	}
}

#define traceFragment_delete(frag) 														\
	traceFragment_clean(frag);															\
	free(frag);

void traceFragment_clean(struct traceFragment* frag);

#endif