#ifndef IRNORMALISZE_H
#define IRNORMALISZE_H

#include "ir.h"

void ir_normalize(struct ir* ir);
void ir_normalize_concrete(struct ir* ir);

void ir_normalize_remove_dead_code(struct ir* ir,  uint8_t* modification);
void ir_normalize_simplify_instruction(struct ir* ir,  uint8_t* modification, uint8_t final);
void ir_normalize_distribute_immediate(struct ir* ir, uint8_t* modification);
void ir_normalize_factor_instruction(struct ir* ir, uint8_t* modification);
void ir_normalize_remove_common_subexpression(struct ir* ir, uint8_t* modification);

void ir_normalize_merge_associative_operation(struct ir* ir, enum irOpcode opcode);

#endif