#ifndef IOCHECKER_H
#define IOCHECKER_H

#include "array.h"

struct ioChecker{
	struct array reference_array;
	/* other stuff later like thread pool, submission queue etc*/
};

struct ioChecker* ioChecker_create();
int32_t ioChecker_init(struct ioChecker* checker);
void ioChecker_clean(struct ioChecker* checker);
void ioChecker_delete(struct ioChecker* checker);

#endif