#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "../variableRange.h"
#include "../base.h"

// #define SEED 		5190
#define MAX_SIZE 	12
#define NB_TEST 	131072

#ifndef SEED
#define SEED (uint32_t)getpid()
#endif

#define ENABLE_RANGE_TEST_ADD 0
#define ENABLE_RANGE_TEST_AND 0
#define ENABLE_RANGE_TEST_OR  0
#define ENABLE_RANGE_TEST_SUB 0
#define ENABLE_RANGE_TEST_XOR 1

static uint32_t variableRange_init_rand(struct variableRange* range){
	uint32_t size;

	size = rand() % MAX_SIZE;
	range->mask 	= bitmask64(size);
	range->disp 	= rand() & range->mask;
	range->scale 	= rand() % (size + 1); 	/* fuzzy scale */
	range->index 	= rand() & range->mask; /* fuzzy index */

	return size;
}

int32_t main(void){
	struct variableRange 			range1;
	struct variableRange 			range2;
	struct variableRange 			range3;
	struct variableRangeIterator 	it1;
	struct variableRangeIterator 	it2;
	uint64_t 						value1;
	uint64_t 						value2;
	uint64_t 						value3;
	uint32_t 						i;
	uint32_t 						j;
	uint32_t 						k;
	uint32_t 						size;

	log_info_m("seed=%d", SEED);
	srand(SEED);

	log_info("Test iterator & value include");
	for (i = 0; i < NB_TEST; i++){
		variableRange_init_rand(&range1);
		for (variableRangeIterator_init(&it1, &range1), j = 0; variableRangeIterator_get_next(&it1, &value1); j++){
			if (!variableRange_is_value_include(&range1, value1)){
				log_err_m("%u", i);
				printf("\t%llx not in ", value1); variableRange_print(&range1); printf("\n");
				return EXIT_FAILURE;
			}
		}
		if (j != range1.index + 1){
			log_err_m("%u", i);
			printf("\tfound %u element(s) in ", j); variableRange_print(&range1); printf("\n");
			return EXIT_FAILURE;
		}
	}

	log_info("Test resize");
	for (i = 0; i < NB_TEST; i++){
		size = variableRange_init_rand(&range1);
		if (!size){
			continue;
		}
		value2 = rand() % MAX_SIZE;
		memcpy(&range2, &range1, sizeof(struct variableRange));
		variableRange_resize(&range2, value2);
		for (variableRangeIterator_init(&it1, &range1); variableRangeIterator_get_next(&it1, &value1); ){
			value3 = value1 & ~(0xffffffffffffffff << value2);
			if (!variableRange_is_value_include(&range2, value3)){
				log_err_m("%u", i);
				printf("\t%llx = (%llx & %llx) not in ", value3, value1, ~(0xffffffffffffffff << value2)); variableRange_print(&range2); printf(" ("); variableRange_print(&range1); printf(" resized by %llx)\n", ~(0xffffffffffffffff << value2));
				return EXIT_FAILURE;
			}
		}
	}

	log_info("Test neg");
	for (i = 0; i < NB_TEST; i++){
		size = variableRange_init_rand(&range1);
		if (!size){
			continue;
		}
		value2 = rand() % MAX_SIZE;
		memcpy(&range2, &range1, sizeof(struct variableRange));
		variableRange_neg(&range2, value2);
		for (variableRangeIterator_init(&it1, &range1); variableRangeIterator_get_next(&it1, &value1); ){
			value3 = (-value1) & ~(0xffffffffffffffff << value2);
			if (!variableRange_is_value_include(&range2, value3)){
				log_err_m("%u", i);
				printf("\t%llx = (-%llx & %llx) not in ", value3, value1, ~(0xffffffffffffffff << value2)); variableRange_print(&range2); printf(" ("); variableRange_print(&range1); printf(" neg in %llx)\n", ~(0xffffffffffffffff << value2));
				return EXIT_FAILURE;
			}
		}
	}

	log_info("Test add value");
	for (i = 0; i < NB_TEST; i++){
		size = variableRange_init_rand(&range1);
		if (!size){
			continue;
		}
		value2 = rand() % MAX_SIZE;
		memcpy(&range2, &range1, sizeof(struct variableRange));
		variableRange_add_value(&range2, value2, MAX_SIZE);
		for (variableRangeIterator_init(&it1, &range1); variableRangeIterator_get_next(&it1, &value1); ){
			value3 = (value1 + value2) & bitmask64(MAX_SIZE);
			if (!variableRange_is_value_include(&range2, value3)){
				log_err_m("%u", i);
				printf("\t%llx = (%llu + %llu) not in ", value3, value1, value2); variableRange_print(&range2); printf(" ("); variableRange_print(&range1); printf(" + %llu)\n", value2);
				return EXIT_FAILURE;
			}
		}
	}

	log_info("Test and value");
	for (i = 0; i < NB_TEST; i++){
		size = variableRange_init_rand(&range1);
		if (!size){
			continue;
		}
		value2 = rand() % size;
		memcpy(&range2, &range1, sizeof(struct variableRange));
		variableRange_and_value(&range2, value2);
		for (variableRangeIterator_init(&it1, &range1); variableRangeIterator_get_next(&it1, &value1); ){
			value3 = value1 & value2;
			if (!variableRange_is_value_include(&range2, value3)){
				log_err_m("%u", i);
				printf("\t%llx = (%llx & %llx) not in ", value3, value1, value2); variableRange_print(&range2); printf(" ("); variableRange_print(&range1); printf(" & %llx)\n", value2);
				return EXIT_FAILURE;
			}
		}
	}

	log_info("Test or value");
	for (i = 0; i < NB_TEST; i++){
		size = variableRange_init_rand(&range1);
		if (!size){
			continue;
		}
		value2 = rand() % MAX_SIZE;
		memcpy(&range2, &range1, sizeof(struct variableRange));
		variableRange_or_value(&range2, value2, MAX_SIZE);
		for (variableRangeIterator_init(&it1, &range1); variableRangeIterator_get_next(&it1, &value1); ){
			value3 = value1 | value2;
			if (!variableRange_is_value_include(&range2, value3)){
				log_err_m("%u", i);
				printf("\t%llx = (%llx | %llx) not in ", value3, value1, value2); variableRange_print(&range2); printf(" ("); variableRange_print(&range1); printf(" | %llx)\n", value2);
				return EXIT_FAILURE;
			}
		}
	}

	log_info("Test shl value");
	for (i = 0; i < NB_TEST; i++){
		size = variableRange_init_rand(&range1);
		if (!size){
			continue;
		}
		value2 = rand() % size;
		memcpy(&range2, &range1, sizeof(struct variableRange));
		variableRange_shl_value(&range2, value2, MAX_SIZE);
		variableRange_check_format(&range2);
		for (variableRangeIterator_init(&it1, &range1); variableRangeIterator_get_next(&it1, &value1); ){
			value3 = (value1 << value2) & bitmask64(MAX_SIZE);
			if (!variableRange_is_value_include(&range2, value3)){
				log_err_m("%u", i);
				printf("\t%llx = (%llx << %llu) not in ", value3, value1, value2); variableRange_print(&range2); printf(" ("); variableRange_print(&range1); printf(" << %llu)\n", value2);
				return EXIT_FAILURE;
			}
		}
	}

	log_info("Test shr value");
	for (i = 0; i < NB_TEST; i++){
		size = variableRange_init_rand(&range1);
		if (!size){
			continue;
		}
		value2 = rand() % size;
		memcpy(&range2, &range1, sizeof(struct variableRange));
		variableRange_shr_value(&range2, value2);
		for (variableRangeIterator_init(&it1, &range1); variableRangeIterator_get_next(&it1, &value1); ){
			value3 = value1 >> value2;
			if (!variableRange_is_value_include(&range2, value3)){
				log_err_m("%u", i);
				printf("\t%llx = (%llx >> %llu) not in ", value3, value1, value2); variableRange_print(&range2); printf(" ("); variableRange_print(&range1); printf(" >> %llu)\n", value2);
				return EXIT_FAILURE;
			}
		}
	}

	log_info("Test sub value");
	for (i = 0; i < NB_TEST; i++){
		size = variableRange_init_rand(&range1);
		if (!size){
			continue;
		}
		value2 = rand() % MAX_SIZE;
		memcpy(&range2, &range1, sizeof(struct variableRange));
		variableRange_sub_value(&range2, value2, MAX_SIZE);
		for (variableRangeIterator_init(&it1, &range1); variableRangeIterator_get_next(&it1, &value1); ){
			value3 = (value1 - value2) & bitmask64(MAX_SIZE);
			if (!variableRange_is_value_include(&range2, value3)){
				log_err_m("%u", i);
				printf("\t%llx = (%llu - %llu) not in ", value3, value1, value2); variableRange_print(&range2); printf(" ("); variableRange_print(&range1); printf(" - %llu)\n", value2);
				return EXIT_FAILURE;
			}
		}
	}

	log_info("Test xor value");
	for (i = 0; i < NB_TEST; i++){
		size = variableRange_init_rand(&range1);
		if (!size){
			continue;
		}
		value2 = rand() % MAX_SIZE;
		memcpy(&range2, &range1, sizeof(struct variableRange));
		variableRange_xor_value(&range2, value2, MAX_SIZE);
		for (variableRangeIterator_init(&it1, &range1); variableRangeIterator_get_next(&it1, &value1); ){
			value3 = value1 ^ value2;
			if (!variableRange_is_value_include(&range2, value3)){
				log_err_m("%u", i);
				printf("\t%llx = (%llx ^ %llx) not in ", value3, value1, value2); variableRange_print(&range2); printf(" ("); variableRange_print(&range1); printf(" ^ %llx)\n", value2);
				return EXIT_FAILURE;
			}
		}
	}

	#if ENABLE_RANGE_TEST_ADD == 1
	log_info("Test add range");
	for (i = 0; i < NB_TEST; i++){
		if (!variableRange_init_rand(&range1)){
			continue;
		}
		if (!variableRange_init_rand(&range2)){
			continue;
		}

		memcpy(&range3, &range1, sizeof(struct variableRange));
		size = rand() % MAX_SIZE + 1;
		variableRange_add_range(&range3, &range2, size);

		for (variableRangeIterator_init(&it1, &range1); variableRangeIterator_get_next(&it1, &value1); ){
			for (variableRangeIterator_init(&it2, &range2); variableRangeIterator_get_next(&it2, &value2); ){
				value3 = (value1 + value2) & bitmask64(size);
				if (!variableRange_is_value_include(&range3, value3)){
					log_err_m("%u", i);
					printf("\t%llx = (%llx + %llx mask %llx) not in ", value3, value1, value2, bitmask64(size)); variableRange_print(&range3); printf(" ("); variableRange_print(&range1); printf(" + "); variableRange_print(&range2); printf(")\n");
					return EXIT_FAILURE;
				}
			}
		}
	}
	#endif

	#if ENABLE_RANGE_TEST_AND == 1
	log_info("Test and range");
	for (i = 0; i < NB_TEST; i++){
		if (!variableRange_init_rand(&range1)){
			continue;
		}
		if (!variableRange_init_rand(&range2)){
			continue;
		}

		memcpy(&range3, &range1, sizeof(struct variableRange));
		size = rand() % MAX_SIZE + 1;
		variableRange_and_range(&range3, &range2, size);

		for (variableRangeIterator_init(&it1, &range1); variableRangeIterator_get_next(&it1, &value1); ){
			for (variableRangeIterator_init(&it2, &range2); variableRangeIterator_get_next(&it2, &value2); ){
				value3 = value1 & value2 & bitmask64(size);
				if (!variableRange_is_value_include(&range3, value3)){
					log_err_m("%u", i);
					printf("\t%llx = (%llx & %llx mask %llx) not in ", value3, value1, value2, bitmask64(size)); variableRange_print(&range3); printf(" ("); variableRange_print(&range1); printf(" & "); variableRange_print(&range2); printf(")\n");
					return EXIT_FAILURE;
				}
			}
		}
	}
	#endif

	#if ENABLE_RANGE_TEST_OR == 1
	log_info("Test or range");
	for (i = 0; i < NB_TEST; i++){
		if (!variableRange_init_rand(&range1)){
			continue;
		}
		if (!variableRange_init_rand(&range2)){
			continue;
		}

		memcpy(&range3, &range1, sizeof(struct variableRange));
		size = rand() % MAX_SIZE + 1;
		variableRange_or_range(&range3, &range2, size);

		for (variableRangeIterator_init(&it1, &range1); variableRangeIterator_get_next(&it1, &value1); ){
			for (variableRangeIterator_init(&it2, &range2); variableRangeIterator_get_next(&it2, &value2); ){
				value3 = (value1 | value2) & bitmask64(size);
				if (!variableRange_is_value_include(&range3, value3)){
					log_err_m("%u", i);
					printf("\t%llx = (%llx | %llx mask %llx) not in ", value3, value1, value2, bitmask64(size)); variableRange_print(&range3); printf(" ("); variableRange_print(&range1); printf(" | "); variableRange_print(&range2); printf(")\n");
					return EXIT_FAILURE;
				}
			}
		}
	}
	#endif

	#if ENABLE_RANGE_TEST_SUB == 1
	log_info("Test sub range");
	for (i = 0; i < NB_TEST; i++){
		if (!variableRange_init_rand(&range1)){
			continue;
		}
		if (!variableRange_init_rand(&range2)){
			continue;
		}

		memcpy(&range3, &range1, sizeof(struct variableRange));
		size = rand() % MAX_SIZE + 1;
		variableRange_sub_range(&range3, &range2, size);

		for (variableRangeIterator_init(&it1, &range1); variableRangeIterator_get_next(&it1, &value1); ){
			for (variableRangeIterator_init(&it2, &range2); variableRangeIterator_get_next(&it2, &value2); ){
				value3 = (value1 - value2) & bitmask64(size);
				if (!variableRange_is_value_include(&range3, value3)){
					log_err_m("%u", i);
					printf("\t%llx = (%llx - %llx mask %llx) not in ", value3, value1, value2, bitmask64(size)); variableRange_print(&range3); printf(" ("); variableRange_print(&range1); printf(" - "); variableRange_print(&range2); printf(")\n");
					return EXIT_FAILURE;
				}
			}
		}
	}
	#endif

	#if ENABLE_RANGE_TEST_XOR == 1
	log_info("Test xor range");
	for (i = 0; i < NB_TEST; i++){
		if (!variableRange_init_rand(&range1)){
			continue;
		}
		if (!variableRange_init_rand(&range2)){
			continue;
		}

		memcpy(&range3, &range1, sizeof(struct variableRange));
		size = rand() % MAX_SIZE + 1;
		variableRange_xor_range(&range3, &range2, size);

		for (variableRangeIterator_init(&it1, &range1); variableRangeIterator_get_next(&it1, &value1); ){
			for (variableRangeIterator_init(&it2, &range2); variableRangeIterator_get_next(&it2, &value2); ){
				value3 = (value1 ^ value2) & bitmask64(size);
				if (!variableRange_is_value_include(&range3, value3)){
					log_err_m("%u", i);
					printf("\t%llx = (%llx ^ %llx mask %llx) not in ", value3, value1, value2, bitmask64(size)); variableRange_print(&range3); printf(" ("); variableRange_print(&range1); printf(" ^ "); variableRange_print(&range2); printf(")\n");
					return EXIT_FAILURE;
				}
			}
		}
	}
	#endif

	log_info("Test include range");
	for (i = 0; i < NB_TEST; i++){
		if (!variableRange_init_rand(&range1)){
			continue;
		}
		if (!variableRange_init_rand(&range2)){
			continue;
		}

		j = variableRange_is_range_include(&range1, &range2);

		for (variableRangeIterator_init(&it2, &range2), k = 1; variableRangeIterator_get_next(&it2, &value2); ){
			if (!variableRange_is_value_include(&range1, value2)){
				k = 0;
				if (j){
					log_err_m("%u", i);
					printf("\t"); variableRange_print(&range2); printf(" not included in "); variableRange_print(&range1); printf(" counter example is %llx\n", value2);
					return EXIT_FAILURE;
				}
			}
		}
		if (k && !j){
			log_err_m("%u", i);
			printf("\t"); variableRange_print(&range2); printf(" included in "); variableRange_print(&range1); printf("\n");
			return EXIT_FAILURE;
		}
	}

	log_info("Test intersect range");
	for (i = 0; i < NB_TEST; i++){
		if (!variableRange_init_rand(&range1)){
			continue;
		}
		if (!variableRange_init_rand(&range2)){
			continue;
		}

		j = variableRange_is_range_intersect(&range1, &range2);

		for (variableRangeIterator_init(&it2, &range2), k = 0; variableRangeIterator_get_next(&it2, &value2); ){
			if (variableRange_is_value_include(&range1, value2)){
				k = 1;
				if (!j){
					log_err_m("%u", i);
					printf("\t"); variableRange_print(&range2); printf(" intersect "); variableRange_print(&range1); printf(" counter example is %llx\n", value2);
					return EXIT_FAILURE;
				}
			}
		}
		if (!k && j){
			log_err_m("%u", i);
			printf("\t"); variableRange_print(&range2); printf(" does not intersect "); variableRange_print(&range1); printf("\n");
			return EXIT_FAILURE;
		}
	}

	return EXIT_SUCCESS;
}
