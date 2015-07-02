#ifndef IREXPRESSION_H
#define IREXPRESSION_H

#include <stdint.h>

#include "ir.h"

void ir_normalize_affine_expression(struct ir* ir,  uint8_t* modification);

#endif