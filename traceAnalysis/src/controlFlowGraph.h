#ifndef CONTROLFLOWGRAPH_H
#define CONTROLFLOWGRAPH_H

#include "basicBlock.h"
#include "instruction.h"

#define CONTROLFLOWGRAPH_DEFAULT_NB_BB		256
#define CONTROLFLOWGRAPH_DEFAULT_NB_EDGE	256

#define BASICBLOCK_FROM_ADDRESS_TO_OFFSET(cfg, add)  (((char*)add - (char*)cfg->blocks)/sizeof(struct basicBlock))

struct controlFlowGraph{
	struct basicBlock* 			blocks;
	struct edge*				edges;
	int 						nb_block;
	int 						nb_edge;
	int 						nb_allocated_block;
	int 						nb_allocated_edge;
	struct basicBlock*			entry_point;			/* attention pour le moment pas utilisé */
	struct basicBlock*			exit_point;				/* attention pour le moment pas utilisé */
	struct instructionPointer 	ip;
};

struct controlFlowGraph* controlFlowGraph_create();
int controlFlowGraph_add(struct controlFlowGraph* cfg, struct instruction* ins);
int controlFlowGraph_get_edge_src_offset(struct controlFlowGraph* cfg, int edge_offset);
int controlFlowGraph_get_edge_dst_offset(struct controlFlowGraph* cfg, int edge_offset);
void controlFlowGraph_delete(struct controlFlowGraph* cfg);

#endif