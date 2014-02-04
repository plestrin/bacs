#include <stdlib.h>
#include <stdio.h>

#include "../workPercent.h"

#define NB_WORK_UNIT 1000000009

int main(){
	struct workPercent 	work;
	uint32_t 			i;

	workPercent_init(&work, "Task performed: ", WORKPERCENT_ACCURACY_2, NB_WORK_UNIT);

	for (i = 0; i < NB_WORK_UNIT; i++){
		workPercent_notify(&work, 1);
	}

	workPercent_conclude(&work);

	return 0;
}