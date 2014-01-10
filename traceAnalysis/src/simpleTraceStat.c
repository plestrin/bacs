#include <stdlib.h>
#include <stdio.h>

#include "simpleTraceStat.h"


struct simpleTraceStat* simpleTraceStat_create(){
	struct simpleTraceStat* stat;

	stat = (struct simpleTraceStat*)malloc(sizeof(struct simpleTraceStat));
	if (stat != NULL){
		simpleTraceStat_init(stat);
	}
	else{
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
	}

	return stat;
}

void simpleTraceStat_init(struct simpleTraceStat* stat){
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

int32_t simpleTraceStat_process(struct simpleTraceStat* stat, struct traceFragment* frag){
	uint32_t 			i;
	uint32_t 			j;
	struct instruction* ins;

	if (stat != NULL && frag != NULL){

		for (j = 0; j < array_get_length(&(frag->instruction_array)); j++){
			ins = (struct instruction*)array_get(&(frag->instruction_array), j);

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
	}

	return 0;
}

struct multiColumnPrinter* simpleTraceStat_init_MultiColumnPrinter(){
	struct multiColumnPrinter* printer;

	printer = multiColumnPrinter_create(stdout, 8, NULL, NULL, NULL);
	if (printer != NULL){
		multiColumnPrinter_set_column_size(printer, 0, 5);
		multiColumnPrinter_set_column_size(printer, 2, 10);
		multiColumnPrinter_set_column_size(printer, 3, 10);
		multiColumnPrinter_set_column_size(printer, 4, 10);
		multiColumnPrinter_set_column_size(printer, 5, 10);

		multiColumnPrinter_set_column_type(printer, 0, MULTICOLUMN_TYPE_UINT32);
		multiColumnPrinter_set_column_type(printer, 2, MULTICOLUMN_TYPE_UINT32);
		multiColumnPrinter_set_column_type(printer, 3, MULTICOLUMN_TYPE_UINT32);
		multiColumnPrinter_set_column_type(printer, 4, MULTICOLUMN_TYPE_UINT32);
		multiColumnPrinter_set_column_type(printer, 5, MULTICOLUMN_TYPE_UINT32);
		#if defined ARCH_32
		multiColumnPrinter_set_column_type(printer, 6, MULTICOLUMN_TYPE_HEX_32);
		multiColumnPrinter_set_column_type(printer, 7, MULTICOLUMN_TYPE_HEX_32);
		#elif defined ARCH_64
		multiColumnPrinter_set_column_type(printer, 6, MULTICOLUMN_TYPE_HEX_64);
		multiColumnPrinter_set_column_type(printer, 7, MULTICOLUMN_TYPE_HEX_64);
		#else
		#error Please specify an architecture {ARCH_32 or ARCH_64}
		#endif

		multiColumnPrinter_set_title(printer, 0, (char*)"Index");
		multiColumnPrinter_set_title(printer, 1, (char*)"Tag");
		multiColumnPrinter_set_title(printer, 2, (char*)"Dyn Ins");
		multiColumnPrinter_set_title(printer, 3, (char*)"Diff Ins");
		multiColumnPrinter_set_title(printer, 4, (char*)"Mem read");
		multiColumnPrinter_set_title(printer, 5, (char*)"Mem write");
		multiColumnPrinter_set_title(printer, 6, (char*)"Min PC");
		multiColumnPrinter_set_title(printer, 7, (char*)"Max PC");
	}
	else{
		printf("ERROR: in %s, unable to init multiColumnPrinter\n", __func__);
	}

	return printer;
}

void simpleTraceStat_print(struct multiColumnPrinter* printer, uint32_t index, char* tag, struct simpleTraceStat* stat){
	if (stat != NULL){
		if (printer == NULL){
			printf("*** SimpleTraceStat ***\n");
			printf("\tDynamic instruction(s): \t%u\n", stat->nb_dynamic_instruction);
			printf("\tDifferent instruction(s): \t%u\n", stat->nb_different_instruction);

			printf("\tMemory read: \t\t\t%u (1 byte: %u, 2 bytes: %u, 4 bytes: %u)\n", stat->nb_mem_read, stat->nb_mem_read_1, stat->nb_mem_read_2, stat->nb_mem_read_4);
			printf("\tMemory write: \t\t\t%u (1 byte: %u, 2 bytes: %u, 4 bytes: %u)\n", stat->nb_mem_write, stat->nb_mem_write_1, stat->nb_mem_write_2, stat->nb_mem_write_4);

			#if defined ARCH_32
			printf("\tMin program counter: \t\t0x%x\n", stat->min_pc);
			printf("\tMax program counter: \t\t0x%x\n", stat->max_pc);
			#elif defined ARCH_64
			#pragma GCC diagnostic ignored "-Wformat" /* ISO C90 does not support the ‘ll’ gnu_printf length modifier */
			printf("\tMin program counter: \t\t0x%llx\n", stat->min_pc);
			#pragma GCC diagnostic ignored "-Wformat" /* ISO C90 does not support the ‘ll’ gnu_printf length modifier */
			printf("\tMax program counter: \t\t0x%llx\n", stat->max_pc);
			#else
			#error Please specify an architecture {ARCH_32 or ARCH_64}
			#endif
		}
		else{
			multiColumnPrinter_print(printer, index, tag, stat->nb_dynamic_instruction, stat->nb_different_instruction, stat->nb_mem_read, stat->nb_mem_write, stat->min_pc, stat->max_pc, NULL);
		}
	}
}

void simpleTraceStat_delete(struct simpleTraceStat* stat){
	if (stat != NULL){
		free(stat);
	}
}