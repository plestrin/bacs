#ifndef VARIABLERANGE_H
#define VARIABLERANGE_H

#include <stdint.h>
#include "base.h"

struct variableRange{
	uint64_t index;
	uint32_t scale;
	uint64_t disp;
	uint64_t mask;
};

static inline void variableRange_init_cst(struct variableRange* range, uint64_t value, uint64_t mask){
	range->index 	= 0;
	range->scale 	= 0;
	range->disp 	= value & mask;
	range->mask 	= mask;
}

#define variableRange_init_cst_(range, value, size_bit) variableRange_init_cst(range, value, bitmask64((size_bit)))

static inline void variableRange_init_top(struct variableRange* range, uint64_t mask){
	range->index 	= mask;
	range->scale 	= 0;
	range->disp 	= 0;
	range->mask 	= mask;
}

#define variableRange_init_top_(range, size_bit) variableRange_init_top(range, bitmask64((size_bit)))

static inline void variableRange_init_mask(struct variableRange* range, uint64_t mask, uint32_t size_bit){
	range->mask = bitmask64(size_bit);
	range->disp = 0;

	if (!(mask &= range->mask)){
		range->index = 0;
		range->scale = 0;
	}
	else{
		range->scale = __builtin_ctzll(mask);
		range->index = mask >> range->scale;
	}
}

#define variableRange_is_cst(range) 	(!(range)->index)
#define variableRange_get_cst(range) 	((range)->disp & (range)->mask)

#define variableRange_is_overflow(range) (((((range)->index << (range)->scale) + (range)->disp) & (range)->mask) < (range)->disp)

void variableRange_add_value(struct variableRange* range, uint64_t value, uint32_t size_bit);
void variableRange_and_value(struct variableRange* range, uint64_t value);

static inline void variableRange_mod_value(struct variableRange* range, uint32_t value){
	uint64_t mask = bitmask64(value) & range->mask;

	range->disp &= mask;
	range->mask = mask;
	if (range->index & ~(range->mask >> range->scale)){
		range->index = range->mask >> range->scale;
	}
}

void variableRange_shl_value(struct variableRange* range, uint64_t value, uint32_t size_bit);
void variableRange_shr_value(struct variableRange* range, uint64_t value);
void variableRange_or_value (struct variableRange* range, uint64_t value, uint32_t size_bit);
void variableRange_sub_value(struct variableRange* range, uint64_t value, uint32_t size_bit);
void variableRange_xor_value(struct variableRange* range, uint64_t value, uint32_t size_bit);

#define variableRange_must_upscale(range, size_bit) (~((range)->mask | ((0xffffffffffffffff << (size_bit)) + ((size_bit) == 64))))

void variableRange_upscale_value(struct variableRange* range, uint32_t value);
void variableRange_resize(struct variableRange* range, uint32_t size_bit);
void variableRange_neg(struct variableRange* range, uint32_t size_bit);

void variableRange_add_range(struct variableRange* range1, const struct variableRange* range2, uint32_t size_bit);
void variableRange_and_range(struct variableRange* range1, const struct variableRange* range2, uint32_t size_bit);
void variableRange_or_range (struct variableRange* range1, const struct variableRange* range2, uint32_t size_bit);
void variableRange_shl_range(struct variableRange* range1, const struct variableRange* range2, uint32_t size_bit);
void variableRange_shr_range(struct variableRange* range1, const struct variableRange* range2, uint32_t size_bit);
void variableRange_sub_range(struct variableRange* range1, const struct variableRange* range2, uint32_t size_bit);
void variableRange_xor_range(struct variableRange* range1, const struct variableRange* range2, uint32_t size_bit);

int32_t variableRange_is_value_include(const struct variableRange* range, uint64_t value);
int32_t variableRange_is_range_include(const struct variableRange* range1, const struct variableRange* range2);
int32_t variableRange_is_range_intersect(const struct variableRange* range1, const struct variableRange* range2);

#define variableRange_is_mask_compact(mask) (!((((mask) & (-(mask))) + (mask)) & (mask)))

void variableRange_check_format(const struct variableRange* range);

void variableRange_print(const struct variableRange* range);

struct variableRangeIterator{
	const struct variableRange* range;
	uint64_t 					index;
	uint32_t 					stop;
};

static inline void variableRangeIterator_init(struct variableRangeIterator* it, const struct variableRange* range){
	it->range 	= range;
	it->index 	= 0;
	it->stop 	= 0;
}

static inline uint32_t variableRangeIterator_get_next(struct variableRangeIterator* it, uint64_t* value){
	if (!it->stop){
		*value = ((it->index << it->range->scale) + it->range->disp) & it->range->mask;
		if (it->index == it->range->index){
			it->stop = 1;
		}
		it->index ++;

		return 1;
	}

	return 0;
}

#endif
