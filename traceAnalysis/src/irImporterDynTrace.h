#ifndef IRIMPORTERDYNTRACER_H
#define IRIMPORTERDYNTRACER_H

#include <stdint.h>

#include "ir.h"

int32_t irImporterDynTrace_import(struct ir* ir);

#define irImporterDynTrace_add_operation(ir, opcode) 			ir_add_output((ir), (opcode))
#define irImporterDynTrace_add_input(ir, operand) 				ir_add_input((ir), (operand))
#define irImporterDynTrace_add_dependence(ir, src, dst, type) 	ir_add_dependence((ir), (src), (dst), (type))

#endif