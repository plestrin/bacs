#ifndef GRAPHMINING_H
#define GRAPHMINING_H

#include <stdint.h>

#include "graph.h"
#include "array.h"

#define GRAPHMINING_MIN_OCCURENCE 2

#define node_get_mining_ptr(node) ((struct labelTabItem*)(node)->mining_ptr)

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

#define sizeof_path(length) ((length) * sizeof(uint32_t))

struct frequentKPath{
	uint32_t 				nb_path;
	uint32_t 				length;
	struct path* 			paths;
};

#define frequentKPath_clean(frequent_k_path) 	free((frequent_k_path)->paths);
#define get_path_value(paths, index, length) 	(*(struct path*)((uint32_t*)(paths) + ((length) * (index))))
#define get_path_ptr(paths, index, length) 		(struct path*)((uint32_t*)(paths) + ((length) * (index)))

struct candidatKPath{
	struct candidatKPath* 	prev;
	struct candidatKPath* 	next;
	uint32_t 				length;
	struct path 			path;
};

#define sizeof_candidatKPath(length) (sizeof(struct candidatKPath) + ((length) - 2) * sizeof(uint32_t))

struct frequentPathFastAccess{
	uint32_t 				length;
	struct path* 			path;
};

struct graphMatriceLine{
	uint32_t 				label;
	uint32_t 				node[1];
};

#define sizeof_graphMatriceLine(nb_path) (sizeof(struct graphMatriceLine) + ((nb_path) - 1) * sizeof(uint32_t))

struct graphMatrice{
	uint32_t 				nb_node;
	struct graphMatriceLine lines[1];
};

#define sizeof_graphMatrice(nb_path, nb_node) 					(sizeof(struct graphMatrice) - sizeof(struct graphMatriceLine) + sizeof_graphMatriceLine(nb_path) * (nb_node))
#define graphMatrice_get_line(graph_matrice, index, nb_path) 	((struct graphMatriceLine*)((char*)((graph_matrice)->lines) + sizeof_graphMatriceLine(nb_path) * (index)))

struct graphMatriceBuffer{
	uint32_t 				nb_graph_matrice;
	struct graphMatrice** 	graph_matrices;
};

struct graphMatriceBuffer* graphMatriceBuffer_create(uint32_t nb_graph_matrice, uint32_t graph_matrice_tot_size);
#define graphMatriceBuffer_delete(graph_matrice_buffer) free(graph_matrice_buffer);

struct graphMining{
	uint32_t 				nb_label;
	struct labelTabItem* 	label_tab;
	struct labelFastAccess* label_fast;

	struct array 			frequent_path_array;
	struct array 			frequent_graph_array;
};

struct graphMining* graphMining_create();
int32_t graphMining_init(struct graphMining* graph_mining);

int32_t graphMining_init_label_tab(struct graph* graph, struct graphMining* graph_mining, uint32_t(*node_get_label)(struct node*));
int32_t graphMining_init_label_fast(struct graph* graph, struct graphMining* graph_mining);
int32_t graphMining_search_frequent_edge(struct graphMining* graph_mining, struct frequentKPath* frequent_k_path);
int32_t graphMining_search_frequent_path(struct graphMining* graph_mining, uint32_t k);
int32_t graphMining_search_frequent_2_graph(struct graphMining* graph_mining);

uint32_t graphMining_evaluate_path_frequency(struct graphMining* graph_mining, struct path* path, uint32_t length);
uint32_t graphMining_count_path(uint32_t* labels, struct node* node, uint32_t length);

struct possibleAssignementHeader{
	uint32_t 	nb_possible_assignement;
	uint32_t 	node_offset;
};

struct possibleAssignement{
	uint32_t 								nb_node;
	struct possibleAssignementHeader* 		headers;
	struct node** 							nodes;
};

uint32_t graphMining_evaluate_subgraph_frequency(struct graphMining* graph_mining, struct graphMatrice* graph_matrice, uint32_t nb_path);
uint32_t graphMining_count_subgraph(struct graphMatrice* graph_matrice, uint32_t nb_path, struct node** assignement, uint32_t nb_assignement, struct possibleAssignement* possible_assignement);

struct possibleAssignement* possibleAssignement_create(uint32_t nb_node, uint32_t nb_assignement);
struct possibleAssignement* possibleAssignement_create_init_first(struct graphMining* graph_mining, struct graphMatrice* graph_matrice, uint32_t nb_path);
struct possibleAssignement* possibleAssignement_duplicate(struct possibleAssignement* possible_assignement, uint32_t new_assignement_index, struct node* new_assignement_value);
void possibleAssignement_update(struct possibleAssignement* possible_assignement, struct graphMatrice* graph_matrice, uint32_t nb_assignement, uint32_t nb_path);

#define possibleAssignement_delete(possible_assignement) free(possible_assignement);

int32_t graphMining_combine_paths(struct frequentPathFastAccess* path_access1, struct frequentPathFastAccess* path_access2, struct array* graph_array);
struct graphMatrice* graphMining_create_graphMatrice_from_path(struct frequentPathFastAccess* path_access1, struct frequentPathFastAccess* path_access2, uint32_t* merge, uint32_t nb_merge);

int32_t graphMining_are_graphMatrice_isomorphic(struct graphMatrice* m1, struct graphMatrice* m2, uint32_t nb_path);

int32_t graphMining_mine(struct graph* graph, uint32_t(*node_get_label)(struct node*));

void graphMining_clean(struct graphMining* graph_mining);

#define graphMining_delete(graph_mining) 																					\
	graphMining_clean(graph_mining); 																						\
	free(graph_mining)

#endif