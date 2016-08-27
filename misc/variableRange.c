#include <stdlib.h>
#include <stdio.h>

#include "variableRange.h"
#include "base.h"

void variableRange_shl_value(struct variableRange* range, uint64_t value){
	if (value >= 64){
		variableRange_init_cst(range, 0, range->mask);
	}
	else{
		range->disp = (range->disp << value) & range->mask;
		if (range->index){
			if (value + range->scale >= 64 && !(range->mask >> (value + range->scale))){
				range->index = 0;
				range->scale = 0;
			}
			else{
				range->scale += value;
			}
		}
	}
}

void variableRange_shr_value(struct variableRange* range, uint64_t value){
	if (value >= 64 || !(range->mask >> value)){
		variableRange_init_cst(range, 0, range->mask);
	}
	else{
		if (range->index){
			if (range->scale > value){
				range->disp = range->disp >> value;
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
			range->disp = range->disp >> value;
		}
	}
}

int32_t variableRange_is_value_include(const struct variableRange* range, uint64_t value){
	if (value & ~range->mask){
		return 0;
	}

	value = (value - range->disp) & range->mask;
	if (value & ~(0xffffffffffffffff << range->scale)){
		return 0;
	}

	value >>= range->scale;
	if (value > range->index){
		return 0;
	}

	return 1;
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

#if 0
/* #define DEBUG_RANGE */

static int32_t variableRange_seg_include_pt(uint64_t seg_lo, uint64_t seg_up, uint64_t pt){
	if (seg_lo <= seg_up){
		return ((seg_lo <= pt) && (seg_up >= pt));
	}
	else{
		return ((seg_lo <= pt) || (seg_up >= pt));
	}
}

static int32_t variableRange_seg_intersect_seg(uint64_t seg1_lo, uint64_t seg1_up, uint64_t seg2_lo, uint64_t seg2_up){
	if (seg1_lo <= seg1_up){
		if (seg2_lo <= seg2_up){
			return ((seg1_lo <= seg2_up) && (seg1_up >= seg2_lo));
		}
		else{
			return ((seg1_lo <= seg2_up) || (seg1_up >= seg2_lo));
		}
	}
	else{
		if (seg2_lo <= seg2_up){
			return ((seg1_lo >= seg2_up) || (seg1_up >= seg2_lo));
		}
		else{
			return 1;
		}
	}
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

void variableRange_init_mask(struct variableRange* range, uint64_t mask, uint32_t size){
	range->size_mask = variableRange_get_size_mask(size);
	if ((mask & range->size_mask) == 0){
		range->index_lo 	= 0;
		range->index_up 	= 0;
		range->scale 		= 0xffffffff;
		range->disp 		= 0; 	
	}
	else{
		range->index_lo 	= 0;
		range->index_up 	= 0xffffffffffffffff >> (__builtin_clzll(mask & range->size_mask) + __builtin_ctzll(mask & range->size_mask));
		range->scale 		= __builtin_ctzll(mask & range->size_mask);
		range->disp 		= 0;
	}
}

void variableRange_add(struct variableRange* range_dst, const struct variableRange* range_src, uint32_t size){
	if (variableRange_is_cst(range_dst)){
		range_dst->index_lo 	= range_src->index_lo;
		range_dst->index_up 	= range_src->index_up;
		range_dst->scale 		= range_src->scale;
		range_dst->disp 		= range_src->disp + range_dst->disp;
		range_dst->size_mask 	= max(range_dst->size_mask, range_src->size_mask) & variableRange_get_size_mask(size);
	}
	else if (variableRange_is_cst(range_src)){
		range_dst->disp 		= range_dst->disp + range_src->disp;
		range_dst->size_mask 	= max(range_dst->size_mask, range_src->size_mask) & variableRange_get_size_mask(size);
	}
	else if (range_dst->scale == range_src->scale){
		uint64_t len_dst;
		uint64_t len_src;
		uint64_t size_mask;
		uint64_t disp_dst;
		uint64_t disp_src;

		if (range_dst->size_mask < range_src->size_mask && range_dst->index_lo > range_dst->index_up){
			#ifdef DEBUG_RANGE
			log_warn("over approximation due to size mismatch");
			#endif
			len_dst = variableRange_get_index_mask(range_dst);
			disp_dst = range_dst->disp;
		}
		else{
			len_dst = (range_dst->index_up - range_dst->index_lo) & variableRange_get_index_mask(range_dst);
			disp_dst = (range_dst->disp + (range_dst->index_lo << range_dst->scale)) & range_dst->size_mask;	
		}
		
		if (range_src->size_mask < range_dst->size_mask && range_src->index_lo > range_src->index_up){
			#ifdef DEBUG_RANGE
			log_warn("over approximation due to size mismatch");
			#endif
			len_src = variableRange_get_index_mask(range_src);
			disp_src = range_src->disp;
		}
		else{
			len_src = (range_src->index_up - range_src->index_lo) & variableRange_get_index_mask(range_src);
			disp_src = (range_src->disp + (range_src->index_lo << range_src->scale)) & range_src->size_mask;
		}

		size_mask = max(range_dst->size_mask, range_src->size_mask) & variableRange_get_size_mask(size);

		if (len_dst >= (size_mask >> range_dst->scale) - len_src){
			range_dst->index_lo 	= 0;
			range_dst->index_up 	= 0xffffffffffffffff;
			range_dst->disp 		= range_dst->disp + range_src->disp;
			range_dst->size_mask 	= size_mask;
		}
		else{
			range_dst->index_lo 	= 0;
			range_dst->index_up 	= len_dst + len_src;
			range_dst->size_mask 	= size_mask;
			range_dst->disp 		= disp_dst + disp_src;
		}
	}
	else{
		#ifdef DEBUG_RANGE
		log_warn("unable to add ranges with different scale");
		#endif
		variableRange_init_size(range_dst, size);
		return;
	}

	variableRange_pack(range_dst);
}

void variableRange_and(struct variableRange* range_dst, const struct variableRange* range_src, uint32_t size){
	if (!variableRange_is_cst(range_src) && !variableRange_is_cst(range_dst)){
		#ifdef DEBUG_RANGE
		log_warn("unable to mask by a non constant value");
		#endif
		variableRange_bitwise_heuristic(range_dst, range_src, size);
		return;
	}

	if (variableRange_is_cst(range_src) && variableRange_is_cst(range_dst)){
		range_dst->disp			= range_dst->disp & range_src->disp;
		range_dst->size_mask 	= variableRange_get_size_mask(size);
	}
	else{
		uint64_t 	mask;
		int32_t 	mask_ctz;

		if (variableRange_is_cst(range_dst)){
 			mask = range_dst->disp;
 			memcpy(range_dst, range_src, sizeof(struct variableRange));
		}
		else{
			mask = range_src->disp;
		}

		range_dst->disp &= mask;
		mask = mask >> range_dst->scale;
		if (mask == 0){
			range_dst->index_up 	= 0;
			range_dst->index_lo 	= 0;
			range_dst->scale 		= 0xffffffff;
			range_dst->size_mask 	= variableRange_get_size_mask(size);
		}
		else{
			mask_ctz = __builtin_ctzll(mask);
			if (range_dst->index_lo > range_dst->index_up){
				if (mask_ctz){
					range_dst->scale += mask_ctz;
					mask = mask >> mask_ctz;
				}
				range_dst->index_up = mask;
			}
			else{
				if (mask_ctz){
					range_dst->index_up = range_dst->index_up >> mask_ctz;
					range_dst->scale 	= range_dst->scale + mask_ctz;
					mask = mask >> mask_ctz;
				}

				range_dst->index_up = min(range_dst->index_up, mask);
			}
			range_dst->index_lo 	= 0;
			range_dst->size_mask 	= (0xffffffffffffffff >> (__builtin_clzll(mask) - range_dst->scale)) & variableRange_get_size_mask(size);
		}
	}

	variableRange_pack(range_dst);
}

void variableRange_shl(struct variableRange* range_dst, const struct variableRange* range_src, uint32_t size){
	if (!variableRange_is_cst(range_src)){
		#ifdef DEBUG_RANGE
		log_warn("unable to shift left by a non constant value");
		#endif
		variableRange_init_size(range_dst, size);
		return;
	}

	if (!variableRange_is_cst(range_dst)){
		if (range_dst->index_lo > range_dst->index_up){
			range_dst->index_up = 0xffffffffffffffff;
			range_dst->index_lo = 0;
		}
		else if ((range_dst->index_up & (~(variableRange_get_index_mask(range_dst) >> range_src->disp))) != (range_dst->index_lo & (~(variableRange_get_index_mask(range_dst) >> range_src->disp)))){
			range_dst->index_up = 0xffffffffffffffff;
			range_dst->index_lo = 0;
		}

		range_dst->scale 	= range_dst->scale + range_src->disp;
	}
	range_dst->disp 		= range_dst->disp << range_src->disp;
	range_dst->size_mask 	= range_dst->size_mask & variableRange_get_size_mask(size);

	variableRange_pack(range_dst);
}

void variableRange_shr(struct variableRange* range_dst, const struct variableRange* range_src, uint32_t size){
	if (!variableRange_is_cst(range_src)){
		#ifdef DEBUG_RANGE
		log_warn("unable to shift right by a non constant value");
		#endif
		variableRange_init_size(range_dst, size);
		return;
	}

	if (range_src->disp >= size){
		variableRange_init_cst(range_dst, 0, size);
		return;
	}

	if (!variableRange_is_cst(range_dst)){
		if (range_dst->scale >= range_src->disp){
			range_dst->scale 	= range_dst->scale - range_src->disp;
		}
		else if(range_dst->index_lo > range_dst->index_up){
			range_dst->index_lo = range_dst->index_lo >> (range_src->disp - range_dst->scale);
			range_dst->index_up = range_dst->index_up >> (range_src->disp - range_dst->scale);
			range_dst->scale 	= 0;

			if (range_dst->index_lo == range_dst->index_up){
				if (range_dst->index_up != 0){
					range_dst->index_up --;
				}
				else{
					range_dst->index_lo ++;
				}
			}
		}
		else{
			range_dst->index_lo = range_dst->index_lo >> (range_src->disp - range_dst->scale);
			range_dst->index_up = range_dst->index_up >> (range_src->disp - range_dst->scale);
			range_dst->scale 	= 0;
		}
	}

	range_dst->disp 		= range_dst->disp >> range_src->disp;
	range_dst->size_mask 	= range_dst->size_mask >> range_src->disp;

	variableRange_pack(range_dst);
}

void variableRange_or(struct variableRange* range_dst, const struct variableRange* range_src, uint32_t size){
	uint64_t over_range_src;
	uint64_t over_range_dst;

	over_range_src = variableRange_get_index_mask(range_src);
	if (range_src->index_lo <= range_src->index_up){
		over_range_src &= (0xffffffffffffffff >> __builtin_clzll(range_src->index_up));
	}

	over_range_dst = variableRange_get_index_mask(range_dst);
	if (range_dst->index_lo <= range_dst->index_up){
		over_range_dst &= (0xffffffffffffffff >> __builtin_clzll(range_dst->index_up));
	}

	if (range_src->scale > range_dst->scale){
		over_range_src = over_range_src << (range_src->scale - range_dst->scale);
		range_dst->index_lo = 0;
		range_dst->index_up = over_range_src | over_range_dst;
	}
	else if (range_src->scale < range_dst->scale){
		over_range_dst = over_range_dst << (range_dst->scale - range_src->scale);
		range_dst->index_lo = 0;
		range_dst->index_up = over_range_src | over_range_dst;
		range_dst->scale = range_src->scale;
	}
	else{
		range_dst->index_lo = 0;
		range_dst->index_up = over_range_src | over_range_dst;
	}

	range_dst->disp = (range_src->disp | range_dst->disp) & (~(range_dst->index_up << range_dst->scale));

	range_dst->size_mask = variableRange_get_size_mask(size);

	variableRange_pack(range_dst);
}

void variableRange_bitwise_heuristic(struct variableRange* range_dst, const struct variableRange* range_src, uint32_t size){
	if (range_src->disp != 0 || range_dst->disp != 0){
		#ifdef DEBUG_RANGE
		log_warn("bitwise heuristic cannot be applied if disp is non zero");
		#endif
		variableRange_init_size(range_dst, size);
		return;
	}

	if (range_src->index_lo > range_src->index_up){
		if (range_dst->index_lo > range_dst->index_up){
			range_dst->index_lo = 0;
			range_dst->index_up = max(variableRange_get_index_mask(range_src), variableRange_get_index_mask(range_dst));
		}
		else{
			range_dst->index_lo = 0;
			range_dst->index_up = max(variableRange_get_index_mask(range_src), 0xffffffffffffffff >> __builtin_clzll(range_dst->index_up));
		}
	}
	else{
		if (range_dst->index_lo > range_dst->index_up){
			range_dst->index_lo = 0;
			range_dst->index_up = max(0xffffffffffffffff >> __builtin_clzll(range_src->index_up), variableRange_get_index_mask(range_dst));
		}
		else{
			range_dst->index_lo = 0;
			range_dst->index_up = max(0xffffffffffffffff >> __builtin_clzll(range_src->index_up), 0xffffffffffffffff >> __builtin_clzll(range_dst->index_up));
		}
	}

	range_dst->scale = min(range_src->scale, range_dst->scale);
	range_dst->size_mask = variableRange_get_size_mask(size);

	variableRange_pack(range_dst);
}

int32_t variableRange_intersect(const struct variableRange* range1, const struct variableRange* range2){
	if (variableRange_is_cst(range1) && variableRange_is_cst(range2)){
		return variableRange_include(range2, range1);
	}
	else if (variableRange_is_cst(range1)){
		return variableRange_include(range2, range1);
	}
	else if (variableRange_is_cst(range2)){
		return variableRange_include(range1, range2);
	}
	else{
		uint64_t index1_lo;
		uint64_t index1_up;
		uint64_t index2_lo;
		uint64_t index2_up;

		if ((range2->disp - range1->disp) & (~(0xffffffffffffffffU << min(range1->scale, range2->scale)))){
			return 0;
		}

		index1_lo = variableRange_get_lower_bound(range1);
		index1_up = variableRange_get_upper_bound(range1);

		index2_lo = variableRange_get_lower_bound(range2);
		index2_up = variableRange_get_upper_bound(range2);

		if (range1->scale > range2->scale){
			uint64_t length;
			uint64_t offset;

			length = (range2->index_up - range2->index_lo) & variableRange_get_index_mask(range2);
			offset = ((index2_lo - range1->disp) >> range2->scale) & variableRange_get_size_mask(range1->scale - range2->scale);

			if (length < (0x0000000000000001U << (range1->scale - range2->scale))){
				if (offset == 0){
					return variableRange_seg_include_pt(range1->index_lo, range1->index_up, (index2_lo - range1->disp) >> range1->scale);
				}
				else if (offset < (((0x0000000000000001U << (range1->scale - range2->scale)) - length) & variableRange_get_index_mask(range2))){
					return 0;
				}
			}
		}
		else{
			uint64_t length;
			uint64_t offset;

			length = (range1->index_up - range1->index_lo) & variableRange_get_index_mask(range1);
			offset = ((index1_lo - range2->disp) >> range1->scale) & variableRange_get_size_mask(range2->scale - range1->scale);

			if (length < (0x0000000000000001U << (range2->scale - range1->scale))){
				if (offset == 0){
					return variableRange_seg_include_pt(range2->index_lo, range2->index_up, (index1_lo - range2->disp) >> range2->scale);
				}
				else if (offset < (((0x0000000000000001U << (range2->scale - range1->scale)) - length) & variableRange_get_index_mask(range1))){
					return 0;
				}
			}
		}

		if (range1->index_lo > range1->index_up){
			if (range2->index_lo > range2->index_up){
				return variableRange_seg_intersect_seg(index1_lo, (variableRange_get_index_mask(range1) << range1->scale) + range1->disp, index2_lo, (variableRange_get_index_mask(range2) << range2->scale) + range2->disp) || variableRange_seg_intersect_seg(index1_lo, (variableRange_get_index_mask(range1) << range1->scale) + range1->disp, range2->disp, index2_up) || variableRange_seg_intersect_seg(range1->disp, index1_up, index2_lo, (variableRange_get_index_mask(range2) << range2->scale) + range2->disp) || variableRange_seg_intersect_seg(range1->disp, index1_up, range2->disp, index2_up);
			}
			else{
				return variableRange_seg_intersect_seg(index1_lo, (variableRange_get_index_mask(range1) << range1->scale) + range1->disp, index2_lo, index2_up) || variableRange_seg_intersect_seg(range1->disp, index1_up, index2_lo, index2_up);
			}
		}
		else{
			if (range2->index_lo > range2->index_up){
				return variableRange_seg_intersect_seg(index1_lo, index1_up, index2_lo, (variableRange_get_index_mask(range2) << range2->scale) + range2->disp) || variableRange_seg_intersect_seg(index1_lo, index1_up, range2->disp, index2_up);
			}
			else{
				return variableRange_seg_intersect_seg(index1_lo, index1_up, index2_lo, index2_up);
			}
		}
	}
}

int32_t variableRange_include(const struct variableRange* range1, const struct variableRange* range2){
	if (variableRange_is_cst(range1) && variableRange_is_cst(range2)){
		return (variableRange_get_cst(range1) == variableRange_get_cst(range2));
	}
	else if (variableRange_is_cst(range1)){
		return ((variableRange_get_cst(range1) == variableRange_get_lower_bound(range2)) && (variableRange_get_cst(range1) == variableRange_get_upper_bound(range2)));
	}
	else if (variableRange_is_cst(range2)){
		uint64_t reduce_value;

		if (variableRange_get_cst(range2) > range1->size_mask){
			return 0;
		}

		reduce_value = (range2->disp - range1->disp) & range1->size_mask;
		if ((reduce_value & (~(0xffffffffffffffff << range1->scale))) == 0){
			reduce_value = reduce_value >> range1->scale;
			if (range1->index_lo <= range1->index_up){
				return (reduce_value >= range1->index_lo && reduce_value <= range1->index_up);
			}
			else{
				return (reduce_value >= range1->index_lo || reduce_value <= range1->index_up);
			}
		}
		else{
			return 0;
		}
	}
	else{
		if (range2->scale >= range1->scale){
			if (((range1->disp - range2->disp) & (~(0xffffffffffffffff << range1->scale))) == 0){
				uint64_t index1_lo;
				uint64_t index1_up;
				uint64_t index2_lo;
				uint64_t index2_up;
				uint64_t gap;

				index1_lo = variableRange_get_lower_bound(range1);
				index1_up = variableRange_get_upper_bound(range1);

				index2_lo = variableRange_get_lower_bound(range2);
				index2_up = variableRange_get_upper_bound(range2);

				if (range1->size_mask < range2->size_mask){
					if (index2_lo > range1->size_mask || index2_up > range1->size_mask){
						return 0;
					}
					else{
						if (range2->index_lo > range2->index_up){
							return 0;
						}
					}
				}

				gap = (range1->index_lo - range1->index_up) & variableRange_get_index_mask(range1);
				if (gap == 1){
					return 1;
				}
				if (gap <= (0x0000000000000001U << (range2->scale - range1->scale))){
					if ((((index1_up - range2->disp) >> range1->scale) & variableRange_get_size_mask(range2->scale - range1->scale)) <= (((0x0000000000000001U << (range2->scale - range1->scale)) - gap) & variableRange_get_index_mask(range1))){
						return 1;
					}
				}

				if (range2->index_lo > range2->index_up){
					return variableRange_seg_include_seg(index1_lo, index1_up, index2_lo, (variableRange_get_index_mask(range2) << range2->scale) + range2->disp, range1->size_mask) && variableRange_seg_include_seg(index1_lo, index1_up, range2->disp, index2_up, range1->size_mask);
				}
				else{
					return variableRange_seg_include_seg(index1_lo, index1_up, index2_lo, index2_up, range1->size_mask);
				}
			}
			else{
				return 0;
			}
		}
		else{
			return 0;
		}
	}
}

#endif