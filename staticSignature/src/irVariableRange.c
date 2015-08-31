#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "irVariableRange.h"
#include "base.h"

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
		if (i == 0){
			ptr = ir_operation_get_range(ir_node_get_operation(node_cursor));
			if (ptr == NULL){
				irVariableRange_compute(node_cursor, &(operation->operation_type.inst.range), seed);
			}
			else{
				irVariableRange_compute(node_cursor, ptr, seed);
				memcpy(&(operation->operation_type.inst.range), ptr, sizeof(struct variableRange));
			}

			operation->operation_type.inst.range.size_mask = variableRange_get_size_mask(operation->size);
			variableRange_pack(&(operation->operation_type.inst.range));
		}
		else{
			ptr = ir_operation_get_range(ir_node_get_operation(node_cursor));
			if (ptr == NULL){
				ptr = &range;
			}

			irVariableRange_compute(node_cursor, ptr, seed);
			func(&(operation->operation_type.inst.range), ptr, operation->size);
		}
	}
}

static void irVariableRange_apply_shift(struct node* node, void(*func)(struct variableRange*,const struct variableRange*,uint32_t), uint32_t seed){
	struct edge* 			edge_cursor;
	struct node* 			node_cursor;
	struct variableRange 	range;
	struct variableRange* 	ptr;
	struct irOperation* 	operation;

	operation = ir_node_get_operation(node);

	edge_cursor = node_get_head_edge_dst(node);
	if (edge_cursor == NULL){
		log_err("incorrect instruction format, run check");
		variableRange_init_size(&(operation->operation_type.inst.range), operation->size);
		return;
	}
	node_cursor = edge_get_src(edge_cursor);

	if (ir_edge_get_dependence(edge_cursor)->type == IR_DEPENDENCE_TYPE_DIRECT){
		ptr = ir_operation_get_range(ir_node_get_operation(node_cursor));
		if (ptr == NULL){
			irVariableRange_compute(node_cursor, &(operation->operation_type.inst.range), seed);
		}
		else{
			irVariableRange_compute(node_cursor, ptr, seed);
			memcpy(&(operation->operation_type.inst.range), ptr, sizeof(struct variableRange));
		}

		operation->operation_type.inst.range.size_mask = variableRange_get_size_mask(operation->size);
		variableRange_pack(&(operation->operation_type.inst.range));

		edge_cursor = edge_get_next_dst(edge_cursor);
		node_cursor = edge_get_src(edge_cursor);

		ptr = ir_operation_get_range(ir_node_get_operation(node_cursor));
		if (ptr == NULL){
			ptr = &range;
		}

		irVariableRange_compute(node_cursor, ptr, seed);
		func(&(operation->operation_type.inst.range), ptr, operation->size);
	}
	else{
		edge_cursor = edge_get_next_dst(edge_cursor);
		if (edge_cursor == NULL){
			log_err("incorrect instruction format, run check");
			variableRange_init_size(&(operation->operation_type.inst.range), operation->size);
			return;
		}
		node_cursor = edge_get_src(edge_cursor);

		ptr = ir_operation_get_range(ir_node_get_operation(node_cursor));
		if (ptr == NULL){
			irVariableRange_compute(node_cursor, &(operation->operation_type.inst.range), seed);
		}
		else{
			irVariableRange_compute(node_cursor, ptr, seed);
			memcpy(&(operation->operation_type.inst.range), ptr, sizeof(struct variableRange));
		}

		operation->operation_type.inst.range.size_mask = variableRange_get_size_mask(operation->size);
		variableRange_pack(&(operation->operation_type.inst.range));

		edge_cursor = edge_get_prev_dst(edge_cursor);
		node_cursor = edge_get_src(edge_cursor);

		ptr = ir_operation_get_range(ir_node_get_operation(node_cursor));
		if (ptr == NULL){
			ptr = &range;
		}

		irVariableRange_compute(node_cursor, ptr, seed);
		func(&(operation->operation_type.inst.range), ptr, operation->size);
	}

}

void irVariableRange_compute(struct node* node, struct variableRange* range_dst, uint32_t seed){
	struct irOperation* operation;

	operation = ir_node_get_operation(node);
	switch(operation->type){
		case IR_OPERATION_TYPE_IN_REG 	: {
			variableRange_init_size(range_dst, operation->size);
			break;
		}
		case IR_OPERATION_TYPE_IN_MEM 	: {
			variableRange_init_size(range_dst, operation->size);
			break;
		}
		case IR_OPERATION_TYPE_OUT_MEM 	: {
			break;
		}
		case IR_OPERATION_TYPE_IMM 		: {
			variableRange_init_cst(range_dst, ir_imm_operation_get_unsigned_value(operation), operation->size);
			break;
		}
		case IR_OPERATION_TYPE_INST 	: {
			if (seed == operation->operation_type.inst.seed){
				return;
			}

			switch(operation->operation_type.inst.opcode){
				case IR_ADD 	: {
					irVariableRange_apply_all_operand(node, variableRange_add, seed);
					break;
				}
				case IR_AND 	: {
					irVariableRange_apply_all_operand(node, variableRange_and, seed);
					break;
				}
				case IR_MOVZX 	: {
					variableRange_init_size(&(operation->operation_type.inst.range), ir_node_get_operation(edge_get_src(node_get_head_edge_dst(node)))->size);
					break;
				}
				case IR_OR	: {
					irVariableRange_apply_all_operand(node, variableRange_bitwise_heuristic, seed);
					break;
				}
				case IR_SHL 	: {
					irVariableRange_apply_shift(node, variableRange_shl, seed);
					break;
				}
				case IR_SHR 	: {
					irVariableRange_apply_shift(node, variableRange_shr, seed);
					break;
				}
				case IR_XOR	: {
					irVariableRange_apply_all_operand(node, variableRange_bitwise_heuristic, seed);
					break;
				}
				default 		: {
					variableRange_init_size(range_dst, operation->size);
					break;
				}
			}

			operation->operation_type.inst.seed = seed;

			break;
		}
		case IR_OPERATION_TYPE_SYMBOL 	: {
			variableRange_init_size(range_dst, 64);
			break;
		}
	}
}

void irVariableRange_get_range_add_buffer(struct variableRange* dst_range, struct node** node_buffer, uint32_t nb_node, uint32_t size, uint32_t seed){
	uint32_t 				i;
	struct variableRange 	range;
	struct variableRange* 	ptr;

	variableRange_init_cst(dst_range, 0, size);
	for (i = 0; i < nb_node; i++){
		ptr = ir_operation_get_range(ir_node_get_operation(node_buffer[i]));
		if (ptr == NULL){
			ptr = &range;
		}

		irVariableRange_compute(node_buffer[i], ptr, seed);
		variableRange_add(dst_range, ptr, size);
	}
}

void irVariableRange_get_range_and_buffer(struct variableRange* dst_range, struct node** node_buffer, uint32_t nb_node, uint32_t size, uint32_t seed){
	uint32_t 				i;
	struct variableRange 	range;
	struct variableRange* 	ptr;

	variableRange_init_cst(dst_range, 0xffffffffffffffff, size);
	for (i = 0; i < nb_node; i++){
		ptr = ir_operation_get_range(ir_node_get_operation(node_buffer[i]));
		if (ptr == NULL){
			ptr = &range;
		}
		
		irVariableRange_compute(node_buffer[i], ptr, seed);
		variableRange_and(dst_range, ptr, size);
	}
}
