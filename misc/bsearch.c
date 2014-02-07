#include "bsearch.h"

void* bsearch_r(const void* key, const void* base, size_t num, size_t size, __compar_d_fn_t compare, void* arg){
	size_t 		low = 0;
	size_t 		up = num;
	size_t 		idx;
	const void 	*p;
	int 		comparison;

	while (low < up){
		idx = (low + up) / 2;
		p = (void *) (((const char *) base) + (idx * size));
		comparison = (*compare)(key, p, arg);
		if (comparison < 0){
			up = idx;
		}
		else if (comparison > 0){
			low = idx + 1;
		}
		else{
			return (void*)p;
		}
	}

	return NULL;
}