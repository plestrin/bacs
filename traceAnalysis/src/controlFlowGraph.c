#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "controlFlowGraph.h"


static struct basicBlock* controlFlowGraph_create_basicBlock(struct controlFlowGraph* cfg);
static int controlFlowGraph_search_ins(struct controlFlowGraph* cfg, struct instruction* ins);
static int controlFlowGraph_split_basicBlock_after(struct controlFlowGraph* cfg);
static int controlFlowGraph_split_basicBlock_before(struct controlFlowGraph* cfg, int bb_orig_offset, struct basicBlock** bb_new, struct instruction* ins);

static struct edge* controlFlowGraph_create_edge(struct controlFlowGraph* cfg);
static int controlFlowGraph_search_edge(struct controlFlowGraph* cfg, struct instructionPointer* ip);
static int controlFlowGraph_add_edge(struct controlFlowGraph* cfg);
static int controlFlowGraph_add_edge_split_after(struct controlFlowGraph* cfg, struct basicBlock* bb, int nb_execution);
static int controlFlowGraph_add_edge_split_before(struct controlFlowGraph* cfg, struct basicBlock* bb_orig, struct basicBlock* bb_new, int nb_execution);


struct controlFlowGraph* controlFlowGraph_create(){
	struct controlFlowGraph* cfg;

	cfg = (struct controlFlowGraph*)malloc(sizeof(struct controlFlowGraph));
	if (cfg == NULL){
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
	}
	else{
		cfg->blocks = (struct basicBlock*)malloc(sizeof(struct basicBlock) * CONTROLFLOWGRAPH_DEFAULT_NB_BB);
		if (cfg->blocks == NULL){
			printf("ERROR: in %s, unable to allocate memory\n", __func__);
			free(cfg);
			cfg = NULL;
		}
		else{
			cfg->edges = (struct edge*)malloc(sizeof(struct edge) * CONTROLFLOWGRAPH_DEFAULT_NB_EDGE);
			if (cfg->edges == NULL){
				printf("ERROR: in %s, unable to allocate memory\n", __func__);
				free(cfg->blocks);
				free(cfg);
				cfg = NULL;
			}
			else{
				cfg->nb_block 				= 0;
				cfg->nb_edge 				= 0;
				cfg->nb_allocated_block 	= CONTROLFLOWGRAPH_DEFAULT_NB_BB;
				cfg->nb_allocated_edge 		= CONTROLFLOWGRAPH_DEFAULT_NB_EDGE;
				cfg->entry_point 			= NULL;
				cfg->exit_point 			= NULL;

				instructionPointer_init(&(cfg->ip));
			}
		}
	}

	return cfg;
}

int controlFlowGraph_add(struct controlFlowGraph* cfg, struct instruction* ins){
	int 				result = -1;
	struct basicBlock*	bb;
	int 				bb_offset;


	if (cfg != NULL){
		if (!instructionPointer_is_valid(&(cfg->ip))){
			bb = controlFlowGraph_create_basicBlock(cfg);
			if(bb == NULL){
				printf("ERROR: in %s, unable to create basic block\n", __func__);
			}
			else{
				result = basicBlock_add_ins(bb, BASICBLOCK_FROM_ADDRESS_TO_OFFSET(cfg, bb), ins, &(cfg->ip));
			}
		}
		else{
			if (basicBlock_get_instruction(cfg->blocks + instructionPointer_get_basicBlock_offset(&(cfg->ip)), instructionPointer_get_instruction_offset(&(cfg->ip)))->pc_next == ins->pc){
				if (basicBlock_contain_ins(cfg->blocks + instructionPointer_get_basicBlock_offset(&(cfg->ip)), ins)){
					result = basicBlock_add_ins(cfg->blocks + instructionPointer_get_basicBlock_offset(&(cfg->ip)), instructionPointer_get_basicBlock_offset(&(cfg->ip)), ins, &(cfg->ip));
				}
				else{
					bb_offset = controlFlowGraph_search_ins(cfg, ins);
					if (bb_offset < 0){
						bb = cfg->blocks + instructionPointer_get_basicBlock_offset(&(cfg->ip));
						if (basicBlock_is_expendable(bb)){
							result = basicBlock_add_ins(bb, BASICBLOCK_FROM_ADDRESS_TO_OFFSET(cfg, bb), ins, &(cfg->ip));
						}
						else{

							bb = controlFlowGraph_create_basicBlock(cfg);
							if(bb == NULL){
								printf("ERROR: in %s, unable to create basic block\n", __func__);
							}
							else{
								result = basicBlock_add_ins(bb, BASICBLOCK_FROM_ADDRESS_TO_OFFSET(cfg, bb), ins, &(cfg->ip));
								if (!result){
									result= controlFlowGraph_add_edge(cfg);
									if (result){
										printf("ERROR: in %s, unable to add edge to the CFG\n", __func__);
									}
								}
								else{
									printf("ERROR: in %s, unable to add instruction to the given basic block\n", __func__);
								}	
							}
						}
					}
					else{
						bb = cfg->blocks + bb_offset;
						if (basicBlock_start_with_ins(bb, ins)){
							result = basicBlock_add_ins(bb, BASICBLOCK_FROM_ADDRESS_TO_OFFSET(cfg, bb), ins, &(cfg->ip));
							if (!result){
								result= controlFlowGraph_add_edge(cfg);
								if (result){
									printf("ERROR: in %s, unable to add edge to the CFG\n", __func__);
								}
							}
							else{
								printf("ERROR: in %s, unable to add instruction to the given basic block\n", __func__);
							}
						}
						else{
							printf("ERROR: in %s, this case is not meant to happen!\n", __func__);
						}
					}
				}
			}
			else{
				if ((basicBlock_get_nb_instruction(cfg->blocks + instructionPointer_get_basicBlock_offset(&(cfg->ip))) != instructionPointer_get_instruction_offset(&(cfg->ip)) + 1)){
					result = controlFlowGraph_split_basicBlock_after(cfg);
					if (result){
						printf("ERROR: in %s, unable to split basic block\n", __func__);
					}
				}


				bb_offset = controlFlowGraph_search_ins(cfg, ins);
				if (bb_offset < 0){
					bb = controlFlowGraph_create_basicBlock(cfg);
					if(bb == NULL){
						printf("ERROR: in %s, unable to create basic block\n", __func__);
					}
					else{
						result = basicBlock_add_ins(bb, BASICBLOCK_FROM_ADDRESS_TO_OFFSET(cfg, bb), ins, &(cfg->ip));
						if (!result){
							result= controlFlowGraph_add_edge(cfg);
							if (result){
								printf("ERROR: in %s, unable to add edge to the CFG\n", __func__);
							}
						}
						else{
							printf("ERROR: in %s, unable to add instruction to the given basic block\n", __func__);
						}
					}
				}
				else{
					if (basicBlock_start_with_ins(cfg->blocks + bb_offset, ins)){
						result = basicBlock_add_ins(cfg->blocks + bb_offset, bb_offset, ins, &(cfg->ip));
						if (!result){
							result= controlFlowGraph_add_edge(cfg);
							if (result){
								printf("ERROR: in %s, unable to add edge to the CFG\n", __func__);
							}
						}
						else{
							printf("ERROR: in %s, unable to add instruction to the given basic block\n", __func__);
						}
					}
					else{
						result = controlFlowGraph_split_basicBlock_before(cfg, bb_offset, &bb, ins);
						if (result){
							printf("ERROR: in %s, unable to split destination basic block\n", __func__);
						}
						else{
							result = basicBlock_add_ins(bb, BASICBLOCK_FROM_ADDRESS_TO_OFFSET(cfg, bb), ins, &(cfg->ip));
							if (result){
								printf("ERROR: in %s, unable to add instruction to the given basic block\n", __func__);
							}
							else{
								result= controlFlowGraph_add_edge(cfg);
								if (result){
									printf("ERROR: in %s, unable to add edge to the CFG\n", __func__);
								}
							}
						}
					}
				}
			}
		}
	}

	return result;
}

int controlFlowGraph_get_edge_src_offset(struct controlFlowGraph* cfg, int edge_offset){
	int result = -1;
	int i;

	for (i = 0; i < cfg->nb_block; i++){
		if (edge_is_src(cfg->edges + edge_offset, cfg->blocks + i)){
			result = i;
			break;
		}
	}

	if (result == -1){
		printf("ERROR: in %s, unable to find edge src. This case is not meant to happen!\n", __func__);
	}

	return result;
}

int controlFlowGraph_get_edge_dst_offset(struct controlFlowGraph* cfg, int edge_offset){
	int result = -1;
	int i;

	for (i = 0; i < cfg->nb_block; i++){
		if (edge_is_dst(cfg->edges + edge_offset, cfg->blocks + i)){
			result = i;
			break;
		}
	}

	if (result == -1){
		printf("ERROR: in %s, unable to find edge dst. This case is not meant to happen!\n", __func__);
	}

	return result;
}

void controlFlowGraph_delete(struct controlFlowGraph* cfg){
	int i;

	if (cfg != NULL){
		if (cfg->blocks != NULL){
			for (i = 0; i < cfg->nb_block; i++){
				basicBlock_clean(cfg->blocks + i);
			}
			free(cfg->blocks);
		}
		if (cfg->edges != NULL){
			free(cfg->edges);
		}
		free(cfg);
	}
}

static int controlFlowGraph_search_ins(struct controlFlowGraph* cfg, struct instruction* ins){
	int i;
	int result = -1;

	for (i = 0; i < cfg->nb_block; i++){
		if (basicBlock_contain_ins(cfg->blocks + i, ins)){
			result = i;
			break;
		}
	}

	return result;
}

static struct basicBlock* controlFlowGraph_create_basicBlock(struct controlFlowGraph* cfg){
	struct basicBlock* bb = NULL;

	if (cfg->nb_allocated_block == cfg->nb_block){
		cfg->blocks = (struct basicBlock*)realloc(cfg->blocks, sizeof(struct basicBlock)*(cfg->nb_allocated_block + CONTROLFLOWGRAPH_DEFAULT_NB_BB));
		if (cfg->blocks == NULL){
			printf("ERROR: in %s, unable to realloc memory\n", __func__);
		}
		else{
			cfg->nb_allocated_block += CONTROLFLOWGRAPH_DEFAULT_NB_BB;
			bb = cfg->blocks + cfg->nb_block;

			if (basicBlock_init(bb)){
				printf("ERROR: in %s, unable to init basic block\n", __func__);
				bb = NULL;
			}
			else{
				cfg->nb_block ++;
			}
		}
	}
	else{
		bb = cfg->blocks + cfg->nb_block;
		
		if (basicBlock_init(bb)){
			printf("ERROR: in %s, unable to init basic block\n", __func__);
			bb  =NULL;
		}
		else{
			cfg->nb_block ++;
		}
	}

	return bb;
}

static int controlFlowGraph_split_basicBlock_before(struct controlFlowGraph* cfg, int bb_orig_offset, struct basicBlock** bb_new, struct instruction* ins){
	int 				nb_execution_edge;
	int 				result = -1;

	*bb_new = controlFlowGraph_create_basicBlock(cfg);
	if (*bb_new == NULL){
		printf("ERROR: in %s, unable to create basic block\n", __func__);
	}
	else{
		nb_execution_edge = basicBlock_get_nb_execution(cfg->blocks + bb_orig_offset);
		
		result = basicBlock_split_before(cfg->blocks + bb_orig_offset, bb_orig_offset, *bb_new, BASICBLOCK_FROM_ADDRESS_TO_OFFSET(cfg, *bb_new), ins, &(cfg->ip));
		if (result){
			printf("ERROR: in %s, unable to slipt basic block\n", __func__);
		}
		else{
			result = controlFlowGraph_add_edge_split_before(cfg, cfg->blocks + bb_orig_offset, *bb_new, nb_execution_edge);
			if (result){
				printf("ERROR: in %s, unable to add edge\n", __func__);
			}
		}
	}

	return result;
}

static int controlFlowGraph_split_basicBlock_after(struct controlFlowGraph* cfg){
	int 				nb_execution_edge;
	struct basicBlock*	bb;
	int 				result = -1;

	bb = controlFlowGraph_create_basicBlock(cfg);
	if (bb == NULL){
		printf("ERROR: in %s, unable to create basic block\n", __func__);
	}
	else{
		nb_execution_edge = basicBlock_get_nb_execution(cfg->blocks + instructionPointer_get_basicBlock_offset(&(cfg->ip))) - 1;

		result = basicBlock_split_after(bb, cfg->blocks, &(cfg->ip));
		if (result){
			printf("ERROR: in %s, unable to slipt basic block\n", __func__);
		}
		else{
			result = controlFlowGraph_add_edge_split_after(cfg, bb, nb_execution_edge);
			if (result){
				printf("ERROR: in %s, unable to add edge\n", __func__);
			}
		}
	}
	
	return result;
}

static struct edge* controlFlowGraph_create_edge(struct controlFlowGraph* cfg){
	struct edge* edge = NULL;

	if (cfg->nb_allocated_edge == cfg->nb_edge){
		cfg->edges = (struct edge*)realloc(cfg->edges, sizeof(struct edge)*(cfg->nb_allocated_edge + CONTROLFLOWGRAPH_DEFAULT_NB_EDGE));
		if (cfg->edges == NULL){
			printf("ERROR: in %s, unable to realloc memory\n", __func__);
		}
		else{
			cfg->nb_allocated_edge += CONTROLFLOWGRAPH_DEFAULT_NB_EDGE;
			edge = cfg->edges + cfg->nb_edge;
			cfg->nb_edge ++;
		}
	}
	else{
		edge = cfg->edges + cfg->nb_edge;
		cfg->nb_edge ++;
	}

	return edge;
}

static int controlFlowGraph_search_edge(struct controlFlowGraph* cfg, struct instructionPointer* ip){
	int i;
	int result = -1;

	for (i = 0; i < cfg->nb_edge; i++){
		if (edge_equal(cfg->edges + i, cfg->blocks, ip)){
			result = i;
			break;
		}
	}

	return result;
}

static int controlFlowGraph_add_edge(struct controlFlowGraph* cfg){
	struct edge* 	edge;
	int 			edge_offset;

	edge_offset = controlFlowGraph_search_edge(cfg, &(cfg->ip));
	if (edge_offset < 0){
		edge = controlFlowGraph_create_edge(cfg);
		if (edge == NULL){
			printf("ERROR: in %s, unable to create edge\n", __func__);
			return -1;
		}
		edge_init(edge, cfg->blocks, &(cfg->ip), 1);
	}
	else{
		edge = cfg->edges + edge_offset;
		edge_increment(edge, 1);
	}

	return 0;
}

static int controlFlowGraph_add_edge_split_after(struct controlFlowGraph* cfg, struct basicBlock* bb, int nb_execution){
	struct instructionPointer 	ip;
	struct edge*				edge;
	int 						edge_offset;
	int 						result = -1;

	ip.bb_offset 				= BASICBLOCK_FROM_ADDRESS_TO_OFFSET(cfg, bb);
	ip.instruction_offset 		= 0;
	ip.prev_bb_offset 			= instructionPointer_get_basicBlock_offset(&(cfg->ip));
	ip.prev_instruction_offset 	= 0;

	edge_offset = controlFlowGraph_search_edge(cfg, &ip);
	if (edge_offset < 0){
		edge = controlFlowGraph_create_edge(cfg);
		if (edge == NULL){
			printf("ERROR: in %s, unable to create edge\n", __func__);
		}
		else{
			edge_init(edge, cfg->blocks, &ip, nb_execution);
			result = 0;
		}
	}
	else{
		edge = cfg->edges + edge_offset;
		edge_increment(edge, nb_execution);
		result = 0;
	}

	return result;

}

static int controlFlowGraph_add_edge_split_before(struct controlFlowGraph* cfg, struct basicBlock* bb_orig, struct basicBlock* bb_new, int nb_execution){
	struct instructionPointer 	ip;
	struct edge*				edge;
	int 						edge_offset;
	int 						result = -1;

	ip.bb_offset 				= BASICBLOCK_FROM_ADDRESS_TO_OFFSET(cfg, bb_new);
	ip.instruction_offset 		= 0;
	ip.prev_bb_offset 			= BASICBLOCK_FROM_ADDRESS_TO_OFFSET(cfg, bb_orig);
	ip.prev_instruction_offset 	= 0;

	edge_offset = controlFlowGraph_search_edge(cfg, &ip);
	if (edge_offset < 0){
		edge = controlFlowGraph_create_edge(cfg);
		if (edge == NULL){
			printf("ERROR: in %s, unable to create edge\n", __func__);
		}
		else{
			edge_init(edge, cfg->blocks, &ip, nb_execution);
			result = 0;
		}
	}
	else{
		edge = cfg->edges + edge_offset;
		edge_increment(edge, nb_execution);
		result = 0;
	}

	return result;

}
