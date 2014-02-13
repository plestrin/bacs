#ifndef IOCHECKER_H
#define IOCHECKER_H

#include <stdint.h>

#include "argSet.h"
#include "fastOutputSearch.h"
#include "array.h"
#include "workQueue.h"
#ifdef VERBOSE
#include "multiWorkPercent.h"
#endif

#define IOCHECKER_NB_THREAD 							4
#define IOCHECKER_MIN_SIZE_ACCELERATOR 					512

struct ioChecker{
	struct array 				reference_array;
	struct workQueue 			queue;
	#ifdef VERBOSE
	struct multiWorkPercent		multi_percent;
	#endif
};

struct checkJob{
	struct ioChecker* 			checker;
	uint32_t 					primitive_index;
	struct argSet* 				arg_set;
	struct fastOutputSearch* 	accelerator;
	#ifdef VERBOSE
	struct multiWorkPercent*	multi_percent;
	#endif
};

struct ioChecker* ioChecker_create();
int32_t ioChecker_init(struct ioChecker* checker);

void ioChecker_load(struct ioChecker* checker, void* arg);

int32_t ioChecker_submit_argSet(struct ioChecker* checker, struct argSet* arg_set);

static inline void ioChecker_start(struct ioChecker* checker){
	workQueue_start(&(checker->queue));
}

static inline void ioChecker_check(struct ioChecker* checker){
	workQueue_start(&(checker->queue));
	workQueue_wait(&(checker->queue));
}

static inline void ioChecker_wait(struct ioChecker* checker){
	workQueue_wait(&(checker->queue));
}

void ioChecker_print(struct ioChecker* checker);

void ioChecker_empty(struct ioChecker* checker);

void ioChecker_clean(struct ioChecker* checker);
void ioChecker_delete(struct ioChecker* checker);

#endif