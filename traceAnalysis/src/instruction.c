#include <stdlib.h>
#include <stdio.h>

#include "instruction.h"

void instruction_print(struct instruction *ins){
	uint32_t i;

	if (ins != NULL){
		printf("*** Instruction %p ***\n", (void*)ins);
		printf("\tPC: \t\t0x%08lx\n", ins->pc);
		printf("\tPC next: \t0x%08lx\n", ins->pc_next);
		printf("\tOpcode: \t%u\n", ins->opcode);

		for ( i = 0; i < INSTRUCTION_MAX_NB_DATA; i++){
			if (INSTRUCTION_DATA_TYPE_IS_VALID(ins->data[i].type)){
				if (INSTRUCTION_DATA_TYPE_IS_READ(ins->data[i].type)){
					printf("\tData read:\n");
				}
				else{
					printf("\tData write:\n");
				}

				if (INSTRUCTION_DATA_TYPE_IS_MEM(ins->data[i].type)){
					printf("\t\tMem: \t0x%08lx\n", ins->data[i].location.address);
				}
				else{
					/* a completer */
				}
				switch(ins->data[i].size){
				case 1 	: {printf("\t\tValue: \t0x%02x\n", ins->data[i].value & 0x000000ff); break;}
				case 2 	: {printf("\t\tValue: \t0x%02x\n", ins->data[i].value & 0x0000ffff); break;}
				case 4 	: {printf("\t\tValue: \t0x%02x\n", ins->data[i].value & 0xffffffff); break;}
				default : {printf("WARNING: in %s, unexpected data size\n", __func__); break;}
				}
				printf("\t\tSize: \t%u\n", ins->data[i].size);
			}
		}
	}
}