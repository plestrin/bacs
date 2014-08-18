#ifndef IRNORMALISZE_H
#define IRNORMALISZE_H

#include "ir.h"


void ir_normalize(struct ir* ir);
void ir_normalize_simplify_instruction(struct ir* ir);
void ir_normalize_merge_associative_operation(struct ir* ir, enum irOpcode opcode);
void ir_normalize_propagate_expression(struct ir* ir, uint8_t* modification);
void ir_normalize_simplify_memory_access(struct ir* ir, uint8_t* modification);
void ir_normalize_detect_rotation(struct ir* ir);
void ir_normalize_detect_first_byte_extract(struct ir* ir);
void ir_normalize_detect_second_byte_extract(struct ir* ir);

#endif