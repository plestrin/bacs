#ifndef IRSATURATE_H
#define IRSATURATE_H

#include <stdint.h>

#include "array.h"
#include "ir.h"
#include "codeSignature.h"

#define IRSATURATE_ASSOSEQ_MAX_INPUT 	5
#define IRSATURATE_ASSOSEQ_MAX_OUTPUT	2

#define irSaturate_opcode_is_associative(opcode) (((opcode) == IR_ADD) || ((opcode) == IR_XOR))

struct assoSeq{
	enum irOpcode 	opcode;
	uint32_t 		nb_input;
	uint32_t 		buffer_input[IRSATURATE_ASSOSEQ_MAX_INPUT];
	uint32_t 		nb_output;
	uint32_t 		buffer_output[IRSATURATE_ASSOSEQ_MAX_OUTPUT];
	uint32_t 		nb_node;
};

struct saturateRules{
	struct array 	asso_seq_array;
};

#define saturateRules_init(saturate_rules) 										\
	array_init(&((saturate_rules)->asso_seq_array), sizeof(struct assoSeq))

void saturateRules_learn_associative_conflict(struct saturateRules* saturate_rules, struct codeSignatureCollection* collection);
void saturateRules_print(struct saturateRules* saturate_rules);

void irSaturate_saturate(struct saturateRules* saturate_rules, struct ir* ir);
#define irSaturate_unsaturate(ir)  												\
	saturateLayer_reset(ir, &(ir->saturate_layer));

#define saturateRules_reset(saturate_rules) 									\
	array_empty(&((saturate_rules)->asso_seq_array));

#define saturateRules_clean(saturate_rules) 									\
	array_clean(&((saturate_rules)->asso_seq_array));

enum saturateElement_type{
	SATURATE_ELEMENT_NODE,
	SATURATE_ELEMENT_EDGE,
	SATURATE_ELEMENT_INVALID
};

struct saturateElement{
	enum saturateElement_type 		type;
	union {
		struct {
			enum irOperationType 	type;
			enum irOpcode 			opcode;
			uint8_t 				size;
			struct node*			ptr;

		} 							node;
		struct {
			enum irDependenceType 	type;
			int32_t 				src_id;
			struct node*			src_ptr;
			int32_t 				dst_id;
			struct node*			dst_ptr;
			struct edge*			ptr;
		} 							edge;
	} 								element_type;
};

#define saturateLayer_init(layer)												\
	array_init(layer, sizeof(struct saturateElement))

#define saturateLayer_is_empty(layer) (array_get_length(layer) == 0)

static inline int32_t saturateLayer_push_operation(struct array* layer, enum irOpcode opcode){
	struct saturateElement element;

	element.type 						= SATURATE_ELEMENT_NODE;
	element.element_type.node.type 		= IR_OPERATION_TYPE_INST;
	element.element_type.node.opcode 	= opcode;
	element.element_type.node.size 		= 32;
	element.element_type.node.ptr 		= NULL;

	return array_add(layer, &element);
}

static inline void saturateLayer_push_edge(struct array* layer, int32_t src_id, struct node* src_ptr, int32_t dst_id, struct node* dst_ptr){
	struct saturateElement element;

	if ((src_id < 0 && src_ptr == NULL) || (dst_id < 0 && dst_ptr == NULL)){
		printf("ERROR: in %s, incorrect element id for src or dst\n", __func__);
		return;
	}

	element.type 						= SATURATE_ELEMENT_EDGE;
	element.element_type.edge.type 		= IR_DEPENDENCE_TYPE_DIRECT;
	element.element_type.edge.src_id 	= src_id;
	element.element_type.edge.src_ptr 	= src_ptr;
	element.element_type.edge.dst_id 	= dst_id;
	element.element_type.edge.dst_ptr 	= dst_ptr;
	element.element_type.edge.ptr 		= NULL;

	if (array_add(layer, &element) < 0){
		printf("ERROR: in %s, unable to add edge to saturation layer\n", __func__);
	}
}

void saturateLayer_remove_redundant_element(struct array* layer);

void saturateLayer_commit(struct ir* ir, struct array* layer);

void saturateLayer_remove(struct ir* ir, struct array* layer);

#define saturateLayer_reset(ir, layer) 											\
	saturateLayer_remove(ir, layer); 											\
	array_empty(layer);

#define saturateLayer_clean(ir, layer)											\
	saturateLayer_remove(ir, layer); 											\
	array_clean(layer);

#endif