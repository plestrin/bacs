#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "fastOutputSearch.h"
#include "argBuffer.h"
#include "bsearch.h"

int32_t fastOutputSearch_value_qsort(struct outputEntry* entry1, struct outputEntry* entry2, struct outputMapping* mapping);
int32_t fastOutputSearch_value_bsearch(char* key, struct outputEntry* entry, struct outputMapping* mapping);
int32_t fastOutputSearch_size_qsort(struct argBuffer* arg1, struct argBuffer* arg2);

void fastOutputSearch_clean_(struct fastOutputSearch* search); /* No reference counting for this routine */


struct fastOutputSearch* fastOutputSearch_create(struct array* array, uint32_t* output_length, uint32_t nb_output_length){
	struct fastOutputSearch* search;

	search = (struct fastOutputSearch*)malloc(sizeof(struct fastOutputSearch));
	if (search != NULL){
		if (fastOutputSearch_init(search, array, output_length, nb_output_length)){
			printf("ERROR: in %s, unable to init fastOutputSearch\n", __func__);
			free(search);
			search = NULL;
		}
	}
	else{
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
	}

	return search;
}

int32_t fastOutputSearch_init(struct fastOutputSearch* search, struct array* array, uint32_t* output_length, uint32_t nb_output_length){
	uint32_t 			i;
	uint32_t 			j;
	uint32_t 			k;
	uint32_t 			l;
	uint32_t 			nb_entries;
	struct argBuffer* 	output;
	uint8_t 			valid_length;

	search->array 			= array;
	search->reference_count = 1;
	if (pthread_mutex_init(&(search->reference_protector), NULL)){
		printf("ERROR: in %s, unable to init mutex\n", __func__);
	}

	search->decrease_length = array_create_mapping(array, (int32_t(*)(void*,void*))fastOutputSearch_size_qsort);
	if (search->decrease_length == NULL){
		printf("ERROR: in %s, unable to create array mapping\n", __func__);
	}

	for (l = 0; l < FASTOUTPUTSEARCH_MAX_OUTPUT_LENGTH; l++){
		for (i = 0, valid_length = 0; i < nb_output_length; i++){
			if (l + 1 == output_length[i]){
				valid_length = 1;
				break;
			}
		}

		if (valid_length){
			for (i = 0, nb_entries = 0; i < array_get_length(array); i++){
				output = (struct argBuffer*)array_get(array, i);
				if (output->size >= l + 1){
					for (j = 0; j <= output->size - (l + 1); j += output->access_size){
						nb_entries ++;
					}
				}
			}

			#ifdef VERBOSE
			printf("\tCreating %u bit(s) accelerator: %u entrie(s)\n", (l+1)*8, nb_entries);
			#endif

			search->mapping[l].length 		= l + 1;
			search->mapping[l].nb_entries 	= nb_entries;
			search->mapping[l].search 		= search;
			search->mapping[l].entries 		= (struct outputEntry*)malloc(sizeof(struct outputEntry) * nb_entries);
			if (search->mapping[l].entries == NULL){
				printf("ERROR: in %s, unable to allocate memory\n", __func__);
				return -1;
			}

			for (i = 0, k = 0; i < array_get_length(array); i++){
				output = (struct argBuffer*)array_get(array, i);
				if (output->size >= l + 1){
					for (j = 0; j <= output->size - (l + 1); j += output->access_size){
						search->mapping[l].entries[k].argBuffer_index = i;
						search->mapping[l].entries[k].argBuffer_offset = j;
						k ++;
					}
				}
			}

			qsort_r(search->mapping[l].entries, search->mapping[l].nb_entries, sizeof(struct outputEntry), (__compar_d_fn_t)fastOutputSearch_value_qsort, &(search->mapping[l]));
		}
		else{
			search->mapping[l].length 		= l + 1;
			search->mapping[l].nb_entries 	= 0;
			search->mapping[l].entries 		= NULL;
			search->mapping[l].search 		= search;
		}
	}

	return 0;
}

void fastOutputSearch_incr_ref(struct fastOutputSearch* search){
	if (pthread_mutex_lock(&(search->reference_protector))){
		printf("ERROR: in %s, unable to lock mutex\n", __func__);
	}

	search->reference_count ++;

	if (pthread_mutex_unlock(&(search->reference_protector))){
		printf("ERROR: in %s, unable to unlock mutex\n", __func__);
	}
}

int32_t fastOutputSearch_search(struct fastOutputSearch* search, char* output, uint32_t output_length){
	int32_t 			result = -1;
	uint32_t 			i;
	struct argBuffer* 	arg;

	if (output_length != 0 && output_length <= FASTOUTPUTSEARCH_MAX_OUTPUT_LENGTH && search->mapping[output_length - 1].entries != NULL){
		if (bsearch_r(output, search->mapping[output_length - 1].entries, search->mapping[output_length - 1].nb_entries, sizeof(struct outputEntry), (__compar_d_fn_t)fastOutputSearch_value_bsearch, &(search->mapping[output_length - 1])) != NULL){
			result = 0;
		}
	}
	else{
		for (i = 0; i < array_get_length(search->array); i++){
			arg = (struct argBuffer*)array_get(search->array, search->decrease_length[i]);
			if (arg->size >= output_length){
				if (argBuffer_search(arg, output, output_length) >= 0){
					result = 0;
					break;
				}
			}
			else{
				break;
			}
		}
	}

	return result;
}

void fastOutputSearch_clean(struct fastOutputSearch* search){
	uint32_t reference_count;

	if (pthread_mutex_lock(&(search->reference_protector))){
		printf("ERROR: in %s, unable to lock mutex\n", __func__);
	}

	search->reference_count --;
	reference_count = search->reference_count;

	if (pthread_mutex_unlock(&(search->reference_protector))){
		printf("ERROR: in %s, unable to unlock mutex\n", __func__);
	}
	if (reference_count == 0){
		fastOutputSearch_clean_(search);
	}
}

void fastOutputSearch_clean_(struct fastOutputSearch* search){
	uint32_t i;

	for (i = 0; i < FASTOUTPUTSEARCH_MAX_OUTPUT_LENGTH; i++){
		if (search->mapping[i].entries != NULL){
			free(search->mapping[i].entries);
		}
	}

	if (search->decrease_length != NULL){
		free(search->decrease_length);
	}

	pthread_mutex_destroy(&(search->reference_protector));
}

void fastOutputSearch_delete(struct fastOutputSearch* search){
	uint32_t reference_count;

	if (search != NULL){
		if (pthread_mutex_lock(&(search->reference_protector))){
			printf("ERROR: in %s, unable to lock mutex\n", __func__);
		}

		search->reference_count --;
		reference_count = search->reference_count;

		if (pthread_mutex_unlock(&(search->reference_protector))){
			printf("ERROR: in %s, unable to unlock mutex\n", __func__);
		}

		if (reference_count == 0){
			fastOutputSearch_clean_(search);
			free(search);
		}
	}
}

/* ===================================================================== */
/* Sorting routines						                                 */
/* ===================================================================== */

int32_t fastOutputSearch_value_qsort(struct outputEntry* entry1, struct outputEntry* entry2, struct outputMapping* mapping){
	struct argBuffer* arg1 			= (struct argBuffer*)array_get(mapping->search->array, entry1->argBuffer_index);
	struct argBuffer* arg2 			= (struct argBuffer*)array_get(mapping->search->array, entry2->argBuffer_index);

	return memcmp(arg1->data + entry1->argBuffer_offset, arg2->data + entry2->argBuffer_offset, mapping->length);
}

int32_t fastOutputSearch_value_bsearch(char* key, struct outputEntry* entry, struct outputMapping* mapping){
	struct argBuffer* arg 			= (struct argBuffer*)array_get(mapping->search->array, entry->argBuffer_index);

	return memcmp(key, arg->data + entry->argBuffer_offset, mapping->length);
}

int32_t fastOutputSearch_size_qsort(struct argBuffer* arg1, struct argBuffer* arg2){
	return arg2->size - arg1->size;
}