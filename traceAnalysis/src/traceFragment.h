#ifndef TRACEFRAGMENT_H
#define TRACEFRAGMENT_H

#include <stdint.h>
#include <string.h>

#include "codeMap.h"
#include "instruction.h"
#include "ir.h"
#include "memAccess.h"
#include "regAccess.h"
#include "array.h"
#include "trace.h"
#include "argSet.h"

#define TRACEFRAGMENT_TAG_LENGTH 32

enum fragmentType{
	TRACEFRAGMENT_TYPE_NONE,
	TRACEFRAGMENT_TYPE_LOOP
};

struct fragmentCallback{
	void(*specific_delete)(void*);
	void*(*specific_clone)(void*);
};

struct traceFragment{
	char 						tag[TRACEFRAGMENT_TAG_LENGTH];
	struct trace 				trace;
	struct ir* 					ir;
	enum fragmentType 			type;
	void* 						specific_data;
	struct fragmentCallback*  	callback;
	
	struct memAccess* 			read_memory_array;
	struct memAccess* 			write_memory_array;
	uint32_t 					nb_memory_read_access;
	uint32_t 					nb_memory_write_access;

	struct regAccess* 			read_register_array;
	struct regAccess* 			write_register_array;
	uint32_t 					nb_register_read_access;
	uint32_t 					nb_register_write_access;
};

struct traceFragment* codeFragment_create(enum fragmentType type, void* specific_data, struct fragmentCallback* callback);
int32_t traceFragment_init(struct traceFragment* frag, enum fragmentType type, void* specific_data, struct fragmentCallback* callback);

static inline uint32_t traceFragment_get_nb_instruction(struct traceFragment* frag){
	return frag->trace.nb_instruction;
}

double traceFragment_opcode_percent(struct traceFragment* frag, int nb_opcode, uint32_t* opcode, int nb_excluded_opcode, uint32_t* excluded_opcode);

int32_t traceFragment_create_mem_array(struct traceFragment* frag);
void traceFragment_remove_read_after_write(struct traceFragment* frag);

int32_t traceFragment_create_reg_array(struct traceFragment* frag);

void traceFragment_print_location(struct traceFragment* frag, struct codeMap* cm);

static inline void traceFragment_print_instruction(struct traceFragment* frag){
	trace_print(&(frag->trace), 0, frag->trace.nb_instruction, NULL);
}

static inline void traceFragment_analyse_operand(struct traceFragment* frag){
	trace_analyse_operand(&(frag->trace));
}

static inline void traceFragment_create_ir(struct traceFragment* frag){
	if (frag->ir != NULL){
		printf("WARNING: in %s, an IR has already been built for the current fragment - deleting\n", __func__);
		ir_delete(frag->ir);
	}
	frag->ir = ir_create(&(frag->trace));
	if (frag->ir != NULL){
		ir_pack_input_register(frag->ir);
	}
}

static inline void traceFragment_printDot_ir(struct traceFragment* frag){
	if (frag->ir != NULL){
		ir_printDot(frag->ir);
	}
	else{
		printf("ERROR: in %s, the IR is NULL for the current fragment\n", __func__);
	}
}

static inline void traceFragment_extract_arg_ir(struct traceFragment* frag, struct argSet* set){
	if (frag->ir != NULL){
		 ir_extract_arg(frag->ir, set);
	}
	else{
		printf("ERROR: in %s, the IR is NULL for the current fragment\n", __func__);
	}
}

void traceFragment_delete(struct traceFragment* frag);
void traceFragment_clean(struct traceFragment* frag);

#endif