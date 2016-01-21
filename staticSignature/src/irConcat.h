#ifndef IRCONCAT_H
#define IRCONCAT_H

#include <stdio.h>

#include "ir.h"

int32_t irOperation_copy(void* data_dst, const void* data_src, void* arg);
int32_t irDependence_copy(void* data_dst, const void* data_src, void* arg);
int32_t ir_concat(struct ir* ir_dst, const struct ir* ir_src);

#endif