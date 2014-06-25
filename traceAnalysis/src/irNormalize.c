#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "irNormalize.h"

#define IR_NORMALIZE_TRANSLATE_ROL_IMM 		1	/* IR must be obtained either by TRACE or ASM */
#define IR_NORMALIZE_TRANSLATE_SUB_IMM 		1 	/* IR must be obtained either by TRACE or ASM */
#define IR_NORMALIZE_REPLACE_XOR_FF 		1 	/* IR must be obtained by ASM */
#define IR_NORMALIZE_MERGE_TRANSITIVE_ADD 	1 	/* IR must be obtained either by TRACE or ASM */
#define IR_NORMALIZE_PROPAGATE_EXPRESSION 	1 	/* IR must be obtained either by TRACE or ASM */

void ir_normalize(struct ir* ir){
	#if IR_NORMALIZE_TRANSLATE_ROL_IMM == 1
	ir_normalize_translate_rol_imm(ir);
	#endif
	#if IR_NORMALIZE_TRANSLATE_SUB_IMM == 1
	ir_normalize_translate_sub_imm(ir);
	#endif
	#if IR_NORMALIZE_REPLACE_XOR_FF == 1
	ir_normalize_replace_xor_ff(ir);
	#endif
	#if IR_NORMALIZE_MERGE_TRANSITIVE_ADD == 1
	ir_normalize_merge_transitive_add(ir);
	#endif
	#if IR_NORMALIZE_PROPAGATE_EXPRESSION == 1
	ir_normalize_propagate_expression(ir);
	#endif
}

/* WARNING this routine might be incorrect (see below) */
void ir_normalize_translate_rol_imm(struct ir* ir){
	struct node* 			node_cursor;
	struct edge* 			edge_cursor;
	struct irOperation* 	operation;
	struct irOperation* 	imm_value;
	
	for(node_cursor = graph_get_head_node(&(ir->graph)); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
		operation = ir_node_get_operation(node_cursor);
		if (operation->type == IR_OPERATION_TYPE_OUTPUT){
			if (operation->operation_type.output.opcode == IR_ROL){
				operation->operation_type.output.opcode = IR_ROR;
			}
			else{
				continue;
			}
		}
		else if (operation->type == IR_OPERATION_TYPE_INNER){
			if (operation->operation_type.inner.opcode == IR_ROL){
				operation->operation_type.inner.opcode = IR_ROR;
			}
			else{
				continue;
			}
		}
		else{
			continue;
		}
		
		switch(node_cursor->nb_edge_dst){
			case 1 : {
				break;
			}
			case 2 : {
				/* WARNING this is not perfect since we don't know the operand size */
				edge_cursor = node_get_head_edge_dst(node_cursor);
				while(edge_cursor != NULL && ir_node_get_operation(edge_get_src(edge_cursor))->type != IR_OPERATION_TYPE_IMM){
					edge_cursor = edge_get_next_dst(edge_cursor);
				}
				if (edge_cursor != NULL){
					imm_value = ir_node_get_operation(edge_get_src(edge_cursor));
					imm_value->operation_type.imm.value = 32 - imm_value->operation_type.imm.value;
				}
				else{
					printf("WARNING: in %s, this case (ROL with 2 operands but no IMM) is not supposed to happen\n", __func__);
				}
				break;
			}
			default : {
				printf("WARNING: in %s, this case (ROL with %u operand(s)) is not supposed to happen\n", __func__, node_cursor->nb_edge_dst);
				break;
			}
		}
	}
}

/* WARNING this routine might be incorrect (see below) */
void ir_normalize_translate_sub_imm(struct ir* ir){
	struct node* 			node_cursor;
	struct edge* 			edge_cursor;
	struct irOperation* 	operation;
	struct irOperation* 	imm_value;
	enum irOpcode* 			opcode_ptr;
	
	for(node_cursor = graph_get_head_node(&(ir->graph)); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
		operation = ir_node_get_operation(node_cursor);
		if (operation->type == IR_OPERATION_TYPE_OUTPUT){
			if (operation->operation_type.output.opcode == IR_SUB){
				opcode_ptr = &(operation->operation_type.output.opcode);
			}
			else{
				continue;
			}
		}
		else if (operation->type == IR_OPERATION_TYPE_INNER){
			if (operation->operation_type.inner.opcode == IR_SUB){
				opcode_ptr = &(operation->operation_type.inner.opcode);
			}
			else{
				continue;
			}
		}
		else{
			continue;
		}
		
		switch(node_cursor->nb_edge_dst){
			case 1 : {
				*opcode_ptr = IR_ADD;
				break;
			}
			case 2 : {
				edge_cursor = node_get_head_edge_dst(node_cursor);
				while(edge_cursor != NULL && ir_node_get_operation(edge_get_src(edge_cursor))->type != IR_OPERATION_TYPE_IMM){
					edge_cursor = edge_get_next_dst(edge_cursor);
				}
				if (edge_cursor != NULL){
					imm_value = ir_node_get_operation(edge_get_src(edge_cursor));
					imm_value->operation_type.imm.value = (uint64_t)(-imm_value->operation_type.imm.value); /* I am not sure if this is correct */
					*opcode_ptr = IR_SUB;
				}
				break;
			}
			default : {
				printf("WARNING: in %s, this case (SUB with %u operand(s)) is not supposed to happen\n", __func__, node_cursor->nb_edge_dst);
				break;
			}
		}
	}
}

/* WARNING this routine might be incorrect (see below) */
void ir_normalize_replace_xor_ff(struct ir* ir){
	struct node* 			node_cursor;
	struct edge* 			edge_cursor;
	struct irOperation* 	operation;
	enum irOpcode* 			opcode_ptr;

	for(node_cursor = graph_get_head_node(&(ir->graph)); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
		if (node_cursor->nb_edge_dst == 2){
			operation = ir_node_get_operation(node_cursor);
			if (operation->type == IR_OPERATION_TYPE_OUTPUT){
				if (operation->operation_type.output.opcode != IR_XOR){
					continue;
				}
				else{
					opcode_ptr = &(operation->operation_type.output.opcode);
				}
			}
			else if (operation->type == IR_OPERATION_TYPE_INNER){
				if (operation->operation_type.inner.opcode != IR_XOR){
					continue;
				}
				else{
					opcode_ptr = &(operation->operation_type.inner.opcode);
				}
			}
			else{
				continue;
			}

			for(edge_cursor = node_get_head_edge_dst(node_cursor); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
				operation = ir_node_get_operation(edge_get_src(edge_cursor));
				if (operation->type == IR_OPERATION_TYPE_IMM){
					/* This is not correct the size of the XOR must be known (signed unsigned stuff ?) */
					#pragma GCC diagnostic ignored "-Wlong-long" /* use of C99 long long integer constant */
					if (ir_imm_operation_get_unsigned_value(operation) == 0xffffffff){
						if (edge_get_src(edge_cursor)->nb_edge_src == 1){
							ir_remove_node(ir, edge_get_src(edge_cursor));
						}
						*opcode_ptr = IR_NOT;
						break;
					}
				}
			}
		}
	}
}

void ir_normalize_merge_transitive_add(struct ir* ir){
	struct node* 			node_cursor1;
	struct node* 			node_cursor2;
	struct edge* 			edge_cursor2;
	struct irOperation* 	operation;

	for(node_cursor1 = graph_get_head_node(&(ir->graph)); node_cursor1 != NULL; node_cursor1 = node_get_next(node_cursor1)){
		operation = ir_node_get_operation(node_cursor1);
		if (operation->type == IR_OPERATION_TYPE_OUTPUT){
			if (operation->operation_type.output.opcode != IR_ADD){
				continue;
			}
		}
		else if (operation->type == IR_OPERATION_TYPE_INNER){
			if (operation->operation_type.inner.opcode != IR_ADD){
				continue;
			}
		}
		else{
			continue;
		}

		start:
		for(edge_cursor2 = node_get_head_edge_dst(node_cursor1); edge_cursor2 != NULL; edge_cursor2 = edge_get_next_dst(edge_cursor2)){
			node_cursor2  = edge_get_src(edge_cursor2);
			if (node_cursor2->nb_edge_src == 1){
				operation = ir_node_get_operation(node_cursor2);
				if (operation->type == IR_OPERATION_TYPE_OUTPUT){
					if (operation->operation_type.output.opcode != IR_ADD){
						continue;
					}
					else{
						if (operation->operation_type.output.prev == NULL){
							ir->output_linkedList = operation->operation_type.output.next;
						}
						else{
							ir_node_get_operation(operation->operation_type.output.prev)->operation_type.output.next = operation->operation_type.output.next;
						}

						if (operation->operation_type.output.next != NULL){
							ir_node_get_operation(operation->operation_type.output.next)->operation_type.output.prev = operation->operation_type.output.prev;
						}
					}
				}
				else if (operation->type == IR_OPERATION_TYPE_INNER){
					if (operation->operation_type.inner.opcode != IR_ADD){
						continue;
					}
				}
				else{
					continue;
				}

				graph_merge_node(&(ir->graph), node_cursor1, node_cursor2);
				ir_remove_node(ir, node_cursor2);
				goto start;
			}
		}
	}
}

void ir_normalize_propagate_expression(struct ir* ir){
	struct node* 			node_cursor1;
	struct node* 			node_cursor2;
	struct edge* 			edge_cursor1;
	struct edge* 			edge_cursor2;
	struct irOperation* 	operation1;
	struct irOperation* 	operation2;
	uint32_t 				i;
	uint8_t* 				taken;
	uint32_t 				max_nb_edge_dst = 0;
	uint32_t 				nb_match;
	enum irOpcode* 			opcode_ptr1;
	enum irOpcode* 			opcode_ptr2;

	for(node_cursor1 = graph_get_head_node(&(ir->graph)); node_cursor1 != NULL; node_cursor1 = node_get_next(node_cursor1)){
		if (node_cursor1->nb_edge_dst > max_nb_edge_dst){
			max_nb_edge_dst = node_cursor1->nb_edge_dst;
		}
	}

	if (max_nb_edge_dst == 0){
		return;
	}

	taken = (uint8_t*)alloca(sizeof(uint8_t) * max_nb_edge_dst);

	start:
	for(node_cursor1 = graph_get_head_node(&(ir->graph)); node_cursor1 != NULL; node_cursor1 = node_get_next(node_cursor1)){
		operation1 = ir_node_get_operation(node_cursor1);
		if (operation1->type == IR_OPERATION_TYPE_OUTPUT || operation1->type == IR_OPERATION_TYPE_INNER){

			if (operation1->type == IR_OPERATION_TYPE_OUTPUT){
				opcode_ptr1 = &(operation1->operation_type.output.opcode);
			}
			else{
				opcode_ptr1 = &(operation1->operation_type.inner.opcode);
			}

			for(node_cursor2 = node_get_next(node_cursor1); node_cursor2 != NULL; node_cursor2 = node_get_next(node_cursor2)){
				operation2 = ir_node_get_operation(node_cursor2);
				if (operation2->type == IR_OPERATION_TYPE_OUTPUT || operation2->type == IR_OPERATION_TYPE_INNER){

					if (operation2->type == IR_OPERATION_TYPE_OUTPUT){
						opcode_ptr2 = &(operation2->operation_type.output.opcode);
					}
					else{
						opcode_ptr2 = &(operation2->operation_type.inner.opcode);
					}

					if (*opcode_ptr1 == *opcode_ptr2 && node_cursor1->nb_edge_dst == node_cursor2->nb_edge_dst){
						memset(taken, 0, sizeof(uint8_t) * node_cursor1->nb_edge_dst);

						for (edge_cursor1 = node_get_head_edge_dst(node_cursor1); edge_cursor1 != NULL; edge_cursor1 = edge_get_next_dst(edge_cursor1)){
							for (edge_cursor2 = node_get_head_edge_dst(node_cursor2), i = 0; edge_cursor2 != NULL; edge_cursor2 = edge_get_next_dst(edge_cursor2), i++){
								if (!taken[i]){
									if (edge_get_src(edge_cursor1) == edge_get_src(edge_cursor2)){
										taken[i] = 1;
										break;
									}
									else if (ir_node_get_operation(edge_get_src(edge_cursor1))->type == IR_OPERATION_TYPE_IMM && ir_node_get_operation(edge_get_src(edge_cursor2))->type == IR_OPERATION_TYPE_IMM){
										struct irOperation* operation11;
										struct irOperation* operation22;

										operation11 = ir_node_get_operation(edge_get_src(edge_cursor1));
										operation22 = ir_node_get_operation(edge_get_src(edge_cursor2));

										if (operation11->operation_type.imm.width == operation22->operation_type.imm.width && operation11->operation_type.imm.signe == operation22->operation_type.imm.signe && operation11->operation_type.imm.value == operation22->operation_type.imm.value){
											taken[i] = 1;
											break;
										}
									}
								}
							}

							if (edge_cursor2 == NULL){
								break;
							}
						}

						for (i = 0, nb_match = 0; i < node_cursor1->nb_edge_dst; i++){
							nb_match += taken[i];
						}

						if (nb_match == node_cursor1->nb_edge_dst){
							if (operation1->type == IR_OPERATION_TYPE_OUTPUT){
								graph_transfert_src_edge(&(ir->graph), node_cursor1, node_cursor2);
								ir_remove_node(ir, node_cursor2);
							}
							else{
								graph_transfert_src_edge(&(ir->graph), node_cursor2, node_cursor1);
								ir_remove_node(ir, node_cursor1);
							}
							goto start;
						}
					}
				}
			}
		}
	}
}