#ifndef INSTRUCTION_H
#define INSTRUCTION_H


struct instruction{
	unsigned long pc;
	unsigned long pc_next;
};


void instruction_print(struct instruction *ins);

#endif