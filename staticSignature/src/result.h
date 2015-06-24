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

struct signatureLink{
	struct virtualNode 		virtual_node;
	uint32_t 				edge_desc;
};

enum resultState{
	RESULTSTATE_IDLE,
	RESULTSTATE_PUSH,
};

struct result{
	enum resultState 			state;
	struct codeSignature* 		signature;
	uint32_t 					nb_occurrence;
	struct signatureLink* 		in_mapping_buffer;
	struct signatureLink* 		ou_mapping_buffer;
	struct virtualNode*			intern_node_buffer;
	struct node** 				symbol_node_buffer;
};

int32_t result_init(struct result* result, struct codeSignature* code_signature, struct array* assignement_array);

#define result_get_nb_internal_node(result) ((result)->signature->graph.nb_node - ((result)->signature->nb_frag_tot_in + (result)->signature->nb_frag_tot_out))

void result_push(struct result* result, struct ir* ir);
void result_pop(struct result* result, struct ir* ir);
void result_get_footprint(struct result* result, uint32_t index, struct set* set);
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

enum parameterSimilarity parameterSimilarity_get(struct node** parameter_list1, uint32_t size1, struct node** parameter_list2, uint32_t size2);

struct parameterMapping{
	uint32_t 					nb_fragment;
	size_t 						node_buffer_offset;
	enum parameterSimilarity 	similarity;
};

struct parameterMapping* parameterMapping_create(struct codeSignature* signature);
int32_t parameterMapping_init(struct parameterMapping* mapping, struct codeSignature* signature);

#define parameterMapping_get_size(code_signature) (sizeof(struct parameterMapping) * ((code_signature)->nb_parameter_in + (code_signature)->nb_parameter_out) + sizeof(struct node*) * ((code_signature)->nb_frag_tot_in + (code_signature)->nb_frag_tot_out))
#define parameterMapping_get_node_buffer(mapping) ((struct node**)((char*)(mapping) + (mapping)->node_buffer_offset))

int32_t parameterMapping_fill(struct parameterMapping* mapping, struct result* result, uint32_t index);

/*
   TODO list3:
   	- try to some clustering not to push to many symbols (can be done in TODO list 2)
   	- find lowest common ancestor

   TODO list4:
   	- list occurrence of a fragment in trace just to know which larger fragment can we create
   	- while building new IR try to import previously create IR
   	- normalize IR with pushed results


   TDOD list5:
	- (OPT) add description for parameters in the signature definition file (for a better printing at the end to tag edges)
	- (OPT) try to give advice on the fragment size in terms of bbl (just a useful feature)
*/

#endif