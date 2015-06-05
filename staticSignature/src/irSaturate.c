#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "irSaturate.h"

#define IRSATURATE_MAX_RECURSION_LEVEL 4


/* ===================================================================== */
/* Selection functions						                             */
/* ===================================================================== */

struct selection{
	uint32_t 		nb_input;
	uint32_t 		nb_output;
	struct node* 	buffer_input[ASSOSIG_MAX_INPUT];
	struct node* 	buffer_output[ASSOSIG_MAX_OUTPUT];
	struct node* 	equivalent_node;	
	struct array 	pending_array;
};

#define selection_init(selection) 				(array_init(&((selection)->pending_array), sizeof(struct node*)))
#define selection_add_pending(selection, node) 	array_add(&((selection)->pending_array), &(node))
#define selection_get_nb_pending(selection) 	array_get_length(&((selection)->pending_array))
#define selection_get_pending(selection, index) (*((struct node**)array_get(&((selection)->pending_array), index)))
#define selection_flush(selection) 				array_empty(&((selection)->pending_array));
#define selection_clean(selection) 				array_clean(&((selection)->pending_array));

/* ===================================================================== */
/* AssoGraph functions						                             */
/* ===================================================================== */

struct assoGraph{
	enum irOpcode 	opcode;
	uint32_t 		nb_input;
	uint32_t 		nb_output;
	struct node** 	buffer_input;
	struct node** 	buffer_output;
	struct array 	node_array;
};

static int32_t assoGraph_init(struct assoGraph* asso_graph, uint32_t nb_node);
static void assoGraph_reset(struct assoGraph* asso_graph, enum irOpcode);

#define ASSOGRAPH_NODE_TAG_FREE 	0x00000000
#define ASSOGRAPH_NODE_TAG_GRAPH 	0x80000000
#define ASSOGRAPH_NODE_TAG_DONE 	0x40000000
#define ASSOGRAPH_NODE_TAG_PATH 	0x20000000

#define assoGraph_is_node_free(node) 	((uint32_t)((node)->ptr) == ASSOGRAPH_NODE_TAG_FREE)
#define assoGraph_is_node_graph(node) 	((uint32_t)((node)->ptr) & ASSOGRAPH_NODE_TAG_GRAPH)
#define assoGraph_is_node_done(node) 	((uint32_t)((node)->ptr) == ASSOGRAPH_NODE_TAG_DONE)
#define assoGraph_is_node_path(node) 	((uint32_t)((node)->ptr) & ASSOGRAPH_NODE_TAG_PATH)

#define assoGraph_node_get_score(node) 	((uint32_t)((node)->ptr) & 0x0fffffff)

#define assoGraph_node_set_free(node) 	(node)->ptr = (void*)ASSOGRAPH_NODE_TAG_FREE
#define assoGraph_node_set_graph(node) 	(node)->ptr = (void*)ASSOGRAPH_NODE_TAG_GRAPH
#define assoGraph_node_set_done(node) 	(node)->ptr = (void*)ASSOGRAPH_NODE_TAG_DONE
#define assoGraph_node_set_path(node) 	(node)->ptr = (void*)(ASSOGRAPH_NODE_TAG_GRAPH | ASSOGRAPH_NODE_TAG_PATH)
#define assoGraph_node_set_score(node, score) (node)->ptr = (void*)(((score) & 0x0fffffff) | ASSOGRAPH_NODE_TAG_GRAPH | ASSOGRAPH_NODE_TAG_PATH)

static void assoGraph_extract(struct assoGraph* asso_graph, struct node* node);
static int32_t assoGraph_check_selection(struct assoGraph* asso_graph, struct selection* selection);

#define assoGraph_clean(asso_graph) 																\
	array_clean(&((asso_graph)->node_array)); 														\
	free((asso_graph)->buffer_input); 																\
	free((asso_graph)->buffer_output);

static int32_t assoGraph_init(struct assoGraph* asso_graph, uint32_t nb_node){
	asso_graph->buffer_input = (struct node**)malloc(sizeof(struct node*) * nb_node);
	asso_graph->buffer_output = (struct node**)malloc(sizeof(struct node*) * nb_node);
	if (asso_graph->buffer_input == NULL || asso_graph->buffer_output == NULL){
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
		if (asso_graph->buffer_input != NULL){
			free(asso_graph->buffer_input);
		}
		if (asso_graph->buffer_output != NULL){
			free(asso_graph->buffer_output);
		}
		return -1;
	}

	if (array_init(&(asso_graph->node_array), sizeof(struct node*))){
		printf("ERROR: in %s, unable to init array\n", __func__);
		free(asso_graph->buffer_input);
		free(asso_graph->buffer_output);
		return -1;
	}

	return 0;
}

static void assoGraph_reset(struct assoGraph* asso_graph, enum irOpcode opcode){
	uint32_t i;

	for (i = 0; i < array_get_length(&(asso_graph->node_array)); i++){
		assoGraph_node_set_done(*(struct node**)array_get(&(asso_graph->node_array), i));
	}

	array_empty(&(asso_graph->node_array));
	asso_graph->opcode = opcode;
	asso_graph->nb_input = 0;
	asso_graph->nb_output = 0;
}

static void assoGraph_extract(struct assoGraph* asso_graph, struct node* node){
	struct edge* 		edge_cursor;
	struct node* 		node_cursor;
	struct irOperation* operation;

	if (array_add(&(asso_graph->node_array), &node) < 0){
		printf("ERROR: in %s, unable to add element to array\n", __func__);
		return;
	}

	assoGraph_node_set_graph(node);

	for (edge_cursor = node_get_head_edge_src(node); edge_cursor != NULL; edge_cursor = edge_get_next_src(edge_cursor)){
		node_cursor = edge_get_dst(edge_cursor);
		operation = ir_node_get_operation(node_cursor);

		if (assoGraph_is_node_free(node_cursor) && operation->type == IR_OPERATION_TYPE_INST && operation->operation_type.inst.opcode == asso_graph->opcode){
			assoGraph_extract(asso_graph, node_cursor);
		}
		else{
			asso_graph->buffer_output[asso_graph->nb_output ++] = node_cursor;
		}
	}

	for (edge_cursor = node_get_head_edge_dst(node); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
		node_cursor = edge_get_src(edge_cursor);
		operation = ir_node_get_operation(node_cursor);

		if (assoGraph_is_node_free(node_cursor) && operation->type == IR_OPERATION_TYPE_INST && operation->operation_type.inst.opcode == asso_graph->opcode){
			assoGraph_extract(asso_graph, node_cursor);
		}
		else{
			asso_graph->buffer_input[asso_graph->nb_input ++] = node_cursor;
		}
	}
}

static int32_t assoGraph_check_selection(struct assoGraph* asso_graph, struct selection* selection){
	uint32_t 		i;
	uint32_t 		j;
	uint32_t 		k;
	struct edge* 	edge_buffer[IRSATURATE_MAX_RECURSION_LEVEL];
	struct node*	node;
	struct edge* 	edge_cursor;
	uint32_t 		dst;

	for (i = 0; i < array_get_length(&(asso_graph->node_array)); i++){
		assoGraph_node_set_graph(*(struct node**)array_get(&(asso_graph->node_array), i));
	}
	
	for (i = 0, selection->equivalent_node = NULL; i < selection->nb_input; i++){
		edge_buffer[0] = node_get_head_edge_src(selection->buffer_input[i]);
		for (j = 0; ; ){
			if (edge_buffer[j] == NULL){
				if (j == 0){
					break;
				}
				else{
					j --;
					edge_buffer[j] = edge_get_next_src(edge_buffer[j]);
				}
			}
			else{
				node = edge_get_dst(edge_buffer[j]);
				if (assoGraph_is_node_graph(node)){
					if (assoGraph_node_get_score(node) == i){
						assoGraph_node_set_score(node, i + 1);
						if (i + 1 == selection->nb_input){
							for (k = 0; k < selection->nb_output; k++){
								for (edge_cursor = node_get_head_edge_src(node); edge_cursor != NULL; edge_cursor = edge_get_next_src(edge_cursor)){
									if (edge_get_dst(edge_cursor) == selection->buffer_output[k]){
										break;
									}
								}
								if (edge_cursor == NULL){
									break;
								}
							}
							if (k == selection->nb_output){
								if (selection->equivalent_node){
									printf("WARNING: in %s, find several candidates\n", __func__);
								}
								selection->equivalent_node = node;
								dst = j + 1;
							}
						}
					}
					else if (assoGraph_node_get_score(node) == i + 1){
						printf("ERROR: in %s, the associative graph is not a tree\n", __func__);
						return 1;
					}
					else{
						assoGraph_node_set_path(node);
					}

					if (j + 1 != IRSATURATE_MAX_RECURSION_LEVEL){
						j ++;
						edge_buffer[j] = node_get_head_edge_src(node);
					}
					else{
						edge_buffer[j] = edge_get_next_src(edge_buffer[j]);
					}
				}
				else{
					edge_buffer[j] = edge_get_next_src(edge_buffer[j]);
				}
			}
		}
	}

	if (selection->equivalent_node){
		edge_buffer[0] = node_get_head_edge_dst(selection->equivalent_node);
		for (i = 0; ; ){
			if (edge_buffer[i] == NULL){
				if (i == 0){
					break;
				}
				else{
					i --;
					edge_buffer[i] = edge_get_next_dst(edge_buffer[i]);
				}
			}
			else{
				node = edge_get_src(edge_buffer[i]);
				if (assoGraph_is_node_path(node)){
					i ++;
					edge_buffer[i] = node_get_head_edge_dst(node);
				}
				else{
					for (j = 0; j < selection->nb_input; j++){
						if (node == selection->buffer_input[j]){
							break;
						}
					}
					if (j == selection->nb_input){
						if (selection_add_pending(selection, node) < 0){
							printf("ERROR: in %s, unable to add element to array\n", __func__);
						}
					}
					edge_buffer[i] = edge_get_next_dst(edge_buffer[i]);
				}
			}
		}

		if (dst == 1 && selection_get_nb_pending(selection) == 0){
			return 1;
		}
	}
	else{
		return 1;
	}

	return 0;
}

/* ===================================================================== */
/* AssoSig functions						                             */
/* ===================================================================== */

static uint32_t asso_sig_id_generator = 1;

#define assoSig_get_new_id() (asso_sig_id_generator ++)

#define assoSig_init(asso_sig, opcode_) 																		\
	(asso_sig)->id = assoSig_get_new_id(); 																		\
	(asso_sig)->opcode = opcode_; 																				\
	(asso_sig)->nb_input = 0; 																					\
	(asso_sig)->nb_output = 0; 																					\
	(asso_sig)->nb_node = 0;

static void assoSig_search(struct node* node, struct assoSig* asso_sig);
static int32_t assoSig_check(struct assoSig* asso_sig);
static void assoSig_format(struct assoSig* asso_sig);
static int32_t assoSig_compare(struct assoSig* asso_sig1, struct assoSig* asso_sig2);

static void assoSig_search(struct node* node, struct assoSig* asso_sig){
	struct edge* 			edge_cursor;
	struct node* 			node_cursor;
	struct signatureNode* 	operation;

	asso_sig->nb_node ++;
	node->ptr = (void*)asso_sig->id;

	for (edge_cursor = node_get_head_edge_src(node); edge_cursor != NULL; edge_cursor = edge_get_next_src(edge_cursor)){
		node_cursor = edge_get_dst(edge_cursor);
		operation = (struct signatureNode*)&(node_cursor->data);

		if (node_cursor->ptr == 0 && operation->type == SIGNATURE_NODE_TYPE_OPCODE && operation->node_type.opcode == asso_sig->opcode){
			assoSig_search(node_cursor, asso_sig);
		}
		else{
			if (asso_sig->nb_output < ASSOSIG_MAX_OUTPUT){
				asso_sig->buffer_output[asso_sig->nb_output ++] = signatureNode_get_label(node_cursor);
			}
			else{
				printf("ERROR: in %s, ASSOSIG_MAX_OUTPUT has been reached\n", __func__);
			}
		}
	}

	for (edge_cursor = node_get_head_edge_dst(node); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
		node_cursor = edge_get_src(edge_cursor);
		operation = (struct signatureNode*)&(node_cursor->data);

		if (node_cursor->ptr == 0 && operation->type == SIGNATURE_NODE_TYPE_OPCODE && operation->node_type.opcode == asso_sig->opcode){
			assoSig_search(node_cursor, asso_sig);
		}
		else{
			if (asso_sig->nb_input < ASSOSIG_MAX_INPUT){
				asso_sig->buffer_input[asso_sig->nb_input ++] = signatureNode_get_label(node_cursor);
			}
			else{
				printf("ERROR: in %s, ASSOSIG_MAX_INPUT has been reached\n", __func__);
			}
		}
	}
}

int32_t irSaturate_compare_label(const void* a, const void* b){
	if (*(uint32_t*)a > *(uint32_t*)b){
		return -1;
	}
	else if (*(uint32_t*)a < *(uint32_t*)b){
		return 1;
	}
	else{
		return 0;
	}
}

static int32_t assoSig_check(struct assoSig* asso_sig){
	if (asso_sig->nb_node > 1){
		printf("WARNING: in %s, associative signature spread over multiple nodes -> skip\n", __func__);
		return 1;
	}

	if (asso_sig->nb_input == 0){
		return 1;
	}

	return 0;
}

static void assoSig_format(struct assoSig* asso_sig){
	uint32_t i;

	if (asso_sig->nb_input == 1){
		asso_sig->buffer_input[asso_sig->nb_input ++] = SUBGRAPHISOMORPHISM_JOKER_LABEL;
	}

	qsort(asso_sig->buffer_input, asso_sig->nb_input, sizeof(uint32_t), irSaturate_compare_label);
	qsort(asso_sig->buffer_output, asso_sig->nb_output, sizeof(uint32_t), irSaturate_compare_label);

	for (i = 0; i < asso_sig->nb_input; i++){
		if (asso_sig->buffer_input[i] != SUBGRAPHISOMORPHISM_JOKER_LABEL){
			asso_sig->nb_reschedule_pending_in = i;
			break;
		}
	}
}

static int32_t assoSig_compare(struct assoSig* asso_sig1, struct assoSig* asso_sig2){
	if ((asso_sig1->opcode != asso_sig2->opcode) || (asso_sig1->nb_input != asso_sig2->nb_input) || (asso_sig1->nb_output != asso_sig2->nb_output)){
		return 1;
	}

	return memcmp(asso_sig1->buffer_input, asso_sig2->buffer_input, sizeof(uint32_t)) | memcmp(asso_sig1->buffer_output, asso_sig2->buffer_output, sizeof(uint32_t));
}

/* ===================================================================== */
/* SaturateBuilder functions						                     */
/* ===================================================================== */

enum saturateElement_type{
	SATURATE_ELEMENT_NODE,
	SATURATE_ELEMENT_EQUIVALENT_NODE,
	SATURATE_ELEMENT_EDGE,
	SATURATE_ELEMENT_SHARED_NODE,
	SATURATE_ELEMENT_SHARED_EDGE
};

struct saturateElement{
	enum saturateElement_type 		type;
	struct unode* 					layer_node;
	void* 							ptr;
	union {
		struct {
			enum irOperationType 	type;
			enum irOpcode 			opcode;
			uint8_t 				size;

		} 							node;
		struct {
			enum irOperationType 	type;
			enum irOpcode 			opcode;
			uint8_t 				size;
			struct node*			equi;
		} 							equi_node;
		struct {
			enum irDependenceType 	type;
			int32_t 				src_id;
			struct node*			src_ptr;
			int32_t 				dst_id;
			struct node*			dst_ptr;
		} 							edge;
		
		struct {
			uint32_t 				id;
		} 							shared;
	} 								element_type;
};

struct saturateBuilder{
	struct ugraph* 					graph_layer;
	struct array 					element_array;
};

static inline int32_t saturateBuilder_init(struct saturateBuilder* builder, struct ugraph* graph_layer){
	builder->graph_layer = graph_layer;
	return array_init(&(builder->element_array), sizeof(struct saturateElement));
}

#define saturateBuilder_clean(builder) array_clean(&((builder)->element_array))

union saturateElementWrapper{
	struct{
		union saturateElementWrapper* 	src_head;
		union saturateElementWrapper* 	dst_head;
		union saturateElementWrapper* 	ll;
		uint32_t 						id;
	} 									node;
	struct{
		union saturateElementWrapper* 	src_ll;
		union saturateElementWrapper* 	dst_ll;
		struct saturateElement* 		ptr;
		uint32_t 						id;
	} 									edge;
};

static uint8_t saturateBuilder_compute_pearson_hash(union saturateElementWrapper* wrapper);
static int32_t saturateBuilder_compare_saturate_node(union saturateElementWrapper* wrapper1, union saturateElementWrapper* wrapper2);
static int32_t saturateBuilder_compare_saturate_edge(struct saturateElement* element1, struct saturateElement* element2);
static void saturateBuilder_remove_redundant_element(struct saturateBuilder* builder);
static int32_t saturateBuilder_push_node(struct saturateBuilder* builder, struct unode** layer_node, enum irOpcode opcode);
static int32_t saturateBuilder_push_equivalent_node(struct saturateBuilder* builder, struct unode** layer_node, enum irOpcode opcode, struct node* equivalent_node);
static int32_t saturateBuilder_push_edge(struct saturateBuilder* builder, struct unode** layer_node, int32_t src_id, struct node* src_ptr, int32_t dst_id, struct node* dst_ptr);
static void saturateBuilder_push_selection(struct saturateBuilder* builder, struct selection* selection, struct assoSig* asso_sig);
static int32_t saturateBuilder_commit_node(struct saturateElement* element, struct ir* ir);
static int32_t saturateBuilder_commit_equivalent_node(struct saturateElement* element, struct ir* ir);
static int32_t saturateBuilder_commit_edge(struct saturateBuilder* builder, struct saturateElement* element, struct ir* ir);
static int32_t saturateBuilder_commit_shared_node(struct saturateBuilder* builder, struct saturateElement* element, struct ir* ir);
static int32_t saturateBuilder_commit_shared_edge(struct saturateBuilder* builder, struct saturateElement* element, struct ir* ir);
static int32_t saturateBuilder_commit_element(struct saturateBuilder* builder, struct saturateElement* element, struct ir* ir);
static void saturateBuilder_commit(struct saturateBuilder* builder, struct ir* ir);

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

static uint8_t saturateBuilder_compute_pearson_hash(union saturateElementWrapper* wrapper){
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

static int32_t saturateBuilder_compare_saturate_node(union saturateElementWrapper* wrapper1, union saturateElementWrapper* wrapper2){
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

static int32_t saturateBuilder_compare_saturate_edge(struct saturateElement* element1, struct saturateElement* element2){
	if ((element1->element_type.edge.dst_ptr != NULL && element2->element_type.edge.dst_ptr != NULL) && (element1->element_type.edge.dst_ptr == element2->element_type.edge.dst_ptr)){
		return 0;
	}
	if ((element1->element_type.edge.dst_ptr == NULL && element2->element_type.edge.dst_ptr == NULL) && (element1->element_type.edge.dst_id == element2->element_type.edge.dst_id)){
		return 0;
	}

	return 1;
}

static void saturateBuilder_remove_redundant_element(struct saturateBuilder* builder){
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

	wrapper_buffer = (union saturateElementWrapper*)malloc(sizeof(union saturateElementWrapper) * array_get_length(&(builder->element_array)));
	if (wrapper_buffer == NULL){
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
		return;
	}

	for (i = 0; i < array_get_length(&(builder->element_array)); i++){
		saturate_element = (struct saturateElement*)array_get(&(builder->element_array), i);

		switch(saturate_element->type){
			case SATURATE_ELEMENT_NODE :
			case SATURATE_ELEMENT_EQUIVALENT_NODE : {
				wrapper_buffer[i].node.src_head = NULL;
				wrapper_buffer[i].node.dst_head = NULL;
				wrapper_buffer[i].node.ll = NULL;
				wrapper_buffer[i].node.id = i;
				break;
			}
			case SATURATE_ELEMENT_EDGE : {
				wrapper_buffer[i].edge.ptr = saturate_element;
				wrapper_buffer[i].edge.id = i;
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
			case SATURATE_ELEMENT_SHARED_NODE : {
				break;
			}
			case SATURATE_ELEMENT_SHARED_EDGE : {
				break;
			}
		}
	}

	while(!stop){
		memset(hash_map, 0, sizeof(hash_map));
		stop = 1;

		for (i = 0; i < array_get_length(&(builder->element_array)); i++){
			saturate_element = (struct saturateElement*)array_get(&(builder->element_array), i);

			if (saturate_element->type == SATURATE_ELEMENT_NODE || saturate_element->type == SATURATE_ELEMENT_EQUIVALENT_NODE){
				/* Search duplicate edge */
				for (edge_cursor1 = wrapper_buffer[i].node.src_head; edge_cursor1 != NULL; edge_cursor1 = edge_cursor1->edge.src_ll){
					if (edge_cursor1->edge.ptr->type != SATURATE_ELEMENT_SHARED_EDGE){
						for (edge_cursor2 = edge_cursor1->edge.src_ll; edge_cursor2 != NULL; edge_cursor2 = edge_cursor2->edge.src_ll){
							if (saturateBuilder_compare_saturate_edge(edge_cursor1->edge.ptr, edge_cursor2->edge.ptr) == 0){
								edge_cursor2->edge.ptr->type = SATURATE_ELEMENT_SHARED_EDGE;
								edge_cursor2->edge.ptr->element_type.shared.id = edge_cursor1->edge.id;
							}
						}
					}
				}

				/* Search duplicate node */
				hash = saturateBuilder_compute_pearson_hash(wrapper_buffer + i);
				for (wrapper_cursor = hash_map[hash]; wrapper_cursor != NULL; wrapper_cursor = wrapper_cursor->node.ll){
					if (saturateBuilder_compare_saturate_node(wrapper_cursor, wrapper_buffer + i) == 0){
						saturate_element->type = SATURATE_ELEMENT_SHARED_NODE;
						saturate_element->element_type.shared.id = wrapper_cursor->node.id;

						for (edge_cursor1 = wrapper_buffer[i].node.dst_head, edge_cursor2 = wrapper_cursor->node.dst_head; edge_cursor1 != NULL && edge_cursor2 != NULL; edge_cursor1 = edge_cursor1->edge.dst_ll, edge_cursor2 = edge_cursor2->edge.dst_ll){
							edge_cursor1->edge.ptr->type = SATURATE_ELEMENT_SHARED_EDGE;
							edge_cursor1->edge.ptr->element_type.shared.id = edge_cursor2->edge.id;
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

static int32_t saturateBuilder_push_node(struct saturateBuilder* builder, struct unode** layer_node, enum irOpcode opcode){
	struct saturateElement element;

	if (*layer_node == NULL){
		*layer_node = graphLayer_new_layer(builder->graph_layer);
		if (*layer_node == NULL){
			printf("ERROR: in %s, unable to create new layer\n", __func__);
			return -1;
		}
	}

	element.type 						= SATURATE_ELEMENT_NODE;
	element.layer_node 					= *layer_node;
	element.ptr 						= NULL;
	element.element_type.node.type 		= IR_OPERATION_TYPE_INST;
	element.element_type.node.opcode 	= opcode;
	element.element_type.node.size 		= 32;

	return array_add(&(builder->element_array), &element);
}

static int32_t saturateBuilder_push_equivalent_node(struct saturateBuilder* builder, struct unode** layer_node, enum irOpcode opcode, struct node* equivalent_node){
	struct saturateElement element;

	if (*layer_node == NULL){
		*layer_node = graphLayer_new_layer(builder->graph_layer);
		if (*layer_node == NULL){
			printf("ERROR: in %s, unable to create new layer\n", __func__);
			return -1;
		}
	}

	element.type 							= SATURATE_ELEMENT_EQUIVALENT_NODE;
	element.layer_node 						= *layer_node;
	element.ptr 							= NULL;
	element.element_type.equi_node.type 	= IR_OPERATION_TYPE_INST;
	element.element_type.equi_node.opcode 	= opcode;
	element.element_type.equi_node.size 	= 32;
	element.element_type.equi_node.equi 	= equivalent_node;

	return array_add(&(builder->element_array), &element);
}

static int32_t saturateBuilder_push_edge(struct saturateBuilder* builder, struct unode** layer_node, int32_t src_id, struct node* src_ptr, int32_t dst_id, struct node* dst_ptr){
	struct saturateElement element;

	if ((src_id < 0 && src_ptr == NULL) || (dst_id < 0 && dst_ptr == NULL)){
		printf("ERROR: in %s, incorrect element id for src or dst\n", __func__);
		return - 1;
	}

	if (*layer_node == NULL){
		*layer_node = graphLayer_new_layer(builder->graph_layer);
		if (*layer_node == NULL){
			printf("ERROR: in %s, unable to create new layer\n", __func__);
			return -1;
		}
	}

	element.type 						= SATURATE_ELEMENT_EDGE;
	element.layer_node 					= *layer_node;
	element.ptr 						= NULL;
	element.element_type.edge.type 		= IR_DEPENDENCE_TYPE_DIRECT;
	element.element_type.edge.src_id 	= src_id;
	element.element_type.edge.src_ptr 	= src_ptr;
	element.element_type.edge.dst_id 	= dst_id;
	element.element_type.edge.dst_ptr 	= dst_ptr;

	return array_add(&(builder->element_array), &element);
}

static void saturateBuilder_push_selection(struct saturateBuilder* builder, struct selection* selection, struct assoSig* asso_sig){
	uint32_t 		i;
	int32_t 		index_signature;
	int32_t 		index_pending;
	struct unode* 	layer_node = NULL;

	if (selection_get_nb_pending(selection) > 0){ 
		switch (asso_sig->nb_reschedule_pending_in){
			case 0 : {
				break;
			}
			case 1 : {
				index_signature = saturateBuilder_push_equivalent_node(builder, &layer_node, asso_sig->opcode, selection->equivalent_node);
				index_pending = saturateBuilder_push_node(builder, &layer_node, asso_sig->opcode);
				for (i = 0; i < asso_sig->nb_input; i++){
					if (i == 0){
						saturateBuilder_push_edge(builder, &layer_node, 0, selection->buffer_input[i], index_pending, NULL);
						saturateBuilder_push_edge(builder, &layer_node, index_pending, NULL, index_signature, NULL);
					}
					else{
						saturateBuilder_push_edge(builder, &layer_node, 0, selection->buffer_input[i], index_signature, NULL);
					}
				}
				for (i = 0; i < selection_get_nb_pending(selection); i++){
					saturateBuilder_push_edge(builder, &layer_node, 0, selection_get_pending(selection, i), index_pending, NULL);
				}
				break;
			}
			default : {
				printf("WARNING: in %s, I don't know where I should reschedule pending operation(s)\n", __func__);
				break;
			}
		}
	}
	else{
		index_signature = saturateBuilder_push_equivalent_node(builder, &layer_node, asso_sig->opcode, selection->equivalent_node);
		for (i = 0; i < selection->nb_input; i++){
			saturateBuilder_push_edge(builder, &layer_node, 0, selection->buffer_input[i], index_signature, NULL);
		}
	}
}

static int32_t saturateBuilder_commit_node(struct saturateElement* element, struct ir* ir){
	if (element->element_type.node.type == IR_OPERATION_TYPE_INST){
		element->ptr = ir_add_inst(ir, IR_INSTRUCTION_INDEX_UNKOWN, element->element_type.node.size, element->element_type.node.opcode);
		if (element->ptr == NULL){
			printf("ERROR: in %s, unable to add operation to IR\n", __func__);
			return - 1;
		}
		if (irLayer_add_node(element->layer_node, element->ptr)){
			printf("ERROR: in %s, unable to add node to layer\n", __func__);
			return -1;
		}
	}
	else{
		printf("ERROR: in %s, node type not implemented\n", __func__);
		return -1;
	}

	return 0;
}

static int32_t saturateBuilder_commit_equivalent_node(struct saturateElement* element, struct ir* ir){
	struct irEquivalenceClass* 	class;
	struct setIterator 			iterator;
	struct edge* 				edge_cursor;
	struct edge* 				new_edge;
	struct node** 				equi_node_ptr;

	if (element->element_type.equi_node.type == IR_OPERATION_TYPE_INST){
		element->ptr = ir_add_inst(ir, IR_INSTRUCTION_INDEX_UNKOWN, element->element_type.equi_node.size, element->element_type.equi_node.opcode);
		if (element->ptr == NULL){
			printf("ERROR: in %s, unable to add operation to IR\n", __func__);
			return - 1;
		}
		if (irLayer_add_node(element->layer_node, element->ptr)){
			printf("ERROR: in %s, unable to add equivalent node to layer\n", __func__);
			return -1;
		}

		class = ir_node_equivalenceClass_get(element->element_type.equi_node.equi);
		if (class == NULL){
			class = irEquivalenceClass_add_node(NULL, element->element_type.equi_node.equi);
		}

		if (class != NULL){
			if ((equi_node_ptr = (struct node**)setIterator_get_first(class, &iterator)) != NULL){
				for (edge_cursor = node_get_head_edge_src(*equi_node_ptr) ; edge_cursor != NULL; edge_cursor = edge_get_next_src(edge_cursor)){
					new_edge = ir_add_dependence(ir, element->ptr, edge_get_dst(edge_cursor), ir_edge_get_dependence(edge_cursor)->type);
					if (new_edge == NULL){
						printf("ERROR: in %s, unable to add dependence to IR\n", __func__);
						return -1;
					}
					else{
						if (irLayer_add_edge(element->layer_node, new_edge)){
							printf("ERROR: in %s, unable to add edge to layer\n", __func__);
							return -1;
						}
					}
				}
			}
			else{
				printf("ERROR: in %s, this case is not supposed to happen: referenced but empty equivalentClass\n", __func__);
				return -1;
			}

			if (irEquivalenceClass_add_node(class, element->ptr) == NULL){
				printf("ERROR: in %s, unable to add node to equivalenceClass\n", __func__);
				return -1;
			}
		}
		else{
			printf("ERROR: in %s, unable to add node to equivalenceClass\n", __func__);
			return -1;
		}
	}
	else{
		printf("ERROR: in %s, equivalent node type not implemented\n", __func__);
		return -1;
	}

	return 0;
}

static int32_t saturateBuilder_commit_edge(struct saturateBuilder* builder, struct saturateElement* element, struct ir* ir){
	struct saturateElement* 	src_ptr;
	struct saturateElement* 	dst_ptr;
	struct irEquivalenceClass* 	class;
	struct setIterator 			iterator;
	struct node** 				src_node_ptr;
	struct edge* 				new_edge;

	if (element->element_type.edge.src_ptr == NULL){
		src_ptr = (struct saturateElement*)array_get(&(builder->element_array), element->element_type.edge.src_id);
		if (saturateBuilder_commit_element(builder, src_ptr, ir)){
			printf("ERROR: in %s, unable to commit element\n", __func__);
			return -1;
		}
		element->element_type.edge.src_ptr = src_ptr->ptr;
	}

	if (element->element_type.edge.dst_ptr == NULL){
		dst_ptr = (struct saturateElement*)array_get(&(builder->element_array), element->element_type.edge.dst_id);
		if (saturateBuilder_commit_element(builder, dst_ptr, ir)){
			printf("ERROR: in %s, unable to commit element\n", __func__);
			return -1;
		}
		element->element_type.edge.dst_ptr = dst_ptr->ptr;
	}

		
	element->ptr = ir_add_dependence(ir, element->element_type.edge.src_ptr, element->element_type.edge.dst_ptr, element->element_type.edge.type);
	if (element->ptr == NULL){
		printf("ERROR: in %s, unable to add dependence to IR\n", __func__);
		return -1;
	}
	else{
		if (irLayer_add_edge(element->layer_node, element->ptr)){
			printf("ERROR: in %s, unable to add edge to layer\n", __func__);
			return -1;
		}
	}

	class = ir_node_equivalenceClass_get(element->element_type.edge.src_ptr);
	if (class != NULL){
		for (src_node_ptr = (struct node**)setIterator_get_first(class, &iterator); src_node_ptr != NULL; src_node_ptr = (struct node**)setIterator_get_next(&iterator)){
			if (*src_node_ptr != element->element_type.edge.src_ptr){
				new_edge = ir_add_dependence(ir, *src_node_ptr, element->element_type.edge.dst_ptr, element->element_type.edge.type);
				if (new_edge == NULL){
					printf("ERROR: in %s, unable to add dependence to IR\n", __func__);
					return -1;
				}
				else{
					if (irLayer_add_edge(element->layer_node, new_edge)){
						printf("ERROR: in %s, unable to add edge to layer\n", __func__);
						return -1;
					}
				}
			}
		}
	}

	return 0;
}

static int32_t saturateBuilder_commit_shared_node(struct saturateBuilder* builder, struct saturateElement* element, struct ir* ir){
	struct saturateElement* shared_element;

	shared_element = (struct saturateElement*)array_get(&(builder->element_array), element->element_type.shared.id);
	if (saturateBuilder_commit_element(builder, shared_element, ir)){
		printf("ERROR: in %s, unable to commit element\n", __func__);
		return -1;
	}

	element->ptr = shared_element->ptr;
	if (irLayer_add_node(element->layer_node, element->ptr)){
		printf("ERROR: in %s, unable to add node to layer\n", __func__);
		return -1;
	}

	return 0;
}

static int32_t saturateBuilder_commit_shared_edge(struct saturateBuilder* builder, struct saturateElement* element, struct ir* ir){
	struct saturateElement* shared_element;

	shared_element = (struct saturateElement*)array_get(&(builder->element_array), element->element_type.shared.id);
	if (saturateBuilder_commit_element(builder, shared_element, ir)){
		printf("ERROR: in %s, unable to commit element\n", __func__);
		return -1;
	}

	element->ptr = shared_element->ptr;
	if (irLayer_add_edge(element->layer_node, element->ptr)){
		printf("ERROR: in %s, unable to add node to layer\n", __func__);
		return -1;
	}

	return 0;
}

static int32_t saturateBuilder_commit_element(struct saturateBuilder* builder, struct saturateElement* element, struct ir* ir){
	if (element->ptr == NULL){
		switch(element->type){
			case SATURATE_ELEMENT_NODE 				: {return saturateBuilder_commit_node(element, ir);}
			case SATURATE_ELEMENT_EQUIVALENT_NODE 	: {return saturateBuilder_commit_equivalent_node(element, ir);}
			case SATURATE_ELEMENT_EDGE 				: {return saturateBuilder_commit_edge(builder, element, ir);}
			case SATURATE_ELEMENT_SHARED_NODE 		: {return saturateBuilder_commit_shared_node(builder, element, ir);}
			case SATURATE_ELEMENT_SHARED_EDGE 		: {return saturateBuilder_commit_shared_edge(builder, element, ir);}
		}
	}
	
	return 0;
}

static void saturateBuilder_commit(struct saturateBuilder* builder, struct ir* ir){
	uint32_t i;

	for (i = 0; i < array_get_length(&(builder->element_array)); i++){
		saturateBuilder_commit_element(builder, (struct saturateElement*)array_get(&(builder->element_array), i), ir);
	}
}

/* ===================================================================== */
/* SaturateRules functions						                         */
/* ===================================================================== */

void saturateRules_learn_associative_conflict(struct saturateRules* saturate_rules, struct codeSignatureCollection* collection){
	struct codeSignature* 	signature;
	struct node* 			node_cursor_signature;
	struct node* 			node_cursor_operation;
	struct signatureNode*	operation;
	struct assoSig 			asso_sig;
	uint32_t 				i;

	for (node_cursor_signature = graph_get_head_node(&(collection->syntax_graph)); node_cursor_signature != NULL; node_cursor_signature = node_get_next(node_cursor_signature)){
		signature = syntax_node_get_codeSignature(node_cursor_signature);

		for (node_cursor_operation = graph_get_head_node(&(signature->graph)); node_cursor_operation != NULL; node_cursor_operation = node_get_next(node_cursor_operation)){
			node_cursor_operation->ptr = 0;
		}

		for (node_cursor_operation = graph_get_head_node(&(signature->graph)); node_cursor_operation != NULL; node_cursor_operation = node_get_next(node_cursor_operation)){
			operation = (struct signatureNode*)&(node_cursor_operation->data);
			if (node_cursor_operation->ptr == 0 && operation->type == SIGNATURE_NODE_TYPE_OPCODE && irSaturate_opcode_is_associative(operation->node_type.opcode)){
				assoSig_init(&asso_sig, operation->node_type.opcode);
				assoSig_search(node_cursor_operation, &asso_sig);
				if (!assoSig_check(&asso_sig)){
					assoSig_format(&asso_sig);
					for (i = 0; i < array_get_length(&(saturate_rules->asso_sig_array)); i++){
						if (!assoSig_compare(&asso_sig, (struct assoSig*)array_get(&(saturate_rules->asso_sig_array), i))){
							break;
						}
					}
					if (i == array_get_length(&(saturate_rules->asso_sig_array))){
						if (array_add(&(saturate_rules->asso_sig_array), &asso_sig) < 0){
							printf("ERROR: in %s, unable to add element to array\n", __func__);
						}
					}
				}
			}
		}
	}
}

void saturateRules_print(struct saturateRules* saturate_rules){
	uint32_t 		i;
	uint32_t 		j;
	struct assoSig* asso_sig;

	if (array_get_length(&(saturate_rules->asso_sig_array))){
		printf("Associative conflict(s) detected in the signature collection:\n");
		
		for (i = 0; i < array_get_length(&(saturate_rules->asso_sig_array)); i++){
			asso_sig = (struct assoSig*)array_get(&(saturate_rules->asso_sig_array), i);

			printf("\t- %s {", irOpcode_2_string(asso_sig->opcode));
			for (j = 0; j < asso_sig->nb_input; j++){
				if (j == asso_sig->nb_input - 1){
					printf("0x%08x} -> {", asso_sig->buffer_input[j]);
				}
				else{
					printf("0x%08x, ", asso_sig->buffer_input[j]);
				}
			}

			for (j = 0; j < asso_sig->nb_output; j++){
				if (j == asso_sig->nb_output - 1){
					printf("0x%08x", asso_sig->buffer_output[j]);
				}
				else{
					printf("0x%08x, ", asso_sig->buffer_output[j]);
				}
			}
			printf("}\n");
		}
	}
}

static void irSaturate_asso(struct saturateRules* saturate_rules, struct ir* ir, struct saturateBuilder* builder);

void irSaturate_saturate(struct saturateRules* saturate_rules, struct ir* ir){
	struct saturateBuilder builder;

	if (saturateBuilder_init(&builder, &(ir->graph_layer))){
		printf("ERROR: in %s, unable to init saturateBuilder\n", __func__);
		return;
	}

	irSaturate_asso(saturate_rules, ir, &builder);

	saturateBuilder_remove_redundant_element(&builder);
	saturateBuilder_commit(&builder, ir);

	saturateBuilder_clean(&builder);
}


struct matchHeader{
	uint32_t nb_match;
	uint32_t state;
	uint32_t match_offset;
};

#define matchHeader_get_state_offset(header) ((header)->state + (header)->match_offset)

static void irSaturate_asso(struct saturateRules* saturate_rules, struct ir* ir, struct saturateBuilder* builder){
	uint32_t 			i;
	uint32_t 			j;
	uint32_t 			k;
	struct assoGraph 	asso_graph;
	struct node* 		node_cursor;
	struct irOperation* operation;
	struct assoSig* 	asso_sig;
	struct matchHeader 	input_match_header[ASSOSIG_MAX_INPUT];
	struct matchHeader 	output_match_header[ASSOSIG_MAX_OUTPUT];
	struct node** 		parameter_buffer = NULL;
	uint32_t 			parameter_buffer_size;
	struct selection 	selection;

	if (assoGraph_init(&asso_graph, ir->graph.nb_node)){
		printf("ERROR: in %s, unable to init assoGraph\n", __func__);
		return;
	}

	if (selection_init(&selection)){
		printf("ERROR: in %s, unable to init selection\n", __func__);
		assoGraph_clean(&asso_graph);
		return;
	}

	for (node_cursor = graph_get_head_node(&(ir->graph)); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
		assoGraph_node_set_free(node_cursor);
	}

	for (node_cursor = graph_get_head_node(&(ir->graph)); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
		operation = ir_node_get_operation(node_cursor);
		if (assoGraph_is_node_free(node_cursor) && operation->type == IR_OPERATION_TYPE_INST && irSaturate_opcode_is_associative(operation->operation_type.inst.opcode)){
			assoGraph_reset(&asso_graph, operation->operation_type.inst.opcode);
			assoGraph_extract(&asso_graph, node_cursor);

			for (i = 0; i < array_get_length(&(saturate_rules->asso_sig_array)); i++){
				asso_sig = (struct assoSig*)array_get(&(saturate_rules->asso_sig_array), i);
				if (asso_sig->opcode == asso_graph.opcode && asso_sig->nb_input <= asso_graph.nb_input && asso_sig->nb_output <= asso_graph.nb_output){

					if (parameter_buffer == NULL){
						parameter_buffer_size = ASSOSIG_MAX_INPUT * asso_graph.nb_input + ASSOSIG_MAX_OUTPUT * asso_graph.nb_output;
						parameter_buffer = (struct node**)malloc(sizeof(struct node*) * parameter_buffer_size);
						
						if (parameter_buffer == NULL){
							printf("ERROR: in %s, unable to allocate memory\n", __func__);
							continue;
						}
					}

					for (j = 0; j < asso_sig->nb_input; j++){
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

						for (k = 0; k < asso_graph.nb_input; k++){
							if (asso_sig->buffer_input[j] == irNode_get_label(asso_graph.buffer_input[k]) || asso_sig->buffer_input[j] == SUBGRAPHISOMORPHISM_JOKER_LABEL){
								if (input_match_header[j].match_offset + input_match_header[j].nb_match == parameter_buffer_size){
									parameter_buffer_size = ASSOSIG_MAX_INPUT * asso_graph.nb_input + ASSOSIG_MAX_OUTPUT * asso_graph.nb_output;
									parameter_buffer = (struct node**)realloc(parameter_buffer, sizeof(struct node*) * parameter_buffer_size);
									if (parameter_buffer == NULL){
										printf("ERROR: in %s unable to realloc memory\n", __func__);
										goto exit;
									}
								}
								parameter_buffer[input_match_header[j].match_offset + input_match_header[j].nb_match] = asso_graph.buffer_input[k];
								input_match_header[j].nb_match ++;
							}
						}
						if (input_match_header[j].nb_match == 0){
							goto next;
						}
					}

					for (j = 0; j < asso_sig->nb_output; j++){
						if (j == 0){
							output_match_header[j].nb_match = 0;
							output_match_header[j].state = 0;
							output_match_header[j].match_offset = input_match_header[asso_sig->nb_input - 1].match_offset + input_match_header[asso_sig->nb_input - 1].nb_match;
						}
						else{
							output_match_header[j].nb_match = 0;
							output_match_header[j].state = 0;
							output_match_header[j].match_offset = output_match_header[j - 1].match_offset + output_match_header[j - 1].nb_match;
						}

						for (k = 0; k < asso_graph.nb_output; k++){
							if (asso_sig->buffer_output[j] == irNode_get_label(asso_graph.buffer_output[k]) || asso_sig->buffer_output[j] == SUBGRAPHISOMORPHISM_JOKER_LABEL){
								if (output_match_header[j].match_offset + output_match_header[j].nb_match == parameter_buffer_size){
									parameter_buffer_size = ASSOSIG_MAX_INPUT * asso_graph.nb_input + ASSOSIG_MAX_OUTPUT * asso_graph.nb_output;
									parameter_buffer = (struct node**)realloc(parameter_buffer, sizeof(struct node*) * parameter_buffer_size);
									if (parameter_buffer == NULL){
										printf("ERROR: in %s unable to realloc memory\n", __func__);
										goto exit;
									}
								}
								parameter_buffer[output_match_header[j].match_offset + output_match_header[j].nb_match] = asso_graph.buffer_output[k];
								output_match_header[j].nb_match ++;
							}
						}
						if (output_match_header[j].nb_match == 0){
							goto next;
						}
					}

					selection.nb_input = asso_sig->nb_input;
					selection.nb_output = asso_sig->nb_output;

					while(1){
						for (j = 0; j < asso_sig->nb_input;){
							selection.buffer_input[j] = parameter_buffer[matchHeader_get_state_offset(input_match_header + j)];
							for (k = 0; k < j; k++){
								if (selection.buffer_input[k] == selection.buffer_input[j]){
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

						for (j = 0; j < asso_sig->nb_output;){
							selection.buffer_output[j] = parameter_buffer[matchHeader_get_state_offset(output_match_header + j)];
							for (k = 0; k < j; k++){
								if (selection.buffer_output[k] == selection.buffer_output[j]){
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

						if (!assoGraph_check_selection(&asso_graph, &selection)){
							saturateBuilder_push_selection(builder, &selection, asso_sig);
							selection_flush(&selection);
						}

						if (asso_sig->nb_output == 0){
							goto inc_input;
						}

						j = asso_sig->nb_output - 1;
						while (output_match_header[j].state == output_match_header[j].nb_match - 1 && j != 0){
							output_match_header[j].state = 0;
							j--;
						}
						if (j != 0){
							output_match_header[j].state ++;
						}
						else{
							inc_input:
							j = asso_sig->nb_input - 1;
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
	selection_clean(&selection);
	if (parameter_buffer != NULL){
		free(parameter_buffer);
	}
	assoGraph_clean(&asso_graph);
}