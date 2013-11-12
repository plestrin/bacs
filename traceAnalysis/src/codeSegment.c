#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "codeSegment.h"

struct codeSegment* codeSegment_create(){
	struct codeSegment* seg = (struct codeSegment*)malloc(sizeof(struct codeSegment));

	if (seg != NULL){
		codeSegment_init(seg);
	}
	else{
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
	}

	return seg;
}

void codeSegment_init(struct codeSegment* seg){
	if (seg != NULL){
		seg->instructions = (struct instruction*)malloc(sizeof(struct instruction) * CODESEGMENT_INSTRUCTION_BATCH);
		if (seg->instructions != NULL){
			seg->nb_allocated_instruction = CODESEGMENT_INSTRUCTION_BATCH;
		}
		else{
			printf("ERROR: in %s, unable to allocate memory\n", __func__);
			seg->nb_allocated_instruction = 0;
		}
		seg->nb_instruction = 0;
	}
	else{
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
	}
}

int codeSegment_add_instruction(struct codeSegment* seg, struct instruction* ins){
	struct instruction* new_buffer;

	if (seg != NULL){
		if (seg->nb_allocated_instruction == seg->nb_instruction){
			new_buffer = (struct instruction*)realloc(seg->instructions, sizeof(struct instruction) * (seg->nb_allocated_instruction + CODESEGMENT_INSTRUCTION_BATCH));
			if (new_buffer != NULL){
				seg->instructions = new_buffer;
				seg->nb_allocated_instruction += CODESEGMENT_INSTRUCTION_BATCH;
			}
			else{
				printf("ERROR: in %s, unable to allocate memory\n", __func__);
				return -1;
			}
		}

		memcpy(seg->instructions + seg->nb_instruction, ins, sizeof(struct instruction));
		seg->nb_instruction ++;

		return seg->nb_instruction - 1;
	}

	return -1;
}

int codeSegment_search_instruction(struct codeSegment* seg, struct instruction* ins){
	int result = -1;
	int i;

	if (seg != NULL){
		for (i = 0; i < seg->nb_instruction; i++){
			if (instruction_compare(ins, seg->instructions + i)){
				result = i;
				break;
			}
		}
	}

	return result;
}

void codeSegment_delete(struct codeSegment* seg){
	if (seg != NULL){
		codeSegment_clean(seg);
		
		free(seg);
	}
}

void codeSegment_clean(struct codeSegment* seg){
	if (seg != NULL){
		if (seg->instructions != NULL){
			free(seg->instructions);
		}
	}
}

