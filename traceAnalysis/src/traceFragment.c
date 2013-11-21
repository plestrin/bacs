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

