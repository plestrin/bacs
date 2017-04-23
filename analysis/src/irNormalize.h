#ifndef IRNORMALISZE_H
#define IRNORMALISZE_H

#include "ir.h"

#define IR_NORMALIZE_FOLD_CONSTANT 				1
#define IR_NORMALIZE_DETECT_CST_EXPRESSION 		1
#define IR_NORMALIZE_SIMPLIFY_INSTRUCTION		1
#define IR_NORMALIZE_REMOVE_SUBEXPRESSION 		1
#define IR_NORMALIZE_SIMPLIFY_MEMORY_ACCESS 	1
#define IR_NORMALIZE_EXPAND_VARIABLE			1
#define IR_NORMALIZE_DISTRIBUTE_CST 			1
#define IR_NORMALIZE_COALESCE_MEMORY_ACCESS 	1
#define IR_NORMALIZE_AFFINE_EXPRESSION 			1
#define IR_NORMALIZE_MERGE_CST 					1

void ir_normalize(struct ir* ir);
#if IR_NORMALIZE_SIMPLIFY_MEMORY_ACCESS == 1
void ir_normalize_concrete(struct ir* ir);
#else
#define ir_normalize_concrete(ir)
#endif
void ir_normalize_light(struct ir* ir);

#endif
