#ifndef DIJKSTRA_H
#define DIJKSTRA_H

#include <stdint.h>

#include "graph.h"
#include "array.h"

#define DIJKSTRA_INVALID_DST 0xffffffff

int32_t dijkstra_dst_to(struct graph* graph, struct node* node, uint32_t* dst_buffer);
int32_t dijkstra_dst_from(struct graph* graph, struct node* node, uint32_t* dst_buffer);
int32_t dijkstra_dst(struct graph* graph, struct node* node, uint32_t* dst_buffer);

int32_t dijkstra_min_path(struct graph* graph, struct node** src_node, uint32_t nb_src, struct node** dst_node, uint32_t nb_dst, struct array** path, uint32_t(*edge_get_distance)(void*));

#endif