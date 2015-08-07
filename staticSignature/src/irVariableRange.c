#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "irVariableRange.h"
#include "ir.h"
#include "base.h"

#define ir_node_get_range(node) ((struct variableRange*)((node)->ptr))

void irVariableRange_compute(struct node* node, struct variableRange* range){
	struct irOperation* 	operation;
	struct variableRange* 	range_ptr;

	if (range == NULL){
		range_ptr = ir_node_get_range(node);
	}
	else{
		range_ptr = range;
	}

	operation = ir_node_get_operation(node);
	switch(operation->type){
		case IR_OPERATION_TYPE_IN_REG 	: {
			variableRange_init_size(range_ptr, operation->size);
			break;
		}
		case IR_OPERATION_TYPE_IN_MEM 	: {
			variableRange_init_size(range_ptr, operation->size);
			break;
		}
		case IR_OPERATION_TYPE_OUT_MEM 	: {
			break;
		}
		case IR_OPERATION_TYPE_IMM 		: {
			variableRange_init_cst(range_ptr, ir_imm_operation_get_unsigned_value(operation), operation->size);
			break;
		}
		case IR_OPERATION_TYPE_INST 	: {
			switch(operation->operation_type.inst.opcode){
				case IR_ADD 	: {
					struct edge* 			operand_cursor;
					struct variableRange 	operand_range;

					variableRange_init_cst(range_ptr, 0, operation->size);

					for (operand_cursor = node_get_head_edge_dst(node); operand_cursor != NULL; operand_cursor = edge_get_next_dst(operand_cursor)){
						if (range != NULL){
							irVariableRange_compute(edge_get_src(operand_cursor), &operand_range);
							variableRange_add(range_ptr, &operand_range, operation->size);
						}
						else{
							variableRange_add(range_ptr, ir_node_get_range(edge_get_src(operand_cursor)), operation->size);
						}
					}
					break;
				}
				case IR_AND 	: {
					struct edge* 			operand_cursor;
					struct variableRange 	operand_range;

					variableRange_init_cst(range_ptr, 0xffffffffffffffff, operation->size);

					for (operand_cursor = node_get_head_edge_dst(node); operand_cursor != NULL; operand_cursor = edge_get_next_dst(operand_cursor)){
						if (range != NULL){
							irVariableRange_compute(edge_get_src(operand_cursor), &operand_range);
							variableRange_and(range_ptr, &operand_range, operation->size);
						}
						else{
							variableRange_and(range_ptr, ir_node_get_range(edge_get_src(operand_cursor)), operation->size);
						}
					}
					break;
				}
				case IR_MOVZX 	: {
					variableRange_init_size(range_ptr, ir_node_get_operation(edge_get_src(node_get_head_edge_dst(node)))->size);
					break;
				}
				case IR_SHL 	: {
					struct edge* 			operand_cursor;
					struct variableRange 	operand_range;

					operand_cursor = node_get_head_edge_dst(node);
					if (ir_edge_get_dependence(operand_cursor)->type == IR_DEPENDENCE_TYPE_DIRECT){
						if (range != NULL){
							irVariableRange_compute(edge_get_src(operand_cursor), range_ptr);
							irVariableRange_compute(edge_get_src(edge_get_next_dst(operand_cursor)), &operand_range);
							variableRange_shl(range_ptr, &operand_range, operation->size);
						}
						else{
							memcpy(range_ptr, ir_node_get_range(edge_get_src(operand_cursor)), sizeof(struct variableRange));
							variableRange_shl(range_ptr, ir_node_get_range(edge_get_src(edge_get_next_dst(operand_cursor))), operation->size);
						}
					}
					else{
						if (range != NULL){
							irVariableRange_compute(edge_get_src(edge_get_next_dst(operand_cursor)), range_ptr);
							irVariableRange_compute(edge_get_src(operand_cursor), &operand_range);
							variableRange_shl(range_ptr, &operand_range, operation->size);
						}
						else{
							memcpy(range_ptr, ir_node_get_range(edge_get_src(edge_get_next_dst(operand_cursor))), sizeof(struct variableRange));
							variableRange_shl(range_ptr, ir_node_get_range(edge_get_src(operand_cursor)), operation->size);
						}
					}
					break;
				}
				case IR_SHR 	: {
					struct edge* 			operand_cursor;
					struct variableRange 	operand_range;

					operand_cursor = node_get_head_edge_dst(node);
					if (ir_edge_get_dependence(operand_cursor)->type == IR_DEPENDENCE_TYPE_DIRECT){
						if (range != NULL){
							irVariableRange_compute(edge_get_src(operand_cursor), range_ptr);
							irVariableRange_compute(edge_get_src(edge_get_next_dst(operand_cursor)), &operand_range);
							variableRange_shr(range_ptr, &operand_range, operation->size);
						}
						else{
							memcpy(range_ptr, ir_node_get_range(edge_get_src(operand_cursor)), sizeof(struct variableRange));
							variableRange_shr(range_ptr, ir_node_get_range(edge_get_src(edge_get_next_dst(operand_cursor))), operation->size);
						}
					}
					else{
						if (range != NULL){
							irVariableRange_compute(edge_get_src(edge_get_next_dst(operand_cursor)), range_ptr);
							irVariableRange_compute(edge_get_src(operand_cursor), &operand_range);
							variableRange_shr(range_ptr, &operand_range, operation->size);
						}
						else{
							memcpy(range_ptr, ir_node_get_range(edge_get_src(edge_get_next_dst(operand_cursor))), sizeof(struct variableRange));
							variableRange_shr(range_ptr, ir_node_get_range(edge_get_src(operand_cursor)), operation->size);
						}
					}
					break;
				}
				default 		: {
					variableRange_init_size(range_ptr, operation->size);
					break;
				}
			}
			break;
		}
		case IR_OPERATION_TYPE_SYMBOL 	: {
			variableRange_init_size(range_ptr, 64);
			break;
		}
	}
}

void irVariableRange_get_range_add_buffer(struct variableRange* dst_range, struct node** node_buffer, uint32_t nb_node, uint32_t size, uint32_t cache){
	uint32_t 				i;
	struct variableRange 	node_range;

	variableRange_init_cst(dst_range, 0, size);
	for (i = 0; i < nb_node; i++){
		if (cache){
			variableRange_add(dst_range, ir_node_get_range(node_buffer[i]), size);
		}
		else{
			irVariableRange_compute(node_buffer[i], &node_range);
			variableRange_add(dst_range, &node_range, size);
		}
	}
}

void irVariableRange_get_range_and_buffer(struct variableRange* dst_range, struct node** node_buffer, uint32_t nb_node, uint32_t size, uint32_t cache){
	uint32_t 				i;
	struct variableRange 	node_range;

	variableRange_init_cst(dst_range, 0xffffffffffffffff, size);
	for (i = 0; i < nb_node; i++){
		if (cache){
			variableRange_and(dst_range, ir_node_get_range(node_buffer[i]), size);
		}
		else{
			irVariableRange_compute(node_buffer[i], &node_range);
			variableRange_and(dst_range, &node_range, size);
		}
	}
}
