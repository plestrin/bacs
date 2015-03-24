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
	stat->nb_reg_read 				= 0;
	stat->nb_reg_read_1 			= 0;
	stat->nb_reg_read_2 			= 0;
	stat->nb_reg_read_4 			= 0;
	stat->nb_reg_write 				= 0;
	stat->nb_reg_write_1 			= 0;
	stat->nb_reg_write_2 			= 0;
	stat->nb_reg_write_4 			= 0;
	stat->min_pc 					= 0xffffffff;
	stat->max_pc 					= 0x00000000;
}

int32_t simpleTraceStat_process(struct simpleTraceStat* stat, struct traceFragment* frag){
	uint32_t 			i;
	uint32_t 			j;
	struct operand* 	operands;

	if (stat != NULL && frag != NULL){

		for (j = 0; j < traceFragment_get_nb_instruction(frag); j++){
			operands = trace_get_ins_operands(&(frag->trace), j);

			stat->nb_dynamic_instruction ++;

			if (stat->min_pc > frag->trace.instructions[j].pc){
				stat->min_pc = frag->trace.instructions[j].pc;
			}
			if (stat->max_pc < frag->trace.instructions[j].pc){
				stat->max_pc = frag->trace.instructions[j].pc;
			}

			for ( i = 0; i < frag->trace.instructions[j].nb_operand; i++){
				if (OPERAND_IS_READ(operands[i])){
					if (OPERAND_IS_MEM(operands[i])){
						stat->nb_mem_read ++;
						switch(operands[i].size){
							case 1 	: {stat->nb_mem_read_1 ++; break;}
							case 2 	: {stat->nb_mem_read_2 ++; break;}
							case 4 	: {stat->nb_mem_read_4 ++; break;}
							default : {printf("WARNING: in %s, unexpected mem read data size: %u (ins: %s, operand: %u)\n", __func__, operands[i].size, xed_iclass_enum_t2str(frag->trace.instructions[j].opcode), i); break;}
						}
					}
					else if (OPERAND_IS_REG(operands[i])){
						stat->nb_reg_read ++;
						switch(operands[i].size){
							case 1 	: {stat->nb_reg_read_1 ++; break;}
							case 2 	: {stat->nb_reg_read_2 ++; break;}
							case 4 	: {stat->nb_reg_read_4 ++; break;}
							default : {printf("WARNING: in %s, unexpected reg read data size: %u (ins: %s, operand: %u)\n", __func__, operands[i].size, xed_iclass_enum_t2str(frag->trace.instructions[j].opcode), i); break;}
						}
					}
					else{
						printf("ERROR: in %s, unexpected data type\n", __func__);
					}
				}
				else{
					if (OPERAND_IS_MEM(operands[i])){
						stat->nb_mem_write ++;
						switch(operands[i].size){
							case 1 	: {stat->nb_mem_write_1 ++; break;}
							case 2 	: {stat->nb_mem_write_2 ++; break;}
							case 4 	: {stat->nb_mem_write_4 ++; break;}
							default : {printf("WARNING: in %s, unexpected mem write data size: %u (ins: %s, operand: %u)\n", __func__, operands[i].size, xed_iclass_enum_t2str(frag->trace.instructions[j].opcode), i); break;}
						}
					}
					else if (OPERAND_IS_REG(operands[i])){
						stat->nb_reg_write ++;
						switch(operands[i].size){
							case 1 	: {stat->nb_reg_write_1 ++; break;}
							case 2 	: {stat->nb_reg_write_2 ++; break;}
							case 4 	: {stat->nb_reg_write_4 ++; break;}
							default : {printf("WARNING: in %s, unexpected reg write data size: %u (ins: %s, operand: %u)\n", __func__, operands[i].size, xed_iclass_enum_t2str(frag->trace.instructions[j].opcode), i); break;}
						}
					}
					else{
						printf("ERROR: in %s, unexpected data type\n", __func__);
					}
				}
			}
		}
	}

	return 0;
}

struct multiColumnPrinter* simpleTraceStat_init_MultiColumnPrinter(){
	struct multiColumnPrinter* printer;

	printer = multiColumnPrinter_create(stdout, 10, NULL, NULL, NULL);
	if (printer != NULL){
		multiColumnPrinter_set_column_size(printer, 0, 5);
		multiColumnPrinter_set_column_size(printer, 1, TRACEFRAGMENT_TAG_LENGTH);
		multiColumnPrinter_set_column_size(printer, 2, 10);
		multiColumnPrinter_set_column_size(printer, 3, 10);
		multiColumnPrinter_set_column_size(printer, 4, 8);
		multiColumnPrinter_set_column_size(printer, 5, 9);
		multiColumnPrinter_set_column_size(printer, 6, 8);
		multiColumnPrinter_set_column_size(printer, 7, 9);

		multiColumnPrinter_set_column_type(printer, 0, MULTICOLUMN_TYPE_UINT32);
		multiColumnPrinter_set_column_type(printer, 2, MULTICOLUMN_TYPE_UINT32);
		multiColumnPrinter_set_column_type(printer, 3, MULTICOLUMN_TYPE_UINT32);
		multiColumnPrinter_set_column_type(printer, 4, MULTICOLUMN_TYPE_UINT32);
		multiColumnPrinter_set_column_type(printer, 5, MULTICOLUMN_TYPE_UINT32);
		multiColumnPrinter_set_column_type(printer, 6, MULTICOLUMN_TYPE_UINT32);
		multiColumnPrinter_set_column_type(printer, 7, MULTICOLUMN_TYPE_UINT32);
		#if defined ARCH_32
		multiColumnPrinter_set_column_type(printer, 8, MULTICOLUMN_TYPE_HEX_32);
		multiColumnPrinter_set_column_type(printer, 9, MULTICOLUMN_TYPE_HEX_32);
		#elif defined ARCH_64
		multiColumnPrinter_set_column_type(printer, 8, MULTICOLUMN_TYPE_HEX_64);
		multiColumnPrinter_set_column_type(printer, 9, MULTICOLUMN_TYPE_HEX_64);
		#else
		#error Please specify an architecture {ARCH_32 or ARCH_64}
		#endif

		multiColumnPrinter_set_title(printer, 0, (char*)"Index");
		multiColumnPrinter_set_title(printer, 1, (char*)"Tag");
		multiColumnPrinter_set_title(printer, 2, (char*)"Dyn Ins");
		multiColumnPrinter_set_title(printer, 3, (char*)"Diff Ins");
		multiColumnPrinter_set_title(printer, 4, (char*)"Mem read");
		multiColumnPrinter_set_title(printer, 5, (char*)"Mem write");
		multiColumnPrinter_set_title(printer, 6, (char*)"Reg read");
		multiColumnPrinter_set_title(printer, 7, (char*)"Reg write");
		multiColumnPrinter_set_title(printer, 8, (char*)"Min PC");
		multiColumnPrinter_set_title(printer, 9, (char*)"Max PC");
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

			printf("\tRegister read: \t\t\t%u (1 byte: %u, 2 bytes: %u, 4 bytes: %u)\n", stat->nb_reg_read, stat->nb_reg_read_1, stat->nb_reg_read_2, stat->nb_reg_read_4);
			printf("\tRegister write: \t\t%u (1 byte: %u, 2 bytes: %u, 4 bytes: %u)\n", stat->nb_reg_write, stat->nb_reg_write_1, stat->nb_reg_write_2, stat->nb_reg_write_4);

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
			multiColumnPrinter_print(printer, index, tag, stat->nb_dynamic_instruction, stat->nb_different_instruction, stat->nb_mem_read, stat->nb_mem_write, stat->nb_reg_read, stat->nb_reg_write, stat->min_pc, stat->max_pc, NULL);
		}
	}
}

void simpleTraceStat_delete(struct simpleTraceStat* stat){
	if (stat != NULL){
		free(stat);
	}
}