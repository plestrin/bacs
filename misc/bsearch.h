#ifndef BSEARCH_H
#define BSEARCH_H

#define _GNU_SOURCE

#include <stdlib.h>

void* bsearch_r(const void* key, const void* base, size_t num, size_t size, __compar_d_fn_t compare, void* arg);

#endif