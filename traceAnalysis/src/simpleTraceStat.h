#ifndef SIMPLETRACESTAT_H
#define SIMPLETRACESTAT_H

#include <stdint.h>

#include "instruction.h"

struct simpleTraceStat{
	uint32_t 	nb_dynamic_instruction;
	uint32_t 	nb_different_instruction; /* pour l'instant je ne sais pas gérer cette valeur penser à faire un tstatic instruction */
	uint32_t 	nb_mem_read;
	uint32_t 	nb_mem_read_1;
	uint32_t 	nb_mem_read_2;
	uint32_t 	nb_mem_read_4;
	uint32_t 	nb_mem_write;
	uint32_t 	nb_mem_write_1;
	uint32_t 	nb_mem_write_2;
	uint32_t 	nb_mem_write_4;
	uint64_t 	min_pc;
	uint64_t 	max_pc;
};

struct simpleTraceStat* simpleTraceStat_create();
int32_t simpleTraceStat_add_instruction(struct simpleTraceStat* stat, struct instruction* ins);
void simpleTraceStat_print(struct simpleTraceStat* stat);
void simpleTraceStat_delete(struct simpleTraceStat* stat);

#endif