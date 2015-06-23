#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "irVariableRange.h"
#include "ir.h"

#define ir_node_get_range(node) ((struct irVariableRange*)((node)->ptr))

static inline void irVariableRange_init_range(struct irVariableRange* range_dst, struct irVariableRange* range_src){
	memcpy(range_dst, range_src, sizeof(struct irVariableRange));
}

static inline void irVariableRange_init_size(struct irVariableRange* range_dst, uint32_t size){
	range_dst->lower_bound = 0;
	range_dst->upper_bound = (0xffffffffffffffff >> (64 - size));
}

static inline void irVariableRange_init_constant(struct irVariableRange* range_dst, uint64_t constant){
	range_dst->lower_bound = constant;
	range_dst->upper_bound = constant;
}

static inline void irVariableRange_add_range(struct irVariableRange* range_dst, struct irVariableRange* range_src, uint32_t modulo){
	uint64_t len_dst;
	uint64_t len_src;

	len_dst = (range_dst->upper_bound - range_dst->lower_bound) & (0xffffffffffffffff >> (64 - modulo));
	len_src = (range_src->upper_bound - range_src->lower_bound) & (0xffffffffffffffff >> (64 - modulo));

	if (len_dst >= (0xffffffffffffffff >> (64 - modulo)) - len_src){
		range_dst->lower_bound = 0;
		range_dst->upper_bound = (0xffffffffffffffff >> (64 - modulo));
	}
	else{
		range_dst->lower_bound = (range_dst->lower_bound + range_src->lower_bound) & (0xffffffffffffffff >> (64 - modulo));
		range_dst->upper_bound = (range_dst->upper_bound + range_src->upper_bound) & (0xffffffffffffffff >> (64 - modulo));
	}
}

static inline void irVariableRange_and_range(struct irVariableRange* range_dst, struct irVariableRange* range_src, uint32_t modulo){
	uint64_t max_val;

	if (range_dst->upper_bound < range_dst->lower_bound){
		max_val = 0xffffffffffffffff >> (64 - modulo);
	}
	else{
		max_val = range_dst->upper_bound;
	}
	if (range_src->upper_bound >= range_src->lower_bound){
		max_val = (max_val < range_src->upper_bound) ? max_val : range_src->upper_bound;
	}

	range_dst->lower_bound = 0;
	range_dst->upper_bound = (0xffffffffffffffff >> __builtin_clzll(max_val));
}

static inline void irVariableRange_shr_range(struct irVariableRange* range_dst, struct irVariableRange* range_src, uint32_t modulo){
	range_dst->lower_bound = (range_dst->lower_bound >> range_src->lower_bound) & (0xffffffffffffffff >> (64 - modulo));
	range_dst->upper_bound = (range_dst->upper_bound >> range_dst->upper_bound) & (0xffffffffffffffff >> (64 - modulo));
}

void irVariableRange_compute(struct node* node){
	struct irOperation* 	operation;
	struct edge* 			operand_cursor;
	struct irVariableRange* range;

	operation = ir_node_get_operation(node);
	switch(operation->type){
		case IR_OPERATION_TYPE_IN_REG 	: {
			irVariableRange_init_size(ir_node_get_range(node), operation->size);
			break;
		}
		case IR_OPERATION_TYPE_IN_MEM 	: {
			irVariableRange_init_size(ir_node_get_range(node), operation->size);
			break;
		}
		case IR_OPERATION_TYPE_OUT_MEM 	: {
			break;
		}
		case IR_OPERATION_TYPE_IMM 		: {
			irVariableRange_init_constant(ir_node_get_range(node), ir_imm_operation_get_unsigned_value(operation));
			break;
		}
		case IR_OPERATION_TYPE_INST 	: {
			switch(operation->operation_type.inst.opcode){
				case IR_ADD 	: {
					range = ir_node_get_range(node);
					irVariableRange_init_constant(range, 0);

					for (operand_cursor = node_get_head_edge_dst(node); operand_cursor != NULL; operand_cursor = edge_get_next_dst(operand_cursor)){
						irVariableRange_add_range(range, ir_node_get_range(edge_get_src(operand_cursor)), operation->size);
					}
					break;
				}
				case IR_AND 	: {
					range = ir_node_get_range(node);
					irVariableRange_init_size(range, operation->size);

					for (operand_cursor = node_get_head_edge_dst(node); operand_cursor != NULL; operand_cursor = edge_get_next_dst(operand_cursor)){
						irVariableRange_and_range(range, ir_node_get_range(edge_get_src(operand_cursor)), operation->size);
					}
					break;
				}
				case IR_MOVZX 	: {
					irVariableRange_init_size(ir_node_get_range(node), ir_node_get_operation(edge_get_src(node_get_head_edge_dst(node)))->size);
					break;
				}
				case IR_SHR 	: {
					struct edge* operand1;
					struct edge* operand2;

					range = ir_node_get_range(node);

					operand1 = node_get_head_edge_dst(node);
					operand2 = edge_get_next_dst(operand1);

					if (ir_edge_get_dependence(operand1) == IR_DEPENDENCE_TYPE_DIRECT){
						irVariableRange_init_range(range, ir_node_get_range(edge_get_src(operand1)));
						irVariableRange_shr_range(range, ir_node_get_range(edge_get_src(operand2)), operation->size);
					}
					else{
						irVariableRange_init_range(range, ir_node_get_range(edge_get_src(operand2)));
						irVariableRange_shr_range(range, ir_node_get_range(edge_get_src(operand1)), operation->size);
					}

					break;
				}
				default 		: {
					irVariableRange_init_size(ir_node_get_range(node), operation->size);
					break;
				}
			}
			break;
		}
		case IR_OPERATION_TYPE_SYMBOL 	: {
			range = ir_node_get_range(node);

			range->lower_bound = 0;
			range->upper_bound = 0xffffffffffffffff;
			
			break;
		}
	}
}

void irVariableRange_get_range_additive_list(struct irVariableRange* dst_range, struct node** node_buffer, uint32_t nb_node){
	uint32_t i;

	irVariableRange_init_constant(dst_range, 0);
	for (i = 0; i < nb_node; i++){
		irVariableRange_add_range(dst_range, ir_node_get_range(node_buffer[i]), 32);
	}
}

uint32_t irVariableRange_intersect(struct irVariableRange* range1, struct irVariableRange* range2){
	if (range1->lower_bound < range2->lower_bound){
		if (range1->upper_bound < range1->lower_bound){
			return 1;
		}
		else if (range1->upper_bound >= range2->lower_bound){
			return 1;
		}
		else{
			return 0;
		}
	}
	else{
		if (range2->upper_bound < range2->lower_bound){
			return 1;
		}
		else if (range2->upper_bound >= range1->lower_bound){
			return 1;
		}
		else{
			return 0;
		}
	}
}

void irVariableRange_print(struct irVariableRange* range){
	if (range->upper_bound < range->lower_bound){
		printf("Range %p: 0x%llx -> ?? ; 0x0 -> 0x%llx", (void*)range, range->lower_bound, range->upper_bound);
	}
	else{
		printf("Range %p: 0x%llx -> 0x%llx", (void*)range, range->lower_bound, range->upper_bound);
	}
}