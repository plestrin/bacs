#ifndef ARGSET_H
#define ARGSET_H

#include "array.h"
#include "argBuffer.h"

#define ARGSET_TAG_MAX_LENGTH 32

struct argSet{
	char 			tag[ARGSET_TAG_MAX_LENGTH];
	struct array*	input;
	struct array*	output;
};

int32_t argSet_init(struct argSet* set, char* tag);

int32_t argSet_combine(struct argSet* set_dst, struct argSet** set_src, uint32_t nb_set_src, struct argSet* set_result);

void argSet_clean(struct argSet* set);

#endif