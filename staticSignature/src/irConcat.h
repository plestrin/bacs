#ifndef IRCONCAT_H
#define IRCONCAT_H

#include <stdio.h>

#include "ir.h"
#include "irRenameEngine.h"

int32_t irOperation_copy(void* data_dst, const void* data_src, void* arg);
int32_t irDependence_copy(void* data_dst, const void* data_src, void* arg);
int32_t ir_concat(struct ir* ir_dst, struct ir* ir_src, struct irRenameEngine* engine);

#endif