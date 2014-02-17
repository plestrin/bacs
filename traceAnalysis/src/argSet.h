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

void argSet_clean(struct argSet* set);

#endif