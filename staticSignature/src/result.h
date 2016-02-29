#ifndef RESULT_H
#define RESULT_H

#include <stdint.h>

#include "codeSignature.h"
#include "set.h"

struct virtualNode{
	struct node* 	node;
	struct result* 	result;
	uint32_t 		index;
};

#define virtualNode_get_node(virtual_node) (virtual_node).result->symbol_node_buffer[(virtual_node).index];
#define virtualNode_push(virtual_node) 								\
	if ((virtual_node).node == NULL){ 								\
		(virtual_node).node = virtualNode_get_node(virtual_node); 	\
	}

#define virtualNode_pop(virtual_node) 								\
	if ((virtual_node).result != NULL){ 							\
		(virtual_node).node = NULL; 								\
	}

struct signatureLink{
	struct virtualNode 		virtual_node;
	uint32_t 				edge_desc;
};

struct result{
	struct codeSignature* 	code_signature;
	uint32_t 				nb_occurrence;
	uint32_t 				nb_node_in;
	uint32_t 				nb_node_ou;
	uint32_t 				nb_node_intern;
	struct signatureLink* 	in_mapping_buffer;
	struct signatureLink* 	ou_mapping_buffer;
	struct virtualNode*		intern_node_buffer;
	struct node** 			symbol_node_buffer;
};

int32_t result_init(struct result* result, struct codeSignature* code_signature, struct array* assignement_array);

void result_push(struct result* result, struct ir* ir);
void result_pop(struct result* result, struct ir* ir);
void result_get_node_footprint(struct result* result, uint32_t index, struct set* set);
void result_remove_edge_footprint(struct result* result, struct ir* ir);
void result_print(struct result* result);

#define result_clean(result) 										\
	free((result)->in_mapping_buffer); 								\
	free((result)->ou_mapping_buffer); 								\
	free((result)->intern_node_buffer); 							\
	free((result)->symbol_node_buffer);

enum parameterSimilarity{
	PARAMETER_EQUAL 	= 0x00000000,
	PARAMETER_PERMUT 	= 0x00000001,
	PARAMETER_SUBSET 	= 0x00000003,
	PARAMETER_SUPERSET 	= 0x00000007,
	PARAMETER_OVERLAP 	= 0x0000000f,
	PARAMETER_DISJOINT 	= 0x0000001f
};

struct parameterMapping{
	uint32_t 					nb_fragment;
	struct node**				ptr_buffer;
	enum parameterSimilarity 	similarity;
};

enum parameterSimilarity parameterMapping_get_similarity(const struct parameterMapping* parameter_mapping1, const struct parameterMapping* parameter_mapping2);
void parameterMapping_print_location(const struct parameterMapping* mapping);

struct symbolMapping{
	uint32_t 					nb_parameter;
	struct parameterMapping* 	mapping_buffer;
};

struct symbolMapping* symbolMapping_create_from_result(struct result* result, uint32_t index);
struct symbolMapping* symbolMapping_create_from_ir(struct node* node);

int32_t symbolMapping_may_append(struct symbolMapping* mapping_dst, struct symbolMapping* mapping_src);

#endif