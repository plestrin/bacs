#ifndef TRACEFRAGMENT_H
#define TRACEFRAGMENT_H

#define TRACEFRAGMENT_INSTRUCTION_BATCH	64

#include "instruction.h"

struct traceFragment{
	struct instruction* instructions;
	int 				nb_allocated_instruction;
	int 				nb_instruction;
};

struct traceFragment* codeFragment_create();
void traceFragment_init(struct traceFragment* frag);
int traceFragment_add_instruction(struct traceFragment* frag, struct instruction* ins);
int traceFragment_search_pc(struct traceFragment* frag, struct instruction* ins);
struct instruction* traceFragment_get_last_instruction(struct traceFragment* frag);
float traceFragment_opcode_percent(struct traceFragment* frag, int nb_opcode, uint32_t* opcode, int nb_excluded_opcode, uint32_t* excluded_opcode);
void traceFragement_delete(struct traceFragment* frag);
void traceFragment_clean(struct traceFragment* frag);

#endif