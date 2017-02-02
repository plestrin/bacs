#ifndef IRNORMALISZE_H
#define IRNORMALISZE_H

#include "ir.h"

void ir_normalize(struct ir* ir);
void ir_normalize_concrete(struct ir* ir);

/* should be removed from this header */
int32_t ir_normalize_simplify_concrete_instruction(struct ir* ir);

#endif
