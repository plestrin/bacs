#ifndef DIJKSTRA_H
#define DIJKSTRA_H

#include <stdint.h>
#include <string.h>

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

struct dijkstraPathStep{
	struct edge* 				edge;
	enum dijkstraPathDirection 	dir;
};

struct dijkstraPath{
	struct array* 	step_array;
	struct node* 	reached_node;
};

#define dijkstraPath_init(path) 											\
	(path).step_array = NULL; 												\
	(path).reached_node = NULL;

#define dijkstraPath_clean(path) 											\
	if ((path).step_array != NULL){ 										\
		array_delete((path).step_array); 									\
	}

int32_t dijkstra_min_path(struct graph* graph, struct node** buffer_src, uint32_t nb_src, struct node** buffer_dst, uint32_t nb_dst, struct dijkstraPath* path, uint32_t(*edge_get_distance)(void*));

#endif