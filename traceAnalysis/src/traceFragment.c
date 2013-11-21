#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "traceFragment.h"

struct traceFragment* codeSegment_create(){
	struct traceFragment* frag = (struct traceFragment*)malloc(sizeof(struct traceFragment));

	if (frag != NULL){
		traceFragment_init(frag);
	}
	else{
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
	}

	return frag;
}

void traceFragment_init(struct traceFragment* frag){
	if (frag != NULL){
		frag->instructions = (struct instruction*)malloc(sizeof(struct instruction) * TRACEFRAGMENT_INSTRUCTION_BATCH);
		if (frag->instructions != NULL){
			frag->nb_allocated_instruction = TRACEFRAGMENT_INSTRUCTION_BATCH;
		}
		else{
			printf("ERROR: in %s, unable to allocate memory\n", __func__);
			frag->nb_allocated_instruction = 0;
		}
		frag->nb_instruction = 0;
	}
	else{
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
	}
}

int traceFragment_add_instruction(struct traceFragment* frag, struct instruction* ins){
	struct instruction* new_buffer;

	if (frag != NULL){
		if (frag->nb_allocated_instruction == frag->nb_instruction){
			new_buffer = (struct instruction*)realloc(frag->instructions, sizeof(struct instruction) * (frag->nb_allocated_instruction + TRACEFRAGMENT_INSTRUCTION_BATCH));
			if (new_buffer != NULL){
				frag->instructions = new_buffer;
				frag->nb_allocated_instruction += TRACEFRAGMENT_INSTRUCTION_BATCH;
			}
			else{
				printf("ERROR: in %s, unable to allocate memory\n", __func__);
				return -1;
			}
		}

		memcpy(frag->instructions + frag->nb_instruction, ins, sizeof(struct instruction));
		frag->nb_instruction ++;

		return frag->nb_instruction - 1;
	}

	return -1;
}

int traceFragment_search_pc(struct traceFragment* frag, struct instruction* ins){
	int result = -1;
	int i;

	if (frag != NULL){
		for (i = 0; i < frag->nb_instruction; i++){
			if (instruction_compare_pc(ins, frag->instructions + i)){
				result = i;
				break;
			}
		}
	}

	return result;
}

struct instruction* traceFragment_get_last_instruction(struct traceFragment* frag){
	struct instruction* instruction = NULL;

	if (frag != NULL){
		if (frag->nb_instruction > 0){
			instruction = frag->instructions + (frag->nb_instruction - 1);
		}
	}

	return instruction;
}

float traceFragment_opcode_percent(struct traceFragment* frag, int nb_opcode, uint32_t* opcode, int nb_excluded_opcode, uint32_t* excluded_opcode){
	float 	result = 0;
	int 	i;
	int 	j;
	int 	nb_effective_instruction = 0;
	int 	nb_found_instruction = 0;
	char 	excluded;

	if (frag != NULL){
		for (i = 0; i < frag->nb_instruction; i++){
			excluded = 0;
			if (excluded_opcode != NULL){
				for (j = 0; j < nb_excluded_opcode; j++){
					if (frag->instructions[i].opcode == excluded_opcode[j]){
						excluded = 1;
						break;
					}
				}
			}

			if (!excluded){
				nb_effective_instruction++;
				if (opcode != NULL){
					for (j = 0; j < nb_opcode; j++){
						if (frag->instructions[i].opcode == opcode[j]){
							nb_found_instruction ++;
							break;
						}
					}
				}
			}
		}

		result = (float)nb_found_instruction/(float)((nb_effective_instruction == 0)?1:nb_effective_instruction);
	}

	return result;
}

void traceFragment_delete(struct traceFragment* frag){
	if (frag != NULL){
		traceFragment_clean(frag);
		
		free(frag);
	}
}

void traceFragment_clean(struct traceFragment* frag){
	if (frag != NULL){
		if (frag->instructions != NULL){
			free(frag->instructions);
		}
	}
}

