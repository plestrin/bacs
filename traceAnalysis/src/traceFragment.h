#ifndef TRACEFRAGMENT_H
#define TRACEFRAGMENT_H

#include <stdint.h>
#include <string.h>

#include "codeMap.h"
#include "instruction.h"
#include "memAccess.h"
#include "regAccess.h"
#include "array.h"
#include "trace.h"

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

void traceFragment_delete(struct traceFragment* frag);
void traceFragment_clean(struct traceFragment* frag);

#endif