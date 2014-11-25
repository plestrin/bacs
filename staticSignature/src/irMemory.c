#include <stdlib.h>
#include <stdio.h>

#include "irMemory.h"

int32_t compare_order_memoryNode(const void* arg1, const void* arg2);

static enum irOpcode ir_normalize_choose_part_opcode(uint8_t size_src, uint8_t size_dst){
	if (size_src == 32 && size_dst == 8){
		return IR_PART1_8;
	}
	else{
		printf("ERROR: in %s, this case is not implemented (src=%u, dst=%u)\n", __func__, size_src, size_dst);
		return IR_PART1_8;
	}
}

void ir_normalize_simplify_memory_access(struct ir* ir, uint8_t* modification){
	struct node* 	node_cursor;
	struct edge* 	edge_cursor;
	uint32_t 		nb_mem_access;
	struct node** 	access_list;
	uint32_t 		i;

	for(node_cursor = graph_get_head_node(&(ir->graph)); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
		if (node_cursor->nb_edge_src > 1){
			for (edge_cursor = node_get_head_edge_src(node_cursor), nb_mem_access = 0; edge_cursor != NULL; edge_cursor = edge_get_next_src(edge_cursor)){
				if (ir_edge_get_dependence(edge_cursor)->type == IR_DEPENDENCE_TYPE_ADDRESS && (ir_node_get_operation(edge_get_dst(edge_cursor))->type == IR_OPERATION_TYPE_IN_MEM || ir_node_get_operation(edge_get_dst(edge_cursor))->type == IR_OPERATION_TYPE_OUT_MEM)){
					nb_mem_access ++;
				}
			}
			if (nb_mem_access > 1){
				access_list = (struct node**)malloc(sizeof(struct node*) * nb_mem_access);
				if (access_list == NULL){
					printf("ERROR: in %s, unable to allocate memory\n", __func__);
					continue;
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

						for (edge_cursor = node_get_head_edge_dst(access_list[i - 1]); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
							if (ir_edge_get_dependence(edge_cursor)->type != IR_DEPENDENCE_TYPE_ADDRESS){
								stored_value = edge_get_src(edge_cursor);
							}
						}

						if (stored_value != NULL){
							if (operation_prev->size > operation_next->size){
								ir_convert_node_to_inst(access_list[i], ir_normalize_choose_part_opcode(operation_prev->size, operation_next->size), operation_next->size)
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
						if (operation_prev->size > operation_next->size){
							ir_convert_node_to_inst(access_list[i], ir_normalize_choose_part_opcode(operation_prev->size, operation_next->size), operation_next->size)
							graph_remove_edge(&(ir->graph), graph_get_edge(node_cursor, access_list[i]));

							if (ir_add_dependence(ir, access_list[i - 1], access_list[i], IR_DEPENDENCE_TYPE_DIRECT) == NULL){
								printf("ERROR: in %s, unable to add new dependency to IR\n", __func__);
							}

							access_list[i] = access_list[i - 1];
						}
						else if (operation_prev->size < operation_next->size){
							ir_convert_node_to_inst(access_list[i - 1], ir_normalize_choose_part_opcode(operation_next->size, operation_prev->size), operation_prev->size)
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
						if (operation_prev->size > operation_next->size){
							printf("WARNING: in %s, simplification of memory access of different size (case STORE (%u bits) -> STORE (%u bits))\n", __func__, operation_prev->size, operation_next->size);
							continue;
						}

						ir_remove_node(ir, access_list[i - 1]);
						*modification = 1;
						continue;
					}
				}

				free(access_list);
			}
		}
	}
}

int32_t compare_order_memoryNode(const void* arg1, const void* arg2){
	struct node* access1 = *(struct node**)arg1;
	struct node* access2 = *(struct node**)arg2;
	struct irOperation* op1 = ir_node_get_operation(access1);
	struct irOperation* op2 = ir_node_get_operation(access2);
	uint32_t order1;
	uint32_t order2;

	if (op1->type == IR_OPERATION_TYPE_IN_MEM){
		order1 = op1->operation_type.in_mem.order;
	}
	else{
		order1 = op1->operation_type.out_mem.order;	
	}

	if (op2->type == IR_OPERATION_TYPE_IN_MEM){
		order2 = op2->operation_type.in_mem.order;
	}
	else{
		order2 = op2->operation_type.out_mem.order;	
	}

	if (order1 < order2){
		return -1;
	}
	else if (order1 > order2){
		return 1;
	}
	else{
		return 0;
	}
}