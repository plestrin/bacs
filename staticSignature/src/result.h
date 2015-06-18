#ifndef RESULT_H
#define RESULT_H

#include <stdint.h>

#include "codeSignature.h"

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
void result_push(struct result* result, struct ir* ir);
void result_pop(struct result* result, struct ir* ir);
void result_print(struct result* result);

#define result_clean(result) 										\
	free((result)->in_mapping_buffer); 								\
	free((result)->ou_mapping_buffer); 								\
	free((result)->intern_node_buffer); 							\
	free((result)->symbol_node_buffer);

/*
   TODO list2:
   	- write the export wrapper: fragment to export signature and so on
   	- pop the result to the IR
   	- (OPT) clean what can be cleaned (experimental part)
   	- (OPT) try to give advice on the fragment size in terms of bbl (just a usefull feature)

   TODO list3:
   	- try to some clustering not to push to many symbols (can be done in TODO list 2)
   	- find path between symbols
   	- lowest common ancestor

   TODO list4:
   	- list occurence of a fragment in trace juste to know which larger fragement can we create
   	- while building new IR try to import previously create IR
*/

#endif