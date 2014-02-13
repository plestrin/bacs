#ifndef WORKQUEUE_H
#define WORKQUEUE_H

#include <stdint.h>
#include <pthread.h>
#include <semaphore.h>

struct job{
	void* 		arg;
	void(*routine)(void*);
	struct job* prev;
	struct job* next;
};

struct workQueue{
	uint32_t 			nb_thread;
	pthread_t*			threads;
	struct job* 		job_head;
	struct job* 		job_tail;
	uint8_t 			exit_flag;
	sem_t 				thread_waiter;
	pthread_mutex_t 	queue_protector;
};

struct workQueue* workQueue_create(uint32_t nb_thread);
int32_t workQueue_init(struct workQueue* queue, uint32_t nb_thread);

void workQueue_start(struct workQueue* queue);

int32_t workQueue_submit(struct workQueue* queue, void(*routine)(void*), void* arg);

void workQueue_wait(struct workQueue* queue);

void workQueue_clean(struct workQueue* queue);
void workQueue_delete(struct workQueue* queue);


#endif