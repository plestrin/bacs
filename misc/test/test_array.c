#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#include "../array.h"

#define OBJECT_SIZE 		88
#define NB_OBJECT 			36523
#define ARRAY2_INITAL_FILL 	819
#define COPY_OFFSET 		46
#define COPY_LENGTH 		903
#define ARRAY2_FINAL_FILL 	456
#define NB_QUERY 			512001238

int main(){
	struct array 	array1;
	struct array 	array2;
	char 			obj[OBJECT_SIZE];
	uint32_t 		i;
	char* 			fetch;
	struct timespec start_time;
	struct timespec stop_time;
	double 			duration;

	if (array_init(&array1, OBJECT_SIZE)){
		printf("ERROR: in %s, unable to init array1\n", __func__);
		return 0;
	}

	if (array_init(&array2, OBJECT_SIZE)){
		printf("ERROR: in %s, unable to init array2\n", __func__);
		return 0;
	}

	printf("*** TEST 1 ***\n");
	printf("Object size: %u\n", OBJECT_SIZE);
	printf("Nb object:   %u\n", NB_OBJECT);
	printf("Memory used: %u\n", OBJECT_SIZE * NB_OBJECT);
	printf("Alloc size:  %u\n", ARRAY_DEFAULT_ALLOC_SIZE);
	printf("Page size:   %u\n", ARRAY_DEFAULT_PAGE_SIZE);
	printf("Page used:   %u\n", (OBJECT_SIZE * NB_OBJECT) / ARRAY_DEFAULT_PAGE_SIZE);
	printf("Buffer used: %u\n", (OBJECT_SIZE * NB_OBJECT) % ARRAY_DEFAULT_PAGE_SIZE);

	for (i = 0; i < NB_OBJECT; i++){
		memset(obj, i, OBJECT_SIZE);

		if (array_add(&array1, obj) != (int32_t)i){
			printf("ERROR: in %s, unable to add obj %u\n", __func__, i);
			break;
		}
	}

	for (i = NB_OBJECT; i > 0; i--){
		fetch = (char*)array_get(&array1, i-1);

		memset(obj, i - 1, OBJECT_SIZE);
		if (memcmp(obj, fetch, OBJECT_SIZE) != 0){
			printf("ERROR: in %s, fetched object differs from original object %u\n", __func__, i);
		}
	}

	printf("\n*** TEST 2 ***\n");
	printf("Init fill:   %u\n", ARRAY2_INITAL_FILL);
	printf("Copy offset: %u\n", COPY_OFFSET);
	printf("Copy length: %u\n", COPY_LENGTH);
	printf("Final fill:  %u\n", ARRAY2_FINAL_FILL);

	for (i = 0; i < ARRAY2_INITAL_FILL; i++){
		memset(obj, i, OBJECT_SIZE);

		if (array_add(&array2, obj) != (int32_t)i){
			printf("ERROR: in %s, unable to add obj %u\n", __func__, i);
			break;
		}
	}

	if (array_copy(&array1, &array2, COPY_OFFSET, COPY_LENGTH) != COPY_LENGTH){
		printf("ERROR: in %s, unable to copy arrays\n", __func__);
	}

	for (i = 0; i < ARRAY2_FINAL_FILL; i++){
		memset(obj, i, OBJECT_SIZE);

		if (array_add(&array2, obj) != (int32_t)i + COPY_LENGTH + ARRAY2_INITAL_FILL){
			printf("ERROR: in %s, unable to add obj %u\n", __func__, i);
			break;
		}
	}

	for (i = 0; i < array_get_length(&array2); i++){
		fetch = (char*)array_get(&array2, i);

		if (i < ARRAY2_INITAL_FILL){
			memset(obj, i, OBJECT_SIZE);
			if (memcmp(obj, fetch, OBJECT_SIZE) != 0){
				printf("ERROR: in %s, fetched object differs from original object %u\n", __func__, i);
			}
		}
		else if (i < ARRAY2_INITAL_FILL + COPY_LENGTH){
			memset(obj, i + COPY_OFFSET - ARRAY2_INITAL_FILL, OBJECT_SIZE);
			if (memcmp(obj, fetch, OBJECT_SIZE) != 0){
				printf("ERROR: in %s, fetched object differs from original object %u\n", __func__, i);
			}
		}
		else{
			memset(obj, i - (ARRAY2_INITAL_FILL + COPY_LENGTH), OBJECT_SIZE);
			if (memcmp(obj, fetch, OBJECT_SIZE) != 0){
				printf("ERROR: in %s, fetched object differs from original object %u\n", __func__, i);
			}
		}
	}

	printf("\n*** TEST 3 ***\n");
	printf("Nb query: %u\n", NB_QUERY);

	if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start_time)){
		printf("ERROR: in %s, clock_gettime fails\n", __func__);
	}

	for (i = 0, fetch = 0; i < NB_QUERY; i++){
		fetch = (uint32_t)fetch + (char*)array_get(&array1, rand() % NB_OBJECT);
	}

	if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &stop_time)){
		printf("ERROR: in %s, clock_gettime fails\n", __func__);
	}

	duration = (stop_time.tv_sec - start_time.tv_sec) + (stop_time.tv_nsec - start_time.tv_nsec) / 1000000000.;

	printf("Execution time: %.3f s\n", duration);

	array_clean(&array1);
	array_clean(&array2);

	return 0;
}