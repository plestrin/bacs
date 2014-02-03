#include <stdlib.h>
#include <stdio.h>

#include "multiWorkPercent.h"

struct multiWorkPercent* multiWorkPercent_create(uint32_t nb_job, enum workPercent_accuracy accuracy){
	struct multiWorkPercent* multi_percent;

	multi_percent = (struct multiWorkPercent*)malloc(sizeof(struct multiWorkPercent));
	if (multi_percent != NULL){
		if (multiWorkPercent_init(multi_percent, nb_job, accuracy)){
			printf("ERROR: in %s, unable to init multiWorkPercent\n", __func__);
			free(multi_percent);
			multi_percent = NULL;
		}
	}

	return multi_percent;
}

int32_t multiWorkPercent_init(struct multiWorkPercent* multi_percent, uint32_t nb_job, enum workPercent_accuracy accuracy){
	uint32_t i;

	multi_percent->nb_job = nb_job;

	multi_percent->values = (char*)malloc(MULTIWORKPERCENT_COLUMN_SIZE * nb_job);
	if (multi_percent->values == NULL){
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
		return -1;
	}

	multi_percent->thread_id = (pthread_t*)malloc(sizeof(pthread_t) * nb_job);
	if (multi_percent->thread_id == NULL){
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
		free(multi_percent->values);
		return -1;
	}

	multi_percent->work_percent = (struct workPercent*)malloc(sizeof(struct workPercent) * nb_job);
	if (multi_percent->work_percent == NULL){
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
		free(multi_percent->values);
		free(multi_percent->thread_id);
		return -1;
	}

	multi_percent->printer = multiColumnPrinter_create(stdout, nb_job, NULL, NULL, NULL);
	if (multi_percent->printer == NULL){
		printf("ERROR: in %s, unable to create multiColumnPrinter\n", __func__);
		free(multi_percent->work_percent);
		free(multi_percent->values);
		free(multi_percent->thread_id);
		return -1;
	}

	for (i = 0; i < nb_job; i++){
		multi_percent->work_percent[i].line = NULL;
		multi_percent->work_percent[i].accuracy = accuracy;
		multi_percent->work_percent[i].counter = 0;
		multi_percent->work_percent[i].step_counter = 0;

		multiColumnPrinter_set_column_size(multi_percent->printer, i, MULTIWORKPERCENT_COLUMN_SIZE);

		snprintf(multi_percent->values + i*MULTIWORKPERCENT_COLUMN_SIZE, MULTIWORKPERCENT_COLUMN_SIZE, " - ");

		multi_percent->thread_id[i] = 0;
	}

	if (pthread_mutex_init(&(multi_percent->sync), NULL)){
		printf("ERROR: in %s, unable to init mutex\n", __func__);
	}

	return 0;
}

uint32_t multiWorkPercent_get_thread_index(struct multiWorkPercent* multi_percent){
	uint32_t i;
	pthread_t thread_id = pthread_self();


	if (!pthread_mutex_lock(&(multi_percent->sync))){
		for (i = 0; i < multi_percent->nb_job; i ++){
			if (multi_percent->thread_id[i] == thread_id){
				if (pthread_mutex_unlock(&(multi_percent->sync))){
					printf("ERROR: in %s, unable to unlock mutex\n", __func__);
				}
				return i;
			}
			else if (multi_percent->thread_id[i] == 0){
				multi_percent->thread_id[i] = thread_id;
				if (pthread_mutex_unlock(&(multi_percent->sync))){
					printf("ERROR: in %s, unable to unlock mutex\n", __func__);
				}
				return i;
			}
		}
	}
	else{
		printf("ERROR: in %s, unable to lock mutex\n", __func__);
	}

	printf("ERROR: in %s, unable to allocate index to the current thread -> return 0\n", __func__);

	return 0;
}

void multiWorkPercent_start(struct multiWorkPercent* multi_percent, uint32_t job_index, uint64_t nb_unit){
	if (job_index < multi_percent->nb_job){
		multi_percent->work_percent[job_index].nb_unit 	= nb_unit;
		multi_percent->work_percent[job_index].counter 			= 0;
		multi_percent->work_percent[job_index].step_counter 	= 0;

		if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &(multi_percent->work_percent[job_index].start_time))){
			printf("ERROR: in %s, clock_gettime fails\n", __func__);
		}

		snprintf(multi_percent->values + job_index*MULTIWORKPERCENT_COLUMN_SIZE, MULTIWORKPERCENT_COLUMN_SIZE, "0%%");
		if (!pthread_mutex_lock(&(multi_percent->sync))){
			multiColumnPrinter_print_string_line(multi_percent->printer, multi_percent->values, MULTIWORKPERCENT_COLUMN_SIZE);
			if (pthread_mutex_unlock(&(multi_percent->sync))){
				printf("ERROR: in %s, unable to unlock mutex\n", __func__);
			}
		}
		else{
			printf("ERROR: in %s, unable to lock mutex\n", __func__);
		}
	}
}

void multiWorkPercent_notify(struct multiWorkPercent* multi_percent, uint32_t job_index, uint32_t value){
	uint32_t new_step;

	if (job_index < multi_percent->nb_job){
		multi_percent->work_percent[job_index].counter += value;
		if (multi_percent->work_percent[job_index].counter > multi_percent->work_percent[job_index].nb_unit){
			multi_percent->work_percent[job_index].counter = multi_percent->work_percent[job_index].nb_unit;
		}

		switch(multi_percent->work_percent[job_index].accuracy){
		case WORKPERCENT_ACCURACY_0 : {
			new_step = (100 * multi_percent->work_percent[job_index].counter) / multi_percent->work_percent[job_index].nb_unit;
			if (new_step > multi_percent->work_percent[job_index].step_counter){
				multi_percent->work_percent[job_index].step_counter = new_step;
				snprintf(multi_percent->values + job_index*MULTIWORKPERCENT_COLUMN_SIZE, MULTIWORKPERCENT_COLUMN_SIZE, "%u%%", multi_percent->work_percent[job_index].step_counter);
				if (!pthread_mutex_lock(&(multi_percent->sync))){
					multiColumnPrinter_print_string_line(multi_percent->printer, multi_percent->values, MULTIWORKPERCENT_COLUMN_SIZE);
					if (pthread_mutex_unlock(&(multi_percent->sync))){
						printf("ERROR: in %s, unable to unlock mutex\n", __func__);
					}
				}
				else{
					printf("ERROR: in %s, unable to lock mutex\n", __func__);
				}
			}
			break;
		}
		case WORKPERCENT_ACCURACY_1 : {
			new_step = (1000 * multi_percent->work_percent[job_index].counter) / multi_percent->work_percent[job_index].nb_unit;
			if (new_step > multi_percent->work_percent[job_index].step_counter){
				multi_percent->work_percent[job_index].step_counter = new_step;
				snprintf(multi_percent->values + job_index*MULTIWORKPERCENT_COLUMN_SIZE, MULTIWORKPERCENT_COLUMN_SIZE, "%.1f%%", multi_percent->work_percent[job_index].step_counter / 10.);
				if (!pthread_mutex_lock(&(multi_percent->sync))){
					multiColumnPrinter_print_string_line(multi_percent->printer, multi_percent->values, MULTIWORKPERCENT_COLUMN_SIZE);
					if (pthread_mutex_unlock(&(multi_percent->sync))){
						printf("ERROR: in %s, unable to unlock mutex\n", __func__);
					}
				}
				else{
					printf("ERROR: in %s, unable to lock mutex\n", __func__);
				}
			}
			break;
		}
		case WORKPERCENT_ACCURACY_2	: {
			new_step = (10000 * multi_percent->work_percent[job_index].counter) / multi_percent->work_percent[job_index].nb_unit;
			if (new_step > multi_percent->work_percent[job_index].step_counter){
				multi_percent->work_percent[job_index].step_counter = new_step;
				snprintf(multi_percent->values + job_index*MULTIWORKPERCENT_COLUMN_SIZE, MULTIWORKPERCENT_COLUMN_SIZE, "%.2f%%", multi_percent->work_percent[job_index].step_counter / 100.);
				if (!pthread_mutex_lock(&(multi_percent->sync))){
					multiColumnPrinter_print_string_line(multi_percent->printer, multi_percent->values, MULTIWORKPERCENT_COLUMN_SIZE);
					if (pthread_mutex_unlock(&(multi_percent->sync))){
						printf("ERROR: in %s, unable to unlock mutex\n", __func__);
					}
				}
				else{
					printf("ERROR: in %s, unable to lock mutex\n", __func__);
				}
			}
			break;
		}
		}
	}
}

void multiWorkPercent_conclude(struct multiWorkPercent* multi_percent, uint32_t job_index){
	struct timespec stop_time;
	double duration;

	if (job_index < multi_percent->nb_job){
		if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &stop_time)){
			printf("ERROR: in %s, clock_gettime fails\n", __func__);
		}

		duration = (stop_time.tv_sec - multi_percent->work_percent[job_index].start_time.tv_sec) + (stop_time.tv_nsec - multi_percent->work_percent[job_index].start_time.tv_nsec) / 1000000000.;

		snprintf(multi_percent->values + job_index*MULTIWORKPERCENT_COLUMN_SIZE, MULTIWORKPERCENT_COLUMN_SIZE, "100%% - %.3f s", duration);
		if (!pthread_mutex_lock(&(multi_percent->sync))){
			multiColumnPrinter_print_string_line(multi_percent->printer, multi_percent->values, MULTIWORKPERCENT_COLUMN_SIZE);
			printf("\n");
			if (pthread_mutex_unlock(&(multi_percent->sync))){
				printf("ERROR: in %s, unable to unlock mutex\n", __func__);
			}
		}
		else{
			printf("ERROR: in %s, unable to lock mutex\n", __func__);
		}
	}
}

void multiWorkPercent_clean(struct multiWorkPercent* multi_percent){
	pthread_mutex_destroy(&(multi_percent->sync));

	free(multi_percent->values);
	free(multi_percent->work_percent);
	free(multi_percent->thread_id);

	multiColumnPrinter_delete(multi_percent->printer);
}

void multiWorkPercent_delete(struct multiWorkPercent* multi_percent){
	if (multi_percent != NULL){
		multiWorkPercent_clean(multi_percent);
		free(multi_percent);
	}
}