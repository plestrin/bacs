#ifndef IRVARIABLESIZE_H
#define IRVARIABLESIZE_H

#include "ir.h"

struct mask{
	uint64_t 	p1;
	uint64_t 	p2;
};

struct mask* irVariableSize_propogate_mask(struct ir* ir);
void irVariableSize_shrink(struct ir* ir);
void irVariableSize_adapt(struct ir* ir);

#define irVariable_mask_delete(mask_buffer) free(mask_buffer)

extern const uint8_t irRegisterSize[NB_IR_REGISTER];

#define irRegister_get_size(reg) irRegisterSize[reg]

#endif