#ifndef IOCHECKER_H
#define IOCHECKER_H

#include <stdint.h>

#include "array.h"

struct ioChecker{
	struct array 	reference_array;
	uint8_t 		max_nb_input;
	/* other stuff later like thread pool, submission queue etc*/
};

struct ioChecker* ioChecker_create();
int32_t ioChecker_init(struct ioChecker* checker);
void ioChecker_submit_argBuffers(struct ioChecker* checker, struct array* input_args, struct array* output_args);
void ioChecker_print(struct ioChecker* checker);

void ioChecker_handmade_test(struct ioChecker* checker); /* This is a debuging routine */

void ioChecker_clean(struct ioChecker* checker);
void ioChecker_delete(struct ioChecker* checker);

#endif