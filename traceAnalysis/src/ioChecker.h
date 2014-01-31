#ifndef IOCHECKER_H
#define IOCHECKER_H

#include <stdint.h>

#include "argSet.h"
#include "array.h"
#include "workQueue.h"

#define IOCHECKER_NB_THREAD 4

struct ioChecker{
	struct array 		reference_array;
	struct workQueue 	queue;
};

struct checkJob{
	struct ioChecker* 	checker;
	uint32_t 			primitive_index;
	struct argSet* 		arg_set;
};

struct ioChecker* ioChecker_create();
int32_t ioChecker_init(struct ioChecker* checker);

int32_t ioChecker_submit_argSet(struct ioChecker* checker, struct argSet* arg_set);

static inline void ioChecker_check(struct ioChecker* checker){
	workQueue_start(&(checker->queue));
	workQueue_wait(&(checker->queue));
}

void ioChecker_print(struct ioChecker* checker);

void ioChecker_clean(struct ioChecker* checker);
void ioChecker_delete(struct ioChecker* checker);

#endif