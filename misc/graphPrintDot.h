#ifndef GRAPHPRINTDOT_H
#define GRAPHPRINTDOT_H

#include <stdint.h>

#include "graph.h"

#define GRAPHPRINTDOT_DEFAULT_FILE_NAME "graph.dot"

int32_t graphPrintDot_print(struct graph* graph, const char* name, void* arg);

#endif
