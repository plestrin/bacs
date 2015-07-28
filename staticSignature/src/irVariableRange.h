#ifndef IRVARIABLERANGE_H
#define IRVARIABLERANGE_H

#include <stdint.h>

#include "graph.h"

struct irVariableRange{
	uint64_t lower_bound;
	uint64_t upper_bound;
};

#define irVariableRange_is_cst(range) ((range).lower_bound == (range).upper_bound)
#define irVariableRange_cst_equal(range1, range2) ((range1).lower_bound == (range2).lower_bound)

void irVariableRange_init_range(struct irVariableRange* range_dst, struct irVariableRange* range_src);
void irVariableRange_init_size(struct irVariableRange* range_dst, uint32_t size);
void irVariableRange_init_constant(struct irVariableRange* range_dst, uint64_t constant);
void irVariableRange_add_range(struct irVariableRange* range_dst, struct irVariableRange* range_src, uint32_t modulo);
void irVariableRange_and_range(struct irVariableRange* range_dst, struct irVariableRange* range_src, uint32_t modulo);
void irVariableRange_shr_range(struct irVariableRange* range_dst, struct irVariableRange* range_src, uint32_t modulo);

void irVariableRange_compute(struct node* node, struct irVariableRange* range);

void irVariableRange_get_range_additive_list(struct irVariableRange* dst_range, struct node** node_buffer, uint32_t nb_node);

uint32_t irVariableRange_intersect(struct irVariableRange* range1, struct irVariableRange* range2);

void irVariableRange_print(struct irVariableRange* range);

#endif