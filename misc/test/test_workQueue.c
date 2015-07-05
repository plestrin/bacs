#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "../workQueue.h"
#include "../base.h"

#define NB_THREAD 5
#define NB_SUBMIT 14

void counter(void* arg);

int main(){
	struct workQueue 	queue;
	uint32_t 			i;
	uint32_t* 			values;

	if (workQueue_init(&queue, NB_THREAD)){
		log_err("unable to init workQueue");
		return 0;
	}

	workQueue_start(&queue);

	values = (uint32_t*)malloc(NB_SUBMIT * sizeof(uint32_t));
	if (values != NULL){
		for (i = 0; i  < NB_SUBMIT; i++){
			values[i] = rand() & 0x1fffffff;
			if (workQueue_submit(&queue, counter, values + i)){
				log_err("unable to submit job to workQueue");
			}
		}
	}
	else{
		log_err("unable to allocate memory");
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

	log_info_m("counter has reached value: %u", val);
}