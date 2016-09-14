#ifndef IRNORMALISZE_H
#define IRNORMALISZE_H

#include "ir.h"

void ir_normalize(struct ir* ir);
void ir_normalize_concrete(struct ir* ir);

/* should be removed from this header */
int32_t ir_normalize_simplify_instruction(struct ir* ir, uint8_t final);

#define ir_normalize_simplify_instruction_std(ir) ir_normalize_simplify_instruction(ir, 0)
#define ir_normalize_simplify_instruction_lst(ir) ir_normalize_simplify_instruction(ir, 1)

void ir_normalize_simplify_concrete_instruction(struct ir* ir,  uint8_t* modification);

void ir_normalize_remove_common_subexpression(struct ir* ir, uint8_t* modification);

#endif