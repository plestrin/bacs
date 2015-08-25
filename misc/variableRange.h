#ifndef VARIABLERANGE_H
#define VARIABLERANGE_H

#include <stdint.h>

#include "graph.h"

struct variableRange{
	uint64_t 	index_lo;
	uint64_t 	index_up;
	uint32_t 	scale;
	uint64_t 	disp;
	uint64_t 	size_mask;
};

#define variableRange_get_lower_bound(range) ((((range)->index_lo << (range)->scale) + (range)->disp) & (range)->size_mask)
#define variableRange_get_upper_bound(range) ((((range)->index_up << (range)->scale) + (range)->disp) & (range)->size_mask)

#define variableRange_get_size_mask(size) 	(0xffffffffffffffff >> (64 - (size)))
#define variableRange_get_index_mask(range) ((variableRange_is_cst(range)) ? (range)->size_mask : ((range)->size_mask) >> (range)->scale)
#define variableRange_is_cst(range) 		((range)->scale == 0xffffffff)
#define variableRange_get_cst(range) 		((range)->disp)

#define variableRange_init_cst(range, cst, size) 																					\
	(range)->index_lo 	= 0; 																										\
	(range)->index_up 	= 0; 																										\
	(range)->scale 		= 0xffffffff; 																								\
	(range)->disp 		= cst; 																										\
	(range)->size_mask 	= variableRange_get_size_mask(size);

#define variableRange_init_seg(range, lo, up, size) 																				\
	(range)->size_mask 		= variableRange_get_size_mask(size); 																	\
	if (((lo) & (range)->size_mask) == ((up) & (range)->size_mask)){ 																\
		(range)->index_lo 	= 0; 																									\
		(range)->index_up 	= 0; 																									\
		(range)->scale 		= 0xffffffff; 																							\
		(range)->disp 		= 0; 																									\
	} 																																\
	else{ 																															\
		(range)->index_lo 	= lo & (range)->size_mask; 																				\
		(range)->index_up 	= up & (range)->size_mask; 																				\
		(range)->scale 		= 0; 																									\
		(range)->disp 		= 0; 																									\
	}

void variableRange_init_mask(struct variableRange* range, uint64_t mask, uint32_t size);

#define variableRange_is_mask_compact(mask) (((((mask) & (-(mask))) + (mask)) & (mask)) == 0)

#define variableRange_init_size(range, size) variableRange_init_seg(range, 0, variableRange_get_size_mask(size), size)

#define variableRange_pack(range) 																									\
	if ((range)->scale == 0xffffffff){ 																								\
		(range)->disp 		= (range)->disp & (range)->size_mask; 																	\
	} 																																\
	else if ((~(0xffffffffffffffff << (range)->scale)) >= (range)->size_mask){ 														\
		(range)->index_lo 	= 0; 																									\
		(range)->index_up 	= 0; 																									\
		(range)->scale 		= 0xffffffff; 																							\
		(range)->disp 		= (range)->disp & (range)->size_mask; 																	\
	} 																																\
	else if ((range)->scale == 0){ 																									\
		(range)->index_lo 	= ((range)->index_lo + (range)->disp) & (range)->size_mask; 											\
		(range)->index_up 	= ((range)->index_up + (range)->disp) & (range)->size_mask; 											\
		(range)->disp 		= 0; 																									\
		if ((((range)->index_up - (range)->index_lo) & (range)->size_mask) == 0){ 													\
			(range)->disp 		= (range)->index_up; 																				\
			(range)->index_lo 	= 0; 																								\
			(range)->index_up 	= 0; 																								\
			(range)->scale 		= 0xffffffff; 																						\
		} 																															\
		else if ((range)->index_lo - (range)->index_up == 1){ 																		\
			(range)->index_lo 	= 0; 																								\
			(range)->index_up 	= (range)->size_mask; 																				\
		} 																															\
	} 																																\
	else{ 																															\
		if ((((range)->index_up - (range)->index_lo) & variableRange_get_index_mask(range)) == 0){ 									\
			(range)->disp 		= (((range)->index_lo << (range)->scale) + (range)->disp) & (range)->size_mask; 					\
			(range)->index_lo 	= 0; 																								\
			(range)->index_up 	= 0; 																								\
			(range)->scale 		= 0xffffffff; 																						\
		} 																															\
		else if ((range)->index_lo - (range)->index_up == 1){ 																		\
			(range)->index_lo 	= 0; 																								\
			(range)->index_up 	= variableRange_get_index_mask(range); 																\
			(range)->disp 		= (range)->disp & (~(0xffffffffffffffff << (range)->scale)); 										\
		} 																															\
		else{ 																														\
			(range)->index_lo 	= ((range)->index_lo + ((range)->disp >> (range)->scale)) & variableRange_get_index_mask(range); 	\
			(range)->index_up 	= ((range)->index_up + ((range)->disp >> (range)->scale)) & variableRange_get_index_mask(range); 	\
			(range)->disp 		= (range)->disp & (~(0xffffffffffffffff << (range)->scale)); 										\
		} 																															\
	}

void variableRange_add(struct variableRange* range_dst, const struct variableRange* range_src, uint32_t size);
void variableRange_and(struct variableRange* range_dst, const struct variableRange* range_src, uint32_t size);
void variableRange_shl(struct variableRange* range_dst, const struct variableRange* range_src, uint32_t size);
void variableRange_shr(struct variableRange* range_dst, const struct variableRange* range_src, uint32_t size);

void variableRange_bitwise_heuristic(struct variableRange* range_dst, const struct variableRange* range_src, uint32_t size);

int32_t variableRange_intersect(const struct variableRange* range1, const struct variableRange* range2);
int32_t variableRange_include(const struct variableRange* range1, const struct variableRange* range2);

uint64_t variableRange_get_nb_value(const struct variableRange* range);

void variableRange_print(const struct variableRange* range);

#endif