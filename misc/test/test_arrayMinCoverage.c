#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "../arrayMinCoverage.h"
#include "../base.h"

#define NB_CATEGORY 				16
#define MAX_ARRAY_PER_CATEGORY 		5
#define MAX_ELEMENT_PER_ARRAY		32
#define MAX_ELEMENT_RANGE 			0x00000fff

static int32_t compare_uint32_t(const void* arg1, const void* arg2){
	uint32_t n1 = *(uint32_t*)cmp_get_element(arg1);
	uint32_t n2 = *(uint32_t*)cmp_get_element(arg2);

	if (n1 < n2){
		return -1;
	}
	else if (n1 > n2){
		return 1;
	}
	else{
		return 0;
	}
}

static struct array* test_array_create(uint32_t nb_element){
	struct array* 	array;
	uint32_t 		i;
	uint32_t 		element;

	if ((array = array_create(sizeof(uint32_t))) == NULL){
		log_err("unable to create array");
		return NULL;
	}

	for (i = 0; i < nb_element; i++){
		element = rand() & MAX_ELEMENT_RANGE;
		if (array_add(array, &element) < 0){
			log_err("unable to add element to array");
		}
	}

	return array;
}

static void test_array_print(struct array* array){
	uint32_t i;

	fputs("\t{", stdout);
	for (i = 0; i < array_get_length(array); i++){
		printf("%3u", *(uint32_t*)array_get(array, i));
		if (i + 1 != array_get_length(array)){
			putchar(' ');
		}
	}
	fputs("}\n", stdout);
}

static uint64_t estimate_complexity(struct categoryDesc* desc_buffer, uint32_t nb_category){
	uint64_t result = 1;
	uint32_t i;

	for (i = 0; i < nb_category; i++){
		result = result * (uint64_t)desc_buffer[i].nb_element;
		if (result & 0xffffffff00000000ULL){
			return result;
		}
	}

	return result;
}

int32_t main(void){
	uint32_t 				nb_category;
	uint32_t 				i;
	uint32_t 				j;
	struct categoryDesc* 	desc;
	struct array 			array;
	struct array* 			sub_array;
	uint32_t 				nb_element;
	uint32_t 				rand_score;
	uint32_t 				greedy_score;
	uint32_t 				split_score;
	uint32_t 				reshape_score;
	uint32_t 				exact_score;
	time_t 					seed;
	struct timespec 		start_time;
	struct timespec 		stop_time;

	seed = time(NULL);
	// seed = 1456226126;
	srand(seed);
	log_debug_m("Seed is equal to %ld", seed);

	/* INIT */
	nb_category = NB_CATEGORY;
	desc = malloc(sizeof(struct categoryDesc) * nb_category);
	if (desc == NULL){
		log_err("unable to allocate memory");
		return EXIT_FAILURE;
	}

	if (array_init(&array, sizeof(struct array*))){
		log_err("unable to init array");
		return EXIT_FAILURE;
	}

	for (i = 0; i < nb_category; i++){
		desc[i].offset = array_get_length(&array);
		desc[i].nb_element = 1 + (rand() % (MAX_ARRAY_PER_CATEGORY - 1));

		nb_element = 1 + (rand() % (MAX_ELEMENT_PER_ARRAY - 1));

		for (j = 0; j < desc[i].nb_element; j++){
			if ((sub_array = test_array_create(nb_element)) == NULL){
				log_err("test_create_array returned NULL pointer");
				return EXIT_FAILURE;
			}
			else{
				if (array_add(&array, &sub_array) < 0){
					log_err("unable to add element to array");
					return EXIT_FAILURE;
				}
			}
		}
	}

	/* PRINT */
	for (i = 0; i < nb_category; i++){
		printf("Category %u, %u set(s):\n", i + 1, desc[i].nb_element);
		for (j = desc[i].offset; j < desc[i].offset + desc[i].nb_element; j++){
			test_array_print(*(struct array**)array_get(&array, j));
		}
	}

	/* COMPUTE */
	if (arrayMinCoverage_rand_wrapper(&array, nb_category, desc, compare_uint32_t, &rand_score)){
		log_err("arrayMinCoverage_greedy returned an error");
	}
	else{
		if (rand_score != arrayMinCoverage_eval(&array, nb_category, desc, compare_uint32_t)){
			log_err_m("unable to verify rand score, get %u", arrayMinCoverage_eval(&array, nb_category, desc, compare_uint32_t));
		}

		log_info_m("score of arrayMinCoverage_rand is %u", rand_score);
		putchar('\t');
		for (i = 0; i < nb_category; i++){
			if (i + 1 == nb_category){
				printf("%u", desc[i].choice);
			}
			else{
				printf("%u, ", desc[i].choice);
			}
		}
		putchar('\n');
	}

	if (arrayMinCoverage_greedy_wrapper(&array, nb_category, desc, compare_uint32_t, &greedy_score)){
		log_err("arrayMinCoverage_greedy returned an error");
	}
	else{
		if (greedy_score != arrayMinCoverage_eval(&array, nb_category, desc, compare_uint32_t)){
			log_err_m("unable to verify greedy score, get %u", arrayMinCoverage_eval(&array, nb_category, desc, compare_uint32_t));
		}

		log_info_m("score of arrayMinCoverage_greedy is %u", greedy_score);
		putchar('\t');
		for (i = 0; i < nb_category; i++){
			if (i + 1 == nb_category){
				printf("%u", desc[i].choice);
			}
			else{
				printf("%u, ", desc[i].choice);
			}
		}
		putchar('\n');
	}

	if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start_time)){
		log_err("clock_gettime fails");
	}
	if (arrayMinCoverage_reshape_wrapper(&array, nb_category, desc, compare_uint32_t, &reshape_score)){
		log_err("arrayMinCoverage_exact returned an error");
	}
	else{
		if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &stop_time)){
			log_err("clock_gettime fails");
		}

		if (reshape_score != arrayMinCoverage_eval(&array, nb_category, desc, compare_uint32_t)){
			log_err_m("unable to verify reshape score, get %u", arrayMinCoverage_eval(&array, nb_category, desc, compare_uint32_t));
		}

		log_info_m("score of arrayMinCoverage_reshape is %u (runtime: %.3fs)", reshape_score, stop_time.tv_sec - start_time.tv_sec + (float)(stop_time.tv_nsec - start_time.tv_nsec) / 1000000000);
		putchar('\t');
		for (i = 0; i < nb_category; i++){
			if (i + 1 == nb_category){
				printf("%u", desc[i].choice);
			}
			else{
				printf("%u, ", desc[i].choice);
			}
		}
		putchar('\n');
	}

	if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start_time)){
		log_err("clock_gettime fails");
	}
	if (arrayMinCoverage_split_wrapper(&array, nb_category, desc, compare_uint32_t, &split_score)){
		log_err("arrayMinCoverage_split returned an error");
	}
	else{
		if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &stop_time)){
			log_err("clock_gettime fails");
		}

		if (split_score != arrayMinCoverage_eval(&array, nb_category, desc, compare_uint32_t)){
			log_err_m("unable to verify split score, get %u", arrayMinCoverage_eval(&array, nb_category, desc, compare_uint32_t));
		}

		log_info_m("score of arrayMinCoverage_split is %u (runtime: %.3fs)", split_score, stop_time.tv_sec - start_time.tv_sec + (float)(stop_time.tv_nsec - start_time.tv_nsec) / 1000000000);
		putchar('\t');
		for (i = 0; i < nb_category; i++){
			if (i + 1 == nb_category){
				printf("%u", desc[i].choice);
			}
			else{
				printf("%u, ", desc[i].choice);
			}
		}
		putchar('\n');
	}

	if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start_time)){
		log_err("clock_gettime fails");
	}
	if (arrayMinCoverage_exact_wrapper(&array, nb_category, desc, compare_uint32_t, &exact_score)){
		log_err("arrayMinCoverage_exact returned an error");
	}
	else{
		if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &stop_time)){
			log_err("clock_gettime fails");
		}

		if (exact_score != arrayMinCoverage_eval(&array, nb_category, desc, compare_uint32_t)){
			log_err_m("unable to verify exact score, get %u", arrayMinCoverage_eval(&array, nb_category, desc, compare_uint32_t));
		}

		log_info_m("score of arrayMinCoverage_exact is %u (runtime: %.3fs ; complexity: %llu)", exact_score, stop_time.tv_sec - start_time.tv_sec + (float)(stop_time.tv_nsec - start_time.tv_nsec) / 1000000000, estimate_complexity(desc, nb_category));
		putchar('\t');
		for (i = 0; i < nb_category; i++){
			if (i + 1 == nb_category){
				printf("%u", desc[i].choice);
			}
			else{
				printf("%u, ", desc[i].choice);
			}
		}
		putchar('\n');
	}

	if (exact_score > greedy_score || exact_score > split_score || exact_score > reshape_score || exact_score > rand_score){
		log_err("something is wrong with the exact score");
	}
	if (split_score > greedy_score || split_score > reshape_score){
		log_warn("something is disappointing with the split score");
	}
	if (reshape_score > greedy_score){
		log_warn("something is disappointing with the reshape score");
	}

	/* CLEAN */
	for (i = 0; i < array_get_length(&array); i++){
		array_delete(*(struct array**)array_get(&array, i));
	}
	array_clean(&array);

	free(desc);

	return EXIT_SUCCESS;
}
