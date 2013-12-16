#ifndef TRACEFRAGMENT_H
#define TRACEFRAGMENT_H

#include <stdint.h>

#include "instruction.h"
#include "array.h"

#define TRACEFRAGMENT_INSTRUCTION_BATCH	64

struct memAccess{
	uint32_t 	order;
	uint32_t 	value;
	uint64_t 	address;
	uint8_t 	size;
};

struct traceFragment{
	struct array 		instruction_array;
	/*struct instruction* instructions;
	int 				nb_allocated_instruction;
	int 				nb_instruction;*/
	struct memAccess* 	read_memory_array;
	struct memAccess* 	write_memory_array;
	int 				nb_memory_read_access;
	int 				nb_memory_write_access;
};

struct traceFragment* codeFragment_create();
int32_t traceFragment_init(struct traceFragment* frag);

static inline int32_t traceFragment_add_instruction(struct traceFragment* frag, struct instruction* ins){
	return array_add(&(frag->instruction_array), ins);
}

static inline int32_t traceFragment_search_pc(struct traceFragment* frag, struct instruction* ins){
	return array_search_seq_up(&(frag->instruction_array), 0, array_get_length(&(frag->instruction_array)), ins, (int32_t(*)(void*,void*))instruction_compare_pc);
}

struct instruction* traceFragment_get_last_instruction(struct traceFragment* frag);
float traceFragment_opcode_percent(struct traceFragment* frag, int nb_opcode, uint32_t* opcode, int nb_excluded_opcode, uint32_t* excluded_opcode);

int traceFragment_create_mem_array(struct traceFragment* frag);
void traceFragment_print_mem_array(struct memAccess* mem_access, int nb_mem_access);
int traceFragment_remove_read_after_write(struct traceFragment* frag);
struct array* traceFragment_extract_mem_arg_adjacent(struct memAccess* mem_access, int nb_mem_access);	/* Identification rule: adjacent memory locations belong to the same argument */

void traceFragment_delete(struct traceFragment* frag);
void traceFragment_clean(struct traceFragment* frag);

#endif