#include <stdlib.h>
#include <stdio.h>

#include "simpleTraceStat.h"


struct simpleTraceStat* simpleTraceStat_create(){
	struct simpleTraceStat* stat;

	stat = (struct simpleTraceStat*)malloc(sizeof(struct simpleTraceStat));
	if (stat != NULL){
		stat->nb_dynamic_instruction 	= 0;
		stat->nb_different_instruction 	= 0;
		stat->nb_mem_read 				= 0;
		stat->nb_mem_read_1 			= 0;
		stat->nb_mem_read_2 			= 0;
		stat->nb_mem_read_4 			= 0;
		stat->nb_mem_write 				= 0;
		stat->nb_mem_write_1 			= 0;
		stat->nb_mem_write_2 			= 0;
		stat->nb_mem_write_4 			= 0;
		stat->min_pc 					= 0xffffffff;
		stat->max_pc 					= 0x00000000;
	}
	else{
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
	}

	return stat;
}
int32_t simpleTraceStat_add_instruction(struct simpleTraceStat* stat, struct instruction* ins){
	uint32_t i;

	if (stat != NULL && ins != NULL){
		stat->nb_dynamic_instruction ++;

		if (stat->min_pc > ins->pc){
			stat->min_pc = ins->pc;
		}
		if (stat->max_pc < ins->pc){
			stat->max_pc = ins->pc;
		}

		for ( i = 0; i < INSTRUCTION_MAX_NB_DATA; i++){
			if (INSTRUCTION_DATA_TYPE_IS_VALID(ins->data[i].type)){
				if (INSTRUCTION_DATA_TYPE_IS_READ(ins->data[i].type)){
					if (INSTRUCTION_DATA_TYPE_IS_MEM(ins->data[i].type)){
						stat->nb_mem_read ++;
						switch(ins->data[i].size){
						case 1 	: {stat->nb_mem_read_1 ++; break;}
						case 2 	: {stat->nb_mem_read_2 ++; break;}
						case 4 	: {stat->nb_mem_read_4 ++; break;}
						default : {printf("WARNING: in %s, unexpected data size\n", __func__); break;}
						}
					}
					else{
						/* fill the gap - REGISTER */
					}
				}
				else{
					if (INSTRUCTION_DATA_TYPE_IS_MEM(ins->data[i].type)){
						stat->nb_mem_write ++;
						switch(ins->data[i].size){
						case 1 	: {stat->nb_mem_write_1 ++; break;}
						case 2 	: {stat->nb_mem_write_2 ++; break;}
						case 4 	: {stat->nb_mem_write_4 ++; break;}
						default : {printf("WARNING: in %s, unexpected data size\n", __func__); break;}
						}
					}
					else{
						/* fill the gap - REGISTER*/
					}
				}
			}
		}
	}

	return 0;
}

void simpleTraceStat_print(struct simpleTraceStat* stat){
	if (stat != NULL){
		printf("*** SimpleTraceStat ***\n");
		printf("\tDynamic instruction(s): \t%u\n", stat->nb_dynamic_instruction);
		printf("\tDifferent instruction(s): \t%u\n", stat->nb_different_instruction);

		printf("\tMemory read: \t\t\t%u (1 byte: %u, 2 bytes: %u, 4 bytes: %u)\n", stat->nb_mem_read, stat->nb_mem_read_1, stat->nb_mem_read_2, stat->nb_mem_read_4);
		printf("\tMemory write: \t\t\t%u (1 byte: %u, 2 bytes: %u, 4 bytes: %u)\n", stat->nb_mem_write, stat->nb_mem_write_1, stat->nb_mem_write_2, stat->nb_mem_write_4);

		#pragma GCC diagnostic ignored "-Wformat" /* ISO C90 does not support the ‘ll’ gnu_printf length modifier */
		printf("\tMin program counter: \t\t0x%llx\n", stat->min_pc);
		#pragma GCC diagnostic ignored "-Wformat" /* ISO C90 does not support the ‘ll’ gnu_printf length modifier */
		printf("\tMax program counter: \t\t0x%llx\n", stat->max_pc);
	}
}

void simpleTraceStat_delete(struct simpleTraceStat* stat){
	if (stat != NULL){
		free(stat);
	}
}