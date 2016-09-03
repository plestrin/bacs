#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "irVariableRange.h"
#include "base.h"

#define irOperation_init_top(operation) variableRange_init_top_(&((operation)->operation_type.inst.range), (operation)->size)
#define irOperation_shrink(operation) variableRange_mod_value(&((operation)->operation_type.inst.range), (operation)->size);

static void irVariableRange_apply_all_operand(struct node* node, void(*func)(struct variableRange*,const struct variableRange*,uint32_t), uint32_t seed){
	uint32_t 				i;
	struct edge* 			edge_cursor;
	struct node* 			node_cursor;
	struct variableRange 	range;
	struct variableRange* 	ptr;
	struct irOperation* 	operation;

	operation = ir_node_get_operation(node);

	for (edge_cursor = node_get_head_edge_dst(node), i = 0; edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor), i++){
		node_cursor = edge_get_src(edge_cursor);
		if ((ptr = ir_operation_get_range(ir_node_get_operation(node_cursor))) == NULL){
			ptr = &range;
		}

		irVariableRange_compute(node_cursor, ptr, seed);
		if (i){
			func(&(operation->operation_type.inst.range), ptr, operation->size);
		}
		else{
			memcpy(&(operation->operation_type.inst.range), ptr, sizeof(struct variableRange));
			irOperation_shrink(operation);
		}
	}
}

static void irVariableRange_apply_shift(struct node* node, void(*func)(struct variableRange*,const struct variableRange*,uint32_t), uint32_t seed){
	struct node* 			node_operand1 = NULL;
	struct node* 			node_operand2 = NULL;
	struct variableRange 	range;
	struct variableRange* 	ptr;
	struct irOperation* 	operation;
	struct edge* 			edge_cursor;

	operation = ir_node_get_operation(node);

	for (edge_cursor = node_get_head_edge_dst(node); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
		if (ir_edge_get_dependence(edge_cursor)->type == IR_DEPENDENCE_TYPE_DIRECT){
			node_operand1 = edge_get_src(edge_cursor);
		}
		else if (ir_edge_get_dependence(edge_cursor)->type == IR_DEPENDENCE_TYPE_SHIFT_DISP){
			node_operand2 = edge_get_src(edge_cursor);
		}
	}

	if (node_operand1 == NULL || node_operand2 == NULL){
		log_err("incorrect instruction format, run check");
		irOperation_init_top(operation);
		return;
	}

	if ((ptr = ir_operation_get_range(ir_node_get_operation(node_operand1))) == NULL){
		ptr = &range;
	}

	irVariableRange_compute(node_operand1, ptr, seed);
	memcpy(&(operation->operation_type.inst.range), ptr, sizeof(struct variableRange));
	irOperation_shrink(operation);

	if ((ptr = ir_operation_get_range(ir_node_get_operation(node_operand2))) == NULL){
		ptr = &range;
	}

	irVariableRange_compute(node_operand2, ptr, seed);
	func(&(operation->operation_type.inst.range), ptr, operation->size);
}

void irVariableRange_compute(struct node* node, struct variableRange* range_dst, uint32_t seed){
	struct irOperation* operation;

	operation = ir_node_get_operation(node);
	switch(operation->type){
		case IR_OPERATION_TYPE_IN_REG 	: {
			variableRange_init_top_(range_dst, operation->size);
			break;
		}
		case IR_OPERATION_TYPE_IN_MEM 	: {
			variableRange_init_top_(range_dst, operation->size);
			break;
		}
		case IR_OPERATION_TYPE_OUT_MEM 	: {
			break;
		}
		case IR_OPERATION_TYPE_IMM 		: {
			variableRange_init_cst_(range_dst, ir_imm_operation_get_unsigned_value(operation), operation->size);
			break;
		}
		case IR_OPERATION_TYPE_INST 	: {
			if (seed == operation->operation_type.inst.seed){
				return;
			}

			switch(operation->operation_type.inst.opcode){
				case IR_ADD 	: {
					irVariableRange_apply_all_operand(node, variableRange_add_range, seed);
					break;
				}
				case IR_AND 	: {
					irVariableRange_apply_all_operand(node, variableRange_and_range, seed);
					break;
				}
				case IR_MOVZX 	: {
					variableRange_init_top_(&(operation->operation_type.inst.range), ir_node_get_operation(edge_get_src(node_get_head_edge_dst(node)))->size);
					break;
				}
				case IR_OR	: {
					irVariableRange_apply_all_operand(node, variableRange_or_range, seed);
					break;
				}
				case IR_SHL 	: {
					irVariableRange_apply_shift(node, variableRange_shl_range, seed);
					break;
				}
				case IR_SHR 	: {
					irVariableRange_apply_shift(node, variableRange_shr_range, seed);
					break;
				}
				default 		: {
					variableRange_init_top_(range_dst, operation->size);
					break;
				}
			}

			operation->operation_type.inst.seed = seed;

			break;
		}
		case IR_OPERATION_TYPE_SYMBOL 	: {
			variableRange_init_top_(range_dst, 32);
			break;
		}
		case IR_OPERATION_TYPE_NULL 	: {
			variableRange_init_top_(range_dst, operation->size);
			break;
		}
	}
}

void irVariableRange_get_range_add_buffer(struct variableRange* dst_range, struct node** node_buffer, uint32_t nb_node, uint32_t size, uint32_t seed){
	uint32_t 				i;
	struct variableRange 	range;
	struct variableRange* 	ptr;

	variableRange_init_cst_(dst_range, 0, size);
	for (i = 0; i < nb_node; i++){
		ptr = ir_operation_get_range(ir_node_get_operation(node_buffer[i]));
		if (ptr == NULL){
			ptr = &range;
		}

		irVariableRange_compute(node_buffer[i], ptr, seed);
		variableRange_add_range(dst_range, ptr, size);
	}
}

void irVariableRange_get_range_and_buffer(struct variableRange* dst_range, struct node** node_buffer, uint32_t nb_node, uint32_t size, uint32_t seed){
	uint32_t 				i;
	struct variableRange 	range;
	struct variableRange* 	ptr;

	variableRange_init_cst_(dst_range, 0xffffffffffffffff, size);
	for (i = 0; i < nb_node; i++){
		ptr = ir_operation_get_range(ir_node_get_operation(node_buffer[i]));
		if (ptr == NULL){
			ptr = &range;
		}
		
		irVariableRange_compute(node_buffer[i], ptr, seed);
		variableRange_and_range(dst_range, ptr, size);
	}
}
