#ifndef GRAPHPRINTDOT_H
#define GRAPHPRINTDOT_H

#include <stdint.h>

#include "graph.h"
#include "graphLayer.h"

int32_t graphPrintDot_print(struct graph* graph, const char* name, void* arg);
int32_t graphLayerPrintDot_print(struct graphLayer* graph_layer, const char* name, void* arg);

#endif
