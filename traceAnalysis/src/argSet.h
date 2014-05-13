#ifndef ARGSET_H
#define ARGSET_H

#include "instruction.h"
#include "argument.h"
#include "array.h"


#define ARGSET_TAG_MAX_LENGTH 32

struct argSet{
	char 			tag[ARGSET_TAG_MAX_LENGTH];
	struct array*	input;
	struct array*	output;
	void* 			input_tree_root;
};

int32_t argSet_init(struct argSet* set, char* tag);

void argSet_print(struct argSet* set, enum argFragType* type);

int32_t argSet_add_input(struct argSet* set, struct inputArgument* arg);
int32_t argSet_search_input(struct argSet* set, char* buffer, uint32_t buffer_length);

int32_t argSet_add_output(struct argSet* set, struct operand* operand, uint8_t* data);
int32_t argSet_search_output(struct argSet* set, char* buffer, uint32_t buffer_length);

#define argSet_get_nb_input(set) 	(array_get_length((set)->input))
#define argSet_get_nb_output(set) 	(array_get_length((set)->output))

void argSet_clean(struct argSet* set);

#endif