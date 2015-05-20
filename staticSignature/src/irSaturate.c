#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "irSaturate.h"

static uint32_t irSaturate_search_asso_seq(struct node* node, struct assoSeq* asso_seq);
static int32_t irSaturate_check_asso_seq(struct assoSeq* asso_seq);
static int32_t irSaturate_compare_asso_seq(struct assoSeq* asso_seq1, struct assoSeq* asso_seq2);

struct assoGraphHeader{
	enum irOpcode 	opcode;
	uint32_t 		nb_input;
	uint32_t 		nb_output;
	uint32_t 		input_offset;
	uint32_t 		output_offset;
};

static void irSaturate_asso(struct saturateRules* saturate_rules, struct ir* ir);
static void irSaturate_extract_asso_graph(struct node* node, struct assoGraphHeader* header, struct node** input_buffer, struct node** output_buffer);
static int32_t irSaturate_check_asso_subgraph(struct node** selection_input, uint32_t nb_input, struct node** selection_output, uint32_t nb_output, struct node** selection_remaining, uint32_t* nb_remaining);
static int32_t irSaturate_is_node_yet_exist(struct node** selection_input, uint32_t nb_input, struct node** selection_output, uint32_t nb_output);

#define IRSATURATE_MAX_RECURSION_LEVEL 4

void saturateRules_learn_associative_conflict(struct saturateRules* saturate_rules, struct codeSignatureCollection* collection){
	struct codeSignature* 	signature;
	struct node* 			node_cursor_signature;
	struct node* 			node_cursor_operation;
	struct signatureNode*	operation;
	struct assoSeq 			asso_seq;
	uint32_t 				i;

	for (node_cursor_signature = graph_get_head_node(&(collection->syntax_graph)); node_cursor_signature != NULL; node_cursor_signature = node_get_next(node_cursor_signature)){
		signature = syntax_node_get_codeSignature(node_cursor_signature);

		for (node_cursor_operation = graph_get_head_node(&(signature->graph)); node_cursor_operation != NULL; node_cursor_operation = node_get_next(node_cursor_operation)){
			node_cursor_operation->ptr = 0;
		}

		for (node_cursor_operation = graph_get_head_node(&(signature->graph)); node_cursor_operation != NULL; node_cursor_operation = node_get_next(node_cursor_operation)){
			operation = (struct signatureNode*)&(node_cursor_operation->data);
			if (node_cursor_operation->ptr == 0 && operation->type == SIGNATURE_NODE_TYPE_OPCODE && irSaturate_opcode_is_associative(operation->node_type.opcode)){
				asso_seq.opcode = operation->node_type.opcode;
				asso_seq.nb_input = 0;
				asso_seq.nb_output = 0;

				asso_seq.nb_node = irSaturate_search_asso_seq(node_cursor_operation, &asso_seq);
				if (!irSaturate_check_asso_seq(&asso_seq)){
					for (i = 0; i < array_get_length(&(saturate_rules->asso_seq_array)); i++){
						if (!irSaturate_compare_asso_seq(&asso_seq, (struct assoSeq*)array_get(&(saturate_rules->asso_seq_array), i))){
							break;
						}
					}
					if (i == array_get_length(&(saturate_rules->asso_seq_array))){
						if (array_add(&(saturate_rules->asso_seq_array), &asso_seq) < 0){
							printf("ERROR: in %s, unable to add element to array\n", __func__);
						}
					}
				}
			}
		}
	}
}

static uint32_t irSaturate_search_asso_seq(struct node* node, struct assoSeq* asso_seq){
	struct edge* 			edge_cursor;
	struct node* 			node_cursor;
	struct signatureNode* 	operation;
	uint32_t 				result = 1;

	node->ptr = (void*)1;

	for (edge_cursor = node_get_head_edge_src(node); edge_cursor != NULL; edge_cursor = edge_get_next_src(edge_cursor)){
		node_cursor = edge_get_dst(edge_cursor);
		operation = (struct signatureNode*)&(node_cursor->data);

		if (node_cursor->ptr == 0 && operation->type == SIGNATURE_NODE_TYPE_OPCODE && operation->node_type.opcode == asso_seq->opcode){
			result+= irSaturate_search_asso_seq(node_cursor, asso_seq);
		}
		else{
			if (asso_seq->nb_output < IRSATURATE_ASSOSEQ_MAX_OUTPUT){
				asso_seq->buffer_output[asso_seq->nb_output ++] = signatureNode_get_label(node_cursor);
			}
			else{
				printf("ERROR: in %s, IRSATURATE_ASSOSEQ_MAX_OUTPUT has been reached\n", __func__);
			}
		}
	}

	for (edge_cursor = node_get_head_edge_dst(node); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
		node_cursor = edge_get_src(edge_cursor);
		operation = (struct signatureNode*)&(node_cursor->data);

		if (node_cursor->ptr == 0 && operation->type == SIGNATURE_NODE_TYPE_OPCODE && operation->node_type.opcode == asso_seq->opcode){
			result += irSaturate_search_asso_seq(node_cursor, asso_seq);
		}
		else{
			if (asso_seq->nb_input < IRSATURATE_ASSOSEQ_MAX_INPUT){
				asso_seq->buffer_input[asso_seq->nb_input ++] = signatureNode_get_label(node_cursor);
			}
			else{
				printf("ERROR: in %s, IRSATURATE_ASSOSEQ_MAX_INPUT has been reached\n", __func__);
			}
		}
	}

	return result;
}

int32_t irSaturate_compare_label(const void* a, const void* b){
	if (*(uint32_t*)a < *(uint32_t*)b){
		return -1;
	}
	else if (*(uint32_t*)a > *(uint32_t*)b){
		return 1;
	}
	else{
		return 0;
	}
}

static int32_t irSaturate_check_asso_seq(struct assoSeq* asso_seq){
	if (asso_seq->nb_output > 1){
		printf("WARNING: in %s, associative sequence with more than one output -> skip\n", __func__);
		return 1;
	}

	if (asso_seq->nb_node > 1){
		printf("WARNING: in %s, associative sequence spread over multiple nodes -> skip\n", __func__);
		return 1;
	}

	if (asso_seq->nb_input == 1){
		asso_seq->buffer_input[asso_seq->nb_input ++] = SUBGRAPHISOMORPHISM_JOKER_LABEL;
	}

	qsort(asso_seq->buffer_input, asso_seq->nb_input, sizeof(uint32_t), irSaturate_compare_label);
	qsort(asso_seq->buffer_output, asso_seq->nb_output, sizeof(uint32_t), irSaturate_compare_label);

	return 0;
}


static int32_t irSaturate_compare_asso_seq(struct assoSeq* asso_seq1, struct assoSeq* asso_seq2){
	if ((asso_seq1->opcode != asso_seq2->opcode) || (asso_seq1->nb_input != asso_seq2->nb_input) || (asso_seq1->nb_output != asso_seq2->nb_output)){
		return 1;
	}

	return memcmp(asso_seq1->buffer_input, asso_seq2->buffer_input, sizeof(uint32_t)) | memcmp(asso_seq1->buffer_output, asso_seq2->buffer_output, sizeof(uint32_t));
}

void irSaturate_saturate(struct saturateRules* saturate_rules, struct ir* ir){
	if (!saturateLayer_is_empty(&(ir->saturate_layer))){
		#ifdef VERBOSE
		printf("INFO: in %s, removing former saturation layer\n", __func__);
		#endif
		saturateLayer_reset(ir, &(ir->saturate_layer));
	}

	irSaturate_asso(saturate_rules, ir);

	saturateLayer_remove_redundant_element(&(ir->saturate_layer));
	saturateLayer_commit(ir, &(ir->saturate_layer));

	#ifdef VERBOSE
	{
		uint32_t i;
		uint32_t nb_node;
		uint32_t nb_edge;

		for (i = 0, nb_node = 0, nb_edge = 0; i < array_get_length(&(ir->saturate_layer)); i++){
			switch (((struct saturateElement*)array_get(&(ir->saturate_layer), i))->type){
				case SATURATE_ELEMENT_NODE : {
					nb_node ++;
					break;
				}
				case SATURATE_ELEMENT_EDGE : {
					nb_edge ++;
					break;
				}
				case SATURATE_ELEMENT_INVALID : {
					break;
				}
			}
		}
		printf("INFO: in %s, %u node(s) and %u edge(s) has been added to the graph (%u are redundant)\n", __func__, nb_node, nb_edge, array_get_length(&(ir->saturate_layer)) - (nb_node + nb_edge));
	}
	#endif
}

static const uint8_t T[256] = {
	 98,  6, 85,150, 36, 23,112,164,135,207,169,  5, 26, 64,165,219,
	 61, 20, 68, 89,130, 63, 52,102, 24,229,132,245, 80,216,195,115,
	 90,168,156,203,177,120,  2,190,188,  7,100,185,174,243,162, 10,
	237, 18,253,225,  8,208,172,244,255,126,101, 79,145,235,228,121,
	123,251, 67,250,161,  0,107, 97,241,111,181, 82,249, 33, 69, 55,
	 59,153, 29,  9,213,167, 84, 93, 30, 46, 94, 75,151,114, 73,222,
	197, 96,210, 45, 16,227,248,202, 51,152,252,125, 81,206,215,186,
	 39,158,178,187,131,136,  1, 49, 50, 17,141, 91, 47,129, 60, 99,
	154, 35, 86,171,105, 34, 38,200,147, 58, 77,118,173,246, 76,254,
	133,232,196,144,198,124, 53,  4,108, 74,223,234,134,230,157,139,
	189,205,199,128,176, 19,211,236,127,192,231, 70,233, 88,146, 44,
	183,201, 22, 83, 13,214,116,109,159, 32, 95,226,140,220, 57, 12,
	221, 31,209,182,143, 92,149,184,148, 62,113, 65, 37, 27,106,166,
	  3, 14,204, 72, 21, 41, 56, 66, 28,193, 40,217, 25, 54,179,117,
	238, 87,240,155,180,170,242,212,191,163, 78,218,137,194,175,110,
	 43,119,224, 71,122,142, 42,160,104, 48,247,103, 15, 11,138,239 
};

union saturateElementWrapper{
	struct{
		union saturateElementWrapper* 	src_head;
		union saturateElementWrapper* 	dst_head;
		union saturateElementWrapper* 	ll;
	} 									node;
	struct{
		union saturateElementWrapper* 	src_ll;
		union saturateElementWrapper* 	dst_ll;
		struct saturateElement* 		ptr;
	} 									edge;
};

static uint8_t saturateLayer_compute_pearson_hash(union saturateElementWrapper* wrapper){
	union saturateElementWrapper* 	cursor;
	uint8_t 						local_hash;
	uint8_t 						global_hash;

	for (cursor = wrapper->node.dst_head, global_hash = 0; cursor != NULL; cursor = cursor->edge.dst_ll){
		if (cursor->edge.ptr->element_type.edge.src_ptr == NULL){
			local_hash = T[cursor->edge.ptr->element_type.edge.src_id & 0xff];
			local_hash = T[local_hash ^ ((cursor->edge.ptr->element_type.edge.src_id >> 8 ) & 0xff)];
			local_hash = T[local_hash ^ ((cursor->edge.ptr->element_type.edge.src_id >> 16) & 0xff)];
			local_hash = T[local_hash ^ ((cursor->edge.ptr->element_type.edge.src_id >> 24) & 0xff)];
		}
		else{
			local_hash = T[(uint32_t)cursor->edge.ptr->element_type.edge.src_ptr & 0xff];
			local_hash = T[local_hash ^ (((uint32_t)cursor->edge.ptr->element_type.edge.src_ptr >> 8 ) & 0xff)];
			local_hash = T[local_hash ^ (((uint32_t)cursor->edge.ptr->element_type.edge.src_ptr >> 16) & 0xff)];
			local_hash = T[local_hash ^ (((uint32_t)cursor->edge.ptr->element_type.edge.src_ptr >> 24) & 0xff)];
		}
		global_hash ^= local_hash;
	}

	return global_hash;
}

static int32_t saturateLayer_compare_saturate_node(union saturateElementWrapper* wrapper1, union saturateElementWrapper* wrapper2){
	uint32_t 						nb_edge_node1;
	uint32_t 						nb_edge_node2;
	union saturateElementWrapper* 	cursor1;
	union saturateElementWrapper* 	cursor2;

	for (cursor1 = wrapper1->node.dst_head, nb_edge_node1 = 0; cursor1 != NULL; cursor1 = cursor1->edge.dst_ll){
		nb_edge_node1 ++;
	}

	for (cursor2 = wrapper2->node.dst_head, nb_edge_node2 = 0; cursor2 != NULL; cursor2 = cursor2->edge.dst_ll){
		nb_edge_node2 ++;
	}

	if (nb_edge_node1 != nb_edge_node2){
		return 1;
	}

	for (cursor1 = wrapper1->node.dst_head; cursor1 != NULL; cursor1 = cursor1->edge.dst_ll){
		for (cursor2 = wrapper2->node.dst_head; cursor2 != NULL; cursor2 = cursor2->edge.dst_ll){
			if ((cursor1->edge.ptr->element_type.edge.src_ptr != NULL && cursor1->edge.ptr->element_type.edge.src_ptr != NULL) && cursor1->edge.ptr->element_type.edge.src_ptr == cursor2->edge.ptr->element_type.edge.src_ptr){
				break;
			}
			else if ((cursor1->edge.ptr->element_type.edge.src_ptr == NULL && cursor1->edge.ptr->element_type.edge.src_ptr == NULL) && cursor1->edge.ptr->element_type.edge.src_id == cursor2->edge.ptr->element_type.edge.src_id){
				break;
			}
		}
		if (cursor2 == NULL){
			return 1;
		}
	}

	return 0;
}

static int32_t saturateLayer_compare_saturate_edge(union saturateElementWrapper* wrapper1, union saturateElementWrapper* wrapper2){
	if ((wrapper1->edge.ptr->element_type.edge.dst_ptr != NULL && wrapper2->edge.ptr->element_type.edge.dst_ptr != NULL) && (wrapper1->edge.ptr->element_type.edge.dst_ptr == wrapper2->edge.ptr->element_type.edge.dst_ptr)){
		return 0;
	}
	if ((wrapper1->edge.ptr->element_type.edge.dst_ptr == NULL && wrapper2->edge.ptr->element_type.edge.dst_ptr == NULL) && (wrapper1->edge.ptr->element_type.edge.dst_id == wrapper2->edge.ptr->element_type.edge.dst_id)){
		return 0;
	}

	return 1;
}

void saturateLayer_remove_redundant_element(struct array* layer){
	uint32_t 						i;
	union saturateElementWrapper* 	wrapper_buffer;
	struct saturateElement* 		saturate_element;
	union saturateElementWrapper* 	hash_map[256];
	uint32_t 						stop = 0;
	uint8_t 						hash;
	union saturateElementWrapper* 	wrapper_cursor;
	union saturateElementWrapper*	edge_cursor1;
	union saturateElementWrapper*	edge_cursor2;
	union saturateElementWrapper*	tmp;

	wrapper_buffer = (union saturateElementWrapper*)malloc(sizeof(union saturateElementWrapper) * array_get_length(layer));
	if (wrapper_buffer == NULL){
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
		return;
	}

	for (i = 0; i < array_get_length(layer); i++){
		saturate_element = (struct saturateElement*)array_get(layer, i);

		switch(saturate_element->type){
			case SATURATE_ELEMENT_NODE : {
				wrapper_buffer[i].node.src_head = NULL;
				wrapper_buffer[i].node.dst_head = NULL;
				wrapper_buffer[i].node.ll = NULL;
				break;
			}
			case SATURATE_ELEMENT_EDGE : {
				wrapper_buffer[i].edge.ptr = saturate_element;
				if (saturate_element->element_type.edge.src_ptr == NULL){
					wrapper_buffer[i].edge.src_ll = wrapper_buffer[saturate_element->element_type.edge.src_id].node.src_head;
					wrapper_buffer[saturate_element->element_type.edge.src_id].node.src_head = wrapper_buffer + i;
				}
				else{
					wrapper_buffer[i].edge.src_ll = NULL;
				}
				if (saturate_element->element_type.edge.dst_ptr == NULL){
					wrapper_buffer[i].edge.dst_ll = wrapper_buffer[saturate_element->element_type.edge.dst_id].node.dst_head;
					wrapper_buffer[saturate_element->element_type.edge.dst_id].node.dst_head = wrapper_buffer + i;
				}
				else{
					wrapper_buffer[i].edge.dst_ll = NULL;
				}
				break;
			}
			case SATURATE_ELEMENT_INVALID : {
				break;
			}
		}
	}

	while(!stop){
		memset(hash_map, 0, sizeof(hash_map));
		stop = 1;

		for (i = 0; i < array_get_length(layer); i++){
			saturate_element = (struct saturateElement*)array_get(layer, i);

			if (saturate_element->type == SATURATE_ELEMENT_NODE){
				for (edge_cursor1 = wrapper_buffer[i].node.src_head; edge_cursor1 != NULL; edge_cursor1 = edge_cursor1->edge.src_ll){
					if (edge_cursor1->edge.ptr->type != SATURATE_ELEMENT_INVALID){
						for (edge_cursor2 = edge_cursor1->edge.src_ll; edge_cursor2 != NULL; edge_cursor2 = edge_cursor2->edge.src_ll){
							if (saturateLayer_compare_saturate_edge(edge_cursor1, edge_cursor2) == 0){
								edge_cursor2->edge.ptr->type = SATURATE_ELEMENT_INVALID;
							}
						}
					}
				}


				hash = saturateLayer_compute_pearson_hash(wrapper_buffer + i);
				for (wrapper_cursor = hash_map[hash]; wrapper_cursor != NULL; wrapper_cursor = wrapper_cursor->node.ll){
					if (saturateLayer_compare_saturate_node(wrapper_cursor, wrapper_buffer + i) == 0){
						saturate_element->type = SATURATE_ELEMENT_INVALID;

						for (edge_cursor1 = wrapper_buffer[i].node.dst_head; edge_cursor1 != NULL; edge_cursor1 = edge_cursor1->edge.dst_ll){
							edge_cursor1->edge.ptr->type = SATURATE_ELEMENT_INVALID;
						}

						for (edge_cursor1 = wrapper_buffer[i].node.src_head; edge_cursor1 != NULL; edge_cursor1 = tmp){
							edge_cursor1->edge.ptr->element_type.edge.src_id = wrapper_cursor - wrapper_buffer;
							tmp = edge_cursor1->edge.src_ll;
							edge_cursor1->edge.src_ll = wrapper_cursor->node.src_head;
							wrapper_cursor->node.src_head = edge_cursor1;
						}

						stop = 0;
						break;
					}
				}
				if (wrapper_cursor == NULL){
					wrapper_buffer[i].node.ll = hash_map[hash];
					hash_map[hash] = wrapper_buffer + i;
				}
			}
		}
	}

	


	free(wrapper_buffer);
}

void saturateLayer_commit(struct ir* ir, struct array* layer){
	uint32_t 				i;
	struct saturateElement* saturate_element;

	for (i = 0; i < array_get_length(layer); i++){
		saturate_element = (struct saturateElement*)array_get(layer, i);

		switch(saturate_element->type){
			case SATURATE_ELEMENT_NODE : {
				if (saturate_element->element_type.node.type == IR_OPERATION_TYPE_INST){
					saturate_element->element_type.node.ptr = ir_add_inst(ir, IR_INSTRUCTION_INDEX_UNKOWN, saturate_element->element_type.node.size, saturate_element->element_type.node.opcode);
					if (saturate_element->element_type.node.ptr == NULL){
						printf("ERROR: in %s, unable to add operation to IR\n", __func__);
						return;
					}
				}
				else{
					printf("ERROR: in %s, node type not implemented\n", __func__);
				}
				break;
			}
			case SATURATE_ELEMENT_EDGE : {
				if (saturate_element->element_type.edge.src_ptr == NULL){
					saturate_element->element_type.edge.src_ptr = ((struct saturateElement*)array_get(layer, saturate_element->element_type.edge.src_id))->element_type.node.ptr;
				}

				if (saturate_element->element_type.edge.dst_ptr == NULL){
					saturate_element->element_type.edge.dst_ptr = ((struct saturateElement*)array_get(layer, saturate_element->element_type.edge.dst_id))->element_type.node.ptr;
				}

				if (saturate_element->element_type.edge.src_ptr != NULL && saturate_element->element_type.edge.src_ptr != NULL){
					saturate_element->element_type.edge.ptr = ir_add_dependence(ir, saturate_element->element_type.edge.src_ptr, saturate_element->element_type.edge.dst_ptr, saturate_element->element_type.edge.type);
					if (saturate_element->element_type.edge.ptr == NULL){
						printf("ERROR: in %s, unable to add dependence to IR\n", __func__);
					}
				}
				else{
					printf("ERROR: in %s, unable to add edge on the node is missing\n", __func__);
				}
				break;
			}
			case SATURATE_ELEMENT_INVALID : {
				break;
			}
		}
	}
}

void saturateLayer_remove(struct ir* ir, struct array* layer){
	uint32_t 				i;
	struct saturateElement* saturate_element;

	for (i = array_get_length(layer); i > 0; i--){
		saturate_element = (struct saturateElement*)array_get(layer, i - 1);
		switch(saturate_element->type){
			case SATURATE_ELEMENT_NODE : {
				if (saturate_element->element_type.node.ptr != NULL){
					graph_remove_node(&(ir->graph), saturate_element->element_type.node.ptr);
				}
				break;
			}
			case SATURATE_ELEMENT_EDGE : {
				if (saturate_element->element_type.edge.ptr != NULL){
					graph_remove_edge(&(ir->graph), saturate_element->element_type.edge.ptr);
				}
				break;
			}
			case SATURATE_ELEMENT_INVALID : {
				break;
			}
		}
	}
}

struct matchHeader{
	uint32_t nb_match;
	uint32_t state;
	uint32_t match_offset;
};

#define matchHeader_get_state_offset(header) ((header)->state + (header)->match_offset)

static void irSaturate_asso(struct saturateRules* saturate_rules, struct ir* ir){
	uint32_t 				i;
	uint32_t 				j;
	uint32_t 				k;
	struct assoGraphHeader 	graph_header;
	struct node* 			node_cursor;
	struct irOperation* 	operation;
	struct node** 			input_buffer;
	struct node** 			output_buffer;
	struct assoSeq* 		asso_seq;
	struct matchHeader 		input_match_header[IRSATURATE_ASSOSEQ_MAX_INPUT];
	struct matchHeader 		output_match_header[IRSATURATE_ASSOSEQ_MAX_OUTPUT];
	struct node** 			parameter_buffer = NULL;
	uint32_t 				parameter_buffer_size;
	struct node* 			selection_input[IRSATURATE_ASSOSEQ_MAX_INPUT];
	struct node* 			selection_output[IRSATURATE_ASSOSEQ_MAX_OUTPUT];
	struct node** 			selection_remaining = NULL;
	uint32_t 				selection_remaining_size;
	uint32_t 				nb_remaining;

	input_buffer = (struct node**)malloc(sizeof(struct node*) * ir->graph.nb_node);
	output_buffer = (struct node**)malloc(sizeof(struct node*) * ir->graph.nb_node);
	if (input_buffer == NULL || output_buffer == NULL){
		printf("ERROR: in %s unable to allocate memory\n", __func__);
		goto exit;
	}

	for (node_cursor = graph_get_head_node(&(ir->graph)); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
		node_cursor->ptr = 0;
	}

	for (node_cursor = graph_get_head_node(&(ir->graph)); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
		operation = ir_node_get_operation(node_cursor);
		if (node_cursor->ptr == 0 && operation->type == IR_OPERATION_TYPE_INST && irSaturate_opcode_is_associative(operation->operation_type.inst.opcode)){
			graph_header.opcode = operation->operation_type.inst.opcode;
			graph_header.nb_input = 0;
			graph_header.nb_output = 0;

			irSaturate_extract_asso_graph(node_cursor, &graph_header, input_buffer, output_buffer);

			for (i = 0; i < array_get_length(&(saturate_rules->asso_seq_array)); i++){
				asso_seq = (struct assoSeq*)array_get(&(saturate_rules->asso_seq_array), i);
				if (asso_seq->opcode == graph_header.opcode && asso_seq->nb_input <= graph_header.nb_input && asso_seq->nb_output <= graph_header.nb_output){

					if (parameter_buffer == NULL){
						parameter_buffer_size = IRSATURATE_ASSOSEQ_MAX_INPUT * graph_header.nb_input + IRSATURATE_ASSOSEQ_MAX_OUTPUT * graph_header.nb_output;
						parameter_buffer = (struct node**)malloc(sizeof(struct node*) * parameter_buffer_size);
						
						if (parameter_buffer == NULL){
							printf("ERROR: in %s, unable to allocate memory\n", __func__);
							continue;
						}
					}

					for (j = 0; j < asso_seq->nb_input; j++){
						if (j == 0){
							input_match_header[j].nb_match = 0;
							input_match_header[j].state = 0;
							input_match_header[j].match_offset = 0;
						}
						else{
							input_match_header[j].nb_match = 0;
							input_match_header[j].state = 0;
							input_match_header[j].match_offset = input_match_header[j - 1].match_offset + input_match_header[j - 1].nb_match;
						}

						for (k = 0; k < graph_header.nb_input; k++){
							if (asso_seq->buffer_input[j] == irNode_get_label(input_buffer[k]) || asso_seq->buffer_input[j] == SUBGRAPHISOMORPHISM_JOKER_LABEL){
								if (input_match_header[j].match_offset + input_match_header[j].nb_match == parameter_buffer_size){
									parameter_buffer_size = IRSATURATE_ASSOSEQ_MAX_INPUT * graph_header.nb_input + IRSATURATE_ASSOSEQ_MAX_OUTPUT * graph_header.nb_output;
									parameter_buffer = (struct node**)realloc(parameter_buffer, sizeof(struct node*) * parameter_buffer_size);
									if (parameter_buffer == NULL){
										printf("ERROR: in %s unable to realloc memory\n", __func__);
										goto exit;
									}
								}
								parameter_buffer[input_match_header[j].match_offset + input_match_header[j].nb_match] = input_buffer[k];
								input_match_header[j].nb_match ++;
							}
						}
						if (input_match_header[j].nb_match == 0){
							goto next;
						}
					}

					for (j = 0; j < asso_seq->nb_output; j++){
						if (j == 0){
							output_match_header[j].nb_match = 0;
							output_match_header[j].state = 0;
							output_match_header[j].match_offset = input_match_header[asso_seq->nb_input - 1].match_offset + input_match_header[asso_seq->nb_input - 1].nb_match;
						}
						else{
							output_match_header[j].nb_match = 0;
							output_match_header[j].state = 0;
							output_match_header[j].match_offset = output_match_header[j - 1].match_offset + output_match_header[j - 1].nb_match;
						}

						for (k = 0; k < graph_header.nb_output; k++){
							if (asso_seq->buffer_output[j] == irNode_get_label(output_buffer[k]) || asso_seq->buffer_output[j] == SUBGRAPHISOMORPHISM_JOKER_LABEL){
								if (output_match_header[j].match_offset + output_match_header[j].nb_match == parameter_buffer_size){
									parameter_buffer_size = IRSATURATE_ASSOSEQ_MAX_INPUT * graph_header.nb_input + IRSATURATE_ASSOSEQ_MAX_OUTPUT * graph_header.nb_output;
									parameter_buffer = (struct node**)realloc(parameter_buffer, sizeof(struct node*) * parameter_buffer_size);
									if (parameter_buffer == NULL){
										printf("ERROR: in %s unable to realloc memory\n", __func__);
										goto exit;
									}
								}
								parameter_buffer[output_match_header[j].match_offset + output_match_header[j].nb_match] = output_buffer[k];
								output_match_header[j].nb_match ++;
							}
						}
						if (output_match_header[j].nb_match == 0){
							goto next;
						}
					}

					if (selection_remaining == NULL){
						if (graph_header.nb_input > asso_seq->nb_input){
							selection_remaining_size = graph_header.nb_input - asso_seq->nb_input;
							selection_remaining = (struct node**)malloc(sizeof(struct node*) * selection_remaining_size);

							if (selection_remaining == NULL){
								printf("ERROR: in %s, unable to allocate memory\n", __func__);
								continue;
							}
						}
						else{
							selection_remaining_size = 0;
						}
					}
					else if (selection_remaining_size < graph_header.nb_input - asso_seq->nb_input){
						selection_remaining_size = graph_header.nb_input - asso_seq->nb_input;
						selection_remaining = (struct node**)realloc(selection_remaining, sizeof(struct node*) * selection_remaining_size);
						if (selection_remaining == NULL){
							printf("ERROR: in %s unable to realloc memory\n", __func__);
							continue;
						}
					}

					while(1){
						for (j = 0; j < asso_seq->nb_input;){
							selection_input[j] = parameter_buffer[matchHeader_get_state_offset(input_match_header + j)];
							for (k = 0; k < j; k++){
								if (selection_input[k] == selection_input[j]){
									break;
								}
							}
							if (k != j){
								while (input_match_header[j].state == input_match_header[j].nb_match - 1 && j != 0){
									input_match_header[j].state = 0;
									j--;
								}
								if (input_match_header[j].state != input_match_header[j].nb_match - 1){
									input_match_header[j].state ++;
								}
								else{
									goto next;
								}
							}
							else{
								j++;
							}
						}

						for (j = 0; j < asso_seq->nb_output;){
							selection_output[j] = parameter_buffer[matchHeader_get_state_offset(output_match_header + j)];
							for (k = 0; k < j; k++){
								if (selection_output[k] == selection_output[j]){
									break;
								}
							}
							if (k != j){
								while (output_match_header[j].state == output_match_header[j].nb_match - 1 && j != 0){
									output_match_header[j].state = 0;
									j--;
								}
								if (output_match_header[j].state != output_match_header[j].nb_match - 1){
									output_match_header[j].state ++;
								}
								else{
									goto inc_input;
								}
							}
							else{
								j++;
							}
						}

						nb_remaining = selection_remaining_size;
						if (!irSaturate_check_asso_subgraph(selection_input, asso_seq->nb_input, selection_output, asso_seq->nb_output, selection_remaining, &nb_remaining)){
							uint32_t 	reschedule_loc[IRSATURATE_ASSOSEQ_MAX_INPUT];
							uint32_t 	nb_reschedule;
							int32_t 	sequence_index;
							int32_t 	remaining_index;

							if (nb_remaining > 0){
								for (j = 0, nb_reschedule = 0; j < asso_seq->nb_input; j++){
									if (asso_seq->buffer_input[j] == SUBGRAPHISOMORPHISM_JOKER_LABEL){
										reschedule_loc[nb_reschedule ++] = j;
									}
								}
								if (nb_reschedule == 1){
									sequence_index = saturateLayer_push_operation(&(ir->saturate_layer), asso_seq->opcode);
									remaining_index = saturateLayer_push_operation(&(ir->saturate_layer), asso_seq->opcode);
									for (j = 0; j < asso_seq->nb_input; j++){
										if (reschedule_loc[0] != j){
											saturateLayer_push_edge(&(ir->saturate_layer), 0, selection_input[j], sequence_index, NULL);
										}
										else{
											saturateLayer_push_edge(&(ir->saturate_layer), 0, selection_input[j], remaining_index, NULL);
											saturateLayer_push_edge(&(ir->saturate_layer), remaining_index, NULL, sequence_index, NULL);
										}
									}
									for (j = 0; j < asso_seq->nb_output; j++){
										saturateLayer_push_edge(&(ir->saturate_layer), sequence_index, NULL, 0, selection_output[j]);
									}
									for (j = 0; j < nb_remaining; j++){
										saturateLayer_push_edge(&(ir->saturate_layer), 0, selection_remaining[j], remaining_index, NULL);
									}
								}
								else if(nb_reschedule > 1){
									printf("WARNING: in %s, I don't know where I should reschedule pending operation(s)\n", __func__);
								}
							}
							else{
								sequence_index = saturateLayer_push_operation(&(ir->saturate_layer), asso_seq->opcode);
								for (j = 0; j < asso_seq->nb_input; j++){
									saturateLayer_push_edge(&(ir->saturate_layer), 0, selection_input[j], sequence_index, NULL);
								}
								for (j = 0; j < asso_seq->nb_output; j++){
									saturateLayer_push_edge(&(ir->saturate_layer), sequence_index, NULL, 0, selection_output[j]);
								}
							}
						}

						if (asso_seq->nb_output == 0){
							goto inc_input;
						}

						j = asso_seq->nb_output - 1;
						while (output_match_header[j].state == output_match_header[j].nb_match - 1 && j != 0){
							output_match_header[j].state = 0;
							j--;
						}
						if (j != 0){
							output_match_header[j].state ++;
						}
						else{
							inc_input:
							j = asso_seq->nb_input - 1;
							while (input_match_header[j].state == input_match_header[j].nb_match - 1 && j != 0){
								input_match_header[j].state = 0;
								j--;
							}
							if (input_match_header[j].state != input_match_header[j].nb_match - 1){
								input_match_header[j].state ++;
							}
							else{
								goto next;
							}
						}
					}

					next:;
				}
			}
		}
	}


	exit:
	if (selection_remaining != NULL){
		free(selection_remaining);
	}
	if (parameter_buffer != NULL){
		free(parameter_buffer);
	}
	if (input_buffer != NULL){
		free(input_buffer);
	}
	if (output_buffer != NULL){
		free(output_buffer);
	}
}

static void irSaturate_extract_asso_graph(struct node* node, struct assoGraphHeader* header, struct node** input_buffer, struct node** output_buffer){
	struct edge* 		edge_cursor;
	struct node* 		node_cursor;
	struct irOperation* operation;

	node->ptr = (void*)1;

	for (edge_cursor = node_get_head_edge_src(node); edge_cursor != NULL; edge_cursor = edge_get_next_src(edge_cursor)){
		node_cursor = edge_get_dst(edge_cursor);
		operation = ir_node_get_operation(node_cursor);

		if (node_cursor->ptr == 0 && operation->type == IR_OPERATION_TYPE_INST && operation->operation_type.inst.opcode == header->opcode){
			irSaturate_extract_asso_graph(node_cursor, header, input_buffer, output_buffer);
		}
		else{
			output_buffer[header->nb_output ++] = node_cursor;
		}
	}

	for (edge_cursor = node_get_head_edge_dst(node); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
		node_cursor = edge_get_src(edge_cursor);
		operation = ir_node_get_operation(node_cursor);

		if (node_cursor->ptr == 0 && operation->type == IR_OPERATION_TYPE_INST && operation->operation_type.inst.opcode == header->opcode){
			irSaturate_extract_asso_graph(node_cursor, header, input_buffer, output_buffer);
		}
		else{
			input_buffer[header->nb_input ++] = node_cursor;
		}
	}
}

static int32_t irSaturate_check_asso_subgraph(struct node** selection_input, uint32_t nb_input, struct node** selection_output, uint32_t nb_output, struct node** selection_remaining, uint32_t* nb_remaining){
	
	if (irSaturate_is_node_yet_exist(selection_input, nb_input, selection_output, nb_output) == 0){
		return 1;
	}

	if (nb_output == 0){
		printf("ERROR: in %s, no output this case is not implemented yet\n", __func__);
	}
	else if (nb_output == 1){
		uint32_t 			i;
		enum irOpcode 		opcode;
		struct edge* 		edge_buffer[IRSATURATE_MAX_RECURSION_LEVEL];
		uint8_t 			dst[IRSATURATE_ASSOSEQ_MAX_INPUT] = {0};
		struct edge* 		edge_cursor;
		struct node*		node_cursor;
		struct irOperation* operation_cursor;
		uint32_t 			dst_ctr;
		uint32_t 			remaining_offset;


		
		for (edge_cursor = node_get_head_edge_dst(selection_output[0]); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
			node_cursor = edge_get_src(edge_cursor);
			operation_cursor = ir_node_get_operation(node_cursor);

			if (operation_cursor->type == IR_OPERATION_TYPE_INST && irSaturate_opcode_is_associative(operation_cursor->operation_type.inst.opcode)){
				opcode = operation_cursor->operation_type.inst.opcode;
				memset(dst, 0, sizeof(dst));

				edge_buffer[0] = node_get_head_edge_dst(node_cursor);
				for (dst_ctr = 0, remaining_offset = 0; ; ){
					node_cursor = edge_get_src(edge_buffer[dst_ctr]);
					operation_cursor = ir_node_get_operation(node_cursor);

					if (operation_cursor->type == IR_OPERATION_TYPE_INST && operation_cursor->operation_type.inst.opcode == opcode && dst_ctr + 1 < IRSATURATE_MAX_RECURSION_LEVEL){
						edge_buffer[++ dst_ctr] = node_get_head_edge_dst(node_cursor);
					}
					else{
						for (i = 0; i < nb_input; i++){
							if (node_cursor == selection_input[i]){
								if (dst[i] != 0){
									printf("ERROR: in %s, several paths from input to output\n", __func__);
								}
								dst[i] = dst_ctr + 1;
								break;
							}
						}
						if (i == nb_input){
							if (remaining_offset < *nb_remaining){
								selection_remaining[remaining_offset ++] = node_cursor;
							}
							else{
								printf("ERROR: in %s, the selection buffer for the remaining input is full\n", __func__);
							}
						}

						do{
							if (edge_buffer[dst_ctr] == NULL){
								if (dst_ctr != 0){
									dst_ctr --;
									edge_buffer[dst_ctr] = edge_get_next_dst(edge_buffer[dst_ctr]);
								}
								else{
									goto next;
								}
							}
							else{
								edge_buffer[dst_ctr] = edge_get_next_dst(edge_buffer[dst_ctr]);
							}
						} while(edge_buffer[dst_ctr] == NULL);
					}
				}
			}

			next:;
			for (i = 0; i < nb_input; i++){
				if (dst[i] == 0){
					break;
				}
			}
			if (i == nb_input){
				*nb_remaining = remaining_offset;
				return 0;
			}
		}
	}
	else{
		printf("WARNING: in %s, this case (%u output) is not implemented\n", __func__, nb_output);
	}

	return 1;
}

static int32_t irSaturate_is_node_yet_exist(struct node** selection_input, uint32_t nb_input, struct node** selection_output, uint32_t nb_output){
	struct edge* 	edge_cursor1;
	struct edge* 	edge_cursor2;
	struct node* 	node;
	uint32_t 		i;

	if (nb_input){
		for (edge_cursor1 = node_get_head_edge_src(selection_input[0]); edge_cursor1 != NULL; edge_cursor1 = edge_get_next_src(edge_cursor1)){
			node = edge_get_dst(edge_cursor1);

			if (node->nb_edge_dst == nb_input && node->nb_edge_src >= nb_output && ir_node_get_operation(node)->type == IR_OPERATION_TYPE_INST){
				for (edge_cursor2 = node_get_head_edge_dst(node); edge_cursor2 != NULL; edge_cursor2 = edge_get_next_dst(edge_cursor2)){
					for (i = 0; i < nb_input; i++){
						if (edge_get_src(edge_cursor2) == selection_input[i]){
							break;
						}
					}
					if (i == nb_input){
						break;
					}
				}
				if (edge_cursor2 != NULL){
					continue;
				}

				for (i = 0; i < nb_output; i++){
					for (edge_cursor2 = node_get_head_edge_src(node); edge_cursor2 != NULL; edge_cursor2 = edge_get_next_src(edge_cursor2)){
						if (edge_get_dst(edge_cursor2) == selection_output[i]){
							break;
						}
					}
					if (edge_cursor2 == NULL){
						break;
					}
				}
				if (i == nb_output){
					return 0;
				}
			}
		}
	}
	else if (nb_output){
		for (edge_cursor1 = node_get_head_edge_dst(selection_output[0]); edge_cursor1 != NULL; edge_cursor1 = edge_get_next_dst(edge_cursor1)){
			node = edge_get_src(edge_cursor1);

			if (node->nb_edge_src >= nb_output && ir_node_get_operation(node)->type == IR_OPERATION_TYPE_INST){
				for (i = 0; i < nb_output; i++){
					for (edge_cursor2 = node_get_head_edge_src(node); edge_cursor2 != NULL; edge_cursor2 = edge_get_next_src(edge_cursor2)){
						if (edge_get_dst(edge_cursor2) == selection_output[i]){
							break;
						}
					}
					if (edge_cursor2 == NULL){
						break;
					}
				}
				if (i == nb_output){
					return 0;
				}
			}
		}
	}
	else{
		printf("ERROR: in %s, not input and no output, this method should not have been called in a first place\n", __func__);
	}

	return 1;
}

void saturateRules_print(struct saturateRules* saturate_rules){
	uint32_t 		i;
	uint32_t 		j;
	struct assoSeq* asso_seq;

	if (array_get_length(&(saturate_rules->asso_seq_array))){
		printf("Associative conflict(s) detected in the signature collection:\n");
		
		for (i = 0; i < array_get_length(&(saturate_rules->asso_seq_array)); i++){
			asso_seq = (struct assoSeq*)array_get(&(saturate_rules->asso_seq_array), i);

			printf("\t- %s {", irOpcode_2_string(asso_seq->opcode));
			for (j = 0; j < asso_seq->nb_input; j++){
				if (j == asso_seq->nb_input - 1){
					printf("0x%08x} -> {", asso_seq->buffer_input[j]);
				}
				else{
					printf("0x%08x, ", asso_seq->buffer_input[j]);
				}
			}

			for (j = 0; j < asso_seq->nb_output; j++){
				if (j == asso_seq->nb_output - 1){
					printf("0x%08x", asso_seq->buffer_output[j]);
				}
				else{
					printf("0x%08x, ", asso_seq->buffer_output[j]);
				}
			}
			printf("}\n");
		}
	}
}