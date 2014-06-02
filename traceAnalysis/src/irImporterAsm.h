#ifndef IRIMPORTERASM_H
#define IRIMPORTERASM_H

#include <stdint.h>

#include "ir.h"

int32_t irImporterAsm_import(struct ir* ir);

#define irImporterAsm_add_operation(ir, opcode, operand) 	ir_add_output((ir), (opcode), (operand))
#define irImporterAsm_add_input(ir, operand) 				ir_add_input((ir), (operand))
#define irImporterAsm_add_dependence(ir, src, dst, type) 	ir_add_dependence((ir), (src), (dst), (type))


#endif