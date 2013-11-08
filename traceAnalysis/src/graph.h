#ifndef GRAPH_H
#define GRAPH_H

#include "graphNode.h"
#include "graphEdge.h"

#define GRAPH_NODE_BATCH	256
#define GRAPH_EDGE_BATCH	256

/* les méthides que l'on doit prendre en charge:
	- insertion d'un noeud
	- insertion d'un edge
	- parcours du graph de manière efficace - créer des mapping pour pouvoir faire des tris est des parcours rapides.
 */

/*
 - faire une classe builder qui maitient en poiinter vers le dernier élement dans le but de créer des edges ou de rajouter des élément sans cohérence
 */

/* je propose de développer cette absatrction pour construire un arbre deb procédure - mais bien garder en tête la faciliterde généraliser l'interface */
 

struct graph{
	struct graphNode* 			nodes;
	struct graphEdge* 			edges;
	int 						nb_allocated_node;
	int 						nb_allocated_edge;
	int 						nb_node;
	int 						nb_edge;
	unsigned long 				entry_point;
	unsigned long				exit_point;
	struct graphNode_callback* 	callback_node;
	struct graphBuilder 		builder;
};

struct graphBuilder{
	int 						current_node_offset;
	int 						previous_node_offset;
};


struct graph* graph_create(struct graphNode_callback* callback_node);
int graph_add_element(struct graph* graph, void* element);
void graph_delete(struct graph* graph);

#endif