#ifndef DBSCAN_H
#define DBSCAN_H

#include <stdint.h>

#include "array.h"

typedef int32_t(*dbscan_norme)(void*,void*);


struct array* dbscan_cluster(uint32_t nb_element, uint32_t element_size, void* elements, dbscan_norme norme);

#endif