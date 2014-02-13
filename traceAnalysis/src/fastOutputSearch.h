#ifndef FASTOUTPUTSEARCH_H
#define FASTOUTPUTSEARCH_H

#include <stdint.h>
#include <pthread.h>

#include "array.h"

#define FASTOUTPUTSEARCH_MAX_OUTPUT_LENGTH 64

struct outputEntry{
	uint32_t argBuffer_index;
	uint32_t argBuffer_offset;
};

struct outputMapping{
	uint32_t 					length;
	uint32_t 					nb_entries;
	struct outputEntry* 		entries;
	struct fastOutputSearch* 	search;
};

struct fastOutputSearch{
	struct array*  			array;
	struct outputMapping  	mapping[FASTOUTPUTSEARCH_MAX_OUTPUT_LENGTH];
	uint32_t* 				decrease_length;

	uint32_t 				reference_count;
	pthread_mutex_t 		reference_protector;
};

struct fastOutputSearch* fastOutputSearch_create(struct array* array, uint32_t* output_length, uint32_t nb_output_length);
int32_t fastOutputSearch_init(struct fastOutputSearch* search, struct array* array, uint32_t* output_length, uint32_t nb_output_length);
void fastOutputSearch_incr_ref(struct fastOutputSearch* search);

int32_t fastOutputSearch_search(struct fastOutputSearch* search, char* output, uint32_t output_length);

void fastOutputSearch_clean(struct fastOutputSearch* search);
void fastOutputSearch_delete(struct fastOutputSearch* search);



#endif