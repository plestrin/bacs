#ifndef GRAPHPRINTDOT_H
#define GRAPHPRINTDOT_H

#define GRAPHPRINTDOT_DEFAULT_FILE_NAME "graph.dot"

#include "graph.h"

int graphPrintDot_print(struct graph* graph, const char* name);

#endif