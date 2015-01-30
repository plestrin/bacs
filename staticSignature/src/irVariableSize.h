#ifndef IRVARIABLESIZE_H
#define IRVARIABLESIZE_H

#include "ir.h"

extern const uint8_t irRegisterSize[NB_IR_REGISTER];

#define irRegister_get_size(reg) irRegisterSize[reg]

void ir_normalize_expand_variable(struct ir* ir, uint8_t* modification);

#define valid_operand_size_ins_movzx(ins, op) 		((ins)->size > (op)->size && (ins)->size % (op)->size == 0)
#define valid_operand_size_ins_partX_8(ins, op) 	((ins)->size == 8 && (op)->size % 8 == 0 && (op)->size > 8)
#define valid_operand_size_ins_partX_16(ins, op) 	((ins)->size == 16 && (op)->size % 16 == 0 && (op)->size > 16)

#endif