#ifndef SIMPLETRACESTAT_H
#define SIMPLETRACESTAT_H

#include "instruction.h"

struct simpleTraceStat{
	unsigned int 	nb_executed_instruction;
	unsigned int 	nb_different_instruction; /* pour l'instant je ne sias pas gérer cette valeur */
	unsigned long 	min_pc;
	unsigned long 	max_pc;
};

struct simpleTraceStat* simpleTraceStat_create();
int simpleTraceStat_add_instruction(struct simpleTraceStat* stat, struct instruction* ins);
void simpleTraceStat_print(struct simpleTraceStat* stat);
void simpleTraceStat_delete(struct simpleTraceStat* stat);

#endif