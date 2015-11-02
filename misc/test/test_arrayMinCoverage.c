#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "../arrayMinCoverage.h"
#include "../base.h"

#define NB_CATEGORY 				24
#define MAX_ARRAY_PER_CATEGORY 		4
#define MAX_ELEMENT_PER_ARRAY		32
#define MAX_ELEMENT_RANGE 			0x000001ff

static int32_t compare_uint32_t(void* data1, void* data2){
	uint32_t n1 = *(uint32_t*)data1;
	uint32_t n2 = *(uint32_t*)data2;

	if (n1 < n2){
		return -1;
	}
	else if(n1 > n2){
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

int32_t main(void){
	uint32_t 				nb_category;
	uint32_t 				i;
	uint32_t 				j;
	struct categoryDesc* 	desc;
	struct array 			array;
	struct array* 			sub_array;
	uint32_t 				nb_element;
	uint32_t 				score;

	srand(time(NULL));
	
	/* INIT */
	nb_category = 1 + (rand() % (NB_CATEGORY - 1));
	desc = (struct categoryDesc*)malloc(sizeof(struct categoryDesc) * nb_category);
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
	if (arrayMinCoverage_rand(&array, nb_category, desc, compare_uint32_t, &score)){
		log_err("arrayMinCoverage_greedy returned an error");
	}
	else{
		log_info_m("score of arrayMinCoverage_rand is %u", score);
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
	if (arrayMinCoverage_greedy(&array, nb_category, desc, compare_uint32_t, &score)){
		log_err("arrayMinCoverage_greedy returned an error");
	}
	else{
		log_info_m("score of arrayMinCoverage_greedy is %u", score);
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
	if (arrayMinCoverage_split(&array, nb_category, desc, compare_uint32_t)){
		log_err("arrayMinCoverage_split returned an error");
	}
	else{
		score = arrayMinCoverage_eval(&array, nb_category, desc, compare_uint32_t);
		log_info_m("score of arrayMinCoverage_split is %u", score);
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
	if (arrayMinCoverage_exact(&array, nb_category, desc, compare_uint32_t, &score)){
		log_err("arrayMinCoverage_exact returned an error");
	}
	else{
		log_info_m("score of arrayMinCoverage_exact is %u", score);
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

	/* CLEAN */
	for (i = 0; i < array_get_length(&array); i++){
		array_delete(*(struct array**)array_get(&array, i));
	}
	array_clean(&array);

	free(desc);

	return EXIT_SUCCESS;
}