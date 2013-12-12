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

	return 0;
}

void workPercent_notify(struct workPercent* work, uint32_t value){
	work->counter += value;
	if (work->counter > work->nb_unit){
		work->counter = work->nb_unit;
	}

	switch(work->accuracy){
	case WORKPERCENT_ACCURACY_0 : {
		if ((100 * work->counter) % work->nb_unit == 0){
			printf("\r%s%u%% ...", work->line, (uint32_t)((100 * work->counter) / work->nb_unit));
			fflush(stdout);
		}
		break;
	}
	case WORKPERCENT_ACCURACY_1 : {
		if ((1000 * work->counter) % work->nb_unit == 0){
			printf("\r%s%.1f%% ...", work->line, (uint32_t)((1000 * work->counter) / work->nb_unit) / 10.);
			fflush(stdout);
		}
		break;
	}
	case WORKPERCENT_ACCURACY_2	: {
		if ((10000 * work->counter) % work->nb_unit == 0){
			printf("\r%s%.2f%% ...", work->line, (uint32_t)((10000 * work->counter) / work->nb_unit) / 100.);
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