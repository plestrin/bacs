#ifndef IRNORMALISZE_H
#define IRNORMALISZE_H

#include "ir.h"

void ir_normalize(struct ir* ir);
void ir_normalize_translate_rol_imm(struct ir* ir);
void ir_normalize_translate_sub_imm(struct ir* ir);
void ir_normalize_translate_xor_ff(struct ir* ir);
void ir_normalize_merge_transitive_operation(struct ir* ir, enum irOpcode opcode);
void ir_normalize_propagate_expression(struct ir* ir);
void ir_normalize_detect_rotation(struct ir* ir);

#endif