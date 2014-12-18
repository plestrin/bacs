#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "irMemory.h"

#define IRMEMORY_ALIAS_HEURISTIC_ESP 1

int32_t compare_order_memoryNode(const void* arg1, const void* arg2);

#if IRMEMORY_ALIAS_HEURISTIC_ESP == 1
#define IRMEMORY_MAX_BASE_SYM_DST 3

static uint32_t ir_normalize_addr_depend_on_esp(struct node* node, uint32_t dst);

#endif

static enum irOpcode ir_normalize_choose_part_opcode(uint8_t size_src, uint8_t size_dst){
	if (size_src == 32 && size_dst == 8){
		return IR_PART1_8;
	}
	else{
		printf("ERROR: in %s, this case is not implemented (src=%u, dst=%u)\n", __func__, size_src, size_dst);
		return IR_PART1_8;
	}
}

static int32_t ir_normalize_cannot_alias(struct node* addr1, struct node* addr2){
	struct irOperation* op_ad1;
	struct irOperation* op_ad2;
	struct edge*		edge_cursor1;
	struct edge*		edge_cursor2;
	struct node* 		node_cursor1;
	struct node* 		node_cursor2;
	uint8_t* 			operand_buffer2;
	uint8_t 			nb_imm;
	uint8_t 			nb_oth;
	uint32_t 			i;

	if (addr1 == addr2){
		printf("ERROR: in %s, this case is supposed to happen, testing aliasing but get equal nodes\n", __func__);
		return 0;
	}
	
	op_ad1 = ir_node_get_operation(addr1);
	op_ad2 = ir_node_get_operation(addr2);

	#if IRMEMORY_ALIAS_HEURISTIC_ESP == 1
	if (ir_normalize_addr_depend_on_esp(addr1, 0) != ir_normalize_addr_depend_on_esp(addr2, 0)){
		return 0;
	}
	#endif

	if (op_ad1->type == IR_OPERATION_TYPE_IMM && op_ad1->type == IR_OPERATION_TYPE_IMM){
		if (ir_imm_operation_get_unsigned_value(op_ad1) == ir_imm_operation_get_unsigned_value(op_ad2)){
			return 1;
		}
		return 0;
	}
	else if (op_ad1->type == IR_OPERATION_TYPE_INST && op_ad2->type == IR_OPERATION_TYPE_INST){
		if (op_ad1->operation_type.inst.opcode == op_ad2->operation_type.inst.opcode && op_ad1->operation_type.inst.opcode == IR_ADD){
			operand_buffer2 = (uint8_t*)alloca(sizeof(uint8_t) * addr2->nb_edge_dst);
			
			memset(operand_buffer2, 0, sizeof(uint8_t) * addr2->nb_edge_dst);

			for (edge_cursor1 = node_get_head_edge_dst(addr1), nb_imm = 0; edge_cursor1 != NULL; edge_cursor1 = edge_get_next_dst(edge_cursor1)){
				node_cursor1 = edge_get_src(edge_cursor1);

				if (ir_node_get_operation(node_cursor1)->type == IR_OPERATION_TYPE_IMM){
					if (nb_imm){
						return 1;
					}
					
					for (edge_cursor2 = node_get_head_edge_dst(addr2), nb_imm ++; edge_cursor2 != NULL; edge_cursor2 = edge_get_next_dst(edge_cursor2)){
						node_cursor2 = edge_get_src(edge_cursor2);

						if (ir_node_get_operation(node_cursor2)->type == IR_OPERATION_TYPE_IMM && ir_imm_operation_get_unsigned_value(ir_node_get_operation(node_cursor1)) == ir_imm_operation_get_unsigned_value(ir_node_get_operation(node_cursor2))){
							return 1;
						}
					}
				}
				else{
					for (edge_cursor2 = node_get_head_edge_dst(addr2), i = 0; edge_cursor2 != NULL; edge_cursor2 = edge_get_next_dst(edge_cursor2), i++){
						node_cursor2 = edge_get_src(edge_cursor2);

						if (node_cursor2 == node_cursor1){
							operand_buffer2[i] = 1;
							break;
						}
					}
					if (edge_cursor2 == NULL){
						return 1;
					}
				}
			}

			for (edge_cursor2 = node_get_head_edge_dst(addr2), nb_imm = 0, i = 0; edge_cursor2 != NULL; edge_cursor2 = edge_get_next_dst(edge_cursor2), i++){
				if (operand_buffer2[i] == 0){
					node_cursor2 = edge_get_src(edge_cursor2);

					if (ir_node_get_operation(node_cursor2)->type == IR_OPERATION_TYPE_IMM){
						if (nb_imm){
							return 1;
						}

						for (edge_cursor1 = node_get_head_edge_dst(addr1), nb_imm ++; edge_cursor1 != NULL; edge_cursor1 = edge_get_next_dst(edge_cursor1)){
							node_cursor1 = edge_get_src(edge_cursor1);

							if (ir_node_get_operation(node_cursor1)->type == IR_OPERATION_TYPE_IMM && ir_imm_operation_get_unsigned_value(ir_node_get_operation(node_cursor1)) == ir_imm_operation_get_unsigned_value(ir_node_get_operation(node_cursor2))){
								return 1;
							}
						}
					}
					else{
						return 1;
					}
				}
			}

			return 0;
		}
		else{
			printf("WARNING: in %s, found address operands which are (%s - %s)\n", __func__, irOpcode_2_string(op_ad1->operation_type.inst.opcode), irOpcode_2_string(op_ad2->operation_type.inst.opcode));
		}
	}
	else if (op_ad1->type == IR_OPERATION_TYPE_INST){
		if (op_ad1->operation_type.inst.opcode == IR_ADD){
			for (edge_cursor1 = node_get_head_edge_dst(addr1), nb_imm = 0, nb_oth = 0; edge_cursor1 != NULL; edge_cursor1 = edge_get_next_dst(edge_cursor1)){
				node_cursor1 = edge_get_src(edge_cursor1);

				if (ir_node_get_operation(node_cursor1)->type == IR_OPERATION_TYPE_IMM){
					if (nb_imm){
						return 1;
					}
					nb_imm ++;
					if (ir_imm_operation_get_unsigned_value(ir_node_get_operation(node_cursor1)) == 0){
						return 1;
					}
				}
				else{
					if (nb_oth){
						return 1;
					}
					nb_oth ++;
					if (node_cursor1 != addr2){
						return 1;
					}
				}
			}
			if (nb_imm && nb_oth){
				return 0;
			}
			else{
				return 1;
			}
		}
		else{
			printf("WARNING: in %s, found address operand which is %s\n", __func__, irOpcode_2_string(op_ad1->operation_type.inst.opcode));
		}
	}
	else if (op_ad2->type == IR_OPERATION_TYPE_INST){
		if (op_ad2->operation_type.inst.opcode == IR_ADD){
			for (edge_cursor2 = node_get_head_edge_dst(addr2), nb_imm = 0, nb_oth = 0; edge_cursor2 != NULL; edge_cursor2 = edge_get_next_dst(edge_cursor2)){
				node_cursor2 = edge_get_src(edge_cursor2);

				if (ir_node_get_operation(node_cursor2)->type == IR_OPERATION_TYPE_IMM){
					if (nb_imm){
						return 1;
					}
					nb_imm ++;
					if (ir_imm_operation_get_unsigned_value(ir_node_get_operation(node_cursor2)) == 0){
						return 1;
					}
				}
				else{
					if (nb_oth){
						return 1;
					}
					nb_oth ++;
					if (node_cursor2 != addr1){
						return 1;
					}
				}
			}
			if (nb_imm && nb_oth){
				return 0;
			}
			else{
				return 1;
			}
		}
		else{
			printf("WARNING: in %s, found address operand which is %s\n", __func__, irOpcode_2_string(op_ad2->operation_type.inst.opcode));
		}
	}

	return 1;
}

static int32_t ir_normalize_search_alias_conflict(struct node* node1, struct node* node2, enum irOperationType alias_type, enum aliasingStrategy strategy){
	struct node* 			node_cursor;
	struct irOperation* 	operation_cursor;
	struct edge* 			edge_cursor;
	struct node* 			addr1;
	struct node* 			addr_cursor;

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
				addr1 = NULL;
				addr_cursor = NULL;

				for (edge_cursor = node_get_head_edge_dst(node1); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
					if (ir_edge_get_dependence(edge_cursor)->type == IR_DEPENDENCE_TYPE_ADDRESS){
						addr1 = edge_get_src(edge_cursor);
						break;
					}
				}

				for (edge_cursor = node_get_head_edge_dst(node_cursor); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
					if (ir_edge_get_dependence(edge_cursor)->type == IR_DEPENDENCE_TYPE_ADDRESS){
						addr_cursor = edge_get_src(edge_cursor);
						break;
					}
				}

				if (addr1 != NULL && addr_cursor != NULL){
					if (ir_normalize_cannot_alias(addr1, addr_cursor)){
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
	struct node* 	node_cursor;
	struct edge* 	edge_cursor;
	uint32_t 		nb_mem_access;
	struct node** 	access_list = NULL;
	uint32_t 		access_list_alloc_size;
	uint32_t 		i;

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
}

#if IRMEMORY_ALIAS_HEURISTIC_ESP == 1

static uint32_t ir_normalize_addr_depend_on_esp(struct node* node, uint32_t dst){
	struct edge* 		edge_cursor;
	struct node* 		node_cursor;
	struct irOperation* operation_cursor;
	struct irOperation* operation;

	if (dst == 0){
		operation = ir_node_get_operation(node);
		if (operation->type == IR_OPERATION_TYPE_IN_REG && operation->operation_type.in_reg.reg == IR_REG_ESP){
			return 1;
		}
		else if (operation->type != IR_OPERATION_TYPE_INST){
			return 0;
		}
	}

	for (edge_cursor = node_get_head_edge_dst(node); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
		node_cursor = edge_get_src(edge_cursor);
		operation_cursor = ir_node_get_operation(node_cursor);

		if (operation_cursor->type == IR_OPERATION_TYPE_IN_REG && operation_cursor->operation_type.in_reg.reg == IR_REG_ESP){
			return 1;
		}
	}

	if (dst != IRMEMORY_MAX_BASE_SYM_DST){
		for (edge_cursor = node_get_head_edge_dst(node); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
			node_cursor = edge_get_src(edge_cursor);
			operation_cursor = ir_node_get_operation(node_cursor);

			if (operation_cursor->type == IR_OPERATION_TYPE_INST){
				if (ir_normalize_addr_depend_on_esp(node_cursor, dst + 1)){
					return 1;
				}
			}
		}
	}

	return 0;
}

#endif


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