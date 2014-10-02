#ifndef DIJKSTRA_H
#define DIJKSTRA_H

#include <stdint.h>

#include "graph.h"

#define DIJKSTRA_INVALID_DST 0xffffffff

int32_t dijkstra_dst_to(struct graph* graph, struct node* node, uint32_t* dst_buffer);
int32_t dijkstra_dst_from(struct graph* graph, struct node* node, uint32_t* dst_buffer);
int32_t dijkstra_dst(struct graph* graph, struct node* node, uint32_t* dst_buffer);

#endif