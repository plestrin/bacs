#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "irMemory.h"

#include "irVariableRange.h"
#include "dagPartialOrder.h"


#define IRMEMORY_ALIAS_HEURISTIC_ESP 1

int32_t compare_order_memoryNode(const void* arg1, const void* arg2);

static enum irOpcode ir_normalize_choose_part_opcode(uint8_t size_src, uint8_t size_dst){
	if (size_src == 32 && size_dst == 8){
		return IR_PART1_8;
	}
	else if (size_src == 32 && size_dst == 16){
		return IR_PART1_16;
	}
	else{
		printf("ERROR: in %s, this case is not implemented (src=%u, dst=%u)\n", __func__, size_src, size_dst);
		return IR_PART1_8;
	}
}

enum aliasResult{
	MAY_ALIAS,
	MUST_ALIAS,
	CANNOT_ALIAS
};

#define ADDRESS_NB_MAX_DEPENDENCE 32 /* it must not exceed 0x0fffffff because the last bit of the flag is reversed for leave tagging */
#define FINGERPRINT_MAX_RECURSION_LEVEL 5

struct addrFingerprint{
	uint32_t 			nb_dependence;
	#if IRMEMORY_ALIAS_HEURISTIC_ESP == 1
	uint32_t 			flag;
	#endif
	struct {
		struct node* 	node;
		uint32_t 		flag;
		uint32_t 		label;
	} 					dependence[ADDRESS_NB_MAX_DEPENDENCE];
};

static void addrFingerprint_init(struct node* node, struct addrFingerprint* addr_fgp, uint32_t recursion_level, uint32_t parent_label){
	struct irOperation* operation;
	struct edge* 		operand_cursor;
	uint32_t 			nb_dependence;

	if (recursion_level == 0){
		addr_fgp->nb_dependence = 0;
		#if IRMEMORY_ALIAS_HEURISTIC_ESP == 1
		addr_fgp->flag = 0;
		#endif
	}
	else if (recursion_level == FINGERPRINT_MAX_RECURSION_LEVEL){
		return;
	}

	if (addr_fgp->nb_dependence < ADDRESS_NB_MAX_DEPENDENCE){
		addr_fgp->dependence[addr_fgp->nb_dependence].node = node;
		addr_fgp->dependence[addr_fgp->nb_dependence].flag = parent_label & 0x7fffffff;
		addr_fgp->dependence[addr_fgp->nb_dependence].label = addr_fgp->nb_dependence;

		addr_fgp->nb_dependence ++;
	}
	else{
		printf("ERROR: in %s, the max number of dependence has been reached\n", __func__);
		return;
	}

	operation = ir_node_get_operation(node);
	nb_dependence = addr_fgp->nb_dependence;

	switch(operation->type){
		case IR_OPERATION_TYPE_INST 	: {
			if (operation->operation_type.inst.opcode == IR_ADD){
				for (operand_cursor = node_get_head_edge_dst(node); operand_cursor != NULL; operand_cursor = edge_get_next_dst(operand_cursor)){
					addrFingerprint_init(edge_get_src(operand_cursor), addr_fgp, recursion_level + 1, addr_fgp->nb_dependence);
				}
			}
			break;
		}
		#if IRMEMORY_ALIAS_HEURISTIC_ESP == 1
		case IR_OPERATION_TYPE_IN_REG 	: {
			if (operation->operation_type.in_reg.reg == IR_REG_ESP){
				addr_fgp->flag |= 0x00000001;
			}
			break;
		}
		#endif
		default 						: {
			break;
		}
	}

	if (nb_dependence == addr_fgp->nb_dependence){
		addr_fgp->dependence[nb_dependence - 1].flag |= 0x80000000;
	}

	return;
}

static void addrFingerprint_remove(struct addrFingerprint* addr_fgp, uint32_t start, uint32_t label){
	uint32_t i;

	for (i = start; i < addr_fgp->nb_dependence; ){
		if ((addr_fgp->dependence[i].flag & 0x7fffffff) == label){
			if (addr_fgp->dependence[i].flag & 0x80000000){
				addrFingerprint_remove(addr_fgp, i + 1, addr_fgp->dependence[i].label);
			}
			if (i + 1 < addr_fgp->nb_dependence){
				addr_fgp->dependence[i].node 	= addr_fgp->dependence[addr_fgp->nb_dependence - 1].node;
				addr_fgp->dependence[i].flag 	= addr_fgp->dependence[addr_fgp->nb_dependence - 1].flag;
				addr_fgp->dependence[i].label 	= addr_fgp->dependence[addr_fgp->nb_dependence - 1].label;
			}
			addr_fgp->nb_dependence --;
		}
		else{
			i ++;
		}
	}
}

static enum aliasResult ir_normalize_alias_analysis(struct addrFingerprint* addr1_fgp_ptr, struct node* addr2){
	struct addrFingerprint 	addr1_fgp;
	struct addrFingerprint 	addr2_fgp;
	uint32_t 				i;
	uint32_t 				j;
	struct irVariableRange 	range1;
	struct irVariableRange 	range2;
	uint32_t 				nb_dependence_left1;
	uint32_t 				nb_dependence_left2;
	struct node* 			dependence_left1[ADDRESS_NB_MAX_DEPENDENCE];
	struct node* 			dependence_left2[ADDRESS_NB_MAX_DEPENDENCE];

	memcpy(&addr1_fgp, addr1_fgp_ptr, sizeof(struct addrFingerprint));
	addrFingerprint_init(addr2, &addr2_fgp, 0, 0);

	for (i = 0; i < addr1_fgp.nb_dependence; ){
		for (j = 0; j < addr2_fgp.nb_dependence; j++){
			if (addr1_fgp.dependence[i].node == addr2_fgp.dependence[j].node){
				addrFingerprint_remove(&addr1_fgp, i + 1, addr1_fgp.dependence[i].label);
				addrFingerprint_remove(&addr2_fgp, j + 1, addr2_fgp.dependence[j].label);

				if (i + 1 < addr1_fgp.nb_dependence){
					addr1_fgp.dependence[i].node 	= addr1_fgp.dependence[addr1_fgp.nb_dependence - 1].node;
					addr1_fgp.dependence[i].flag 	= addr1_fgp.dependence[addr1_fgp.nb_dependence - 1].flag;
					addr1_fgp.dependence[i].label 	= addr1_fgp.dependence[addr1_fgp.nb_dependence - 1].label;
				}
				addr1_fgp.nb_dependence --;

				if (j + 1 < addr2_fgp.nb_dependence){
					addr2_fgp.dependence[j].node 	= addr2_fgp.dependence[addr2_fgp.nb_dependence - 1].node;
					addr2_fgp.dependence[j].flag 	= addr2_fgp.dependence[addr2_fgp.nb_dependence - 1].flag;
					addr2_fgp.dependence[j].label 	= addr2_fgp.dependence[addr2_fgp.nb_dependence - 1].label;
				}
				addr2_fgp.nb_dependence --;

				goto next;
			}
		}
		i ++;
		next:;
	}

	for (i = 0, nb_dependence_left1 = 0; i < addr1_fgp.nb_dependence; i++){
		if (addr1_fgp.dependence[i].flag & 0x80000000){
			dependence_left1[nb_dependence_left1 ++] = addr1_fgp.dependence[i].node;
		}
	}

	for (j = 0, nb_dependence_left2 = 0; j < addr2_fgp.nb_dependence; j++){
		if (addr2_fgp.dependence[j].flag & 0x80000000){
			dependence_left2[nb_dependence_left2 ++] = addr2_fgp.dependence[j].node;
		}
	}

	irVariableRange_get_range_additive_list(&range1, dependence_left1, nb_dependence_left1);
	irVariableRange_get_range_additive_list(&range2, dependence_left2, nb_dependence_left2);

	if (irVariableRange_is_cst(range1) && irVariableRange_is_cst(range2)){
		if (irVariableRange_cst_equal(range1, range2)){
			return MUST_ALIAS;
		}
		else{
			return CANNOT_ALIAS;
		}
	}
	else if (irVariableRange_intersect(&range1, &range2)){
		#if IRMEMORY_ALIAS_HEURISTIC_ESP == 1
		if ((addr1_fgp.flag & 0x00000001) == (addr2_fgp.flag & 0x00000001)){
			return MAY_ALIAS;
		}
		else{
			return CANNOT_ALIAS;
		}
		#else
		return MAY_ALIAS;
		#endif
	}

	return CANNOT_ALIAS;
}

static int32_t ir_normalize_search_alias_conflict(struct node* node1, struct node* node2, enum irOperationType alias_type, enum aliasingStrategy strategy){
	struct node* 			node_cursor;
	struct irOperation* 	operation_cursor;
	struct edge* 			edge_cursor;
	struct node* 			addr1 = NULL;
	struct node* 			addr_cursor;
	struct addrFingerprint 	addr1_fgp;

	node_cursor = ir_node_get_operation(node1)->operation_type.mem.access.next;
	if (node_cursor == NULL){
		printf("ERROR: in %s, found NULL pointer instead of the expected element\n", __func__);
		return 0;
	}

	while(node_cursor != node2){
		operation_cursor = ir_node_get_operation(node_cursor);
		if (operation_cursor->type == alias_type){
			if (strategy == ALIASING_STRATEGY_STRICT){
				return 1;
			}
			else{
				if (addr1 == NULL){
					for (edge_cursor = node_get_head_edge_dst(node1); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
						if (ir_edge_get_dependence(edge_cursor)->type == IR_DEPENDENCE_TYPE_ADDRESS){
							addr1 = edge_get_src(edge_cursor);
							addrFingerprint_init(addr1, &addr1_fgp, 0, 0);
							break;
						}
					}
				}

				for (edge_cursor = node_get_head_edge_dst(node_cursor), addr_cursor = NULL; edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
					if (ir_edge_get_dependence(edge_cursor)->type == IR_DEPENDENCE_TYPE_ADDRESS){
						addr_cursor = edge_get_src(edge_cursor);
						break;
					}
				}

				if (addr1 != NULL && addr_cursor != NULL){
					if (ir_normalize_alias_analysis(&addr1_fgp, addr_cursor) != CANNOT_ALIAS){
						return 1;
					}
				}
				else{
					printf("ERROR: in %s, memory access with no address operand\n", __func__);
				}
			}
		}
		node_cursor = operation_cursor->operation_type.mem.access.next;
		if (node_cursor == NULL){
			printf("ERROR: in %s, found NULL pointer instead of the expected element\n", __func__);
			break;
		}
	}

	return 0;
}

void ir_normalize_simplify_memory_access(struct ir* ir, uint8_t* modification, enum aliasingStrategy strategy){
	struct node* 			node_cursor;
	struct edge* 			edge_cursor;
	uint32_t 				nb_mem_access;
	struct node** 			access_list = NULL;
	uint32_t 				access_list_alloc_size;
	uint32_t 				i;
	struct irVariableRange* range_buffer;

	if (dagPartialOrder_sort_src_dst(&(ir->graph))){
		printf("ERROR: in %s, unable to sort DAG\n", __func__);
		return;
	}

	range_buffer = (struct irVariableRange*)malloc(sizeof(struct irVariableRange) * ir->graph.nb_node);
	if (range_buffer == NULL){
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
		return;
	}

	for (node_cursor = graph_get_head_node(&(ir->graph)), i = 0; node_cursor != NULL; node_cursor = node_get_next(node_cursor), i++){
		node_cursor->ptr =  range_buffer + i;
		irVariableRange_compute(node_cursor);
	}

	for(node_cursor = graph_get_head_node(&(ir->graph)); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
		if (node_cursor->nb_edge_src > 1){
			for (edge_cursor = node_get_head_edge_src(node_cursor), nb_mem_access = 0; edge_cursor != NULL; edge_cursor = edge_get_next_src(edge_cursor)){
				if (ir_edge_get_dependence(edge_cursor)->type == IR_DEPENDENCE_TYPE_ADDRESS && (ir_node_get_operation(edge_get_dst(edge_cursor))->type == IR_OPERATION_TYPE_IN_MEM || ir_node_get_operation(edge_get_dst(edge_cursor))->type == IR_OPERATION_TYPE_OUT_MEM)){
					nb_mem_access ++;
				}
			}
			if (nb_mem_access > 1){
				if (access_list == NULL || access_list_alloc_size < nb_mem_access * sizeof(struct node*)){
					if (access_list != NULL){
						free(access_list);
					}
					access_list = (struct node**)malloc(sizeof(struct node*) * nb_mem_access);
					access_list_alloc_size = sizeof(struct node*) * nb_mem_access;
					if (access_list == NULL){
						printf("ERROR: in %s, unable to allocate memory\n", __func__);
						continue;
					}
				}

				for (edge_cursor = node_get_head_edge_src(node_cursor), i = 0; edge_cursor != NULL; edge_cursor = edge_get_next_src(edge_cursor)){
					if (ir_edge_get_dependence(edge_cursor)->type == IR_DEPENDENCE_TYPE_ADDRESS && (ir_node_get_operation(edge_get_dst(edge_cursor))->type == IR_OPERATION_TYPE_IN_MEM || ir_node_get_operation(edge_get_dst(edge_cursor))->type == IR_OPERATION_TYPE_OUT_MEM)){
						access_list[i++] = edge_get_dst(edge_cursor);
					}
				}

				qsort(access_list, nb_mem_access, sizeof(struct node*), compare_order_memoryNode);

				for (i = 1; i < nb_mem_access; i++){
					struct irOperation* 		operation_prev;
					struct irOperation* 		operation_next;

					operation_prev = ir_node_get_operation(access_list[i - 1]);
					operation_next = ir_node_get_operation(access_list[i]);

					/* STORE -> LOAD */
					if (operation_prev->type == IR_OPERATION_TYPE_OUT_MEM && operation_next->type == IR_OPERATION_TYPE_IN_MEM){
						struct node* stored_value = NULL;

						if (strategy != ALIASING_STRATEGY_WEAK){
							if (ir_normalize_search_alias_conflict(access_list[i - 1], access_list[i], IR_OPERATION_TYPE_OUT_MEM, strategy)){
								continue;
							}
						}

						for (edge_cursor = node_get_head_edge_dst(access_list[i - 1]); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
							if (ir_edge_get_dependence(edge_cursor)->type != IR_DEPENDENCE_TYPE_ADDRESS){
								stored_value = edge_get_src(edge_cursor);
							}
						}

						if (stored_value != NULL){
							if (operation_prev->size > operation_next->size){
								ir_convert_node_to_inst(access_list[i], ir_normalize_choose_part_opcode(operation_prev->size, operation_next->size), operation_next->size);
								graph_remove_edge(&(ir->graph), graph_get_edge(node_cursor, access_list[i]));

								if (ir_add_dependence(ir, stored_value, access_list[i], IR_DEPENDENCE_TYPE_DIRECT) == NULL){
									printf("ERROR: in %s, unable to add new dependency to IR\n", __func__);
								}

								access_list[i] = access_list[i - 1];
							}
							else if (operation_prev->size < operation_next->size){
								printf("WARNING: in %s, simplification of memory access of different size (case STORE -> LOAD)\n", __func__);
								continue;
							}
							else{
								graph_transfert_src_edge(&(ir->graph), stored_value, access_list[i]);
								ir_remove_node(ir, access_list[i]);

								access_list[i] = access_list[i - 1];
							}
							*modification = 1;
							continue;
						}
						else{
							printf("ERROR: in %s, incorrect memory access pattern in STORE -> LOAD\n", __func__);
						}
					}

					/* LOAD -> LOAD */
					if (operation_prev->type == IR_OPERATION_TYPE_IN_MEM && operation_next->type == IR_OPERATION_TYPE_IN_MEM){

						if (strategy != ALIASING_STRATEGY_WEAK){
							if (ir_normalize_search_alias_conflict(access_list[i - 1], access_list[i], IR_OPERATION_TYPE_OUT_MEM, strategy)){
								continue;
							}
						}

						if (operation_prev->size > operation_next->size){
							ir_convert_node_to_inst(access_list[i], ir_normalize_choose_part_opcode(operation_prev->size, operation_next->size), operation_next->size);
							graph_remove_edge(&(ir->graph), graph_get_edge(node_cursor, access_list[i]));

							if (ir_add_dependence(ir, access_list[i - 1], access_list[i], IR_DEPENDENCE_TYPE_DIRECT) == NULL){
								printf("ERROR: in %s, unable to add new dependency to IR\n", __func__);
							}

							access_list[i] = access_list[i - 1];
						}
						else if (operation_prev->size < operation_next->size){
							ir_convert_node_to_inst(access_list[i - 1], ir_normalize_choose_part_opcode(operation_next->size, operation_prev->size), operation_prev->size);
							graph_remove_edge(&(ir->graph), graph_get_edge(node_cursor, access_list[i - 1]));

							if (ir_add_dependence(ir, access_list[i], access_list[i - 1], IR_DEPENDENCE_TYPE_DIRECT) == NULL){
								printf("ERROR: in %s, unable to add new dependency to IR\n", __func__);
							}
						}
						else{
							graph_transfert_src_edge(&(ir->graph), access_list[i - 1], access_list[i]);
							ir_remove_node(ir, access_list[i]);

							access_list[i] = access_list[i - 1];
						}
						*modification = 1;
						continue;
					}

					/* STORE -> STORE */
					if (operation_prev->type == IR_OPERATION_TYPE_OUT_MEM && operation_next->type == IR_OPERATION_TYPE_OUT_MEM){

						if (strategy != ALIASING_STRATEGY_WEAK){
							if (ir_normalize_search_alias_conflict(access_list[i - 1], access_list[i], IR_OPERATION_TYPE_IN_MEM, strategy)){
								continue;
							}
						}

						if (operation_prev->size > operation_next->size){
							printf("WARNING: in %s, simplification of memory access of different size (case STORE (%u bits) -> STORE (%u bits))\n", __func__, operation_prev->size, operation_next->size);
							continue;
						}

						ir_remove_node(ir, access_list[i - 1]);
						*modification = 1;
						continue;
					}
				}
			}
		}
	}

	if (access_list != NULL){
		free(access_list);
	}
	free(range_buffer);
}


/* ===================================================================== */
/* Sorting routines						                                 */
/* ===================================================================== */

int32_t compare_order_memoryNode(const void* arg1, const void* arg2){
	struct node* access1 = *(struct node**)arg1;
	struct node* access2 = *(struct node**)arg2;
	struct irOperation* op1 = ir_node_get_operation(access1);
	struct irOperation* op2 = ir_node_get_operation(access2);

	if (op1->operation_type.mem.access.order < op2->operation_type.mem.access.order){
		return -1;
	}
	else if (op1->operation_type.mem.access.order > op2->operation_type.mem.access.order){
		return 1;
	}
	else{
		return 0;
	}
}
