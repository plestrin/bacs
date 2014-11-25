#ifndef IRMEMORY_H
#define IRMEMORY_H

#include <stdint.h>

#include "ir.h"

void ir_normalize_simplify_memory_access(struct ir* ir, uint8_t* modification);


#endif