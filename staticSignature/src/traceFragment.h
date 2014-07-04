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

enum fragmentType{
	TRACEFRAGMENT_TYPE_NONE,
	TRACEFRAGMENT_TYPE_LOOP
};

struct traceFragment{
	char 						tag[TRACEFRAGMENT_TAG_LENGTH];
	struct trace 				trace;
	struct ir* 					ir;
	enum fragmentType 			type;
	void* 						specific_data;
};

#define traceFragment_init(frag, type_, specific_data_) 																		\
	(frag)->tag[0]					= '\0'; 																					\
	(frag)->ir 						= NULL; 																					\
	(frag)->type 					= type_; 																					\
	(frag)->specific_data 			= specific_data_;


static inline uint32_t traceFragment_get_nb_instruction(struct traceFragment* frag){
	return frag->trace.nb_instruction;
}

double traceFragment_opcode_percent(struct traceFragment* frag, int nb_opcode, uint32_t* opcode, int nb_excluded_opcode, uint32_t* excluded_opcode);

void traceFragment_print_location(struct traceFragment* frag, struct codeMap* cm);

static inline void traceFragment_print_instruction(struct traceFragment* frag){
	trace_print(&(frag->trace), 0, frag->trace.nb_instruction, NULL);
}

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

static inline void traceFragment_print_io(struct traceFragment* frag){
	if (frag->ir != NULL){
		ir_print_io(frag->ir);
	}
	else{
		printf("ERROR: in %s, the IR is NULL for the current fragment\n", __func__);
	}
}

#define traceFragment_delete(frag) 																								\
	traceFragment_clean(frag);																									\
	free(frag);

void traceFragment_clean(struct traceFragment* frag);

#endif