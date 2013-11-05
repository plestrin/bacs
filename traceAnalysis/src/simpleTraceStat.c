#include <stdlib.h>
#include <stdio.h>

#include "simpleTraceStat.h"


struct simpleTraceStat* simpleTraceStat_create(){
	struct simpleTraceStat* stat;

	stat = (struct simpleTraceStat*)malloc(sizeof(struct simpleTraceStat));
	if (stat != NULL){
		stat->nb_executed_instruction 	= 0;
		stat->nb_different_instruction 	= 0;
		stat->min_pc 					= 0xFFFFFFFF;
		stat->max_pc 					= 0x00000000;
	}
	else{
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
	}

	return stat;
}
int simpleTraceStat_add_instruction(struct simpleTraceStat* stat, struct instruction* ins){
	if (stat != NULL && ins != NULL){
		stat->nb_executed_instruction ++;

		if (stat->min_pc > ins->pc){
			stat->min_pc = ins->pc;
		}
		if (stat->max_pc < ins->pc){
			stat->max_pc = ins->pc;
		}
	}

	return 0;
}

void simpleTraceStat_print(struct simpleTraceStat* stat){
	if (stat != NULL){
		printf("*** SimpleTraceStat ***\n");
		printf("\tExecuted instruction(s): \t%u\n", stat->nb_executed_instruction);
		printf("\tDifferent instruction(s): \t%u\n", stat->nb_different_instruction);
		printf("\tMin program counter: \t\t0x%08lx\n", stat->min_pc);
		printf("\tMax program counter: \t\t0x%08lx\n", stat->max_pc);
	}
}

void simpleTraceStat_delete(struct simpleTraceStat* stat){
	if (stat != NULL){
		free(stat);
	}
}