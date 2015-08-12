#ifndef IRCHECK_H
#define IRCHECK_H

#include <stdint.h>

#include "ir.h"

void ir_check_connectivity(struct ir* ir);
void ir_check_size(struct ir* ir);
void ir_check_order(struct ir* ir);
void ir_check_instruction_index(struct ir* ir);
void ir_check_acyclic(struct ir* ir);

#define ir_check(ir) 				\
	ir_check_connectivity(ir); 		\
	ir_check_size(ir); 				\
	ir_check_acyclic(ir); 			\
	ir_check_instruction_index(ir)

#endif