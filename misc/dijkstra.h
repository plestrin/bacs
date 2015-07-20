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

struct node* dijkstra_lowest_common_ancestor(struct graph* graph, struct node** node_buffer1, uint32_t nb_node_buffer1, struct node** node_buffer2, uint32_t nb_node_buffer2, struct array** path1, struct array** path2, uint32_t(*edge_get_distance)(void*,uint32_t));
struct node* dijkstra_highest_common_descendant(struct graph* graph, struct node** node_buffer1, uint32_t nb_node_buffer1, struct node** node_buffer2, uint32_t nb_node_buffer2, struct array** path1, struct array** path2, uint32_t(*edge_get_distance)(void*));

#endif