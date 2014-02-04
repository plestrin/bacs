#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "../workQueue.h"

#define NB_THREAD 5
#define NB_SUBMIT 14

void counter(void* arg);

int main(){
	struct workQueue 	queue;
	uint32_t 			i;
	uint32_t* 			values;

	if (workQueue_init(&queue, NB_THREAD)){
		printf("ERROR: in %s, unable to init workQueue\n", __func__);
		return 0;
	}

	workQueue_start(&queue);

	values = (uint32_t*)malloc(NB_SUBMIT * sizeof(uint32_t));
	if (values != NULL){
		for (i = 0; i  < NB_SUBMIT; i++){
			values[i] = rand() & 0x1fffffff;
			if (workQueue_submit(&queue, counter, values + i)){
				printf("ERROR: in %s, unable to submit job to workQueue\n", __func__);
			}
		}
	}
	else{
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
	}

	workQueue_wait(&queue);

	workQueue_clean(&queue);

	if (values != NULL){
		free(values);
	}

	return 0;
}

void counter(void* arg){
	uint32_t n = *(uint32_t*)arg;
	uint32_t i;
	uint32_t val = 0;

	for (i = 0; i < n; i++){
		if (i & 0x00000001){
			val ++;
		}
	}

	printf("Counter has reached value: %u\n", val);
}