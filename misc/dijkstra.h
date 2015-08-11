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

int32_t dijkstra_min_path(struct graph* graph, struct node** src_node, uint32_t nb_src, struct node** dst_node, uint32_t nb_dst, struct array** path, uint32_t(*edge_get_distance)(void*));

struct zzPath{
	struct array* path_1_descendant;
	struct array* path_ancestor_descendant;
	struct array* path_ancestor_2;
};

#define zzPath_init(zz_path) memset(zz_path, 0, sizeof(struct zzPath))
#define zzPath_get_descendant(zz_path) ((array_get_length((zz_path)->path_1_descendant) == 0 || array_get_length((zz_path)->path_ancestor_descendant) == 0) ? NULL : edge_get_dst(*(struct edge**)array_get((zz_path)->path_1_descendant, 0)))
#define zzPath_get_ancestor(zz_path) ((array_get_length((zz_path)->path_ancestor_2) == 0) ? NULL : edge_get_src(*(struct edge**)array_get((zz_path)->path_ancestor_2, 0)))

#define zzPath_clean(zz_path) 												\
	if ((zz_path)->path_1_descendant != NULL){ 								\
		array_delete((zz_path)->path_1_descendant); 						\
		(zz_path)->path_1_descendant = NULL; 								\
	} 																		\
	if ((zz_path)->path_ancestor_descendant != NULL){ 						\
		array_delete((zz_path)->path_ancestor_descendant); 					\
		(zz_path)->path_ancestor_descendant = NULL; 						\
	} 																		\
	if ((zz_path)->path_ancestor_2 != NULL){ 								\
		array_delete((zz_path)->path_ancestor_2); 							\
		(zz_path)->path_ancestor_2 = NULL; 									\
	} 																		\

int32_t dijkstra_min_zzPath(struct graph* graph, struct node** node_buffer1, uint32_t nb_node_buffer1, struct node** node_buffer2, uint32_t nb_node_buffer2, struct zzPath* path, uint32_t(*edge_get_distance)(void*));

struct node* dijkstra_lowest_common_ancestor(struct graph* graph, struct node** node_buffer1, uint32_t nb_node_buffer1, struct node** node_buffer2, uint32_t nb_node_buffer2, struct array** path1, struct array** path2, uint32_t(*edge_get_distance)(void*));
struct node* dijkstra_highest_common_descendant(struct graph* graph, struct node** node_buffer1, uint32_t nb_node_buffer1, struct node** node_buffer2, uint32_t nb_node_buffer2, struct array** path1, struct array** path2, uint32_t(*edge_get_distance)(void*));

#endif