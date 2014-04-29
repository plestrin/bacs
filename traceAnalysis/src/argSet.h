#ifndef ARGSET_H
#define ARGSET_H

#include "array.h"
#include "argBuffer.h"

#define ARGSET_TAG_MAX_LENGTH 32

struct argSet{
	char 			tag[ARGSET_TAG_MAX_LENGTH];
	struct array*	input;
	struct array*	output;
	uint32_t* 		output_mapping;
};

int32_t argSet_init(struct argSet* set, char* tag);

void argSet_print(struct argSet* set, enum argLocationType* type);

void argSet_get_nb_mem(struct argSet* set, uint32_t* nb_in, uint32_t* nb_out);
void argSet_get_nb_reg(struct argSet* set, uint32_t* nb_in, uint32_t* nb_out);
void argSet_get_nb_mix(struct argSet* set, uint32_t* nb_in);

int32_t argSet_sort_output(struct argSet* set);

int32_t argSet_search_input(struct argSet* set, char* buffer, uint32_t buffer_length);
int32_t argSet_search_output(struct argSet* set, char* buffer, uint32_t buffer_length);

void argSet_clean(struct argSet* set);

#endif