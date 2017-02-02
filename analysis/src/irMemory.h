#ifndef IRMEMORY_H
#define IRMEMORY_H

#include <stdint.h>

#include "ir.h"

struct node* irMemory_get_first(struct ir* ir);

static inline struct node* irMemory_get_address_node(struct node* node){
	struct edge* edge_cursor;

	for (edge_cursor = node_get_head_edge_dst(node); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
		if (ir_edge_get_dependence(edge_cursor)->type == IR_DEPENDENCE_TYPE_ADDRESS){
			return edge_get_src(edge_cursor);
		}
	}

	return NULL;
}

static inline struct edge* irMemory_get_address_edge(struct node* node){
	struct edge* edge_cursor;

	for (edge_cursor = node_get_head_edge_dst(node); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
		if (ir_edge_get_dependence(edge_cursor)->type == IR_DEPENDENCE_TYPE_ADDRESS){
			return edge_cursor;
		}
	}

	return NULL;
}

int32_t irMemory_simplify(struct ir* ir);
int32_t irMemory_simplify_concrete(struct ir* ir);
void irMemory_print_aliasing(struct ir* ir);
int32_t irMemory_coalesce(struct ir* ir);

#endif
