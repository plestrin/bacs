#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "result.h"

/* ===================================================================== */
/* result routines														 */
/* ===================================================================== */

int32_t result_init(struct result* result, struct codeSignature* code_signature, struct array* assignement_array){
	uint32_t 				i;
	uint32_t 				j;
	struct node**			assignement;
	uint32_t 				nb_input;
	uint32_t 				nb_output;
	uint32_t 				nb_intern;
	struct signatureNode* 	sig_node;
	struct irOperation* 	operation;

	result->state 				= RESULTSTATE_IDLE;
	result->signature 			= code_signature;
	result->nb_occurrence 		= array_get_length(assignement_array);
	result->in_mapping_buffer 	= (struct signatureLink*)malloc(sizeof(struct signatureLink) * result->nb_occurrence * code_signature->nb_frag_tot_in);
	result->ou_mapping_buffer 	= (struct signatureLink*)malloc(sizeof(struct signatureLink) * result->nb_occurrence * code_signature->nb_frag_tot_out);
	result->intern_node_buffer 	= (struct virtualNode*)malloc(sizeof(struct virtualNode) * result->nb_occurrence * result_get_nb_internal_node(result));
	result->symbol_node_buffer 	= (struct node**)calloc(result->nb_occurrence, sizeof(struct node*));

	if (result->in_mapping_buffer == NULL || result->ou_mapping_buffer == NULL || result->intern_node_buffer == NULL || result->symbol_node_buffer == NULL){
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
	
		if (result->in_mapping_buffer != NULL){
			free(result->in_mapping_buffer);
		}
		if (result->ou_mapping_buffer != NULL){
			free(result->ou_mapping_buffer);
		}
		if (result->intern_node_buffer != NULL){
			free(result->intern_node_buffer);
		}
		if (result->symbol_node_buffer != NULL){
			free(result->symbol_node_buffer);
		}

		return -1;
	}

	for (i = 0; i < result->nb_occurrence; i++){
		assignement = (struct node**)array_get(assignement_array, i);
		nb_input 	= 0;
		nb_output 	= 0;
		nb_intern 	= 0;

		for (j = 0; j < code_signature->graph.nb_node; j++){
			sig_node = (struct signatureNode*)&(code_signature->sub_graph_handle->node_tab[j].node->data);
			operation = ir_node_get_operation(assignement[j]);

			if (operation->type == IR_OPERATION_TYPE_SYMBOL){
				if (sig_node->input_number > 0){
					result->in_mapping_buffer[i * code_signature->nb_frag_tot_in + nb_input].virtual_node.node 		= NULL;
					result->in_mapping_buffer[i * code_signature->nb_frag_tot_in + nb_input].virtual_node.result 	= operation->operation_type.symbol.result_ptr;
					result->in_mapping_buffer[i * code_signature->nb_frag_tot_in + nb_input].virtual_node.index 	= operation->operation_type.symbol.index;
					result->in_mapping_buffer[i * code_signature->nb_frag_tot_in + nb_input].edge_desc = IR_DEPENDENCE_MACRO_DESC_SET_INPUT(sig_node->input_frag_order, sig_node->input_number);
					nb_input ++;
				}

				if (sig_node->output_number > 0){
					result->ou_mapping_buffer[i * code_signature->nb_frag_tot_out + nb_output].virtual_node.node 	= NULL;
					result->ou_mapping_buffer[i * code_signature->nb_frag_tot_out + nb_output].virtual_node.result 	= operation->operation_type.symbol.result_ptr;
					result->ou_mapping_buffer[i * code_signature->nb_frag_tot_out + nb_output].virtual_node.index 	= operation->operation_type.symbol.index;
					result->ou_mapping_buffer[i * code_signature->nb_frag_tot_out + nb_output].edge_desc = IR_DEPENDENCE_MACRO_DESC_SET_OUTPUT(sig_node->output_frag_order, sig_node->output_number);
					nb_output ++;
				}

				if (sig_node->input_number == 0 && sig_node->output_number == 0){
					result->intern_node_buffer[i * result_get_nb_internal_node(result) + nb_intern].node 			= NULL;
					result->intern_node_buffer[i * result_get_nb_internal_node(result) + nb_intern].result 			= operation->operation_type.symbol.result_ptr;
					result->intern_node_buffer[i * result_get_nb_internal_node(result) + nb_intern].index 			= operation->operation_type.symbol.index;
					nb_intern ++;
				}
			}
			else{
				if (sig_node->input_number > 0){
					result->in_mapping_buffer[i * code_signature->nb_frag_tot_in + nb_input].virtual_node.node 		= assignement[j];
					result->in_mapping_buffer[i * code_signature->nb_frag_tot_in + nb_input].virtual_node.result 	= NULL;
					result->in_mapping_buffer[i * code_signature->nb_frag_tot_in + nb_input].edge_desc = IR_DEPENDENCE_MACRO_DESC_SET_INPUT(sig_node->input_frag_order, sig_node->input_number);
					nb_input ++;
				}

				if (sig_node->output_number > 0){
					result->ou_mapping_buffer[i * code_signature->nb_frag_tot_out + nb_output].virtual_node.node 	= assignement[j];
					result->ou_mapping_buffer[i * code_signature->nb_frag_tot_out + nb_output].virtual_node.result 	= NULL;
					result->ou_mapping_buffer[i * code_signature->nb_frag_tot_out + nb_output].edge_desc = IR_DEPENDENCE_MACRO_DESC_SET_OUTPUT(sig_node->output_frag_order, sig_node->output_number);
					nb_output ++;
				}

				if (sig_node->input_number == 0 && sig_node->output_number == 0){
					result->intern_node_buffer[i * result_get_nb_internal_node(result) + nb_intern].node 			= assignement[j];
					result->intern_node_buffer[i * result_get_nb_internal_node(result) + nb_intern].result 			= NULL;
					nb_intern ++;
				}
			}
		}
	}

	return 0;
}

void result_push(struct result* result, struct ir* ir){
	uint32_t i;
	uint32_t j;

	result->state = RESULTSTATE_PUSH;

	for (i = 0; i < result->nb_occurrence; i++){
		result->symbol_node_buffer[i] = ir_add_symbol(ir, result, i);
		if (result->symbol_node_buffer[i] == NULL){
			printf("ERROR: in %s, unable to add symbolic node to IR\n", __func__);
			continue;
		}

		for (j = 0; j < result->signature->nb_frag_tot_in; j++){
			if (result->in_mapping_buffer[i * result->signature->nb_frag_tot_in + j].virtual_node.node == NULL){
				result->in_mapping_buffer[i * result->signature->nb_frag_tot_in + j].virtual_node.node = result->in_mapping_buffer[i * result->signature->nb_frag_tot_in + j].virtual_node.result->symbol_node_buffer[result->in_mapping_buffer[i * result->signature->nb_frag_tot_in + j].virtual_node.index];
			}

			if (result->in_mapping_buffer[i * result->signature->nb_frag_tot_in + j].virtual_node.node == NULL){
				printf("ERROR: in %s, unsatisfied input dependence\n", __func__);
				continue;
			}

			if (ir_add_macro_dependence(ir, result->in_mapping_buffer[i * result->signature->nb_frag_tot_in + j].virtual_node.node, result->symbol_node_buffer[i], result->in_mapping_buffer[i * result->signature->nb_frag_tot_in + j].edge_desc) == NULL){
				printf("ERROR: in %s, unable to add dependence to IR\n", __func__);
			}
		}

		for (j = 0; j < result->signature->nb_frag_tot_out; j++){
			if (result->ou_mapping_buffer[i * result->signature->nb_frag_tot_out + j].virtual_node.node == NULL){
				result->ou_mapping_buffer[i * result->signature->nb_frag_tot_out + j].virtual_node.node = result->ou_mapping_buffer[i * result->signature->nb_frag_tot_out + j].virtual_node.result->symbol_node_buffer[result->ou_mapping_buffer[i * result->signature->nb_frag_tot_out + j].virtual_node.index];
			}

			if (result->ou_mapping_buffer[i * result->signature->nb_frag_tot_out + j].virtual_node.node == NULL){
				printf("ERROR: in %s, unsatisfied input dependence\n", __func__);
				continue;
			}

			if (ir_add_macro_dependence(ir, result->symbol_node_buffer[i], result->ou_mapping_buffer[i * result->signature->nb_frag_tot_out + j].virtual_node.node, result->ou_mapping_buffer[i * result->signature->nb_frag_tot_out + j].edge_desc) == NULL){
				printf("ERROR: in %s, unable to add dependence to IR\n", __func__);
			}
		}
	}
}

void result_pop(struct result* result, struct ir* ir){
	uint32_t i;
	uint32_t j;

	result->state = RESULTSTATE_IDLE;

	for (i = 0; i < result->nb_occurrence; i++){
		if (result->symbol_node_buffer[i] != NULL){
			ir_remove_node(ir, result->symbol_node_buffer[i]);	
		}
		result->symbol_node_buffer[i] = NULL;

		for (j = 0; j < result->signature->nb_frag_tot_in; j++){
			if (result->in_mapping_buffer[i * result->signature->nb_frag_tot_in + j].virtual_node.result != NULL){
				result->in_mapping_buffer[i * result->signature->nb_frag_tot_in + j].virtual_node.node = NULL;
			}
		}

		for (j = 0; j < result->signature->nb_frag_tot_out; j++){
			if (result->ou_mapping_buffer[i * result->signature->nb_frag_tot_out + j].virtual_node.result != NULL){
				result->ou_mapping_buffer[i * result->signature->nb_frag_tot_out + j].virtual_node.node = NULL;
			}
		}
	}
}

void result_get_footprint(struct result* result, uint32_t index, struct set* set){
	uint32_t i;

	if (index >= result->nb_occurrence){
		printf("ERROR: in %s, incorrect index %u (max is %u)\n", __func__, index, result->nb_occurrence);
		return;
	}

	for (i = 0; i < result_get_nb_internal_node(result); i++){
		if (result->intern_node_buffer[index * result_get_nb_internal_node(result) + i].node == NULL){
			result_get_footprint(result->intern_node_buffer[index * result_get_nb_internal_node(result) + i].result, result->intern_node_buffer[index * result_get_nb_internal_node(result) + i].index, set);
		}
		else{
			if (set_add(set, &(result->intern_node_buffer[index * result_get_nb_internal_node(result) + i].node))){
				printf("ERROR: in %s, unable to add element to set\n", __func__);
			}
		}
	}
}

#define MAX_CLASS_THRESHOLD 2

static int32_t signatureOccurence_try_append_to_class(struct parameterMapping* class, struct parameterMapping* new_mapping, uint32_t nb_in, uint32_t nb_out);
static void signatureOccurence_print_location(struct parameterMapping* parameter);

static int32_t signatureOccurence_try_append_to_class(struct parameterMapping* class, struct parameterMapping* new_mapping, uint32_t nb_in, uint32_t nb_out){
	uint32_t 					i;
	enum parameterSimilarity* 	similarity;

	similarity = (enum parameterSimilarity*)alloca(sizeof(enum parameterSimilarity) * (nb_in + nb_out));

	for (i = 0; i < nb_in; i++){
		similarity[i] = parameterSimilarity_get(parameterMapping_get_node_buffer(class + i), class[i].nb_fragment, parameterMapping_get_node_buffer(new_mapping + i), class[i].nb_fragment);
		if (similarity[i] == PARAMETER_OVERLAP || similarity[i] == PARAMETER_DISJOINT){
			return -1;
		}
	}

	for (i = 0; i < nb_out; i++){
		similarity[nb_in + i] = parameterSimilarity_get(parameterMapping_get_node_buffer(class + nb_in + i), class[nb_in + i].nb_fragment, parameterMapping_get_node_buffer(new_mapping + nb_in + i), class[nb_in + i].nb_fragment);
		if (similarity[nb_in + i] == PARAMETER_OVERLAP || similarity[nb_in + i] == PARAMETER_DISJOINT){
			return -1;
		}
	}

	for (i = 0; i < nb_in + nb_out; i++){
		class[i].similarity |= similarity[i];
	}

	return 0;
}

static void signatureOccurence_print_location(struct parameterMapping* parameter){
	uint32_t 			i;
	struct irOperation* operation;
	uint8_t 			is_input_buffer;
	struct node* 		address;
	struct node* 		base;
	uint64_t 			buffer_start_offset = 0;
	uint8_t* 			buffer;
	uint8_t 			buffer_access_size 	= 0;
	uint64_t 			offset              = 0;

	if (parameter->nb_fragment == 0){
		return;
	}

	buffer = alloca(sizeof(uint8_t) * (2 * parameter->nb_fragment - 1));
	memset(buffer, 0, sizeof(uint8_t) * (2 * parameter->nb_fragment - 1));

	for (i = 0, is_input_buffer = 1, base = NULL; i < parameter->nb_fragment && is_input_buffer == 1; i++){
		operation = ir_node_get_operation(parameterMapping_get_node_buffer(parameter)[i]);
		if (operation->type == IR_OPERATION_TYPE_IN_MEM){
			address = edge_get_src(node_get_head_edge_dst(parameterMapping_get_node_buffer(parameter)[i]));
			if (ir_node_get_operation(address)->type == IR_OPERATION_TYPE_INST && ir_node_get_operation(address)->operation_type.inst.opcode == IR_ADD && address->nb_edge_dst == 2){
				struct node* op1;
				struct node* op2;

				op1 = edge_get_src(node_get_head_edge_dst(address));
				if (ir_node_get_operation(op1)->type != IR_OPERATION_TYPE_IMM){
					op2 = edge_get_src(edge_get_next_dst(node_get_head_edge_dst(address)));
				}
				else{
					op2 = op1;
					op1 = edge_get_src(edge_get_next_dst(node_get_head_edge_dst(address)));
				}

				if (ir_node_get_operation(op1)->type != IR_OPERATION_TYPE_IMM && ir_node_get_operation(op2)->type == IR_OPERATION_TYPE_IMM){
					if (base == NULL || (base == op1 && buffer_access_size == ir_node_get_operation(parameterMapping_get_node_buffer(parameter)[i])->size)){
						if (base == NULL){
							base = op1;
							buffer_access_size = ir_node_get_operation(parameterMapping_get_node_buffer(parameter)[i])->size;
							buffer_start_offset = ir_imm_operation_get_unsigned_value(ir_node_get_operation(op2));
							if (buffer_start_offset >= (parameter->nb_fragment - 1) * (buffer_access_size / 8)){
								buffer_start_offset -= (parameter->nb_fragment - 1) * (buffer_access_size / 8);
								buffer[parameter->nb_fragment - 1] = 1;
							}
							else{
								buffer[buffer_start_offset / (buffer_access_size /8)] = 1;
								buffer_start_offset = 0;
							}
							
						}
						else{
							offset = ir_imm_operation_get_unsigned_value(ir_node_get_operation(op2)); 
							if (buffer_start_offset <= offset && offset < buffer_start_offset + 2 * parameter->nb_fragment * (buffer_access_size / 8)){
								if ((offset - buffer_start_offset) % (buffer_access_size / 8) == 0){
									if (buffer[(offset - buffer_start_offset) / (buffer_access_size / 8)] == 0){
										buffer[(offset - buffer_start_offset) / (buffer_access_size / 8)] = 1;
									}
									else{
										is_input_buffer = 0;
									}
								}
								else{
									is_input_buffer = 0;
								}
							}
							else{
								is_input_buffer = 0;
							}
						}
					}
					else{
						is_input_buffer = 0;
					}
				}
				else{
					is_input_buffer = 0;
				}
			}
			else{
				is_input_buffer = 0;
			}
		}
		else{
			is_input_buffer = 0;
		}
	}

	if (is_input_buffer){
		for (i = 0; i < 2 * parameter->nb_fragment - 1; i++){
			if (buffer[i]){
				offset = buffer_start_offset + i * (buffer_access_size / 8);
				break;  
			}
		}

		for ( ; i < 2 * parameter->nb_fragment - 1; i++){
			if (!buffer[i]){
				break;
			}
		}

		if ((buffer_start_offset + i * (buffer_access_size / 8)) - offset == parameter->nb_fragment * (buffer_access_size / 8)){
			ir_print_location_node(base, NULL);
			printf("[%llu:%llu]\n", offset, buffer_start_offset + i * (buffer_access_size / 8));
			return;
		}
	}

	printf("{");
	for (i = 0; i < parameter->nb_fragment; i++){
		ir_print_location_node(parameterMapping_get_node_buffer(parameter)[i], NULL);
		if (i != parameter->nb_fragment - 1){
			printf(" ");
		}
		
	}
	printf("}\n");
}

void result_print(struct result* result){
	uint32_t 					i;
	uint32_t 					j;
	struct parameterMapping* 	parameter_mapping;
	struct array* 				class_array 		= NULL;
	struct parameterMapping* 	class;
	struct codeSignature* 		code_signature 		= result->signature;

	parameter_mapping = parameterMapping_create(code_signature);
	if (parameter_mapping == NULL){
		printf("ERROR: in %s, unable to create parameterMapping\n", __func__);
		return;
	}

	class_array = array_create(parameterMapping_get_size(code_signature));
	if (class_array == NULL){
		printf("ERROR: in %s, unable to create array\n", __func__);
		goto exit;
	}

	for (i = 0; i < result->nb_occurrence; i++){
		if (parameterMapping_fill(parameter_mapping, result, i)){
			printf("ERROR: in %s, unable to fetch occurrence %u\n", __func__, i);
			continue;
		}

		for (j = 0; j < array_get_length(class_array); j++){
			class = (struct parameterMapping*)array_get(class_array, j);
			if (signatureOccurence_try_append_to_class(class, parameter_mapping, code_signature->nb_parameter_in, code_signature->nb_parameter_out) == 0){
				break;
			}
		}
		if (j == array_get_length(class_array)){
			if (array_add(class_array, parameter_mapping) < 0){
				printf("ERROR: in %s, unable to add new element to array\n", __func__);
			}
		}
	}

	if (array_get_length(class_array) > MAX_CLASS_THRESHOLD){
		printf("WARNING: in %s, too many result clusters to be printed (%d)\n", __func__, array_get_length(class_array));
		goto exit;
	}

	for (i = 0; i < array_get_length(class_array); i++){
		class = (struct parameterMapping*)array_get(class_array, i);
		
		printf("CLUSTER %u/%u:\n", i + 1, array_get_length(class_array));
		for (j = 0; j < code_signature->nb_parameter_in; j++){
			if (class[j].similarity == PARAMETER_EQUAL){
				printf("\t Parameter I%u EQUAL = ", j);
			}
			else if (class[j].similarity == PARAMETER_PERMUT){
				printf("\t Parameter I%u PERMUT = ", j);
			}
			else{
				printf("\t Parameter I%u UNKNOWN = ", j);
			}
			signatureOccurence_print_location(class + j);
		}
		for (j = 0; j < code_signature->nb_parameter_out; j++){
			if (class[code_signature->nb_parameter_in + j].similarity == PARAMETER_EQUAL){
				printf("\t Parameter O%u EQUAL = ", j);
			}
			else if (class[code_signature->nb_parameter_in + j].similarity == PARAMETER_PERMUT){
				printf("\t Parameter O%u PERMUT = ", j);
			}
			else{
				printf("\t Parameter O%u UNKNOWN = ", j);
			}
			signatureOccurence_print_location(class + code_signature->nb_parameter_in + j);
		}
	}

	exit:
	free(parameter_mapping);
	if (class_array != NULL){
		array_delete(class_array);
	}

}

/* ===================================================================== */
/* parameterMapping routines											 */
/* ===================================================================== */

struct parameterMapping* parameterMapping_create(struct codeSignature* signature){
	struct parameterMapping* mapping;

	mapping = (struct parameterMapping*)malloc(parameterMapping_get_size(signature));
	if (mapping != NULL){
		if (parameterMapping_init(mapping, signature)){
			printf("ERROR: in %s, unable to init parameterMapping\n", __func__);
			free(mapping);
			mapping = NULL;
		}
	}
	else{
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
	}

	return mapping;
}

int32_t parameterMapping_init(struct parameterMapping* mapping, struct codeSignature* signature){
	uint32_t 				i;
	uint32_t 				j;
	struct signatureNode* 	sig_node;

	memset(mapping, 0, parameterMapping_get_size(signature));

	for (i = 0; i < signature->graph.nb_node; i++){
		sig_node = (struct signatureNode*)&(signature->sub_graph_handle->node_tab[i].node->data);

		if (sig_node->input_number > 0){
			mapping[sig_node->input_number - 1].nb_fragment ++;
		}
		if (sig_node->output_number > 0){
			mapping[signature->nb_parameter_in + sig_node->output_number - 1].nb_fragment ++;
		}
	}

	for (i = 0; i < signature->graph.nb_node; i++){
		sig_node = (struct signatureNode*)&(signature->sub_graph_handle->node_tab[i].node->data);

		if (sig_node->input_number > 0){
			if (sig_node->input_frag_order > mapping[sig_node->input_number - 1].nb_fragment){
				printf("ERROR: in %s, input frag number %u is out of range %u\n", __func__, sig_node->input_frag_order, mapping[sig_node->input_number - 1].nb_fragment);
				return -1;
			}
		}
		if (sig_node->output_number > 0){
			if (sig_node->output_frag_order > mapping[signature->nb_parameter_in + sig_node->output_number - 1].nb_fragment){
				printf("ERROR: in %s, output frag number %u is out of range %u\n", __func__, sig_node->output_frag_order, mapping[signature->nb_parameter_in + sig_node->output_number - 1].nb_fragment);
				return -1;
			}
		}
	}

	for (i = 0, j = 0; i < signature->nb_parameter_in; i++){
		mapping[i].node_buffer_offset 	= (signature->nb_parameter_in - i + signature->nb_parameter_out) * sizeof(struct parameterMapping) + j * sizeof(struct node*);
		mapping[i].similarity 			= PARAMETER_EQUAL;
		j += mapping[i].nb_fragment;
	}

	for (i = 0; i < signature->nb_parameter_out; i++){
		mapping[signature->nb_parameter_in + i].node_buffer_offset 	= (signature->nb_parameter_out - i) * sizeof(struct parameterMapping) + j * sizeof(struct node*);
		mapping[signature->nb_parameter_in + i].similarity 			= PARAMETER_EQUAL;
		j += mapping[signature->nb_parameter_in + i].nb_fragment;
	}

	return 0;
}

int32_t parameterMapping_fill(struct parameterMapping* mapping, struct result* result, uint32_t index){
	uint32_t 				i;
	struct signatureLink* 	link;

	for (i = 0; i < result->signature->nb_frag_tot_in; i++){
		link = result->in_mapping_buffer + (index * result->signature->nb_frag_tot_in) + i;
		if (link->virtual_node.node == NULL){
			printf("ERROR: in %s, input node is virtual, I don't kown how to handle that case\n", __func__);
			return -1;
		}

		parameterMapping_get_node_buffer(mapping + (IR_DEPENDENCE_MACRO_DESC_GET_ARG(link->edge_desc) - 1))[IR_DEPENDENCE_MACRO_DESC_GET_FRAG(link->edge_desc) - 1] = link->virtual_node.node;
	}

	for (i = 0; i < result->signature->nb_frag_tot_out; i++){
		link = result->ou_mapping_buffer + (index * result->signature->nb_frag_tot_out) + i;
		if (link->virtual_node.node == NULL){
			printf("ERROR: in %s, output node is virtual, I don't kown how to handle that case\n", __func__);
			return -1;
		}

		parameterMapping_get_node_buffer(mapping + (result->signature->nb_parameter_in + IR_DEPENDENCE_MACRO_DESC_GET_ARG(link->edge_desc) - 1))[IR_DEPENDENCE_MACRO_DESC_GET_FRAG(link->edge_desc) - 1] = link->virtual_node.node;
	}

	return 0;
}

enum parameterSimilarity parameterSimilarity_get(struct node** parameter_list1, uint32_t size1, struct node** parameter_list2, uint32_t size2){
	uint32_t 					i;
	uint32_t 					j;
	enum parameterSimilarity 	result;
	uint32_t 					nb_found;

	if (size1 == size2){
		result = PARAMETER_EQUAL;
	}
	else if (size1 < size2){
		result = PARAMETER_SUPERSET;
	}
	else{
		result = PARAMETER_SUBSET;
	}

	for (i = 0, nb_found = 0; i < size1; i++){
		for (j = 0; j < size2; j++){
			if (parameter_list1[i] == parameter_list2[j]){
				nb_found ++;
				if (i != j){
					result |= PARAMETER_PERMUT;
				}
				break;
			}
		}
		if (j == size2 && nb_found != 0){
			return PARAMETER_OVERLAP;
		}
	}

	if (nb_found == 0){
		return PARAMETER_DISJOINT;
	}
	else{
		return result;
	}
}