#ifndef IRMEMORY_H
#define IRMEMORY_H

#include <stdint.h>

#include "ir.h"

enum aliasingStrategy{
	ALIASING_STRATEGY_WEAK,
	ALIASING_STRATEGY_STRICT,
	ALIASING_STRATEGY_CHECK,
	ALIASING_STRATEGY_PRINT
};

void ir_normalize_simplify_memory_access(struct ir* ir, uint8_t* modification, enum aliasingStrategy strategy);

#define ir_print_aliasing(ir) ir_normalize_simplify_memory_access(ir, NULL, ALIASING_STRATEGY_PRINT)

void ir_simplify_concrete_memory_access(struct ir* ir);

#endif