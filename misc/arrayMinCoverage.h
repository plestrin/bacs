#ifndef ARRAYMINCOVERAGE_H
#define ARRAYMINCOVERAGE_H

#include <stdint.h>

#include "array.h"

#define ARRAYMINCOVERAGE_COMPLEXITY_THRESHOLD 0x00000000ffffffff
#define ARRAYMINCOVERAGE_DETERMINISTIC 0

struct categoryDesc{
	uint32_t 	offset;
	uint32_t 	nb_element;
	uint32_t 	choice;
	void*** 	tagMap_gateway;
};

struct tagMap{
	uint32_t 	nb_element;
	uint32_t* 	map;
};

struct tagMapTreeToken{
	void* 		element;
	uint32_t 	idx;
};

#define cmp_get_element(arg) (((const struct tagMapTreeToken*)(arg))->element)

int32_t    arrayMinCoverage_rand_wrapper(struct array* array, uint32_t nb_category, struct categoryDesc* desc_buffer, int32_t(*compare)(const void*,const void*), uint32_t* score);
int32_t  arrayMinCoverage_greedy_wrapper(struct array* array, uint32_t nb_category, struct categoryDesc* desc_buffer, int32_t(*compare)(const void*,const void*), uint32_t* score);
int32_t   arrayMinCoverage_exact_wrapper(struct array* array, uint32_t nb_category, struct categoryDesc* desc_buffer, int32_t(*compare)(const void*,const void*), uint32_t* score);
int32_t arrayMinCoverage_reshape_wrapper(struct array* array, uint32_t nb_category, struct categoryDesc* desc_buffer, int32_t(*compare)(const void*,const void*), uint32_t* score);
int32_t   arrayMinCoverage_split_wrapper(struct array* array, uint32_t nb_category, struct categoryDesc* desc_buffer, int32_t(*compare)(const void*,const void*), uint32_t* score);
int32_t   arrayMinCoverage_super_wrapper(struct array* array, uint32_t nb_category, struct categoryDesc* desc_buffer, int32_t(*compare)(const void*,const void*), uint32_t* score);

uint32_t arrayMinCoverage_eval(struct array* array, uint32_t nb_category, struct categoryDesc* desc_buffer, int32_t(*compare)(const void*,const void*));

#endif
