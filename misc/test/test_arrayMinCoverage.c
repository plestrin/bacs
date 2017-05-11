#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <signal.h>

#include "../arrayMinCoverage.h"
#include "../base.h"

#define NB_CATEGORY 				10
#define MAX_ARRAY_PER_CATEGORY 		12
#define MAX_ELEMENT_PER_ARRAY		16
#define MAX_ELEMENT_RANGE 			0x00000fff

static volatile int stop;

static void interrrupt_handler(void){
    stop = 1;
}

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

static void desc_buffer_print_choice(uint32_t nb_category, struct categoryDesc* desc_buffer){
	uint32_t i;

	putchar('\t');
	for (i = 0; i < nb_category; i++){
		if (i + 1 == nb_category){
			printf("%u", desc_buffer[i].choice);
		}
		else{
			printf("%u, ", desc_buffer[i].choice);
		}
	}
	putchar('\n');
}

int32_t main(void){
	uint32_t 				nb_category;
	uint32_t 				i;
	uint32_t 				j;
	struct categoryDesc* 	desc;
	struct array 			array;
	struct array* 			sub_array;
	uint32_t 				nb_element;
	uint32_t 				score_rand;
	uint32_t 				score_greedy;
	uint32_t 				score_reshape;
	uint32_t 				score_split;
	uint32_t 				score_super;
	uint32_t 				score_exact;
	time_t 					seed;
	struct timespec 		start_time;
	struct timespec 		stop_time;
	double 					exec_time;
	double 					tot_rand 	= 0.0;
	double 					tot_greedy 	= 0.0;
	double 					tot_reshape = 0.0;
	double 					tot_split 	= 0.0;
	double 					tot_super 	= 0.0;
	double 					tot_exact 	= 0.0;

	uint32_t 				error_rand 		= 0;
	uint32_t 				error_greedy 	= 0;
	uint32_t 				error_reshape 	= 0;
	uint32_t 				error_split 	= 0;

	seed = time(NULL);
	// seed = 1494489756;

	signal(SIGINT, (__sighandler_t)interrrupt_handler);

	for ( ; !stop; ){
		srand(seed);
		log_debug_m("Seed is equal to %ld", seed);
		seed ++;

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
		#define test_method(name) 																											\
		for (i = 0; i < nb_category; i++){ 																									\
			desc[i].choice = 0xffffffff; 																									\
		} 																																	\
		if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start_time)){ 																			\
			log_err("clock_gettime fails"); 																								\
		} 																																	\
		if (arrayMinCoverage_ ## name ## _wrapper(&array, nb_category, desc, compare_uint32_t, &score_ ## name)){ 							\
			log_err("arrayMinCoverage_" #name " returned an stop"); 																		\
			stop = 1; 																														\
		} 																																	\
		else{ 																																\
			if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &stop_time)){ 																		\
				log_err("clock_gettime fails"); 																							\
			} 																																\
																																			\
			if (score_ ## name != arrayMinCoverage_eval(&array, nb_category, desc, compare_uint32_t)){ 										\
				log_err_m("unable to verify " #name " score, get %u", arrayMinCoverage_eval(&array, nb_category, desc, compare_uint32_t)); 	\
				stop = 1; 																													\
			} 																																\
																																			\
			exec_time = stop_time.tv_sec - start_time.tv_sec + (float)(stop_time.tv_nsec - start_time.tv_nsec) / 1000000000; 				\
			tot_ ## name += exec_time; 																										\
																																			\
			log_info_m("score of arrayMinCoverage_" #name " is %u (runtime: %.3fs)", score_ ## name, exec_time); 							\
			desc_buffer_print_choice(nb_category, desc); 																					\
		}

		test_method(rand)
		test_method(greedy)
		test_method(reshape)
		test_method(split)
		test_method(super)
		test_method(exact)

		/* CHECK */
		if (score_rand < score_exact){
			log_err("something is wrong with the exact score");
			stop = 1;
		}
		else{
			error_rand += score_rand - score_exact;
		}

		if (score_greedy < score_exact){
			log_err("something is wrong with the exact score");
			stop = 1;
		}
		else{
			error_greedy += score_greedy - score_exact;
		}

		if (score_reshape < score_exact){
			log_err("something is wrong with the exact score");
			stop = 1;
		}
		else{
			error_reshape += score_reshape - score_exact;
		}

		if (score_split < score_exact){
			log_err("something is wrong with the exact score");
			stop = 1;
		}
		else{
			error_split += score_split - score_exact;
		}

		if (score_exact != score_super){
			log_err("something is wrong with the super score");
			stop = 1;
		}

		/* CLEAN */
		for (i = 0; i < array_get_length(&array); i++){
			array_delete(*(struct array**)array_get(&array, i));
		}
		array_clean(&array);

		free(desc);
	}

	log_info_m("Rand: %f s, Greedy: %f s, Reshape: %f s, Split: %f s, Super: %f s, Exact: %f s", tot_rand, tot_greedy, tot_reshape, tot_split, tot_super, tot_exact);
	log_info_m("Error rand: %u, Error greedy: %u, Error reshape: %u, Error split: %u", error_rand, error_greedy, error_reshape, error_split);

	return EXIT_SUCCESS;
}
