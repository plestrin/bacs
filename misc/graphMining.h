#ifndef GRAPHMINING_H
#define GRAPHMINING_H

#include <stdint.h>

#include "graph.h"
#include "array.h"

#define GRAPHMINING_MIN_OCCURENCE 2

#define node_get_mining_ptr(node) 				((struct labelTabItem*)(node)->mining_ptr)

struct labelTabItem{
	uint32_t 				label;
	struct node* 			node;
	uint32_t 				fastAccess_index;
};

struct labelFastAccess{
	uint32_t 				label;
	uint32_t 				offset;
	uint32_t 				size;
};

struct path{
	uint32_t 				labels[2];
};

#define sizeof_path(size) 						((size) * sizeof(uint32_t))

struct frequentKPath{
	uint32_t 				nb_path;
	uint32_t 				length;
	struct path* 			paths;
};

#define frequentKPath_clean(frequent_k_path) 	free((frequent_k_path)->paths);
#define get_path_value(paths, index, size) 		(*(struct path*)((uint32_t*)(paths) + ((size) * (index))))
#define get_path_ptr(paths, index, size) 		(struct path*)((uint32_t*)(paths) + ((size) * (index)))

struct candidatKPath{
	struct candidatKPath* 	prev;
	struct candidatKPath* 	next;
	uint32_t 				length;
	struct path 			path;
};

#define sizeof_candidatKPath(size) 				(sizeof(struct candidatKPath) + ((size) - 2) * sizeof(uint32_t))

struct graphMining{
	uint32_t 				nb_label;
	struct labelTabItem* 	label_tab;
	struct labelFastAccess* label_fast;

	struct array 			frequent_path_array;
};

struct graphMining* graphMining_create();
int32_t graphMining_init(struct graphMining* graph_mining);

int32_t graphMining_init_label_tab(struct graph* graph, struct graphMining* graph_mining, uint32_t(*node_get_label)(struct node*));
int32_t graphMining_init_label_fast(struct graph* graph, struct graphMining* graph_mining);
int32_t graphMining_search_frequent_edge(struct graphMining* graph_mining, struct frequentKPath* frequent_k_path);
int32_t graphMining_search_frequent_path(struct graphMining* graph_mining, uint32_t k);

uint32_t graphMining_evaluate_path_frequency(struct graphMining* graph_mining, struct path* path, uint32_t length);
uint32_t graphMining_count_path(uint32_t* labels, struct node* node, uint32_t length);

int32_t graphMining_mine(struct graph* graph, uint32_t(*node_get_label)(struct node*));

void graphMining_clean(struct graphMining* graph_mining);

#define graphMining_delete(graph_mining) 																					\
	graphMining_clean(graph_mining); 																						\
	free(graph_mining)

#endif