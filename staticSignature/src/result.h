#ifndef RESULT_H
#define RESULT_H

#include <stdint.h>

#include "codeSignature.h"

struct signatureLink{
	struct node* 				node;
	uint32_t 					edge_desc;
};

enum resultState{
	RESULTSTATE_IDLE,
	RESULTSTATE_PUSH,
};

struct result{
	enum resultState 			state;
	struct codeSignature* 		signature;
	uint32_t 					nb_occurence;
	struct signatureLink* 		in_mapping_buffer;
	struct signatureLink* 		ou_mapping_buffer;
	struct node**				intern_node_buffer;
	struct node** 				symbol_node_buffer;
};

int32_t result_init(struct result* result, struct codeSignature* code_signature, struct array* assignement_array);
void result_push(struct result* result, struct ir* ir);
void result_pop(struct result* result, struct ir* ir);
void result_print(struct result* result);

#define result_clean(result) 							\
	free((result)->in_mapping_buffer); 								\
	free((result)->ou_mapping_buffer); 								\
	free((result)->intern_node_buffer); 							\
	free((result)->symbol_node_buffer);


/* TODO list1:
	- design a result structure
	- push the result structure into an array
	- print result from that structure: try to give advice on the fragment size in terms of bbl (just a usefull feature)

   TODO list2:
   	- make a method to export result to IR: ("select which result to export and so on")

   TODO list3:
   	- built a connectivity graph between high export symbols

   TODO list4:
   	- list occurence of a fragment in trace juste to know which larger fragement can we create
   	- while building new IR try to import previously create IR
*/

#endif