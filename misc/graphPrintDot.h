#ifndef GRAPHPRINTDOT_H
#define GRAPHPRINTDOT_H

#include <stdint.h>

#include "graph.h"
#include "ugraph.h"

int32_t graphPrintDot_print(struct graph* graph, const char* name, void* arg);

int32_t ugraphPrintDot_print(struct ugraph* ugraph, const char* name, void* arg);

#endif
