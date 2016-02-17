#ifndef IRCONCAT_H
#define IRCONCAT_H

#include <stdint.h>

#include "ir.h"

int32_t ir_concat(struct ir* ir_dst, const struct ir* ir_src, uint32_t index);

#endif