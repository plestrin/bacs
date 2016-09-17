#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "irVariableSize.h"
#include "dagPartialOrder.h"
#include "base.h"

const uint8_t irRegisterSize[NB_IR_STD_REGISTER] = {
	32, 	/* IR_REG_EAX 		*/
	16, 	/* IR_REG_AX 		*/
	8, 		/* IR_REG_AH 		*/
	8, 		/* IR_REG_AL 		*/
	32, 	/* IR_REG_EBX 		*/
	16, 	/* IR_REG_BX 		*/
	8, 		/* IR_REG_BH 		*/
	8, 		/* IR_REG_BL 		*/
	32, 	/* IR_REG_ECX 		*/
	16, 	/* IR_REG_CX 		*/
	8, 		/* IR_REG_CH 		*/
	8, 		/* IR_REG_CL 		*/
	32, 	/* IR_REG_EDX 		*/
	16, 	/* IR_REG_DX 		*/
	8, 		/* IR_REG_DH 		*/
	8, 		/* IR_REG_DL 		*/
	32, 	/* IR_REG_ESP 		*/
	16, 	/* IR_REG_SP 		*/
	32, 	/* IR_REG_EBP 		*/
	16, 	/* IR_REG_BP 		*/
	32, 	/* IR_REG_ESI 		*/
	16, 	/* IR_REG_SI 		*/
	32, 	/* IR_REG_EDI 		*/
	16, 	/* IR_REG_DI 		*/
	0, 		/* IR_REG_TMP0 		*/
	0, 		/* IR_REG_TMP1 		*/
	0, 		/* IR_REG_TMP2 		*/
	0 		/* IR_REG_TMP3 		*/
};

static void irVariableSize_remove_size_convertor(struct ir* ir);
static void irVariableSize_add_size_convertor(struct ir* ir);

static void irVariableSize_remove_size_convertor(struct ir* ir){
	struct node* 		node_cursor;
	struct node* 		prev_node_cursor;
	struct irOperation* operation_cursor;

	for (node_cursor = graph_get_head_node(&(ir->graph)), prev_node_cursor = NULL; node_cursor != NULL;){
		operation_cursor = ir_node_get_operation(node_cursor);

		if (operation_cursor->type != IR_OPERATION_TYPE_INST){
			goto next;
		}

		switch (operation_cursor->operation_type.inst.opcode){
			case IR_MOVZX 		: {
				struct node* operand;
				struct node* mask;

				operand = edge_get_src(node_get_head_edge_dst(node_cursor));
				if (ir_node_get_operation(operand)->size == operation_cursor->size){
					if ((uint32_t)(operand->ptr) != operation_cursor->size){
						mask = ir_add_immediate(ir, operation_cursor->size, 0xffffffffffffffff >> (64 - (uint32_t)(operand->ptr)));
						if (mask){
							if (ir_add_dependence(ir, mask, node_cursor, IR_DEPENDENCE_TYPE_DIRECT)){
								operation_cursor->operation_type.inst.opcode = IR_AND;
							}
							else{
								log_err("unable to add dependency to IR");
							}
						}
						else{
							log_err("unable to add immediate to IR");
						}
					}
					else{
						log_err_m("trying to remove unnecessary MOVZX instruction but operand size was incorrect %p", (void*)ir_node_get_operation(operand));
					}
				}

				break;
			}
			case IR_PART1_8 	:
			case IR_PART1_16 	: {
				struct edge* 			edge_cursor;
				struct edge* 			tmp;
				struct irOperation* 	child;
				struct node* 			operand;

				operand = edge_get_src(node_get_head_edge_dst(node_cursor));

				for (edge_cursor = node_get_head_edge_src(node_cursor); edge_cursor != NULL; ){
					child = ir_node_get_operation(edge_get_dst(edge_cursor));

					if (child->size == ir_node_get_operation(operand)->size && !(child->type == IR_OPERATION_TYPE_INST && child->operation_type.inst.opcode == IR_MOVZX)){
						if (ir_add_dependence(ir, operand, edge_get_dst(edge_cursor), ir_edge_get_dependence(edge_cursor)->type) == NULL){
							log_err("unable to add dependency to IR");
						}

						tmp = edge_cursor;
						edge_cursor = edge_get_next_src(edge_cursor);
						ir_remove_dependence(ir, tmp);
					}
					else{
						edge_cursor = edge_get_next_src(edge_cursor);
					}
				}

				break;
			}
			case IR_PART2_8 	: {
				struct edge* 			edge_cursor;
				struct edge* 			tmp;
				struct irOperation* 	child;
				struct node* 			operand;
				struct node* 			shift = NULL;

				operand = edge_get_src(node_get_head_edge_dst(node_cursor));

				for (edge_cursor = node_get_head_edge_src(node_cursor); edge_cursor != NULL;){
					child = ir_node_get_operation(edge_get_dst(edge_cursor));

					if (child->size == ir_node_get_operation(operand)->size && !(child->type == IR_OPERATION_TYPE_INST && child->operation_type.inst.opcode == IR_MOVZX)){
						if (shift == NULL){
							shift = ir_add_inst(ir, IR_OPERATION_INDEX_UNKOWN, child->size, IR_SHR, IR_OPERATION_DST_UNKOWN);
							if (shift){
								struct node* disp;

								disp = ir_add_immediate(ir, child->size, 8);
								if (disp){
									if (ir_add_dependence(ir, disp, shift, IR_DEPENDENCE_TYPE_SHIFT_DISP) == NULL){
										log_err("unable to add dependency to IR");
									}
								}
								else{
									log_err("unable to add immediate to IR");
								}
								if (ir_add_dependence(ir, operand, shift, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
									log_err("unable to add dependency to IR");
								}
							}
							else{
								log_err("unable to add instruction to IR");
								break;
							}
						}

						if (ir_add_dependence(ir, shift, edge_get_dst(edge_cursor), ir_edge_get_dependence(edge_cursor)->type) == NULL){
							log_err("unable to add dependency to IR");
						}

						tmp = edge_cursor;
						edge_cursor = edge_get_next_src(edge_cursor);
						ir_remove_dependence(ir, tmp);
					}
					else{
						edge_cursor = edge_get_next_src(edge_cursor);
					}
				}

				break;
			}
			default 			: {
				break;
			}
		}

		next:
		if (prev_node_cursor != NULL){
			if (node_get_next(prev_node_cursor) != node_cursor){
				node_cursor = node_get_next(prev_node_cursor);
			}
			else{
				prev_node_cursor = node_cursor;
				node_cursor = node_get_next(node_cursor);
			}
		}
		else{
			if (graph_get_head_node(&(ir->graph)) != node_cursor){
				node_cursor = graph_get_head_node(&(ir->graph));
			}
			else{
				prev_node_cursor = node_cursor;
				node_cursor = node_get_next(node_cursor);
			}
		}
	}
}

static void irVariableSize_add_size_convertor(struct ir* ir){
	struct node* 		node_cursor;
	struct irOperation* operation_cursor;
	struct edge* 		edge_cursor;
	struct edge* 		edge_current;
	struct node* 		operand_node;
	struct irOperation* operand_operation;
	struct node* 		new_ins;

	for (node_cursor = graph_get_head_node(&(ir->graph)); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
		operation_cursor = ir_node_get_operation(node_cursor);

		if (operation_cursor->type != IR_OPERATION_TYPE_INST && operation_cursor->type != IR_OPERATION_TYPE_OUT_MEM){
			continue;
		}

		for (edge_cursor = node_get_head_edge_dst(node_cursor); edge_cursor != NULL;){
			edge_current = edge_cursor;
			edge_cursor = edge_get_next_dst(edge_cursor);

			operand_node = edge_get_src(edge_current);
			operand_operation = ir_node_get_operation(operand_node);

			if (operation_cursor->size != operand_operation->size){
				if (operation_cursor->type == IR_OPERATION_TYPE_OUT_MEM && ir_edge_get_dependence(edge_current)->type == IR_DEPENDENCE_TYPE_ADDRESS && operand_operation->size == 32){
					continue;
				}

				if (operation_cursor->type == IR_OPERATION_TYPE_INST && (operation_cursor->operation_type.inst.opcode == IR_SHL || operation_cursor->operation_type.inst.opcode == IR_SHR || operation_cursor->operation_type.inst.opcode == IR_ROR) && ir_edge_get_dependence(edge_current)->type == IR_DEPENDENCE_TYPE_SHIFT_DISP && operand_operation->size == 8){
					continue;
				}

				if (operation_cursor->type == IR_OPERATION_TYPE_INST && operation_cursor->operation_type.inst.opcode == IR_MOVZX && valid_operand_size_ins_movzx(operation_cursor, operand_operation)){
					continue;
				}

				if (operation_cursor->type == IR_OPERATION_TYPE_INST && (operation_cursor->operation_type.inst.opcode == IR_PART1_8 || operation_cursor->operation_type.inst.opcode == IR_PART2_8) && valid_operand_size_ins_partX_8(operation_cursor, operand_operation)){
					continue;
				}

				if (operation_cursor->type == IR_OPERATION_TYPE_INST && operation_cursor->operation_type.inst.opcode == IR_PART1_16 && valid_operand_size_ins_partX_16(operation_cursor, operand_operation)){
					continue;
				}

				if (operation_cursor->type == IR_OPERATION_TYPE_INST && (operation_cursor->operation_type.inst.opcode == IR_DIVQ || operation_cursor->operation_type.inst.opcode == IR_DIVR || operation_cursor->operation_type.inst.opcode == IR_IDIVR || operation_cursor->operation_type.inst.opcode == IR_IDIVR) && ir_edge_get_dependence(edge_current)->type == IR_DEPENDENCE_TYPE_DIRECT && (operand_operation->size / 2) == operation_cursor->size){
					continue;
				}

				/* Add instruction to convert size */
				if (operation_cursor->size > operand_operation->size){
					new_ins = ir_add_inst(ir, IR_OPERATION_INDEX_UNKOWN, operation_cursor->size, IR_MOVZX, IR_OPERATION_DST_UNKOWN);
				}
				else if (operation_cursor->size == 8 && operand_operation->size == 32){
					new_ins = ir_add_inst(ir, IR_OPERATION_INDEX_UNKOWN, 8, IR_PART1_8, IR_OPERATION_DST_UNKOWN);
				}
				else if (operation_cursor->size == 16 && operand_operation->size == 32){
					new_ins = ir_add_inst(ir, IR_OPERATION_INDEX_UNKOWN, 16, IR_PART1_16, IR_OPERATION_DST_UNKOWN);
				}
				else{
					log_err_m("this case is not implemented, size mismatch %u -> %u", operand_operation->size, operation_cursor->size);
					continue;
				}

				if (new_ins == NULL){
					log_err("unable to add operation to IR");
				}
				else{
					if (ir_add_dependence(ir, operand_node, new_ins, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
						log_err("unable to add dependency to IR");
					}

					if (ir_add_dependence(ir, new_ins, node_cursor, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
						log_err("unable to add dependency to IR");
					}

					ir_remove_dependence(ir, edge_current);
				}
			}
		}
	}
}

enum paddingStrategy{
	PADDING_OK,
	PADDING_COMPLAIN,
	PADDING_SECURE,
	PADDING_IGNORE
};

static const enum paddingStrategy insPaddingStrategy[NB_IR_OPCODE] = {
	PADDING_OK, 		/* 0  IR_ADC 		*/
	PADDING_OK, 		/* 1  IR_ADD 		*/
	PADDING_OK, 		/* 2  IR_AND 		*/
	PADDING_OK, 		/* 3  IR_CMOV 		*/
	PADDING_SECURE, 	/* 4  IR_DIVQ 		*/
	PADDING_SECURE, 	/* 5  IR_DIVR 		*/
	PADDING_SECURE, 	/* 6  IR_IDIVQ 		*/
	PADDING_SECURE, 	/* 7  IR_IDIVR 		*/
	PADDING_COMPLAIN, 	/* 8  IR_IMUL 		*/
	PADDING_IGNORE, 	/* 9  IR_LEA 		- not important */
	PADDING_IGNORE, 	/* 10 IR_MOV 		- not important */
	PADDING_IGNORE, 	/* 11 IR_MOVZX 		*/
	PADDING_COMPLAIN, 	/* 12 IR_MUL 		*/
	PADDING_COMPLAIN, 	/* 13 IR_NEG 		*/
	PADDING_OK, 		/* 14 IR_NOT 		*/
	PADDING_OK, 		/* 15 IR_OR 		*/
	PADDING_IGNORE, 	/* 16 IR_PART1_8 	*/
	PADDING_IGNORE, 	/* 17 IR_PART2_8 	*/
	PADDING_IGNORE, 	/* 18 IR_PART1_16 	*/
	PADDING_COMPLAIN, 	/* 19 IR_ROL 		*/
	PADDING_COMPLAIN, 	/* 20 IR_ROR 		*/
	PADDING_COMPLAIN, 	/* 21 IR_SBB 		*/
	PADDING_COMPLAIN, 	/* 22 IR_SHL 		*/
	PADDING_COMPLAIN, 	/* 23 IR_SHLD 		*/
	PADDING_COMPLAIN, 	/* 24 IR_SHR 		*/
	PADDING_COMPLAIN, 	/* 25 IR_SHRD 		*/
	PADDING_COMPLAIN, 	/* 26 IR_SUB 		*/
	PADDING_OK, 		/* 27 IR_XOR 		*/
	PADDING_IGNORE, 	/* 28 IR_LOAD 		- not important */
	PADDING_IGNORE, 	/* 29 IR_STORE 		- not important */
	PADDING_IGNORE, 	/* 30 IR_JOKER 		- not important */
	PADDING_IGNORE, 	/* 31 IR_INVALID 	- not important */
};

void ir_normalize_expand_variable(struct ir* ir, uint8_t* modification){
	struct node* 			node_cursor;
	struct irOperation* 	operation_cursor;
	uint8_t 				local_modification = 0;

	if (dagPartialOrder_sort_dst_src(&(ir->graph))){
		log_err("unable to sort DAG");
		return;
	}

	for (node_cursor = graph_get_head_node(&(ir->graph)); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
		operation_cursor = ir_node_get_operation(node_cursor);
		node_cursor->ptr = (void*)((uint32_t)(operation_cursor->size));

		if (operation_cursor->size < 32){
			switch (operation_cursor->type){
				case IR_OPERATION_TYPE_IN_REG 	: {
					break;
				}
				case IR_OPERATION_TYPE_IN_MEM 	: {
					break;
				}
				case IR_OPERATION_TYPE_OUT_MEM 	: {
					break;
				}
				case IR_OPERATION_TYPE_IMM 		: {
					struct edge* edge_cursor;

					for (edge_cursor = node_get_head_edge_src(node_cursor); edge_cursor != NULL; edge_cursor = edge_get_next_src(edge_cursor)){
						if (ir_node_get_operation(edge_get_dst(edge_cursor))->size == 32){
							operation_cursor->operation_type.imm.value &= 0xffffffffffffffff >> (64 - operation_cursor->size);
							operation_cursor->size = 32;
							
							local_modification = 1;
							break;
						}
					}
					break;
				}
				case IR_OPERATION_TYPE_INST 	: {
					switch (insPaddingStrategy[operation_cursor->operation_type.inst.opcode]){
						case PADDING_OK 		: {
							operation_cursor->size = 32;

							local_modification = 1;
							break;
						}
						case PADDING_COMPLAIN 	: {
							log_warn_m("%s of size: %u, this case is not implemented", irOpcode_2_string(operation_cursor->operation_type.inst.opcode), operation_cursor->size);
							break;
						}
						case PADDING_SECURE 	: {
							#define IRVARIABLESIZE_NB_OPERAND_MAX 16
							struct edge* 	edge_cursor;
							struct node* 	operand;
							struct node* 	mask;
							struct node* 	and_tab[IRVARIABLESIZE_NB_OPERAND_MAX];
							struct edge* 	edge_tab[IRVARIABLESIZE_NB_OPERAND_MAX];
							uint32_t 		i;
							uint32_t 		nb_edge_dst;

							if (node_cursor->nb_edge_dst > IRVARIABLESIZE_NB_OPERAND_MAX){
								log_err("number of operand exceed a static array upper bound");
							}
							else{
								for (edge_cursor = node_get_head_edge_dst(node_cursor), i = 0; edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor), i++){
									operand = edge_get_src(edge_cursor);

									edge_tab[i] = edge_cursor;

									mask = ir_insert_immediate(ir, node_cursor, 32, 0xffffffffffffffff >> (64 - ir_node_get_operation(node_cursor)->size));
									if (mask){
										and_tab[i] = ir_insert_inst(ir, node_cursor, IR_OPERATION_INDEX_UNKOWN, 32, IR_AND, IR_OPERATION_DST_UNKOWN);
										if (and_tab[i]){
											if (ir_add_dependence(ir, mask, and_tab[i], IR_DEPENDENCE_TYPE_DIRECT) == NULL){
												log_err("unable to add dependency to IR");
											}
											if (ir_add_dependence(ir, operand, and_tab[i], IR_DEPENDENCE_TYPE_DIRECT) == NULL){
												log_err("unable to add dependency to IR");
											}
										}
										else{
											log_err("unable to add instruction to IR");
										}
									}
									else{
										log_err("unable to add immediate to IR");
									}
								}

								nb_edge_dst = node_cursor->nb_edge_dst;

								for (i = 0; i < nb_edge_dst; i++){
									if (ir_add_dependence(ir, and_tab[i], node_cursor,ir_edge_get_dependence(edge_tab[i])->type) == NULL){
										log_err("unable to add dependency to IR");
									}
									ir_remove_dependence(ir, edge_tab[i]);
								}

								operation_cursor->size = 32;

								local_modification = 1;
							}
							break;
						}
						case PADDING_IGNORE 	: {
							break;
						}
					}
					break;
				}
				case IR_OPERATION_TYPE_SYMBOL 	: {
					break;
				}
				case IR_OPERATION_TYPE_NULL 	: {
					break;
				}
			}
		}
	}

	if (local_modification){
		irVariableSize_remove_size_convertor(ir);
		irVariableSize_add_size_convertor(ir);

		*modification = 1;
	}
}
