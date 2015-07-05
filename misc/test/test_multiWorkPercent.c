#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "../workQueue.h"
#include "../multiWorkPercent.h"
#include "../base.h"

#define NB_THREAD 5
#define NB_SUBMIT 14

struct counterArg{
	uint32_t 					value;
	struct multiWorkPercent*  	multi_percent;
};

void counter(void* arg);

int main(){
	struct workQueue 			queue;
	struct multiWorkPercent 	multi_percent;
	uint32_t 					i;
	struct counterArg 			args[NB_SUBMIT];


	if (workQueue_init(&queue, NB_THREAD)){
		log_err("unable to init workQueue");
		return 0;
	}

	if (multiWorkPercent_init(&multi_percent, NB_THREAD, WORKPERCENT_ACCURACY_1)){
		log_err("unable to init multiWorkPercent");
		return 0;
	}

	workQueue_start(&queue);

	for (i = 0; i  < NB_SUBMIT; i++){
		args[i].value = rand() & 0x0fffffff;
		args[i].multi_percent = &multi_percent;
		if (workQueue_submit(&queue, counter, args + i)){
			log_err("unable to submit job to workQueue");
		}
	}

	workQueue_wait(&queue);

	workQueue_clean(&queue);
	multiWorkPercent_clean(&multi_percent);

	return 0;
}

void counter(void* arg){
	struct counterArg* 	counter_arg = (struct counterArg*)arg;
	uint32_t 			i;
	uint32_t 			val = 0;
	uint32_t 			index = multiWorkPercent_get_thread_index(counter_arg->multi_percent);

	multiWorkPercent_start(counter_arg->multi_percent, index, counter_arg->value);
	for (i = 0; i < counter_arg->value; i++){
		if (i & 0x00000001){
			val = val + 1;
		}
		multiWorkPercent_notify(counter_arg->multi_percent, index, 1);
	}
	multiWorkPercent_conclude(counter_arg->multi_percent, index);
}