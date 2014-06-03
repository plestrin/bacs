#ifndef UNROLLEDLOOP_H
#define UNROLLEDLOOP_H

#include "ir.h"
#include "array.h"

void ir_search_unrolled(struct ir* ir);

#define MAPPING_SCORE_UNINIT 		-1
#define MAPPING_MAX_PARENT_NODE 	6

struct mapping{
	struct node*					node_x;
	struct node* 					node_y;
	struct nodeMappingContainer* 	container;
};

#define mapping_is_valid(mapping) 		((mapping)->container != NULL)
#define mapping_set_invalid(mapping) 	((mapping)->container = NULL)

struct divergentMapping{
	struct node* 					node_x;
	struct node* 					node_y;
	uint32_t 						num_factor;
	uint32_t 						den_factor;
	struct nodeMappingContainer* 	container;
};

struct nodeMappingContainer{
	int32_t 						score;
	int32_t 						div_score;
	uint32_t 						parent_mapping_offset;
	uint32_t 						parent_nb_mapping;
	uint32_t 						parent_div_mapping_offset;
	uint32_t 						parent_nb_div_mapping;
};

struct opcodeMappingContainer{
	void* 							buffer;
	uint32_t 						nb_element;
	struct nodeMappingContainer* 	node_mapping_container;
	struct node**					node_list;
};

struct irMappingContainer{
	struct opcodeMappingContainer 	opcode_mapping_container[IR_NB_OPCODE];
	struct array 					mapping_array;
	struct array 					divergence_array;
};

struct mappingLLElement{
	struct node* 					node_prev;
	struct node* 					node_next;
	int32_t 						root;
};

struct mappingLLRoot{
	struct mappingLLElement* 		head;
	uint32_t 						nb_element;
};

struct mappingResult{
	struct array 					mapping_ll_elem;
	struct array 					mapping_ll_root;
};

struct irMappingContainer* irMappingContainer_create(struct ir* ir);
int32_t irMappingContainer_init(struct irMappingContainer* ir_mapping_container, struct ir* ir);
void irMappingContainer_compute(struct irMappingContainer* ir_mapping_container);
int32_t irMappingContainer_map(struct irMappingContainer* ir_mapping_container, struct node* node1, struct node* node2, struct mapping* result_mapping);
struct mappingResult* irMappingContainer_extract_result(struct irMappingContainer* ir_mapping_container, struct ir* ir);
void irMappingContainer_print_node_number(struct ir* ir);
void irMappingContainer_print_mapping_score(struct irMappingContainer* ir_mapping_container, struct ir* ir);
void irMappingContainer_clean(struct irMappingContainer* ir_mapping_container);

void mappingResult_add_mapping(struct mappingResult* mapping_result, struct irMappingContainer* ir_mapping_container, struct mapping* mapping);

#define mappingResult_delete(mapping_result) 															\
	array_clean(&((mapping_result)->mapping_ll_elem)); 													\
	array_clean(&((mapping_result)->mapping_ll_root)); 													\
	free(mapping_result)

void mappingResult_print_color(struct mappingResult* mapping_result, struct ir* ir);

#define irMappingContainer_delete(ir_mapping_container) 												\
	irMappingContainer_clean(ir_mapping_container); 													\
	free(ir_mapping_container)

#endif