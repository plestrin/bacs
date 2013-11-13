#ifndef GRAPH_H
#define GRAPH_H

#include "graphNode.h"
#include "graphEdge.h"

#define GRAPH_NODE_BATCH	256
#define GRAPH_EDGE_BATCH	256

struct graphBuilder{
	int 					current_node_offset;
	int 					previous_node_offset;
};

enum graphMapping_type{
	GRAPHMAPPING_NODE,
	GRAPHMAPPING_EDGE
};

struct graphMapping{
	struct graphMapping* 	prev;
	struct graphMapping* 	next;
	enum graphMapping_type 	type;
	int (*compare)(const void*,const void*);
	void** 					map;
	int 					size;
	char 					valid;
};

struct graph{
	struct graphNode* 			nodes;
	struct graphEdge* 			edges;
	int 						nb_allocated_node;
	int 						nb_allocated_edge;
	int 						nb_node;
	int 						nb_edge;
	unsigned long 				entry_point;
	unsigned long				exit_point;
	struct graphNode_callback 	callback_node;
	struct graphBuilder 		builder;
	struct graphMapping*		mapping_head;
	struct graphMapping* 		mapping_tail;
};


struct graph* graph_create();
int graph_add_element(struct graph* graph, void* element);
void graph_delete(struct graph* graph);

struct graphMapping* graph_create_mapping(struct graph* graph, enum graphMapping_type type, int (*compare)(const void*,const void*));
void* graph_search(struct graph* graph, struct graphMapping* mapping, const void* key);
void graph_delete_mapping(struct graph* graph, struct graphMapping* mapping);

#endif