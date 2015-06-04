#include <stdlib.h>
#include <stdio.h>

#include "irLayer.h"

int32_t ir_node_layerSet_add(struct node* node, struct unode* layer_node){
	struct set* layer_set;

	layer_set = ir_node_layerSet_get(node);
	if (layer_set == NULL){
		layer_set = set_create(sizeof(struct irLayer*), LAYERSET_NB_LAYER_PER_BLOCK);
		if (layer_set != NULL){
			if (set_add(layer_set, &layer_node)){
				printf("ERROR: in %s, unable to add element to set\n", __func__);
				set_delete(layer_set);
				return -1;
			}
			else{
				ir_node_layerSet_set(node, layer_set);
				return 0;
			}
		}
		else{
			printf("ERROR: in %s, unable to create set\n", __func__);
			return -1;
		}
	}
	else{
		return set_add(layer_set, &layer_node);
	}
}

int32_t ir_edge_layerSet_add(struct edge* edge, struct unode* layer_node){
	struct set* layer_set;

	layer_set = ir_edge_layerSet_get(edge);
	if (layer_set == NULL){
		layer_set = set_create(sizeof(struct irLayer*), LAYERSET_NB_LAYER_PER_BLOCK);
		if (layer_set != NULL){
			if (set_add(layer_set, &layer_node)){
				printf("ERROR: in %s, unable to add element to set\n", __func__);
				set_delete(layer_set);
				return -1;
			}
			else{
				ir_edge_layerSet_set(edge, layer_set);
				return 0;
			}
		}
		else{
			printf("ERROR: in %s, unable to create set\n", __func__);
			return -1;
		}
	}
	else{
		return set_add(layer_set, &layer_node);
	}
}

void ir_node_layerSet_rem(struct node* node, struct unode* layer_node){
	struct set* layer_set;

	layer_set = ir_node_layerSet_get(node);
	if (layer_set != NULL){
		set_remove(layer_set, &layer_node);
		if (layer_set->nb_element_tot == 0){
			free(layer_set);
			ir_node_layerSet_set(node, NULL);
		}
	}
}

void ir_edge_layerSet_rem(struct edge* edge, struct unode* layer_node){
	struct set* layer_set;

	layer_set = ir_edge_layerSet_get(edge);
	if (layer_set != NULL){
		set_remove(layer_set, &layer_node);
		if (layer_set->nb_element_tot == 0){
			free(layer_set);
			ir_edge_layerSet_set(edge, NULL);
		}
	}
}

int32_t irLayer_init(struct irLayer* layer){
	layer->node_set = set_create(sizeof(struct node*), IRLAYER_NB_NODE_PER_BLOCK);
	layer->edge_set = set_create(sizeof(struct edge*), IRLAYER_NB_EDGE_PER_BLOCK);

	if (layer->node_set == NULL || layer->edge_set == NULL){
		printf("ERROR: in %s, unable to create set\n", __func__);

		if (layer->node_set != NULL){
			set_delete(layer->node_set);
		}
		if (layer->edge_set != NULL){
			set_delete(layer->edge_set);
		}
	}

	return 0;
}

int32_t irlayer_add_node(struct unode* layer_node, struct node* node){
	struct irLayer* layer = graphLayer_node_get_layer(layer_node);

	if (set_add(layer->node_set, &node)){
		printf("ERROR: in %s, unable to add element to set\n", __func__);
		return -1;
	}

	if (ir_node_layerSet_add(node, layer_node)){
		printf("ERROR: in %s, unable to add layer to layerSet\n", __func__);
		set_remove(layer->node_set, &node);
		return -1;
	}

	return 0;
}

int32_t irlayer_add_edge(struct unode* layer_node, struct edge* edge){
	struct irLayer* layer = graphLayer_node_get_layer(layer_node);

	if (set_add(layer->edge_set, &edge)){
		printf("ERROR: in %s, unable to add element to set\n", __func__);
		return -1;
	}

	if (ir_edge_layerSet_add(edge, layer_node)){
		printf("ERROR: in %s, unable to add layer to layerSet\n", __func__);
		set_remove(layer->edge_set, &edge);
		return -1;
	}

	return 0;
}

void irLayer_commit(struct unode* layer_node){
	struct setIterator 	iterator;
	struct node** 		node_ptr;
	struct edge** 		edge_ptr;

	for (edge_ptr = (struct edge**)setIterator_get_first(graphLayer_node_get_layer(layer_node)->edge_set, &iterator); edge_ptr != NULL; edge_ptr = (struct edge**)setIterator_get_next(&iterator)){
		ir_edge_layerSet_rem(*edge_ptr, layer_node);
	}

	for (node_ptr = (struct node**)setIterator_get_first(graphLayer_node_get_layer(layer_node)->node_set, &iterator); node_ptr != NULL; node_ptr = (struct node**)setIterator_get_next(&iterator)){
		ir_node_layerSet_rem(*node_ptr, layer_node);
	}

	set_delete(graphLayer_node_get_layer(layer_node)->node_set);
	set_delete(graphLayer_node_get_layer(layer_node)->edge_set);
}

void irLayer_clean(struct unode* layer_node, struct ir* ir){
	struct setIterator 	iterator;
	struct node** 		node_ptr;
	struct node* 		node;
	struct edge** 		edge_ptr;
	struct edge* 		edge;

	for (edge_ptr = (struct edge**)setIterator_get_first(graphLayer_node_get_layer(layer_node)->edge_set, &iterator); edge_ptr != NULL; edge_ptr = (struct edge**)setIterator_get_next(&iterator)){
		edge = *edge_ptr;
		ir_edge_layerSet_rem(edge, layer_node);
		ir_remove_dependence(ir, edge);
	}

	for (node_ptr = (struct node**)setIterator_get_first(graphLayer_node_get_layer(layer_node)->node_set, &iterator); node_ptr != NULL; node_ptr = (struct node**)setIterator_get_next(&iterator)){
		node = *node_ptr;
		ir_node_layerSet_rem(node, layer_node);
		ir_remove_node(ir, node);
	}

	set_delete(graphLayer_node_get_layer(layer_node)->node_set);
	set_delete(graphLayer_node_get_layer(layer_node)->edge_set);
}

struct irEquivalenceClass* irEquivalenceClass_add_node(struct irEquivalenceClass* class, struct node* node){
	if (ir_node_equivalenceClass_get(node) != NULL){
		if (class != NULL){
			printf("ERROR: in %s, this case is not implemented yet\n", __func__);
		}
		else{
			class = ir_node_equivalenceClass_get(node);
		}
	}
	else{
		if (class != NULL){
			if (set_add(class, &node)){
				printf("ERROR: in %s, unable to add element to set\n", __func__);
				return NULL;
			}
		}
		else{
			class = irEquivalenceClass_create();
			if (class == NULL){
				printf("ERROR: in %s, unable to create irEquivalenceClass\n", __func__);
				return NULL;
			}
			else{
				if (set_add(class, &node)){
					printf("ERROR: in %s, unable to add element to set\n", __func__);
					irEquivalenceClass_delete(class);
					return NULL;
				}
				else{
					ir_node_equivalenceClass_set(node, class);
				}
			}
		}
	}

	return class;
}

void irEquivalenceClass_rem_node(struct node* node){
	struct irEquivalenceClass* class;

	class = ir_node_equivalenceClass_get(node);
	if (class != NULL){
		set_remove(class, &node);
		ir_node_equivalenceClass_set(node, NULL);

		if (irEquivalenceClass_get_nb_node(class) == 0){
			irEquivalenceClass_delete(class);
		}
	}
}

struct unode* graphLayer_new_layer(struct ugraph* ugraph){
	struct irLayer 	layer;
	struct unode* 	unode;

	if (irLayer_init(&layer)){
		printf("ERROR: in %s, unable to init irLayer\n", __func__);
		return NULL;
	}

	unode = graphLayer_add_layer(ugraph, &layer);
	if (unode == NULL){
		printf("ERROR: in %s, unable to element to array\n", __func__);
		set_delete(layer.node_set);
		set_delete(layer.edge_set);
	}

	return unode;
}