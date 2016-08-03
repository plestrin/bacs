#ifndef IRMEMORY_H
#define IRMEMORY_H

#include <stdint.h>

#include "ir.h"

enum aliasType{
	ALIAS_IN,
	ALIAS_OUT,
	ALIAS_ALL
};

struct node* ir_normalize_search_alias_conflict(struct node* node1, struct node* node2, enum aliasType alias_type, uint32_t ir_range_seed);

void ir_simplify_concrete_memory_access(struct ir* ir, uint8_t* modification);

struct node* irMemory_get_first(struct ir* ir);

static inline struct node* irMemory_get_address(struct node* node){
	struct edge* edge_cursor;

	for (edge_cursor = node_get_head_edge_dst(node); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
		if (ir_edge_get_dependence(edge_cursor)->type == IR_DEPENDENCE_TYPE_ADDRESS){
			return edge_get_src(edge_cursor);
		}
	}

	return NULL;
}

int32_t irMemory_simplify(struct ir* ir);
void irMemory_print_aliasing(struct ir* ir);

#endif