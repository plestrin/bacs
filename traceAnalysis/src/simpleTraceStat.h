#ifndef SIMPLETRACESTAT_H
#define SIMPLETRACESTAT_H

#include <stdint.h>

#include "multiColumn.h"
#include "traceFragment.h"

struct simpleTraceStat{
	uint32_t 	nb_dynamic_instruction;
	uint32_t 	nb_different_instruction; /* pour l'instant je ne sais pas gérer cette valeur penser à faire un static instruction */
	uint32_t 	nb_mem_read;
	uint32_t 	nb_mem_read_1;
	uint32_t 	nb_mem_read_2;
	uint32_t 	nb_mem_read_4;
	uint32_t 	nb_mem_write;
	uint32_t 	nb_mem_write_1;
	uint32_t 	nb_mem_write_2;
	uint32_t 	nb_mem_write_4;
	uint32_t 	nb_reg_read;
	uint32_t 	nb_reg_read_1;
	uint32_t 	nb_reg_read_2;
	uint32_t 	nb_reg_read_4;
	uint32_t 	nb_reg_write;
	uint32_t 	nb_reg_write_1;
	uint32_t 	nb_reg_write_2;
	uint32_t 	nb_reg_write_4;
	ADDRESS 	min_pc;
	ADDRESS 	max_pc;
};

struct simpleTraceStat* simpleTraceStat_create();
void simpleTraceStat_init(struct simpleTraceStat* stat);
int32_t simpleTraceStat_process(struct simpleTraceStat* stat, struct traceFragment* frag);
struct multiColumnPrinter* simpleTraceStat_init_MultiColumnPrinter();
void simpleTraceStat_print(struct multiColumnPrinter* printer, uint32_t index, char* tag, struct simpleTraceStat* stat);
void simpleTraceStat_delete(struct simpleTraceStat* stat);

#endif