#include <stdlib.h>
#include <stdio.h>

#include "workPercent.h"


struct workPercent* workPercent_create(char* line, enum workPercent_accuracy accuracy, uint64_t nb_unit){
	struct workPercent* work;

	work = (struct workPercent*)malloc(sizeof(struct workPercent));
	if (work != NULL){
		if (workPercent_init(work, line, accuracy, nb_unit)){
			free(work);
			work = NULL;
		}
	}

	return work;
}

int32_t workPercent_init(struct workPercent* work ,char* line, enum workPercent_accuracy accuracy, uint64_t nb_unit){
	work->line = line;
	work->accuracy = accuracy;
	work->nb_unit = nb_unit;
	work->counter = 0;
	work->step_counter = 0;

	return 0;
}

void workPercent_notify(struct workPercent* work, uint32_t value){
	uint32_t new_step;

	work->counter += value;
	if (work->counter > work->nb_unit){
		work->counter = work->nb_unit;
	}

	switch(work->accuracy){
	case WORKPERCENT_ACCURACY_0 : {
		new_step = (100 * work->counter) / work->nb_unit;
		if (new_step > work->step_counter){
			work->step_counter = new_step;
			printf("\r%s%u%% ...", work->line, work->step_counter);
			fflush(stdout);
		}
		break;
	}
	case WORKPERCENT_ACCURACY_1 : {
		new_step = (1000 * work->counter) / work->nb_unit;
		if (new_step > work->step_counter){
			work->step_counter = new_step;
			printf("\r%s%.1f%% ...", work->line, work->step_counter / 10.);
			fflush(stdout);
		}
		break;
	}
	case WORKPERCENT_ACCURACY_2	: {
		new_step = (10000 * work->counter) / work->nb_unit;
		if (new_step > work->step_counter){
			work->step_counter = new_step;
			printf("\r%s%.2f%% ...", work->line, work->step_counter / 100.);
			fflush(stdout);
		}
		break;
	}
	}	
}

void workPercent_delete(struct workPercent* work){
	if (work != NULL){
		free(work);
	}
}