#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include <stdint.h>

#include "xed-iclass-enum.h"

struct instruction{
	unsigned long 	pc;
	unsigned long 	pc_next;
	uint32_t 		opcode;
};


void instruction_print(struct instruction *ins);

static inline int instruction_compare(struct instruction* ins1, struct instruction* ins2){
	return (ins1->pc == ins2->pc && ins1->pc_next == ins2->pc_next && ins1->opcode == ins2->opcode);
}

#endif