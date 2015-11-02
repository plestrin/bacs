#ifndef ARRAYMINCOVERAGE_H
#define ARRAYMINCOVERAGE_H

#include <stdint.h>

#include "array.h"


#define ARRAYMINCOVERAGE_COMPLEXITY_THRESHOLD 4096ULL

struct categoryDesc{
	uint32_t offset;
	uint32_t nb_element;
	uint32_t choice;
};

int32_t arrayMinCoverage_rand(struct array* array, uint32_t nb_category, struct categoryDesc* desc_buffer, int32_t(*compare)(void*,void*), uint32_t* score);
int32_t arrayMinCoverage_greedy(struct array* array, uint32_t nb_category, struct categoryDesc* desc_buffer, int32_t(*compare)(void*,void*), uint32_t* score);
int32_t arrayMinCoverage_exact(struct array* array, uint32_t nb_category, struct categoryDesc* desc_buffer, int32_t(*compare)(void*,void*), uint32_t* score);

int32_t arrayMinCoverage_auto(struct array* array, uint32_t nb_category, struct categoryDesc* desc_buffer, int32_t(*compare)(void*,void*));
int32_t arrayMinCoverage_split(struct array* array, uint32_t nb_category, struct categoryDesc* desc_buffer, int32_t(*compare)(void*,void*));

uint32_t arrayMinCoverage_eval(struct array* array, uint32_t nb_category, struct categoryDesc* desc_buffer, int32_t(*compare)(void*,void*));

#endif