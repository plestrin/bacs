#ifndef IRIMPORTERASM_H
#define IRIMPORTERASM_H

#include <stdint.h>

#include "ir.h"
#include "memTrace.h"

int32_t irImporterAsm_import(struct ir* ir, struct assembly* assembly, struct memTrace* mem_trace);

#endif