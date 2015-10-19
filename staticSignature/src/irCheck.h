#ifndef IRCHECK_H
#define IRCHECK_H

#include <stdint.h>

#include "ir.h"

uint32_t ir_check_connectivity(struct ir* ir);
uint32_t ir_check_size(struct ir* ir);
uint32_t ir_check_order(struct ir* ir);
uint32_t ir_check_instruction_index(struct ir* ir);
uint32_t ir_check_acyclic(struct ir* ir);
uint32_t ir_check_memory(struct ir* ir);

void ir_check_clean_error_flag(struct ir* ir);

static inline int32_t ir_check(struct ir* ir){
	ir_check_clean_error_flag(ir);
	return ir_check_connectivity(ir) | ir_check_size(ir) | ir_check_acyclic(ir);
}

#endif