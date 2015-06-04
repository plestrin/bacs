#ifndef IRLAYER_H
#define IRLAYER_H

#include <stdint.h>

#include "set.h"
#include "graph.h"
#include "ugraph.h"
#include "ir.h"

#define IRLAYER_NB_NODE_PER_BLOCK 8
#define IRLAYER_NB_EDGE_PER_BLOCK 16

struct irLayer{
	struct set* 	node_set;
	struct set* 	edge_set;
};

#define graphLayer_node_get_layer(unode) ((struct irLayer*)((unode)->data))

int32_t irLayer_init(struct irLayer* layer);
int32_t irlayer_add_node(struct unode* layer_node, struct node* node);
int32_t irlayer_add_edge(struct unode* layer_node, struct edge* edge);

#define irLayer_rem_node(layer_node, node) 									\
	ir_node_layerSet_rem(node, layer_node); 								\
	set_remove(graphLayer_node_get_layer(layer_node)->node_set, &(node));

#define  irLayer_rem_edge(layer_node, edge) 								\
	ir_edge_layerSet_rem(edge, layer_node); 								\
	set_remove(graphLayer_node_get_layer(layer_node)->edge_set, &(edge));

void irLayer_commit(struct unode* layer_node);
void irLayer_clean(struct unode* layer_node, struct ir* ir);

#define IREQUIVALENCECLASS_NB_REF_PER_BLOCK 6

#define irEquivalenceClass set

#define irEquivalenceClass_create() set_create(sizeof(struct node*), IREQUIVALENCECLASS_NB_REF_PER_BLOCK)
#define irEquivalenceClass_get_nb_node(class) ((class)->nb_element_tot)

struct irEquivalenceClass* irEquivalenceClass_add_node(struct irEquivalenceClass* class, struct node* node);
void irEquivalenceClass_rem_node(struct node* node);

#define irEquivalenceClass_delete(class) set_delete(class) 

#define graphLayer_create() 						ugraph_create(sizeof(struct irLayer))
#define graphLayer_init(ugraph) 					ugraph_init(ugraph, sizeof(struct irLayer))
#define graphLayer_add_layer(ugraph, layer) 		ugraph_add_node(ugraph, layer)
#define graphLayer_add_layer_(ugraph) 				ugraph_add_node_(ugraph)

struct unode* graphLayer_new_layer(struct ugraph* ugraph);

#define graphLayer_add_collision(unode1, unode2) 	ugraph_add_edge(unode1, unode2)

#define graphLayer_commit_layer(ugraph, unode) 							\
	irLayer_commit(graphLayer_node_get_layer(unode)); 					\
	ugraph_remove_node(unode);

#define graphLayer_delete_layer(ugraph, unode, ir) 						\
	irLayer_clean(unode, ir); 											\
	ugraph_remove_node(ugraph, unode);

#define graphLayer_clean(ugraph, ir) 									\
	while ((ugraph)->head != NULL){ 									\
		graphLayer_delete_layer(ugraph, (ugraph)->head, ir); 			\
	}

#define graphLayer_delete(ugraph) 										\
	graphLayer_clean(ugraph); 											\
	free(ugraph);

#define LAYERSET_NB_LAYER_PER_BLOCK 8

#define ir_node_layerSet_get(node) 					((struct set*)ir_node_get_operation(node)->layer_set)
#define ir_edge_layerSet_get(edge) 					((struct set*)ir_edge_get_dependence(edge)->layer_set)
#define ir_node_layerSet_set(node, layer_set_) 		(ir_node_get_operation(node)->layer_set = (void*)layer_set_)
#define ir_edge_layerSet_set(edge, layer_set_) 		(ir_edge_get_dependence(edge)->layer_set = (void*)layer_set_)

int32_t ir_node_layerSet_add(struct node* node, struct unode* layer_node);
int32_t ir_edge_layerSet_add(struct edge* edge, struct unode* layer_node);

void ir_node_layerSet_rem(struct node* node, struct unode* layer_node);
void ir_edge_layerSet_rem(struct edge* edge, struct unode* layer_node);

#define ir_node_equivalenceClass_get(node) 			((struct irEquivalenceClass*)ir_node_get_operation(node)->equivalence_class)
#define ir_node_equivalenceClass_set(node, class) 	(ir_node_get_operation(node)->equivalence_class = (void*)class)

#endif