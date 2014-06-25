#ifndef IRIMPORTERDYNTRACER_H
#define IRIMPORTERDYNTRACER_H

#include <stdint.h>

#include "ir.h"

int32_t irImporterDynTrace_import(struct ir* ir);

#define irImporterDynTrace_add_operation(ir, opcode, operand, size) 	ir_add_output((ir), (opcode), (operand), (size))
#define irImporterDynTrace_add_input(ir, operand, size) 				ir_add_input((ir), (operand), (size))
#define irImporterDynTrace_add_dependence(ir, src, dst, type) 			ir_add_dependence((ir), (src), (dst), (type))

#endif