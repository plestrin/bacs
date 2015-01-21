#ifndef IRCHECK_H
#define IRCHECK_H

#include <stdint.h>

#include "ir.h"

void ir_check_connectivity(struct ir* ir);
void ir_check_size(struct ir* ir);

#define ir_check(ir) 				\
	ir_check_connectivity(ir); 		\
	ir_check_size(ir)

#endif