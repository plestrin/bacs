#include <stdlib.h>
#include <stdio.h>

#include "irVariableSize.h"
#include "dagPartialOrder.h"
#include "base.h"

#define IDEAL_SIZE 32

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

static void irVariableSize_remove_size_convertor(struct ir* ir){
	struct irNodeIterator it;

	for (irNodeIterator_get_first(ir, &it); irNodeIterator_get_node(it) != NULL; irNodeIterator_get_next(ir, &it)){
		if (irNodeIterator_get_operation(it)->type != IR_OPERATION_TYPE_INST){
			continue;
		}

		switch (irNodeIterator_get_operation(it)->operation_type.inst.opcode){
			case IR_MOVZX 		: {
				struct node* operand;
				struct node* mask;

				operand = edge_get_src(node_get_head_edge_dst(irNodeIterator_get_node(it)));

				if (ir_node_get_operation(operand)->size == irNodeIterator_get_operation(it)->size){
					if ((uint32_t)(operand->ptr) != irNodeIterator_get_operation(it)->size){
						if ((mask = ir_insert_immediate(ir, irNodeIterator_get_node(it), irNodeIterator_get_operation(it)->size, bitmask64((uint32_t)(operand->ptr)))) != NULL){
							if (ir_add_dependence(ir, mask, irNodeIterator_get_node(it), IR_DEPENDENCE_TYPE_DIRECT)){
								irNodeIterator_get_operation(it)->operation_type.inst.opcode = IR_AND;
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

				operand = edge_get_src(node_get_head_edge_dst(irNodeIterator_get_node(it)));

				for (edge_cursor = node_get_head_edge_src(irNodeIterator_get_node(it)); edge_cursor != NULL; ){
					child = ir_node_get_operation(edge_get_dst(edge_cursor));

					if (child->size == ir_node_get_operation(operand)->size && !(child->type == IR_OPERATION_TYPE_INST && child->operation_type.inst.opcode == IR_MOVZX)){
						ir_add_dependence_check(ir, operand, edge_get_dst(edge_cursor), ir_edge_get_dependence(edge_cursor)->type)

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

				operand = edge_get_src(node_get_head_edge_dst(irNodeIterator_get_node(it)));

				for (edge_cursor = node_get_head_edge_src(irNodeIterator_get_node(it)); edge_cursor != NULL;){
					child = ir_node_get_operation(edge_get_dst(edge_cursor));

					if (child->size == ir_node_get_operation(operand)->size && !(child->type == IR_OPERATION_TYPE_INST && child->operation_type.inst.opcode == IR_MOVZX)){
						if (shift == NULL){
							if ((shift = ir_insert_inst(ir, irNodeIterator_get_node(it), IR_OPERATION_INDEX_UNKOWN, child->size, IR_SHR, IR_OPERATION_DST_UNKOWN)) != NULL){
								struct node* disp;

								if ((disp = ir_insert_immediate(ir, shift, child->size, 8)) != NULL){
									ir_add_dependence_check(ir, disp, shift, IR_DEPENDENCE_TYPE_SHIFT_DISP)
								}
								else{
									log_err("unable to add immediate to IR");
								}
								ir_add_dependence_check(ir, operand, shift, IR_DEPENDENCE_TYPE_DIRECT)
							}
							else{
								log_err("unable to add instruction to IR");
								break;
							}
						}

						ir_add_dependence_check(ir, shift, edge_get_dst(edge_cursor), ir_edge_get_dependence(edge_cursor)->type)

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
				if (operation_cursor->type == IR_OPERATION_TYPE_OUT_MEM && ir_edge_get_dependence(edge_current)->type == IR_DEPENDENCE_TYPE_ADDRESS && operand_operation->size == ADDRESS_SIZE){
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

				if (operation_cursor->type == IR_OPERATION_TYPE_INST && (operation_cursor->operation_type.inst.opcode == IR_DIVQ || operation_cursor->operation_type.inst.opcode == IR_DIVR || operation_cursor->operation_type.inst.opcode == IR_IDIVR || operation_cursor->operation_type.inst.opcode == IR_IDIVQ) && ir_edge_get_dependence(edge_current)->type == IR_DEPENDENCE_TYPE_DIRECT && (operand_operation->size / 2) == operation_cursor->size){
					continue;
				}

				/* Add instruction to convert size */
				if (operation_cursor->size > operand_operation->size){
					new_ins = ir_insert_inst(ir, node_cursor, IR_OPERATION_INDEX_UNKOWN, operation_cursor->size, IR_MOVZX, IR_OPERATION_DST_UNKOWN);
				}
				else if (operation_cursor->size == 8 && operand_operation->size == 32){
					new_ins = ir_insert_inst(ir, node_cursor, IR_OPERATION_INDEX_UNKOWN, 8, IR_PART1_8, IR_OPERATION_DST_UNKOWN);
				}
				else if (operation_cursor->size == 16 && operand_operation->size == 32){
					new_ins = ir_insert_inst(ir, node_cursor, IR_OPERATION_INDEX_UNKOWN, 16, IR_PART1_16, IR_OPERATION_DST_UNKOWN);
				}
				else{
					log_err_m("this case is not implemented, size mismatch %u -> %u", operand_operation->size, operation_cursor->size);
					continue;
				}

				if (new_ins == NULL){
					log_err("unable to add operation to IR");
				}
				else{
					ir_add_dependence_check(ir, operand_node, new_ins, IR_DEPENDENCE_TYPE_DIRECT)
					ir_add_dependence_check(ir, new_ins, node_cursor, ir_edge_get_dependence(edge_current)->type)
					ir_remove_dependence(ir, edge_current);
				}
			}
		}
	}
}

static const uint8_t expand[NB_IR_OPCODE] = {
	1, 	/* 0  IR_ADC 		*/
	1, 	/* 1  IR_ADD 		*/
	1, 	/* 2  IR_AND 		*/
	1, 	/* 3  IR_CMOV 		*/
	0, 	/* 4  IR_DIVQ 		*/
	0, 	/* 5  IR_DIVR 		*/
	0, 	/* 6  IR_IDIVQ 		*/
	0, 	/* 7  IR_IDIVR 		*/
	0, 	/* 8  IR_IMUL 		*/
	0, 	/* 9  IR_LEA 		- not important */
	0, 	/* 10 IR_MOV 		- not important */
	0, 	/* 11 IR_MOVZX 		*/
	0, 	/* 12 IR_MUL 		*/
	1, 	/* 13 IR_NEG 		*/
	1, 	/* 14 IR_NOT 		*/
	1, 	/* 15 IR_OR 		*/
	0, 	/* 16 IR_PART1_8 	*/
	0, 	/* 17 IR_PART2_8 	*/
	0, 	/* 18 IR_PART1_16 	*/
	0, 	/* 19 IR_ROL 		*/
	0, 	/* 20 IR_ROR 		*/
	1, 	/* 21 IR_SBB 		*/
	1, 	/* 22 IR_SHL 		*/
	0, 	/* 23 IR_SHLD 		*/
	0, 	/* 24 IR_SHR 		*/
	0, 	/* 25 IR_SHRD 		*/
	1, 	/* 26 IR_SUB 		*/
	1, 	/* 27 IR_XOR 		*/
	0, 	/* 28 IR_LOAD 		- not important */
	0, 	/* 29 IR_STORE 		- not important */
	0, 	/* 30 IR_JOKER 		- not important */
	0 	/* 31 IR_INVALID 	- not important */
};

int32_t irNormalize_expand_variable(struct ir* ir){
	struct irNodeIterator 	it;
	int32_t 				result;

	if (dagPartialOrder_sort_dst_src(&(ir->graph))){
		log_err("unable to sort DAG");
		return 0;
	}

	for (irNodeIterator_get_first(ir, &it), result = 0; irNodeIterator_get_node(it) != NULL; irNodeIterator_get_next(ir, &it)){
		irNodeIterator_get_node(it)->ptr = (void*)((uint32_t)(irNodeIterator_get_operation(it)->size));

		if (irNodeIterator_get_operation(it)->size < IDEAL_SIZE){
			switch (irNodeIterator_get_operation(it)->type){
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
					struct edge* current_edge_cursor;
					struct edge* next_edge_cursor;
					struct node* new_imm;

					for (current_edge_cursor = node_get_head_edge_src(irNodeIterator_get_node(it)); current_edge_cursor != NULL; current_edge_cursor = next_edge_cursor){
						next_edge_cursor = edge_get_next_src(current_edge_cursor);

						if (ir_edge_get_dependence(current_edge_cursor)->type == IR_DEPENDENCE_TYPE_DIRECT && ir_node_get_operation(edge_get_dst(current_edge_cursor))->size != (uint32_t)(edge_get_dst(current_edge_cursor)->ptr) && ir_node_get_operation(edge_get_dst(current_edge_cursor))->size == IDEAL_SIZE){
							if ((new_imm = ir_insert_immediate(ir, irNodeIterator_get_node(it), IDEAL_SIZE, ir_imm_operation_get_unsigned_value(irNodeIterator_get_operation(it)))) != NULL){
								ir_add_dependence_check(ir, new_imm, edge_get_dst(current_edge_cursor), IR_DEPENDENCE_TYPE_DIRECT)
								ir_remove_dependence(ir, current_edge_cursor);
							}
						}
					}
					break;
				}
				case IR_OPERATION_TYPE_INST 	: {
					if (expand[irNodeIterator_get_operation(it)->operation_type.inst.opcode]){
						irNodeIterator_get_operation(it)->size = IDEAL_SIZE;
						result = 1;
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

	if (result){
		irVariableSize_remove_size_convertor(ir);
		irVariableSize_add_size_convertor(ir);
	}

	#ifdef IR_FULL_CHECK
	ir_check_order(ir);
	#endif

	return result;
}
