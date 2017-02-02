#ifndef IRVARIABLESIZE_H
#define IRVARIABLESIZE_H

#include "ir.h"

extern const uint8_t irRegisterSize[NB_IR_STD_REGISTER];

static inline uint8_t irRegister_get_size(enum irRegister reg){
	if (irRegister_is_std(reg)){
		return irRegisterSize[reg];
	}
	else{
		return irRegister_simd_get_size(reg);
	}
}

int32_t irNormalize_expand_variable(struct ir* ir);

#define valid_operand_size_ins_movzx(ins, op) 		((ins)->size > (op)->size)
#define valid_operand_size_ins_partX_8(ins, op) 	((ins)->size == 8 && (op)->size % 8 == 0 && (op)->size > 8)
#define valid_operand_size_ins_partX_16(ins, op) 	((ins)->size == 16 && (op)->size % 16 == 0 && (op)->size > 16)

#endif
