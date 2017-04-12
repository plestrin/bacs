#ifndef IOREL_H
#define IOREL_H

#include "trace.h"
#include "irBuffer.h"

void trace_search_io(struct trace* trace);
void trace_check_io(struct trace* trace, struct primitiveParameter* prim_para_buffer, uint32_t nb_prim_para);

#endif
