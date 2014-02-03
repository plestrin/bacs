#ifndef MULTIWORKPERCENT_H
#define MULTIWORKPERCENT_H

#include <stdint.h>
#include <pthread.h>

#include "workPercent.h"
#include "multiColumn.h"

#define MULTIWORKPERCENT_COLUMN_SIZE 16

struct multiWorkPercent{
	uint32_t 					nb_job;
	struct workPercent* 		work_percent;
	struct multiColumnPrinter* 	printer;
	char* 						values;
	pthread_t* 					thread_id;
	pthread_mutex_t 			sync;
};

struct multiWorkPercent* multiWorkPercent_create(uint32_t nb_job, enum workPercent_accuracy accuracy);
int32_t multiWorkPercent_init(struct multiWorkPercent* multi_percent, uint32_t nb_job, enum workPercent_accuracy accuracy);

uint32_t multiWorkPercent_get_thread_index();

void multiWorkPercent_start(struct multiWorkPercent* multi_percent, uint32_t job_index, uint64_t nb_unit);
void multiWorkPercent_notify(struct multiWorkPercent* multi_percent, uint32_t job_index, uint32_t value);
void multiWorkPercent_conclude(struct multiWorkPercent* multi_percent, uint32_t job_index);

void multiWorkPercent_clean(struct multiWorkPercent* multi_percent);
void multiWorkPercent_delete(struct multiWorkPercent* multi_percent);


#endif