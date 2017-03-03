#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "variableRange.h"
#include "base.h"

void variableRange_add_value(struct variableRange* range, uint64_t value, uint32_t size_bit){
	if (variableRange_is_cst(range)){
		range->disp &= range->mask;
		range->mask = bitmask64(size_bit);
		range->disp += value;
		range->disp &= range->mask;
	}
	else{
		if (variableRange_must_upscale(range, size_bit)){
			variableRange_upscale_value(range, size_bit);
		}

		range->mask = bitmask64(size_bit);
		range->disp = (range->disp + value) & range->mask;
	}
}

void variableRange_and_value(struct variableRange* range, uint64_t value){
	if (variableRange_is_cst(range)){
		range->disp &= value;
	}
	else{
		range->disp &= value | (0xffffffffffffffff << range->scale);
		value = (value & range->mask) >> range->scale;

		if (!value){
			range->disp &= bitmask64(range->scale);
			range->index = 0;
			range->scale = 0;
		}
		else{
			uint32_t ctz;

			ctz = __builtin_ctzll(value);
			range->index = ((range->index + (range->disp >> range->scale)) >> ctz) - (range->disp >> (range->scale + ctz));
			range->disp &= (0xffffffffffffffff << (range->scale + ctz)) | bitmask64(range->scale);
			range->scale += ctz;

			value >>= ctz;

			range->index = min(value, range->index + (range->disp >> range->scale));
			range->disp &= bitmask64(range->scale);
		}
	}
}

void variableRange_shl_value(struct variableRange* range, uint64_t value, uint32_t size_bit){
	if (value >= 64){
		variableRange_init_cst(range, 0, bitmask64(size_bit));
	}
	else{
		if (variableRange_must_upscale(range, size_bit)){
			variableRange_upscale_value(range, size_bit);
		}

		range->mask = bitmask64(size_bit);
		range->disp = (range->disp << value) & range->mask;
		if (range->index){
			if (value + range->scale >= 64){
				range->index = 0;
			}
			else{
				range->scale += value;
				if (range->index & ~(range->mask >> range->scale)){
					range->index = range->mask >> range->scale;
				}
			}
		}
	}
}

void variableRange_shr_value(struct variableRange* range, uint64_t value){
	if (value >= 64 || !(range->mask >> value)){
		variableRange_init_cst(range, 0, range->mask);
	}
	else{
		range->mask >>= value;
		if (range->index){
			if (range->scale > value){
				range->disp >>= value;
				range->scale -= value;
			}
			else{
				range->disp >>= range->scale;
				range->index = (range->index + range->disp) >> (value - range->scale);
				range->disp >>= (value - range->scale);
				range->index -= range->disp;
				range->scale = 0;
			}
		}
		else{
			range->disp >>= value;
		}
	}
}

void variableRange_or_value(struct variableRange* range, uint64_t value, uint32_t size_bit){
	if (variableRange_is_cst(range)){
		range->mask = bitmask64(size_bit);
		range->disp |= value;
		range->disp &= range->mask;
	}
	else{
		value &= bitmask64(size_bit);
		if ((value & ~(range->mask)) && variableRange_must_upscale(range, size_bit)){
			variableRange_upscale_value(range, size_bit);
		}
		if (range->mask >> size_bit){
			range->mask = bitmask64(size_bit);
		}

		range->disp |= value & bitmask64(range->scale);
		value >>= range->scale;

		if (value){
			uint32_t cto;

			cto = __builtin_ctzll(~value);
			range->disp |= ~(0xffffffffffffffff << (range->scale + cto)) & (0xffffffffffffffff << range->scale);
			range->index = ((range->index + (range->disp >> range->scale)) >> cto) - (range->disp >> (range->scale + cto));
			range->scale += cto;

			value >>= cto;

			if (value){
				range->index = (range->mask >> range->scale) - value;
				range->disp &= bitmask64(range->scale);
				range->disp |= value << range->scale;
			}
		}
	}
}

void variableRange_sub_value(struct variableRange* range, uint64_t value, uint32_t size_bit){
	variableRange_add_value(range, ~value + 1, size_bit);
}

void variableRange_xor_value(struct variableRange* range, uint64_t value, uint32_t size_bit){
	if (variableRange_is_cst(range)){
		range->mask = bitmask64(size_bit);
		range->disp ^= value;
		range->disp &= range->mask;
	}
	else{
		value &= bitmask64(size_bit);
		if ((value & ~(range->mask)) && variableRange_must_upscale(range, size_bit)){
			variableRange_upscale_value(range, size_bit);
		}
		if (range->mask >> size_bit){
			range->mask = bitmask64(size_bit);
		}

		range->disp ^= value & bitmask64(range->scale);
		value >>= range->scale;

		if (value){
			uint32_t clz;

			clz = __builtin_ctzll(value | range->index);
			range->disp &= bitmask64(range->scale);
			range->index = (0xffffffffffffffff >> clz) & (range->mask >> range->scale);
		}
	}
}

void variableRange_upscale_value(struct variableRange* range, uint32_t value){
	range->disp &= range->mask;

	if (range->index){
		if (range->index & ~(range->mask >> range->scale)){
			range->index = range->mask >> range->scale;
			range->disp &= bitmask64(range->scale);
		}
		else if (variableRange_is_overflow(range)){
			range->index = range->mask >> range->scale;
			range->disp &= bitmask64(range->scale);
		}
	}

	range->mask = bitmask64(value);
}

void variableRange_resize(struct variableRange* range, uint32_t size_bit){
	if (range->mask >> size_bit){
		variableRange_mod_value(range, size_bit);
	}
	else if (range->mask != bitmask64(size_bit)){
		variableRange_upscale_value(range, size_bit);
	}
}

void variableRange_neg(struct variableRange* range, uint32_t size_bit){
	if (variableRange_is_cst(range)){
		range->disp &= range->mask;
		range->mask = bitmask64(size_bit);
		range->disp = (~range->disp + 1) & range->mask;
	}
	else{
		if (variableRange_must_upscale(range, size_bit)){
			variableRange_upscale_value(range, size_bit);
		}

		range->mask = bitmask64(size_bit);
		range->disp = (~range->disp + 1 + ((~range->index + 1) << range->scale)) & range->mask;
	}
}

void variableRange_add_range(struct variableRange* range1, const struct variableRange* range2, uint32_t size_bit){
	uint64_t mask = bitmask64(size_bit);

	if (variableRange_is_cst(range2)){
		variableRange_add_value(range1, variableRange_get_cst(range2), size_bit);
	}
	else if (variableRange_is_cst(range1)){
		uint64_t cst = variableRange_get_cst(range1);
		memcpy(range1, range2, sizeof(struct variableRange));
		variableRange_add_value(range1, cst, size_bit);
	}
	else{
		uint64_t index1 	= range1->index;
		uint64_t disp1 		= range1->disp & range1->mask;
		uint64_t index2 	= range2->index;
		uint64_t disp2 		= range2->disp & range2->mask;
		uint32_t scale 		= min(range1->scale, range2->scale);

		if (range1->mask < mask && ((range1->index > (range1->mask >> range1->scale)) || variableRange_is_overflow(range1))){
			index1 = (range1->mask & mask) >> range1->scale;
			disp1 &= ~(0xffffffffffffffff << range1->scale);
		}

		if (range2->mask < mask && ((range2->index > (range2->mask >> range2->scale)) || variableRange_is_overflow(range2))){
			index2 = (range2->mask & mask) >> range2->scale;
			disp2 &= ~(0xffffffffffffffff << range2->scale);
		}

		range1->disp = (disp1 + disp2) & mask;
		range1->mask = mask;

		if ((index2 << (range2->scale - scale)) >= (mask >> scale) - (index1 << (range1->scale -scale))){
			range1->index = mask >> scale;
			range1->disp &= bitmask64(scale);
		}
		else{
			range1->index = (index1 << (range1->scale - scale)) + (index2 << (range2->scale - scale));
		}
		range1->scale = scale;
	}
}

void variableRange_and_range(struct variableRange* range1, const struct variableRange* range2, uint32_t size_bit){
	if (variableRange_is_cst(range2)){
		variableRange_and_value(range1, variableRange_get_cst(range2) & bitmask64(size_bit));
	}
	else if (variableRange_is_cst(range1)){
		uint64_t cst = variableRange_get_cst(range1);
		memcpy(range1, range2, sizeof(struct variableRange));
		variableRange_and_value(range1, cst & bitmask64(size_bit));
	}
	else{
		uint32_t scale;
		uint64_t index_a;
		uint64_t index_b;

		if (range1->scale < range2->scale){
			scale = __builtin_ctzll(range2->disp >> range1->scale);
			scale = min(scale, range2->scale - range1->scale);

			index_a = (range1->index + (range1->disp >> range1->scale)) >> scale;
			index_b = (range2->index << (range2->scale - (range1->scale + scale))) + (range2->disp >> (range1->scale + scale));

			range1->index = min(index_a, index_b);
			range1->disp &= range2->disp & ~(0xffffffffffffffff << range1->scale);
			range1->scale += scale;
		}
		else{
			scale = __builtin_ctzll(range1->disp >> range2->scale);
			scale = min(scale, range1->scale - range2->scale);

			index_a = (range2->index + (range2->disp >> range2->scale)) >> scale;
			index_b = (range1->index << (range1->scale - (range2->scale + scale))) + (range1->disp >> (range2->scale + scale));

			range1->index = min(index_a, index_b);
			range1->disp &= range2->disp & ~(0xffffffffffffffff << range2->scale);
			range1->scale = range2->scale + scale;
		}

		range1->mask = bitmask64(size_bit);
		range1->disp &= range1->mask;
	}
}

void variableRange_or_range(struct variableRange* range1, const struct variableRange* range2, uint32_t size_bit){
	if (variableRange_is_cst(range2)){
		variableRange_or_value(range1, variableRange_get_cst(range2), size_bit);
	}
	else if (variableRange_is_cst(range1)){
		uint64_t cst = variableRange_get_cst(range1);
		memcpy(range1, range2, sizeof(struct variableRange));
		variableRange_or_value(range1, cst, size_bit);
	}
	else{
		uint32_t scale;
		uint64_t index_a;
		uint64_t index_b;

		if ((range2->mask & bitmask64(size_bit) & ~(range1->mask)) && variableRange_must_upscale(range1, size_bit)){
			variableRange_upscale_value(range1, size_bit);
		}

		if (range1->scale < range2->scale){
			scale = __builtin_ctzll(~(range2->disp >> range1->scale));
			scale = min(scale, range2->scale - range1->scale);

			index_a = (range1->index + (range1->disp >> range1->scale)) >> scale;
			index_b = (range2->index << (range2->scale - (range1->scale + scale))) + (range2->disp >> (range1->scale + scale));
			if (index_b > (range2->mask >> (range1->scale + scale))){
				index_b = range2->mask >> (range1->scale + scale);
			}

			range1->scale += scale;
		}
		else{
			scale = __builtin_ctzll(~(range1->disp >> range2->scale));
			scale = min(scale, range1->scale - range2->scale);

			index_a = (range2->index + (range2->disp >> range2->scale)) >> scale;
			if (index_a > (range2->mask >> (range2->scale + scale))){
				index_a = range2->mask >> (range2->scale + scale);
			}
			index_b = (range1->index << (range1->scale - (range2->scale + scale))) + (range1->disp >> (range2->scale + scale));

			range1->scale = range2->scale + scale;
		}

		range1->mask = bitmask64(size_bit);
		range1->index = (0xffffffffffffffff >> __builtin_clzll(index_a | index_b)) & (range1->mask >> range1->scale);
		range1->disp = (range1->disp | range2->disp) & ~(0xffffffffffffffff << range1->scale) & range1->mask;
	}
}

void variableRange_shl_range(struct variableRange* range1, const struct variableRange* range2, uint32_t size_bit){
	if (variableRange_is_cst(range2)){
		variableRange_shl_value(range1, variableRange_get_cst(range2), size_bit);
	}
	else{
		/* can be improved using a lower bound */
		variableRange_init_top(range1, bitmask64(size_bit));
	}
}

void variableRange_shr_range(struct variableRange* range1, const struct variableRange* range2, uint32_t size_bit){
	if (variableRange_is_cst(range2)){
		variableRange_shr_value(range1, variableRange_get_cst(range2));
		variableRange_mod_value(range1, size_bit);
	}
	else{
		/* can be improved using a lower bound */
		variableRange_init_top(range1, bitmask64(size_bit));
	}
}

void variableRange_sub_range(struct variableRange* range1, const struct variableRange* range2, uint32_t size_bit){
	struct variableRange range_tmp;

	memcpy(&range_tmp, range2, sizeof(struct variableRange));
	variableRange_neg(&range_tmp, size_bit);
	variableRange_add_range(range1, &range_tmp, size_bit);
}

void variableRange_xor_range(struct variableRange* range1, const struct variableRange* range2, uint32_t size_bit){
	if (variableRange_is_cst(range2)){
		variableRange_xor_value(range1, variableRange_get_cst(range2), size_bit);
	}
	else if (variableRange_is_cst(range1)){
		uint64_t cst = variableRange_get_cst(range1);
		memcpy(range1, range2, sizeof(struct variableRange));
		variableRange_xor_value(range1, cst, size_bit);
	}
	else{
		uint64_t index;

		if (range1->scale < range2->scale){
			if (variableRange_is_overflow(range1)){
				index = range1->mask >> range1->scale;
			}
			else{
				index = range1->index + (range1->disp >> range1->scale);
			}

			if (variableRange_is_overflow(range2)){
				index |= range2->mask >> range1->scale;
			}
			else{
				index |= (range2->index << (range2->scale - range1->scale)) + (range2->disp >> range1->scale);
			}

			range1->disp = (range1->disp & ~(0xffffffffffffffff << range1->scale)) ^ (range2->disp & ~(0xffffffffffffffff << range1->scale));
		}
		else{
			if (variableRange_is_overflow(range1)){
				index = range1->mask >> range2->scale;
			}
			else{
				index = (range1->index << (range1->scale - range2->scale)) + (range1->disp >> range2->scale);
			}

			if (variableRange_is_overflow(range2)){
				index |= range2->mask >> range2->scale;
			}
			else{
				index |= range2->index + (range2->disp >> range2->scale);
			}

			range1->disp = (range1->disp & ~(0xffffffffffffffff << range2->scale)) ^ (range2->disp & ~(0xffffffffffffffff << range2->scale));
			range1->scale = range2->scale;
		}

		range1->mask = bitmask64(size_bit);
		range1->index = (0xffffffffffffffff >> __builtin_clzll(index)) & (range1->mask >> range1->scale);
	}
}

int32_t variableRange_is_value_include(const struct variableRange* range, uint64_t value){
	if (value & ~range->mask){
		return 0;
	}

	value = (value - range->disp) & range->mask;
	if (value & bitmask64(range->scale)){
		return 0;
	}

	value >>= range->scale;
	if (value > range->index){
		return 0;
	}

	return 1;
}

static int32_t variableRange_seg_include_seg(uint64_t seg1_lo, uint64_t seg1_up, uint64_t seg2_lo, uint64_t seg2_up, uint64_t size_mask){
	if (seg1_lo <= seg1_up){
		if (seg2_lo <= seg2_up){
			return ((seg1_lo <= seg2_lo) && (seg1_up >= seg2_up));
		}
		else{
			return ((seg1_lo == 0) && (seg1_up == size_mask));
		}
	}
	else{
		if (seg2_lo <= seg2_up){
			return ((seg1_lo <= seg2_lo) || (seg1_up >= seg2_up));
		}
		else{
			return ((seg1_lo <= seg2_lo) && (seg1_up >= seg2_up));
		}
	}
}

int32_t variableRange_is_range_include(const struct variableRange* range1, const struct variableRange* range2){
	uint64_t disp1;
	uint64_t disp2;
	uint64_t index1;
	uint64_t index2;
	uint64_t upper_bound1;
	uint64_t upper_bound2;
	uint64_t gap;

	disp1 = range1->disp & range1->mask;
	index1 = range1->index;
	if (range1->index > (range1->mask >> range1->scale)){
		index1 = range1->mask >> range1->scale;
		disp1 &= ~(0xffffffffffffffff << range1->scale);
	}

	disp2 = range2->disp & range2->mask;
	index2 = range2->index;
	if (range2->index > (range2->mask >> range2->scale)){
		index2 = range2->mask >> range2->scale;
		disp2 &= ~(0xffffffffffffffff << range2->scale);
	}

	if (!index2){
		return variableRange_is_value_include(range1, disp2);
	}
	else if (range2->scale >= range1->scale){
		if ((range1->disp - range2->disp) & ~(0xffffffffffffffff << range1->scale)){
			return 0;
		}
		else{
			upper_bound1 = ((index1 << range1->scale) + disp1) & range1->mask;
			upper_bound2 = ((index2 << range2->scale) + disp2) & range2->mask;

			if (range1->mask < range2->mask && (disp2 > range1->mask || ((index2 << range2->scale) + disp2) > range1->mask || ((((index2 << range2->scale) + disp2) & range2->mask) < disp2))){
				return 0;
			}

			gap = (range1->mask >> range1->scale) - index1;
			if (gap <= ~(0xffffffffffffffff << (range2->scale - range1->scale))){
				if ((((upper_bound1 - disp2) & ~(0xffffffffffffffff << range2->scale)) >> range1->scale) + gap <= ~(0xffffffffffffffff << (range2->scale - range1->scale))){
					return 1;
				}
			}

			if ((((index2 << range2->scale) + disp2) & range2->mask) < disp2){
				return variableRange_seg_include_seg(disp1, upper_bound1, disp2, disp2 | (range2->mask & (0xffffffffffffffff << range2->scale)), range1->mask) && variableRange_seg_include_seg(disp1, upper_bound1, disp2 & ~(0xffffffffffffffff << range2->scale), upper_bound2, range1->mask);
			}
			else{
				return variableRange_seg_include_seg(disp1, upper_bound1, disp2, upper_bound2, range1->mask);
			}
		}
	}
	else{
		return 0;
	}
}

static int32_t variableRange_seg_intersect_seg(uint64_t seg1_lo, uint64_t seg1_up, uint64_t seg2_lo, uint64_t seg2_up, uint64_t mask1, uint64_t mask2){
	if (seg1_lo <= seg1_up){
		if (seg2_lo <= seg2_up){
			return ((seg1_lo <= seg2_up) && (seg1_up >= seg2_lo));
		}
		else{
			return (mask2 >= seg1_lo) && ((seg1_lo <= seg2_up) || (seg1_up >= seg2_lo));
		}
	}
	else{
		if (seg2_lo <= seg2_up){
			return (mask1 >= seg2_lo) && ((seg1_lo <= seg2_up) || (seg1_up >= seg2_lo));
		}
		else{
			return 1;
		}
	}
}

int32_t variableRange_is_range_intersect(const struct variableRange* range1, const struct variableRange* range2){
	uint64_t disp1;
	uint64_t disp2;
	uint64_t index1;
	uint64_t index2;
	uint64_t rdisp1;
	uint32_t rscale;

	if (range2->scale > range1->scale){
		return variableRange_is_range_intersect(range2, range1);
	}

	disp1 = range1->disp & range1->mask;
	index1 = range1->index;
	if (range1->index > (range1->mask >> range1->scale)){
		index1 = range1->mask >> range1->scale;
		disp1 &= ~(0xffffffffffffffff << range1->scale);
	}

	disp2 = range2->disp & range2->mask;
	index2 = range2->index;
	if (range2->index > (range2->mask >> range2->scale)){
		index2 = range2->mask >> range2->scale;
		disp2 &= ~(0xffffffffffffffff << range2->scale);
	}

	if (!index2){
		return variableRange_is_value_include(range1, disp2);
	}
	else if (!index1){
		return variableRange_is_value_include(range2, disp1);
	}
	else{
		if ((disp2 - disp1) & ~(0xffffffffffffffff << range2->scale)){
			return 0;
		}

		disp1 >>= range2->scale;
		disp2 >>= range2->scale;

		rscale = range1->scale - range2->scale;
		rdisp1 = disp1 & bitmask64(rscale);

		if ((disp2 & bitmask64(rscale)) > rdisp1){
			uint64_t offset;

			offset = ((bitmask64(rscale) & (range2->mask >> range2->scale)) + 1) - (disp2 & bitmask64(rscale)) + rdisp1;
			if (index2 < offset){
				return 0;
			}
			else{
				index2 -= offset;
				disp2 = (disp2 + offset) & (range2->mask >> range2->scale);
			}
		}
		else if ((rdisp1 - (disp2 & bitmask64(rscale)) > index2) || (rdisp1 & ~(range2->mask >> range2->scale))){
			return 0;
		}

		if (((index2 + disp2) & (range2->mask >> range2->scale) & bitmask64(rscale)) < rdisp1){
			uint64_t offset;

			offset = ((index2 + disp2) & (range2->mask >> range2->scale) & bitmask64(rscale)) + ((uint64_t)0x0000000000000001 << rscale) - rdisp1;
			if (index2 < offset){
				index2 = 0;
			}
			else{
				index2 -= offset;
			}
		}

		disp1 >>= rscale;
		disp2 >>= rscale;
		index2 >>= rscale;

		return variableRange_seg_intersect_seg(disp1, (index1 + disp1) & (range1->mask >> range1->scale), disp2, (index2 + disp2) & (range2->mask >> range1->scale), range1->mask >> range1->scale, range2->mask >> range1->scale);
	}
}

void variableRange_check_format(const struct variableRange* range){
	if (range->index & ~(range->mask >> range->scale)){
		log_warn_m("incorrect index format: 0x%llx", range->index);
	}
}

void variableRange_print(const struct variableRange* range){
	if (!range->index){
		printf("%llx, mask=%llx", range->disp, range->mask);
	}
	else{
		if (!range->scale){
			if (!range->disp){
				printf("[0, %llx], mask=%llx", range->index, range->mask);
			}
			else{
				printf("[0, %llx] + %llx, mask=%llx", range->index, range->disp, range->mask);
			}
		}
		else{
			if (!range->disp){
				printf("[0, %llx] << %u, mask=%llx", range->index, range->scale, range->mask);
			}
			else{
				printf("[0, %llx] << %u + %llx, mask=%llx", range->index, range->scale, range->disp, range->mask);
			}
		}
	}
}
