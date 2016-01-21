#ifndef IRIMPORTERASM_H
#define IRIMPORTERASM_H

#include <stdint.h>

#include "ir.h"
#include "memTrace.h"

int32_t irImporterAsm_import(struct ir* ir, const struct assembly* assembly, const struct memTrace* mem_trace);
int32_t irImporterAsm_import_compound(struct ir* ir, const struct assembly* assembly, const struct memTrace* mem_trace, struct irComponent** ir_component_buffer, uint32_t nb_ir_component);

#endif