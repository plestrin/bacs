#include <stdlib.h>
#include <stdio.h>

#include "instruction.h"

void instruction_print(struct instruction *ins){
	if (ins != NULL){
		printf("*** Instruction %p ***\n", (void*)ins);
		printf("\tPC: \t\t0x%08lx\n", ins->pc);
		printf("\tPC next: \t0x%08lx\n", ins->pc_next);
	}
}