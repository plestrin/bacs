#ifndef DIJKSTRA_H
#define DIJKSTRA_H

#include <stdint.h>

#include "graph.h"
#include "array.h"

#define DIJKSTRA_INVALID_DST 0xffffffff

int32_t dijkstra_dst_to(struct graph* graph, struct node* node, uint32_t* dst_buffer);
int32_t dijkstra_dst_from(struct graph* graph, struct node* node, uint32_t* dst_buffer);
int32_t dijkstra_dst(struct graph* graph, struct node* node, uint32_t* dst_buffer);

enum dijkstraPathDirection{
	PATH_SRC_TO_DST,
	PATH_DST_TO_SRC,
	PATH_INVALID
};

#define dijkstraPathDirection_invert(dir) ((dir == PATH_SRC_TO_DST) ? PATH_DST_TO_SRC : PATH_SRC_TO_DST)

struct dijkstraPathStep{
	struct edge* 				edge;
	enum dijkstraPathDirection 	dir;
};

struct dijkstraPath{
	struct array* 	step_array; 		/* Must be first because there is a cast to an array in the minCoverage computation */
	struct node* 	reached_node;
};

int32_t dijkstra_min_path(struct graph* graph, struct node** buffer_src, uint32_t nb_src, struct node** buffer_dst, uint32_t nb_dst, struct array* path_array, uint64_t(*get_mask)(uint64_t,struct node*,struct edge*,enum dijkstraPathDirection));

#define dijkstraPath_clean(path) array_delete((path)->step_array)

#endif