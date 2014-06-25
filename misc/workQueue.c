#include <stdlib.h>
#include <stdio.h>

#include "workQueue.h"

void* workQueue_thread_exe(void* arg);


struct workQueue* workQueue_create(uint32_t nb_thread){
	struct workQueue* queue;

	queue = (struct workQueue*)malloc(sizeof(struct workQueue));
	if (queue != NULL){
		if (workQueue_init(queue, nb_thread)){
			free(queue);
			queue = NULL;
		}
	}
	else{
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
	}

	return queue;
}

int32_t workQueue_init(struct workQueue* queue, uint32_t nb_thread){
	queue->threads = (pthread_t*)malloc(sizeof(pthread_t) * nb_thread);
	if (queue->threads == NULL){
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
		return -1;
	}

	queue->nb_thread = nb_thread;
	queue->job_head = NULL;
	queue->job_tail = NULL;
	queue->exit_flag = 0;

	if (sem_init(&(queue->thread_waiter), 0, 1)){
		printf("ERROR: in %s, unable to init semaphore\n", __func__);
	}
	if (pthread_mutex_init(&(queue->queue_protector), NULL)){
		printf("ERROR: in %s, unable to init mutex\n", __func__);
	}

	return 0;
}

void workQueue_start(struct workQueue* queue){
	uint32_t i;

	if (sem_wait(&(queue->thread_waiter))){
		printf("ERROR: in %s, unable to lock semaphore\n", __func__);
	}

	for (i = 0; i < queue->nb_thread; i++){
		if (pthread_create(queue->threads + i, NULL, workQueue_thread_exe, (void*)queue)){
			printf("ERROR: in %s, unable to create thread %u\n", __func__, i);
		}
	}
}

void workQueue_wait(struct workQueue* queue){
	uint32_t i;

	if (!pthread_mutex_lock(&(queue->queue_protector))){
		queue->exit_flag = 1;
		if (pthread_mutex_unlock(&(queue->queue_protector))){
			printf("ERROR: in %s, unable to unlock mutex\n", __func__);
		}
		if (sem_post(&(queue->thread_waiter))){
			printf("ERROR: in %s, unable to unlock mutex\n", __func__);
		}
	}
	else{
		printf("ERROR: in %s, unable to lock mutex\n", __func__);
	}

	for (i = 0; i < queue->nb_thread; i++){
		pthread_join (queue->threads[i], NULL);
	}
}

int32_t workQueue_submit(struct workQueue* queue, void(*routine)(void*), void* arg){
	struct job* job;

	job = (struct job*)malloc(sizeof(struct job));
	if (job == NULL){
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
		return -1;
	}

	job->arg = arg;
	job->routine = routine;
	job->prev = NULL;

	if (!pthread_mutex_lock(&(queue->queue_protector))){
		job->next = queue->job_tail;
		if (queue->job_tail != NULL){
			queue->job_tail->prev = job;
		}
		else{
			queue->job_head = job;
		}
		queue->job_tail = job;

		if (pthread_mutex_unlock(&(queue->queue_protector))){
			printf("ERROR: in %s, unable to unlock mutex\n", __func__);
		}
		if (sem_post(&(queue->thread_waiter))){
			printf("ERROR: in %s, unable to unlock sempha\n", __func__);
		}
	}
	else{
		printf("ERROR: in %s, unable to lock mutex\n", __func__);
		free(job);
	}

	return 0;
}

void* workQueue_thread_exe(void* thread_arg){
	struct workQueue* 	queue = (struct workQueue*)thread_arg;
	void* 				arg;
	void(*routine)(void*);
	uint8_t 			continue_flag = 1;
	struct job* 		job;

	while (continue_flag){
		if (!pthread_mutex_lock(&(queue->queue_protector))){
			if (queue->job_head != NULL){
				job = queue->job_head;

				arg = job->arg;
				routine = job->routine;

				queue->job_head = job->prev;
				if (job->prev != NULL){
					job->prev->next = NULL;
				}
				else{
					queue->job_tail = NULL;
				}

				if (pthread_mutex_unlock(&(queue->queue_protector))){
					printf("ERROR: in %s, unable to unlock mutex\n", __func__);
				}

				free(job);

				routine(arg);
			}
			else{
				if (queue->exit_flag){
					continue_flag = 0;
					if (pthread_mutex_unlock(&(queue->queue_protector))){
						printf("ERROR: in %s, unable to unlock mutex\n", __func__);
					}
				}
				else{
					if (pthread_mutex_unlock(&(queue->queue_protector))){
						printf("ERROR: in %s, unable to unlock mutex\n", __func__);
					}
					if (sem_wait(&(queue->thread_waiter))){
						printf("ERROR: in %s, unable to lock mutex\n", __func__);
					}
				}
			}
		}
		else{
			printf("ERROR: in %s, unable to lock mutex\n", __func__);
		}
	}

	if (sem_post(&(queue->thread_waiter))){
		printf("ERROR: in %s, unable to unlock mutex\n", __func__);
	}


	return NULL;
}

void workQueue_clean(struct workQueue* queue){
	free(queue->threads);

	if (queue->job_head != NULL || queue->job_tail != NULL){
		printf("ERROR: in %s, the workQueue does not seem empty - impossible to clean\n", __func__);
	}

	pthread_mutex_destroy(&(queue->queue_protector));
	sem_destroy(&(queue->thread_waiter));
}

void workQueue_delete(struct workQueue* queue){
	if (queue != NULL){
		workQueue_clean(queue);
		free(queue);
	}
}