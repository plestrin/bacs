#ifndef IRVARIABLERANGE_H
#define IRVARIABLERANGE_H

#include <stdint.h>

#include "variableRange.h"
#include "ir.h"

static inline struct variableRange* ir_operation_get_range(struct irOperation* operation){
	if (operation->type != IR_OPERATION_TYPE_INST){
		return NULL;
	}
	else{
		return &(operation->operation_type.inst.range);
	}
}

void irVariableRange_compute(struct node* node, struct variableRange* range_dst, uint32_t seed);

void irVariableRange_get_range_add_buffer(struct variableRange* dst_range, struct node** node_buffer, uint32_t nb_node, uint32_t size, uint32_t seed);
void irVariableRange_get_range_and_buffer(struct variableRange* dst_range, struct node** node_buffer, uint32_t nb_node, uint32_t size, uint32_t seed);

#endif
