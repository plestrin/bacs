#ifndef DAGPARTIALORDER_H
#define DAGPARTIALORDER_H

#include <stdint.h>

#include "graph.h"

int32_t dagPartialOrder_sort_src_dst(struct graph* graph);
int32_t dagPartialOrder_sort_dst_src(struct graph* graph);

#endif