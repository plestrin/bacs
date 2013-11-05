#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "basicBlock.h"

static unsigned long id_generator = 0;


static int basicBlock_search_ins(struct basicBlock* bb, struct instruction* ins);
static struct instruction* basicBlock_create_ins(struct basicBlock* bb);


static inline unsigned long basicBlock_generate_id(){
	id_generator ++;
	return id_generator - 1;
}

int basicBlock_init(struct basicBlock* bb){
	bb->start_address 		= 0;
	bb->end_address  		= 0;
	bb->nb_execution		= 0;
	bb->id_entry			= basicBlock_generate_id();
	bb->id_exit				= basicBlock_generate_id();

	bb->instructions 		= (struct instruction*)malloc(sizeof(struct instruction) * BASICBLOCK_DEFAULT_NB_INSTRUCTION);
	if (bb->instructions == NULL){
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
		return -1;
	}

	bb->nb_instruction = 0;
	bb->nb_allocated_instruction = BASICBLOCK_DEFAULT_NB_INSTRUCTION;

	return 0;
}

int basicBlock_add_ins(struct basicBlock* bb, int bb_offset, struct instruction* ins, struct instructionPointer* ip){
	if ((bb->nb_instruction == 0) && (bb->nb_execution == 0)){
		struct instruction* new_ins = basicBlock_create_ins(bb);
		if (new_ins == NULL){
			printf("ERROR: in %s, unable to create instruction in basic block\n", __func__);
			return -1;
		}
		else{
			memcpy(new_ins, ins, sizeof(struct instruction));
			bb->start_address = ins->pc;
			bb->end_address = ins->pc;
			bb->nb_execution ++;
			
			ip->prev_bb_offset 			= ip->bb_offset;
			ip->prev_instruction_offset = ip->instruction_offset;
			ip->bb_offset 				= bb_offset;
			ip->instruction_offset 		= 0;
		}
	}
	else if (bb->start_address == ins->pc){
		bb->nb_execution ++;
		
		ip->prev_bb_offset 			= ip->bb_offset;
		ip->prev_instruction_offset = ip->instruction_offset;
		ip->bb_offset 				= bb_offset;
		ip->instruction_offset = 0;
	}
	else if (ins->pc > bb->end_address){
		struct instruction* new_ins = basicBlock_create_ins(bb);
		if (new_ins == NULL){
			printf("ERROR: in %s, unable to create instruction in basic block\n", __func__);
			return -1;
		}
		else{
			bb->end_address = ins->pc;
			memcpy(new_ins, ins, sizeof(struct instruction));

			ip->prev_bb_offset 			= ip->bb_offset;
			ip->prev_instruction_offset = ip->instruction_offset;
			ip->bb_offset 				= bb_offset;
			ip->instruction_offset 		= INSTRUCTION_FROM_ADDRESS_TO_OFFSET(bb, new_ins);
		}
	}
	else{
		int current_instruction_offset = basicBlock_search_ins(bb, ins);
		if (current_instruction_offset < 0){
			printf("ERROR: in %s, cn't find instruction in basic block, this case is not meant to happen\n", __func__);
			return -1;
		}
		else{
			ip->prev_bb_offset 			= ip->bb_offset;
			ip->prev_instruction_offset = ip->instruction_offset;
			ip->bb_offset 				= bb_offset;
			ip->instruction_offset 		= current_instruction_offset;
		}
	}

	return 0;
}

int basicBlock_split_after(struct basicBlock* bb, struct basicBlock* blocks, struct instructionPointer* ip){
	int nb_instruction_moved = (blocks + ip->bb_offset)->nb_instruction - (ip->instruction_offset + 1);

	if (nb_instruction_moved > bb->nb_allocated_instruction){
		while(nb_instruction_moved > bb->nb_allocated_instruction){
			bb->nb_allocated_instruction += BASICBLOCK_DEFAULT_NB_INSTRUCTION;
		}
		bb->instructions = (struct instruction*)realloc(bb->instructions, sizeof(struct instruction)*bb->nb_allocated_instruction);
		if (bb->instructions == NULL){
			printf("ERROR: in %s, unable to realloc memory\n", __func__);
			return -1;
		}
	}
	memcpy(bb->instructions, (blocks + ip->bb_offset)->instructions + ip->instruction_offset + 1, nb_instruction_moved*sizeof(struct instruction));
	bb->nb_instruction = nb_instruction_moved;

	bb->start_address = (blocks + ip->bb_offset)->instructions[ip->instruction_offset + 1].pc;
	bb->end_address = (blocks + ip->bb_offset)->end_address;
	bb->nb_execution = (blocks + ip->bb_offset)->nb_execution - 1;
	bb->id_entry = basicBlock_generate_id();
	bb->id_exit = (blocks + ip->bb_offset)->id_exit;

	(blocks + ip->bb_offset)->end_address = (blocks + ip->bb_offset)->instructions[ip->instruction_offset].pc;
	(blocks + ip->bb_offset)->id_exit = basicBlock_generate_id();
	(blocks + ip->bb_offset)->nb_instruction = ip->instruction_offset + 1;

	return 0;
}

int basicBlock_split_before(struct basicBlock* bb_orig, int bb_orig_offset, struct basicBlock* bb_new, int bb_new_offset, struct instruction* ins, struct instructionPointer* ip){
	int nb_instruction_moved = 0;
	int i;

	for (i = 0; i < bb_orig->nb_instruction; i++){
		if (ins->pc == bb_orig->instructions[i].pc){
			nb_instruction_moved = bb_orig->nb_instruction - i;
			break;
		}
	}

	if (nb_instruction_moved > bb_new->nb_allocated_instruction){
		while(nb_instruction_moved > bb_new->nb_allocated_instruction){
			bb_new->nb_allocated_instruction += BASICBLOCK_DEFAULT_NB_INSTRUCTION;
		}
		bb_new->instructions = (struct instruction*)realloc(bb_new->instructions, sizeof(struct instruction)*bb_new->nb_allocated_instruction);
		if (bb_new->instructions == NULL){
			printf("ERROR: in %s, unable to realloc memory\n", __func__);
			return -1;
		}
	}
	memcpy(bb_new->instructions, bb_orig->instructions + i, nb_instruction_moved*sizeof(struct instruction));
	bb_new->nb_instruction = nb_instruction_moved;

	bb_new->start_address = ins->pc;
	bb_new->end_address = bb_orig->end_address;
	bb_new->nb_execution = bb_orig->nb_execution;
	bb_new->id_entry = basicBlock_generate_id();
	bb_new->id_exit = bb_orig->id_exit;

	bb_orig->end_address = bb_orig->instructions[i - 1].pc;
	bb_orig->id_exit = basicBlock_generate_id();
	bb_orig->nb_instruction = i;

	if ((ip->bb_offset == bb_orig_offset) && (ip->instruction_offset >= i)){
		ip->bb_offset = bb_new_offset;
		ip->instruction_offset = ip->instruction_offset - i;
	}

	if ((ip->prev_bb_offset == bb_orig_offset) && (ip->prev_instruction_offset >= i)){
		ip->prev_bb_offset = bb_new_offset;
		ip->prev_instruction_offset = ip->prev_instruction_offset - i;
	}

	return 0;
}

void basicBlock_clean(struct basicBlock* bb){
	if (bb != NULL){
		if (bb->instructions != NULL){
			free(bb->instructions);
		}
	}
}

static int basicBlock_search_ins(struct basicBlock* bb, struct instruction* ins){
	int i;
	int result = -1;

	for (i = 0; i < bb->nb_instruction; i++){
		if (bb->instructions[i].pc == ins->pc){
			result = i;
			break;
		}
	}

	return result;                                                                                                                                                                                                                                                                                                                                                                                                                                   
}

static struct instruction* basicBlock_create_ins(struct basicBlock* bb){
	struct instruction* ins = NULL;

	if (bb->nb_allocated_instruction == bb->nb_instruction){
		bb->instructions = (struct instruction*)realloc(bb->instructions, sizeof(struct instruction)*(bb->nb_allocated_instruction + BASICBLOCK_DEFAULT_NB_INSTRUCTION));
		if (bb->instructions == NULL){
			printf("ERROR: in %s, unable to realloc memory\n", __func__);
		}
		else{
			bb->nb_allocated_instruction += BASICBLOCK_DEFAULT_NB_INSTRUCTION;
			ins = bb->instructions + bb->nb_instruction;
			bb->nb_instruction ++;
		}
	}
	else{
		ins = bb->instructions + bb->nb_instruction;
		bb->nb_instruction ++;
	}

	return ins;
}