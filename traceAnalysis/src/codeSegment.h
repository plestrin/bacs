#ifndef CODESEGMENT_H
#define CODESEGMENT_H

#define CODESEGMENT_INSTRUCTION_BATCH	64

#include "instruction.h"

struct codeSegment{
	struct instruction* instructions;
	int 				nb_allocated_instruction;
	int 				nb_instruction;
};

struct codeSegment* codeSegment_create();
void codeSegment_init(struct codeSegment* seg);
int codeSegment_add_instruction(struct codeSegment* seg, struct instruction* ins);
int codeSegment_search_instruction(struct codeSegment* seg, struct instruction* ins);
void codeSegment_delete(struct codeSegment* seg);
void codeSegment_clean(struct codeSegment* seg);

#endif