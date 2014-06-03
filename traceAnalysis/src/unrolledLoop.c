#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <alloca.h>

#include "unrolledLoop.h"
#include "permutation.h"

int32_t compare_divergentMapping(const void* arg1, const void* arg2);

void irMapping_dotPrint_node_number(void* data, FILE* file, void* arg);
void irMapping_dotPrint_node_color(void* data, FILE* file, void* arg);
void irMapping_dotPrint_edge(void* data, FILE* file, void* arg);
void irMapping_dotPrint_mapping(FILE* file, void* arg);


struct irMappingContainer* irMappingContainer_create(struct ir* ir){
	struct irMappingContainer* ir_mapping_container;

	ir_mapping_container = (struct irMappingContainer*)malloc(sizeof(struct irMappingContainer));
	if (ir_mapping_container != NULL){
		if (irMappingContainer_init(ir_mapping_container, ir)){
			printf("ERROR: in %s, unable to init irMappingContainer\n", __func__);
			free(ir_mapping_container);
			ir_mapping_container = NULL;
		}
	}
	else{
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
	}

	return ir_mapping_container;
}

int32_t irMappingContainer_init(struct irMappingContainer* ir_mapping_container, struct ir* ir){
	struct node* 					node;
	struct irOperation* 			operation;
	struct opcodeMappingContainer* 	opcode_mapping_container;
	uint32_t 						i;
	uint32_t 						j;
	uint32_t 						node_mapping_size;

	if (array_init(&(ir_mapping_container->mapping_array), sizeof(struct mapping))){
		printf("ERROR: in %s, unable to init array\n", __func__);
		return -1;
	}

	if (array_init(&(ir_mapping_container->divergence_array), sizeof(struct divergentMapping))){
		printf("ERROR: in %s, unable to init array\n", __func__);
		array_clean(&(ir_mapping_container->mapping_array));
		return -1;
	}

	for (i = 0; i < IR_NB_OPCODE; i++){
		ir_mapping_container->opcode_mapping_container[i].nb_element 				= 0;
		ir_mapping_container->opcode_mapping_container[i].buffer 					= NULL;
		ir_mapping_container->opcode_mapping_container[i].node_mapping_container 	= NULL;
		ir_mapping_container->opcode_mapping_container[i].node_list 				= NULL;
	}

	node = graph_get_head_node(&(ir->graph));
	while(node != NULL){
		operation = ir_node_get_operation(node);
		switch(operation->type){
			case IR_OPERATION_TYPE_INPUT :{
				break;
			}
			case IR_OPERATION_TYPE_OUTPUT : {
				operation->data = ir_mapping_container->opcode_mapping_container[operation->operation_type.output.opcode].nb_element ++;
				break;
			}
			case IR_OPERATION_TYPE_INNER : {
				operation->data = ir_mapping_container->opcode_mapping_container[operation->operation_type.inner.opcode].nb_element ++;
				break;
			}
		}
		node = node_get_next(node);
	}

	for (i = 0; i < IR_NB_OPCODE; i++){
		opcode_mapping_container = ir_mapping_container->opcode_mapping_container + i;
		if (opcode_mapping_container->nb_element > 1){
			node_mapping_size = (opcode_mapping_container->nb_element * (opcode_mapping_container->nb_element - 1)) / 2;
			opcode_mapping_container->buffer = malloc(sizeof(struct nodeMappingContainer) * node_mapping_size + opcode_mapping_container->nb_element * sizeof(struct node*));
			if (opcode_mapping_container->buffer == NULL){
				printf("ERROR: in %s, unable to allocate memory\n", __func__);
				irMappingContainer_clean(ir_mapping_container);
				return -1;
			}

			opcode_mapping_container->node_mapping_container = (struct nodeMappingContainer*)opcode_mapping_container->buffer;
			opcode_mapping_container->node_list = (struct node**)((char *)opcode_mapping_container->buffer + sizeof(struct nodeMappingContainer) * node_mapping_size);
			for (j = 0; j < node_mapping_size; j++){
				opcode_mapping_container->node_mapping_container[j].score = MAPPING_SCORE_UNINIT;
			}
		}
	}

	node = graph_get_head_node(&(ir->graph));
	while(node != NULL){
		operation = ir_node_get_operation(node);
		switch(operation->type){
			case IR_OPERATION_TYPE_INPUT :{
				break;
			}
			case IR_OPERATION_TYPE_OUTPUT : {
				if (ir_mapping_container->opcode_mapping_container[operation->operation_type.output.opcode].nb_element > 1){
					ir_mapping_container->opcode_mapping_container[operation->operation_type.output.opcode].node_list[operation->data] = node;
				}
				break;
			}
			case IR_OPERATION_TYPE_INNER : {
				if (ir_mapping_container->opcode_mapping_container[operation->operation_type.inner.opcode].nb_element > 1){
					ir_mapping_container->opcode_mapping_container[operation->operation_type.inner.opcode].node_list[operation->data] = node;
				}
				break;
			}
		}
		node = node_get_next(node);
	}

	return 0;
}

void irMappingContainer_compute(struct irMappingContainer* ir_mapping_container){
	uint32_t i;
	uint32_t x;
	uint32_t y;
	uint32_t index;

	for (i = 0; i < IR_NB_OPCODE; i++){
		for (y = 0; y < ir_mapping_container->opcode_mapping_container[i].nb_element; y++){
			for (x = 0; x < y; x++){
				index = x + (y * (y - 1)) / 2;
				if (ir_mapping_container->opcode_mapping_container[i].node_mapping_container[index].score == MAPPING_SCORE_UNINIT){
					ir_mapping_container->opcode_mapping_container[i].node_mapping_container[index].score = irMappingContainer_map(ir_mapping_container, ir_mapping_container->opcode_mapping_container[i].node_list[x], ir_mapping_container->opcode_mapping_container[i].node_list[y], NULL);
				}
			}
		}
	}
}

int32_t irMappingContainer_map(struct irMappingContainer* ir_mapping_container, struct node* node1, struct node* node2, struct mapping* result_mapping){
	struct irOperation* 	operation1;
	struct irOperation* 	operation2;
	enum irOpcode 			opcode;
	uint32_t				nb_parent;
	uint32_t 				index;
	uint8_t 				i;
	struct node* 			node_x;
	struct node* 			node_y;
	struct irOperation* 	operation_x;
	struct irOperation* 	operation_y;

	/* STEP 1: match the opcode */
	operation1 = ir_node_get_operation(node1);
	switch(operation1->type){
		case IR_OPERATION_TYPE_INPUT :{
			if (result_mapping != NULL){
				mapping_set_invalid(result_mapping);
			}
			return 0;
		}
		case IR_OPERATION_TYPE_OUTPUT : {
			opcode = operation1->operation_type.output.opcode;
			break;
		}
		case IR_OPERATION_TYPE_INNER : {
			opcode = operation1->operation_type.inner.opcode;
			break;
		}
	}

	operation2 = ir_node_get_operation(node2);
	switch(operation2->type){
		case IR_OPERATION_TYPE_INPUT :{
			if (result_mapping != NULL){
				mapping_set_invalid(result_mapping);
			}
			return 0;
		}
		case IR_OPERATION_TYPE_OUTPUT : {
			if (opcode != operation2->operation_type.output.opcode){
				if (result_mapping != NULL){
					mapping_set_invalid(result_mapping);
				}
				return 0;
			}
			break;
		}
		case IR_OPERATION_TYPE_INNER : {
			if (opcode != operation2->operation_type.inner.opcode){
				if (result_mapping != NULL){
					mapping_set_invalid(result_mapping);
				}
				return 0;
			}
			break;
		}
	}

	/* STEP 2: try to read corresponding mapping */
	if (operation1->data > operation2->data){
		node_x = node2;
		node_y = node1;
	}
	else if (operation1->data < operation2->data){
		node_x = node1;
		node_y = node2;
	}
	else{
		if (result_mapping != NULL){
			mapping_set_invalid(result_mapping);
		}
		return 1;
	}

	operation_x = ir_node_get_operation(node_x);
	operation_y = ir_node_get_operation(node_y);
	index = operation_x->data + (operation_y->data * (operation_y->data - 1)) / 2;
	if (ir_mapping_container->opcode_mapping_container[opcode].node_mapping_container[index].score != MAPPING_SCORE_UNINIT){
		goto exit;
	}

	/* STEP 3: match the number of parent */
	nb_parent = node_x->nb_edge_dst;
	if (nb_parent != node_y->nb_edge_dst){
		ir_mapping_container->opcode_mapping_container[opcode].node_mapping_container[index].score 						= 1;
		ir_mapping_container->opcode_mapping_container[opcode].node_mapping_container[index].parent_mapping_offset 		= 0;
		ir_mapping_container->opcode_mapping_container[opcode].node_mapping_container[index].parent_nb_mapping 			= 0;
		ir_mapping_container->opcode_mapping_container[opcode].node_mapping_container[index].parent_div_mapping_offset 	= 0;
		ir_mapping_container->opcode_mapping_container[opcode].node_mapping_container[index].parent_nb_div_mapping 		= 0;
		goto exit;
	}

	/* STEP 5: evaluate possible matching for the parent(s) and pick the best */
	if (nb_parent <= MAPPING_MAX_PARENT_NODE){
		void* 						buffer 						= calloc((sizeof(struct mapping) + sizeof(uint32_t) + ((sizeof(uint8_t) * 2 + sizeof(struct mapping) >  sizeof(struct divergentMapping)) ? (sizeof(uint8_t) * 2 + sizeof(struct mapping)) : sizeof(struct divergentMapping))) * nb_parent, 1);
		uint8_t* 					permutation;
		int32_t 					score;
		int32_t 					div_score;
		struct mapping* 			parent_mappings 			= (struct mapping*)((char*)buffer + (sizeof(struct mapping) + sizeof(uint32_t)) * nb_parent);
		int32_t 					max_score 					= 0;
		int32_t 					max_div_score 				= 0;
		struct mapping* 			max_parent_mappings 		= (struct mapping*)buffer;
		struct divergentMapping* 	divergent_parent 			= (struct divergentMapping*)((char*)buffer + (sizeof(struct mapping) + sizeof(uint32_t)) * nb_parent);
		uint32_t 					nb_divergent_parent 		= 0;
		uint32_t 					nb_parent_divergence_list 	= 0;
		uint32_t* 					divergent_offset 			= (uint32_t*)((char*)buffer + sizeof(struct mapping) * nb_parent);

		PERMUTATION_INIT(nb_parent, (char*)buffer + (sizeof(struct mapping) * 2 + sizeof(uint32_t)) * nb_parent, (char*)buffer + (sizeof(struct mapping) * 2 + sizeof(uint32_t) + sizeof(uint8_t)) * nb_parent)

		PERMUTATION_GET_FIRST(permutation)
		while(permutation != NULL){
			struct divergentMapping* 	current_divergent;
			struct divergentMapping* 	smallest_divergent 			= (struct divergentMapping*)0xffffffff;
			uint32_t* 					smallest_offset_ptr;

			for (i = 0, score = 0, div_score = 0; i < nb_parent; i++){
				struct node* 	prt_node1;
				struct node* 	prt_node2;
				int32_t 		value;

				prt_node1 = edge_get_src(node_get_edge_dst(node_x, permutation[i]));
				prt_node2 = edge_get_src(node_get_edge_dst(node_y, i));

				value = irMappingContainer_map(ir_mapping_container, prt_node1, prt_node2, parent_mappings + i);
				score += value;

				if (mapping_is_valid(parent_mappings + i) && parent_mappings[i].node_x->nb_edge_src <= 1 && parent_mappings[i].node_y->nb_edge_src <= 1){
					div_score += parent_mappings[i].container->div_score;
				}
				else{
					div_score += (value == 1)? 1 : 0;
				}
			}

			memset(divergent_offset, 0, sizeof(uint32_t) * nb_parent);
			while (smallest_divergent != NULL){
				for (i = 0, smallest_divergent = NULL; i < nb_parent; i++){
					if (mapping_is_valid(parent_mappings + i)){
						if (divergent_offset[i] < parent_mappings[i].container->parent_nb_div_mapping){
							current_divergent = (struct divergentMapping*)array_get(&(ir_mapping_container->divergence_array), divergent_offset[i] + parent_mappings[i].container->parent_div_mapping_offset);
							if (smallest_divergent == NULL){
								smallest_divergent = current_divergent;
								smallest_offset_ptr = divergent_offset + i;
							}
							else{
								if (smallest_divergent->node_x == current_divergent->node_x && smallest_divergent->node_y == current_divergent->node_y){
									divergent_offset[i] ++;
									score -= current_divergent->container->div_score;
									printf("WARNING: in %s, found duplicate: removing %d (x: %p, y: %p)\n", __func__, current_divergent->container->div_score, (void*)ir_node_get_operation(current_divergent->node_x), (void*)ir_node_get_operation(current_divergent->node_y));
								}
								else if (smallest_divergent->node_x > current_divergent->node_x || (smallest_divergent->node_x == current_divergent->node_x && smallest_divergent->node_y > current_divergent->node_y)){
									smallest_divergent = current_divergent;
									smallest_offset_ptr = divergent_offset + i;
								}
							}
						}
					}
				}
				if (smallest_divergent != NULL){
					for (i = 0; i < nb_parent; i++){
						if (mapping_is_valid(parent_mappings + i)){
							if (smallest_divergent->node_x == parent_mappings[i].node_x && smallest_divergent->node_y == parent_mappings[i].node_y){
								score -= smallest_divergent->container->div_score;
								printf("WARNING: in %s, found duplicate: removing %d (x: %p, y: %p)\n", __func__, smallest_divergent->container->div_score, (void*)ir_node_get_operation(smallest_divergent->node_x), (void*)ir_node_get_operation(smallest_divergent->node_y));
							}
						}
					}
					*smallest_offset_ptr = *smallest_offset_ptr + 1;
				}
			}

			if (score > max_score){
				max_score = score;
				max_div_score = div_score;
				memcpy(max_parent_mappings, parent_mappings, sizeof(struct mapping) * nb_parent);
			}

			PERMUTATION_GET_NEXT(permutation)
		}

		/* STEP 6: save the best mapping in the container */
		ir_mapping_container->opcode_mapping_container[opcode].node_mapping_container[index].score 						= max_score + 1;
		ir_mapping_container->opcode_mapping_container[opcode].node_mapping_container[index].div_score 					= max_div_score + 1;
		ir_mapping_container->opcode_mapping_container[opcode].node_mapping_container[index].parent_mapping_offset 		= array_get_length(&(ir_mapping_container->mapping_array));
		ir_mapping_container->opcode_mapping_container[opcode].node_mapping_container[index].parent_div_mapping_offset 	= array_get_length(&(ir_mapping_container->divergence_array));

		for (i = 0; i < nb_parent; i++){
			if (mapping_is_valid(max_parent_mappings + i)){
				if (array_add(&(ir_mapping_container->mapping_array), max_parent_mappings + i) < 0){
					printf("ERROR: in %s, unable to add mapping to array\n", __func__);
				}

				if (max_parent_mappings[i].node_x->nb_edge_src > 1 || max_parent_mappings[i].node_y->nb_edge_src > 1){
					divergent_parent[nb_divergent_parent].node_x 		= max_parent_mappings[i].node_x;
					divergent_parent[nb_divergent_parent].node_y 		= max_parent_mappings[i].node_y;
					divergent_parent[nb_divergent_parent].num_factor 	= 1;
					divergent_parent[nb_divergent_parent].den_factor 	= (max_parent_mappings[i].node_x->nb_edge_src > max_parent_mappings[i].node_y->nb_edge_src) ? max_parent_mappings[i].node_x->nb_edge_src : max_parent_mappings[i].node_y->nb_edge_src;
					divergent_parent[nb_divergent_parent].container 	= max_parent_mappings[i].container;
					nb_divergent_parent ++;
				}

				if (max_parent_mappings[i].container->parent_nb_div_mapping > 0){
					nb_parent_divergence_list ++;
				}
			}
		}

		if (nb_divergent_parent > 1){
			qsort (divergent_parent, nb_divergent_parent, sizeof(struct divergentMapping), compare_divergentMapping);
            for (i = 1; i < nb_divergent_parent; i++){
            	if (divergent_parent[i].node_x == divergent_parent[i - 1].node_x && divergent_parent[i].node_y == divergent_parent[i - 1].node_y){
            		printf("WARNING: in %s, this case is not implemented yet: duplicate in sorted divergence parent list\n", __func__);
            	}
            }
		}

		if (nb_divergent_parent == 0 && nb_parent_divergence_list < 2){
			for (i = 0; i < nb_parent; i++){
				if (mapping_is_valid(max_parent_mappings + i)){
					if (max_parent_mappings[i].container->parent_nb_div_mapping > 0){
						ir_mapping_container->opcode_mapping_container[opcode].node_mapping_container[index].parent_div_mapping_offset 	= max_parent_mappings[i].container->parent_div_mapping_offset;
						ir_mapping_container->opcode_mapping_container[opcode].node_mapping_container[index].parent_nb_mapping 			= array_get_length(&(ir_mapping_container->mapping_array)) - ir_mapping_container->opcode_mapping_container[opcode].node_mapping_container[index].parent_mapping_offset;
						ir_mapping_container->opcode_mapping_container[opcode].node_mapping_container[index].parent_nb_div_mapping 		= max_parent_mappings[i].container->parent_nb_div_mapping;
						
						free(buffer);
						goto exit;
					}
				}
			}
		}
		else{
			struct divergentMapping* 	current_divergent;
			struct divergentMapping* 	smallest_divergent 			= (struct divergentMapping*)0xffffffff;
			uint32_t* 					smallest_offset_ptr;
			uint32_t 					divergent_parent_offset 	= 0;

			memset(divergent_offset, 0, sizeof(uint32_t) * nb_parent);
			while (smallest_divergent != NULL){
				for (i = 0, smallest_divergent = NULL; i < nb_parent; i++){
					if (mapping_is_valid(max_parent_mappings + i)){
						if (divergent_offset[i] < max_parent_mappings[i].container->parent_nb_div_mapping){
							current_divergent = (struct divergentMapping*)array_get(&(ir_mapping_container->divergence_array), divergent_offset[i] + max_parent_mappings[i].container->parent_div_mapping_offset);
							if (smallest_divergent == NULL){
								smallest_divergent = current_divergent;
								smallest_offset_ptr = divergent_offset + i;
							}
							else if (smallest_divergent->node_x > current_divergent->node_x || (smallest_divergent->node_x == current_divergent->node_x && smallest_divergent->node_y > current_divergent->node_y)){
								smallest_divergent = current_divergent;
								smallest_offset_ptr = divergent_offset + i;
							}
						}
					}
				}
				if (divergent_parent_offset < nb_divergent_parent){
					if (smallest_divergent == NULL){
						smallest_divergent = divergent_parent + divergent_parent_offset;
						smallest_offset_ptr = &divergent_parent_offset;
					}
					else if (smallest_divergent->node_x > divergent_parent[divergent_parent_offset].node_x || (smallest_divergent->node_x == divergent_parent[divergent_parent_offset].node_x && smallest_divergent->node_y > divergent_parent[divergent_parent_offset].node_y)){
						smallest_divergent = divergent_parent + divergent_parent_offset;
						smallest_offset_ptr = &divergent_parent_offset;
					}
				}

				if (smallest_divergent != NULL){
					uint32_t new_num_factor = smallest_divergent->num_factor;
					uint32_t new_den_factor = smallest_divergent->den_factor;

					if ((divergent_parent_offset < nb_divergent_parent) && (smallest_divergent != divergent_parent + divergent_parent_offset) && (smallest_divergent->node_x == divergent_parent[divergent_parent_offset].node_x && smallest_divergent->node_y == divergent_parent[divergent_parent_offset].node_y)){
						if (new_den_factor == divergent_parent[divergent_parent_offset].den_factor){
							new_num_factor += divergent_parent[divergent_parent_offset].num_factor;
						}
						else{
							new_num_factor = (new_num_factor * divergent_parent[divergent_parent_offset].den_factor) + (divergent_parent[divergent_parent_offset].num_factor * new_den_factor);
							new_den_factor = new_den_factor * divergent_parent[divergent_parent_offset].den_factor;
						}
						divergent_parent_offset ++;
						printf("WARNING: in %s, merging div case (%u/%u)\n", __func__, new_num_factor, new_den_factor);
					}
					if ((divergent_parent_offset >= nb_divergent_parent) || (smallest_divergent != divergent_parent + divergent_parent_offset)){
						if (max_parent_mappings[smallest_offset_ptr - divergent_offset].node_x->nb_edge_src > 1 || max_parent_mappings[smallest_offset_ptr - divergent_offset].node_y->nb_edge_src > 1){
							new_den_factor = new_den_factor * ((max_parent_mappings[smallest_offset_ptr - divergent_offset].node_x->nb_edge_src > max_parent_mappings[smallest_offset_ptr - divergent_offset].node_y->nb_edge_src) ? max_parent_mappings[smallest_offset_ptr - divergent_offset].node_x->nb_edge_src : max_parent_mappings[smallest_offset_ptr - divergent_offset].node_y->nb_edge_src);
						}
					}
					for (i = 0; i < nb_parent; i++){
						if (mapping_is_valid(max_parent_mappings + i)){
							if (divergent_offset[i] < max_parent_mappings[i].container->parent_nb_div_mapping){
								current_divergent = (struct divergentMapping*)array_get(&(ir_mapping_container->divergence_array), divergent_offset[i] + max_parent_mappings[i].container->parent_div_mapping_offset);
								if ((current_divergent != smallest_divergent) && (smallest_divergent->node_x == current_divergent->node_x && smallest_divergent->node_y == current_divergent->node_y)){
									if (max_parent_mappings[i].node_x->nb_edge_src > 1 || max_parent_mappings[i].node_y->nb_edge_src > 1){
										printf("WARNING: in %s, this case is not implemented yet: update dem 2\n", __func__);
									}
									if (new_den_factor == current_divergent->den_factor){
										new_num_factor += current_divergent->num_factor;
									}
									else{
										new_num_factor = (new_num_factor * current_divergent->den_factor) + (current_divergent->num_factor * new_den_factor);
										new_den_factor = new_den_factor * current_divergent->den_factor;
									}
									divergent_offset[i] ++;
									printf("WARNING: in %s, merging div case (%u/%u)\n", __func__, new_num_factor, new_den_factor);
								}
							}
						}
					}

					if (new_num_factor != new_den_factor){
						int32_t div_handle;

						div_handle = array_add(&(ir_mapping_container->divergence_array), smallest_divergent);
						if (div_handle < 0){
							printf("ERROR: in %s, unable to add element to the divergence_array\n", __func__);
						}
						else{
							smallest_divergent = (struct divergentMapping*)array_get(&(ir_mapping_container->divergence_array), div_handle);
							smallest_divergent->num_factor = new_num_factor;
							smallest_divergent->den_factor = new_den_factor;
						}
					}
					else{
						printf("WARNING: in %s, removing divergence\n", __func__);
					}
					*smallest_offset_ptr = *smallest_offset_ptr + 1;
				}
			}
		}

		free(buffer);

		ir_mapping_container->opcode_mapping_container[opcode].node_mapping_container[index].parent_nb_mapping 		= array_get_length(&(ir_mapping_container->mapping_array)) - ir_mapping_container->opcode_mapping_container[opcode].node_mapping_container[index].parent_mapping_offset;
		ir_mapping_container->opcode_mapping_container[opcode].node_mapping_container[index].parent_nb_div_mapping 	= array_get_length(&(ir_mapping_container->divergence_array)) - ir_mapping_container->opcode_mapping_container[opcode].node_mapping_container[index].parent_div_mapping_offset;
	}
	else{
		printf("ERROR: in %s, unable to evaluate mapping too many parents: %u\n", __func__, nb_parent);
		if (result_mapping != NULL){
			mapping_set_invalid(result_mapping);
		}
		return 0;
	}
	
	exit:
	if (result_mapping != NULL){
		result_mapping->node_x 		= node_x;
		result_mapping->node_y 		= node_y;
		result_mapping->container 	= ir_mapping_container->opcode_mapping_container[opcode].node_mapping_container + index;
	}
	return ir_mapping_container->opcode_mapping_container[opcode].node_mapping_container[index].score;
}

void mappingResult_add_mapping(struct mappingResult* mapping_result, struct irMappingContainer* ir_mapping_container, struct mapping* mapping){
	struct irOperation* 			operation_x;
	struct irOperation* 			operation_y;
	struct mappingLLElement* 		mapping_ll_x;
	struct mappingLLElement* 		mapping_ll_y;
	uint32_t 						i;

	/* STEP 1: add the current mapping to result structure */
	operation_x = ir_node_get_operation(mapping->node_x);
	operation_y = ir_node_get_operation(mapping->node_y);


	/* pour le debug */
	printf("%d: ", mapping->container->score);
	switch(operation_x->type){
		case IR_OPERATION_TYPE_OUTPUT 	: {
			printf("%s %p", irOpcode_2_string(operation_x->operation_type.output.opcode), (void*)operation_x);
			break;
		}
		case IR_OPERATION_TYPE_INNER 	: {
			printf("%s %p", irOpcode_2_string(operation_x->operation_type.inner.opcode), (void*)operation_x);
			break;
		}
		default : {
			printf("ERROR: in %s, this case is not suppose to happen\n", __func__);
			break;
		}
	}
	printf(" - ");
	switch(operation_y->type){
		case IR_OPERATION_TYPE_OUTPUT 	: {
			printf("%s %p", irOpcode_2_string(operation_y->operation_type.output.opcode), (void*)operation_y);
			break;
		}
		case IR_OPERATION_TYPE_INNER 	: {
			printf("%s %p", irOpcode_2_string(operation_y->operation_type.inner.opcode), (void*)operation_y);
			break;
		}
		default : {
			printf("ERROR: in %s, this case is not suppose to happen\n", __func__);
			break;
		}
	}
	printf("\n");

	if (operation_x->data == 0xffffffff){
		struct mappingLLElement new_mapping_ll_x;
		int32_t 				index_mapping_ll_x;

		new_mapping_ll_x.node_prev 	= NULL;
		new_mapping_ll_x.node_next 	= NULL;
		new_mapping_ll_x.root 		= -1;

		index_mapping_ll_x = array_add(&(mapping_result->mapping_ll_elem), &new_mapping_ll_x);
		if (index_mapping_ll_x < 0){
			printf("ERROR: in %s, unable to add element to array\n", __func__);
			return;
		}
		operation_x->data = index_mapping_ll_x;
	}

	if (operation_y->data == 0xffffffff){
		struct mappingLLElement new_mapping_ll_y;
		int32_t 				index_mapping_ll_y;

		new_mapping_ll_y.node_prev 	= NULL;
		new_mapping_ll_y.node_next 	= NULL;
		new_mapping_ll_y.root 		= -1;

		index_mapping_ll_y = array_add(&(mapping_result->mapping_ll_elem), &new_mapping_ll_y);
		if (index_mapping_ll_y < 0){
			printf("ERROR: in %s, unable to add element to array\n", __func__);
			return;
		}
		operation_y->data = index_mapping_ll_y;
	}

	mapping_ll_x = array_get(&(mapping_result->mapping_ll_elem), operation_x->data);
	mapping_ll_y = array_get(&(mapping_result->mapping_ll_elem), operation_y->data);

	if (mapping_ll_x->node_next != NULL && mapping_ll_y->node_prev != NULL && mapping_ll_x->node_next == mapping->node_y && mapping_ll_y->node_prev == mapping->node_x){
		return;
	}

	if (mapping_ll_x->node_prev != NULL && mapping_ll_y->node_next != NULL && mapping_ll_x->node_prev == mapping->node_y && mapping_ll_y->node_next == mapping->node_x){
		return;
	}

	if (mapping_ll_x->node_next == NULL && mapping_ll_y->node_prev == NULL){
		mapping_ll_x->node_next = mapping->node_y;
		mapping_ll_y->node_prev = mapping->node_x;
		goto parent_mapping;
	}

	if (mapping_ll_x->node_prev == NULL && mapping_ll_y->node_next == NULL){
		mapping_ll_x->node_prev = mapping->node_y;
		mapping_ll_y->node_next = mapping->node_x;
		goto parent_mapping;
	}

	if (mapping_ll_x->node_next == NULL && mapping_ll_y->node_next == NULL){
		struct irOperation* 			sub_operation_x;
		struct irOperation* 			sub_operation_y;
		struct mappingLLElement* 		sub_mapping_ll_x;
		struct mappingLLElement* 		sub_mapping_ll_y;

		sub_operation_x = ir_node_get_operation(mapping_ll_x->node_prev);
		sub_operation_y = ir_node_get_operation(mapping_ll_y->node_prev);
		sub_mapping_ll_x = array_get(&(mapping_result->mapping_ll_elem), sub_operation_x->data);
		sub_mapping_ll_y = array_get(&(mapping_result->mapping_ll_elem), sub_operation_y->data);

		if (sub_mapping_ll_x->node_prev == NULL){
			sub_mapping_ll_x->node_prev = sub_mapping_ll_x->node_next;
			mapping_ll_x->node_next = mapping_ll_x->node_prev;

			mapping_ll_x->node_prev = mapping->node_y;
			mapping_ll_y->node_next = mapping->node_x;
			goto parent_mapping;
		}
		else if (sub_mapping_ll_y->node_prev == NULL){
			sub_mapping_ll_y->node_prev = sub_mapping_ll_y->node_next;
			mapping_ll_y->node_next = mapping_ll_y->node_prev;

			mapping_ll_x->node_next = mapping->node_y;
			mapping_ll_y->node_prev = mapping->node_x;
			goto parent_mapping;
		}

		printf("ERROR: in %s, unable to invert result order\n", __func__);
		return;
	}

	if (mapping_ll_x->node_prev == NULL && mapping_ll_y->node_prev == NULL){
		printf("WARNING: in %s, this case is not implemented yet\n", __func__);
		/* je sais que 

		mapping_ll_y->node_next != NULL
		mapping_ll_x->node_next != NULL
		*/

		goto parent_mapping;
	}

	printf("ERROR: in %s, this case is not meant to happen\n", __func__);
	return;

	/* STEP 2: call recursively this method for every parent mappings */
	parent_mapping:
	for (i = 0; i < mapping->container->parent_nb_mapping; i++){
		mappingResult_add_mapping(mapping_result, ir_mapping_container, (struct mapping*)array_get(&(ir_mapping_container->mapping_array), mapping->container->parent_mapping_offset + i));
	}
}

struct mappingResult* irMappingContainer_extract_result(struct irMappingContainer* ir_mapping_container, struct ir* ir){
	struct mappingResult* 	mapping_result;
	struct node* 			node;
	struct irOperation* 	operation;
	uint32_t 				i;
	uint32_t 				x;
	uint32_t 				y;
	int32_t 				max_score;
	struct mapping 			max_mapping;

	/* STEP 1: allocate and init the result structure */
	mapping_result = (struct mappingResult*)malloc(sizeof(struct mappingResult));
	if (mapping_result == NULL){
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
		return NULL;
	}

	if (array_init(&(mapping_result->mapping_ll_elem), sizeof(struct mappingLLElement))){
		printf("ERROR: in %s, unable to init array\n", __func__);
		free(mapping_result);
		return NULL;
	}
	if (array_init(&(mapping_result->mapping_ll_root), sizeof(struct mappingLLRoot))){
		printf("ERROR: in %s, unable to init array\n", __func__);
		array_clean(&(mapping_result->mapping_ll_elem));
		free(mapping_result);
		return NULL;
	}

	/* STEP 2: clean the node's data */
	node = graph_get_head_node(&(ir->graph));
	while(node != NULL){
		operation = ir_node_get_operation(node);
		operation->data = 0xffffffff;
		node = node_get_next(node);
	}

	/* STEP 3: find the max score it'll be the starting point */
	for (i = 0, max_score = 0; i < IR_NB_OPCODE; i++){
		for (y = 0; y < ir_mapping_container->opcode_mapping_container[i].nb_element; y++){
			for (x = 0; x < y; x++){
				if (ir_mapping_container->opcode_mapping_container[i].node_mapping_container[x + (y * (y - 1)) / 2].score > max_score){
					max_score 				= ir_mapping_container->opcode_mapping_container[i].node_mapping_container[x + (y * (y - 1)) / 2].score;
					max_mapping.node_x 		= ir_mapping_container->opcode_mapping_container[i].node_list[x];
					max_mapping.node_y 		= ir_mapping_container->opcode_mapping_container[i].node_list[y];
					max_mapping.container 	= ir_mapping_container->opcode_mapping_container[i].node_mapping_container + (x + (y * (y - 1)) / 2);
				}
			}
		}
	}

	/* pour le debug */
	printf("Found a max score of %d for mapping: (%p - %p)\n", max_score, (void*)ir_node_get_operation(max_mapping.node_x), (void*)ir_node_get_operation(max_mapping.node_y));
	#if 0
	for (i = 0; i < max_mapping.container->parent_nb_mapping; i++){
		struct mapping* parent_mapping;

		parent_mapping = (struct mapping*)array_get(&(ir_mapping_container->mapping_array), max_mapping.container->parent_mapping_offset + i);
		printf("\t%d: %p - %p\n", parent_mapping->container->score, (void*)ir_node_get_operation(parent_mapping->node_x), (void*)ir_node_get_operation(parent_mapping->node_y));
	}
	#endif
	
	/* STEP 4: traverse the best mapping  */
	mappingResult_add_mapping(mapping_result, ir_mapping_container, &max_mapping);

	/* STEP 5: set linked list root */
	node = graph_get_head_node(&(ir->graph));
	while(node != NULL){
		operation = ir_node_get_operation(node);
		if (operation->data != 0xffffffff){
			struct mappingLLElement* 	initial_el;
			struct mappingLLElement* 	cursor_el;
			struct mappingLLRoot* 		root;

			initial_el = (struct mappingLLElement*)array_get(&(mapping_result->mapping_ll_elem), operation->data);
			cursor_el = initial_el;
			while(cursor_el->node_prev != NULL && cursor_el->root == -1){
				cursor_el = (struct mappingLLElement*)array_get(&(mapping_result->mapping_ll_elem), ir_node_get_operation(cursor_el->node_prev)->data);
			}

			if (cursor_el != initial_el){
				if (cursor_el->root != -1){
					root = (struct mappingLLRoot*)array_get(&(mapping_result->mapping_ll_root), cursor_el->root);
					root->nb_element ++;
					initial_el->root = cursor_el->root;
				}
				else{
					struct mappingLLRoot new_root;

					new_root.head = cursor_el;
					new_root.nb_element = 2;

					initial_el->root= array_add(&(mapping_result->mapping_ll_root), &new_root);
					if (initial_el->root < 0){
						printf("ERROR: in %s, unable to add element to array\n", __func__);
					}
					cursor_el->root = initial_el->root;
				}
			}
			else{
				if (cursor_el->root == -1){
					struct mappingLLRoot new_root;

					new_root.head = cursor_el;
					new_root.nb_element = 1;

					initial_el->root= array_add(&(mapping_result->mapping_ll_root), &new_root);
					if (initial_el->root < 0){
						printf("ERROR: in %s, unable to add element to array\n", __func__);
					}
				}
			}
		}
		node = node_get_next(node);
	}

	return mapping_result;
}

void irMappingContainer_clean(struct irMappingContainer* ir_mapping_container){
	uint32_t i;

	for (i = 0; i < IR_NB_OPCODE; i++){
		if (ir_mapping_container->opcode_mapping_container[i].buffer != NULL){
			free(ir_mapping_container->opcode_mapping_container[i].buffer);
		}
	}

	array_clean(&(ir_mapping_container->mapping_array));
	array_clean(&(ir_mapping_container->divergence_array));
}

void ir_search_unrolled(struct ir* ir){
	struct irMappingContainer* 	ir_mapping_container;
	struct mappingResult* 		mapping_result;

	ir_mapping_container = irMappingContainer_create(ir);
	if (ir_mapping_container == NULL){
		printf("ERROR: in %s, unable to create irMappingContainer\n", __func__);
		return;
	}

	irMappingContainer_compute(ir_mapping_container);

	/*pour le debug */
	irMappingContainer_print_node_number(ir);
	irMappingContainer_print_mapping_score(ir_mapping_container, ir);

	mapping_result = irMappingContainer_extract_result(ir_mapping_container, ir);
	if (mapping_result == NULL){
		printf("ERROR: in %s, extract result return a NULL pointer\n", __func__);
	}
	else{
		mappingResult_print_color(mapping_result, ir);
		mappingResult_delete(mapping_result);
	}

	irMappingContainer_delete(ir_mapping_container);
}

/* ===================================================================== */
/* Sorting functions						                             */
/* ===================================================================== */

int32_t compare_divergentMapping(const void* arg1, const void* arg2){
	struct divergentMapping* dm1 = (struct divergentMapping*)arg1;
	struct divergentMapping* dm2 = (struct divergentMapping*)arg2;

	if (dm1->node_x < dm2->node_x){
		return -1;
	}
	else if (dm1->node_x > dm2->node_x){
		return 1;
	}
	else{
		if (dm1->node_y < dm2->node_y){
			return -1;
		}
		else if (dm1->node_y > dm2->node_y){
			return 1;
		}
		else{
			return 0;
		}
	}
}

/* ===================================================================== */
/* Printing functions						                             */
/* ===================================================================== */

void irMappingContainer_print_node_number(struct ir* ir){
	void(*prev_dotprint_node)(void*,FILE*,void*);
	void(*prev_dotprint_edge)(void*,FILE*,void*);

	prev_dotprint_node = ir->graph.dotPrint_node_data;
	prev_dotprint_edge = ir->graph.dotPrint_edge_data;

	ir->graph.dotPrint_node_data = irMapping_dotPrint_node_number;
	ir->graph.dotPrint_edge_data = irMapping_dotPrint_edge;

	graphPrintDot_print(&((ir)->graph), "number.dot", NULL);

	ir->graph.dotPrint_node_data = prev_dotprint_node;
	ir->graph.dotPrint_edge_data = prev_dotprint_edge;
}

void irMappingContainer_print_mapping_score(struct irMappingContainer* ir_mapping_container, struct ir* ir){
	void(*prev_dotprint_epilogue)(FILE*,void*);

	prev_dotprint_epilogue = ir->graph.dotPrint_epilogue;

	ir->graph.dotPrint_epilogue = irMapping_dotPrint_mapping;

	graphPrintDot_print(&((ir)->graph), "mapping.dot", ir_mapping_container);

	ir->graph.dotPrint_epilogue = prev_dotprint_epilogue;
}

void mappingResult_print_color(struct mappingResult* mapping_result, struct ir* ir){
	void(*prev_dotprint_node)(void*,FILE*,void*);
	void(*prev_dotprint_edge)(void*,FILE*,void*);

	prev_dotprint_node = ir->graph.dotPrint_node_data;
	prev_dotprint_edge = ir->graph.dotPrint_edge_data;

	ir->graph.dotPrint_node_data = irMapping_dotPrint_node_color;
	ir->graph.dotPrint_edge_data = irMapping_dotPrint_edge;

	graphPrintDot_print(&((ir)->graph), "color.dot", mapping_result);

	ir->graph.dotPrint_node_data = prev_dotprint_node;
	ir->graph.dotPrint_edge_data = prev_dotprint_edge;
}

void irMapping_dotPrint_node_color(void* data, FILE* file, void* arg){
	struct irOperation* 		operation = (struct irOperation*)data;
	struct mappingResult* 		mapping_result = (struct mappingResult*)arg;
	const char* 				color;

	if (operation->data != 0xffffffff){
		switch(((struct mappingLLElement*)array_get(&(mapping_result->mapping_ll_elem), operation->data))->root){
			case 0 : {color= "red"; 	break;}
			case 1 : {color= "green"; 	break;}
			case 2 : {color= "blue"; 	break;}
			case 3 : {color= "orange"; 	break;}
			case 4 : {color= "pink"; 	break;}
			case 5 : {color= "yellow"; 	break;}
			case 6 : {color= "brown"; 	break;}
			case 7 : {color= "cyan"; 	break;}
			case 8 : {color= "navy"; 	break;}
			case 9 : {color= "purple"; 	break;}
			default : {color = "back"; printf("WARNING: in %s, not enough color for every one\n", __func__); break;}
		}
	}
	else{
		color = "black";
	}

	switch(operation->type){
		case IR_OPERATION_TYPE_INPUT 		: {
			if (OPERAND_IS_MEM(*(operation->operation_type.input.operand))){
				#if defined ARCH_32
				fprintf(file, "[shape=\"box\",label=\"@%08x\",color=\"%s\"]", operation->operation_type.input.operand->location.address, color);
				#elif defined ARCH_64
				#pragma GCC diagnostic ignored "-Wformat" /* ISO C90 does not support the ‘ll’ gnu_printf length modifier */
				fprintf(file, "[shape=\"box\",label=\"@%llx\",color=\"%s\"]", operation->operation_type.input.operand->location.address, color);
				#else
				#error Please specify an architecture {ARCH_32 or ARCH_64}
				#endif
			}
			else if (OPERAND_IS_REG(*(operation->operation_type.input.operand))){
				fprintf(file, "[shape=\"box\",label=\"%s\",color=\"%s\"]", reg_2_string(operation->operation_type.input.operand->location.reg), color);
			}
			else{
				printf("ERROR: in %s, unexpected data type (REG or MEM)\n", __func__);
				break;
			}
			break;
		}
		case IR_OPERATION_TYPE_OUTPUT 		: {
			fprintf(file, "[shape=\"invhouse\",label=\"%s %p\",color=\"%s\"]", irOpcode_2_string(operation->operation_type.output.opcode), data, color);
			break;
		}
		case IR_OPERATION_TYPE_INNER 		: {
			fprintf(file, "[label=\"%s %p\",color=\"%s\"]", irOpcode_2_string(operation->operation_type.inner.opcode), data, color);
			break;
		}
	}
}

#pragma GCC diagnostic ignored "-Wunused-parameter"
void irMapping_dotPrint_node_number(void* data, FILE* file, void* arg){
	struct irOperation* 		operation = (struct irOperation*)data;

	switch(operation->type){
		case IR_OPERATION_TYPE_INPUT 		: {
			if (OPERAND_IS_MEM(*(operation->operation_type.input.operand))){
				#if defined ARCH_32
				fprintf(file, "[shape=\"box\",label=\"@%08x\"]", operation->operation_type.input.operand->location.address);
				#elif defined ARCH_64
				#pragma GCC diagnostic ignored "-Wformat" /* ISO C90 does not support the ‘ll’ gnu_printf length modifier */
				fprintf(file, "[shape=\"box\",label=\"@%llx\"]", operation->operation_type.input.operand->location.address);
				#else
				#error Please specify an architecture {ARCH_32 or ARCH_64}
				#endif
			}
			else if (OPERAND_IS_REG(*(operation->operation_type.input.operand))){
				fprintf(file, "[shape=\"box\",label=\"%s\"]", reg_2_string(operation->operation_type.input.operand->location.reg));
			}
			else{
				printf("ERROR: in %s, unexpected data type (REG or MEM)\n", __func__);
				break;
			}
			break;
		}
		case IR_OPERATION_TYPE_OUTPUT 		: {
			fprintf(file, "[shape=\"invhouse\",label=\"%s - %u\"]", irOpcode_2_string(operation->operation_type.output.opcode), operation->data);
			break;
		}
		case IR_OPERATION_TYPE_INNER 		: {
			fprintf(file, "[label=\"%s - %u\"]", irOpcode_2_string(operation->operation_type.inner.opcode), operation->data);
			break;
		}
	}
}

#pragma GCC diagnostic ignored "-Wunused-parameter"
void irMapping_dotPrint_edge(void* data, FILE* file, void* arg){
	struct irDependence* dependence = (struct irDependence*)data;

	switch(dependence->type){
		case IR_DEPENDENCE_TYPE_DIRECT 	: {
			break;
		}
		case IR_DEPENDENCE_TYPE_BASE 	: {
			fprintf(file, "[label=\"base\"]");
			break;
		}
		case IR_DEPENDENCE_TYPE_INDEX 	: {
			fprintf(file, "[label=\"index\"]");
			break;
		}
	}
}

void irMapping_dotPrint_mapping(FILE* file, void* arg){
	struct irMappingContainer* 	ir_mapping_container = (struct irMappingContainer*)arg;
	uint32_t 					i;
	uint32_t 					x;
	uint32_t 					y;
	uint32_t 					index;

	for (i = 0; i < IR_NB_OPCODE; i++){
		if (i == IR_NOT){
			for (y = 0; y < ir_mapping_container->opcode_mapping_container[i].nb_element; y++){
				for (x = 0; x < y; x++){
					index = x + (y * (y - 1)) / 2;
					if (ir_mapping_container->opcode_mapping_container[i].node_mapping_container[index].score > 0){
						fprintf(file, "%u -> %u [label=\"%d (%d)\" style=dotted dir=none]\n", (uint32_t)ir_mapping_container->opcode_mapping_container[i].node_list[x], (uint32_t)(uint32_t)ir_mapping_container->opcode_mapping_container[i].node_list[y], ir_mapping_container->opcode_mapping_container[i].node_mapping_container[index].score, ir_mapping_container->opcode_mapping_container[i].node_mapping_container[index].div_score);
					}
				}
			}
		}
	}
}
