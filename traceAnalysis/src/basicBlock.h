#ifndef BASICBLOCK_H
#define BASICBLOCK_H

#include "instruction.h"

#define BASICBLOCK_DEFAULT_NB_INSTRUCTION 32

#define INSTRUCTION_FROM_ADDRESS_TO_OFFSET(bb, add) (((char*)(add) - (char*)(bb->instructions))/sizeof(struct instruction))

struct basicBlock{
	unsigned long 		start_address;
	unsigned long 		end_address;
	int					nb_execution;
	unsigned long 		id_entry;
	unsigned long		id_exit;

	struct instruction*	instructions;
	int 				nb_instruction;
	int 				nb_allocated_instruction;
};

struct edge{
	unsigned long 		src;
	unsigned long 		dst;
	int					nb_execution;
};

struct instructionPointer{
	int	bb_offset;
	int instruction_offset;
	int	prev_bb_offset;
	int prev_instruction_offset;
};


int basicBlock_init(struct basicBlock* bb);
int basicBlock_add_ins(struct basicBlock* bb, int bb_offset, struct instruction* ins, struct instructionPointer* ip);
int basicBlock_split_after(struct basicBlock* bb, struct basicBlock* blocks, struct instructionPointer* ip);
int basicBlock_split_before(struct basicBlock* bb_orig, int bb_orig_offset, struct basicBlock* bb_new, int bb_new_offset, struct instruction* ins, struct instructionPointer* ip);
void basicBlock_clean(struct basicBlock* bb);



/* ===================================================================== */
/* Basic block inline functions	                                         */
/* ===================================================================== */

static inline int basicBlock_contain_ins(struct basicBlock* bb, struct instruction* ins){
	return ((bb->start_address <= ins->pc) && (bb->end_address >= ins->pc));
}

static inline int basicBlock_start_with_ins(struct basicBlock* bb, struct instruction* ins){
	return (bb->start_address == ins->pc);
}

static inline int basicBlock_end_with_ins(struct basicBlock* bb, struct instruction* ins){
	return (bb->end_address == ins->pc);
}

static inline int basicBlock_is_expendable(struct basicBlock* bb){
	return (bb->nb_execution == 1);
}

static inline int basicBlock_get_nb_execution(struct basicBlock* bb){
	return bb->nb_execution;
}

static inline int basicBlock_get_nb_instruction(struct basicBlock* bb){
	return bb->nb_instruction;
}

static inline struct instruction* basicBlock_get_instruction(struct basicBlock* bb, int instruction_offset){
	return bb->instructions + instruction_offset;
}


/* ===================================================================== */
/* Edge inline functions	                             	             */
/* ===================================================================== */

static inline void edge_init(struct edge* edge, struct basicBlock* blocks, struct instructionPointer* ip, int nb_execution){
	edge->src 				= (blocks + ip->prev_bb_offset)->id_exit;
	edge->dst 				= (blocks + ip->bb_offset)->id_entry;
	edge->nb_execution 		= nb_execution;
}

static inline int edge_equal(struct edge* edge , struct basicBlock* blocks, struct instructionPointer* ip){
	return ((edge->src == (blocks + ip->prev_bb_offset)->id_exit) && (edge->dst == (blocks + ip->bb_offset)->id_entry));
}

static inline void edge_increment(struct edge* edge, int nb_execution){
	edge->nb_execution += nb_execution;
}

static inline int edge_is_src(struct edge* edge, struct basicBlock* bb){
	return (edge->src == bb->id_exit);
}

static inline int edge_is_dst(struct edge* edge, struct basicBlock* bb){
	return (edge->dst == bb->id_entry);
}

static inline int edge_get_nb_execution(struct edge* edge){
	return edge->nb_execution;
}


/* ===================================================================== */
/* Edge inline functions	                             	             */
/* ===================================================================== */

static inline void instructionPointer_init(struct instructionPointer* ip){
	ip->bb_offset 				= -1;
	ip->instruction_offset 		= 0;
	ip->prev_bb_offset			= -1;
	ip->prev_instruction_offset = 0;
}

static inline int instructionPointer_is_valid(struct instructionPointer* ip){
	return (ip->bb_offset >= 0);
}

static inline int instructionPointer_get_basicBlock_offset(struct instructionPointer* ip){
	return ip->bb_offset;
}

static inline int instructionPointer_get_instruction_offset(struct instructionPointer* ip){
	return ip->instruction_offset;
}

#endif