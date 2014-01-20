#ifndef ARGBUFFER_H
#define ARGBUFFER_H

#include <stdint.h>

#include "array.h"
#include "address.h"

#define ARGBUFFER_FRAGMENT_MAX_NB_ELEMENT 	9
#define ARGBUFFER_ACCESS_SIZE_UNDEFINED 	-1

enum argLocationType{
	ARG_LOCATION_MEMORY,
	ARG_LOCATION_REGISTER
};

struct argBuffer{
	enum argLocationType 	location_type;
	union {
		ADDRESS 			address;
		uint16_t 			reg;
	} 						location;
	uint32_t 				size;
	int8_t					access_size;
	char* 					data;
};

void argBuffer_print_raw(struct argBuffer* arg);
int32_t argBuffer_clone(struct argBuffer* arg_src, struct argBuffer* arg_dst);
int32_t argBuffer_equal(struct argBuffer* arg1, struct argBuffer* arg2);
int32_t argBuffer_search(struct argBuffer* arg, char* buffer, uint32_t buffer_size);
struct argBuffer* argBuffer_compare(struct argBuffer* arg1, struct argBuffer* arg2);
int32_t argBuffer_try_merge(struct argBuffer* arg1, struct argBuffer* arg2);
void argBuffer_create_fragment_table(struct argBuffer* arg, uint32_t** table_, uint32_t* nb_element_); /* à l'occasion on peut supprimer c'est lourd pour pas grand chose */
void argBuffer_delete(struct argBuffer* arg);

int32_t argBuffer_clone_array(struct array* array_src, struct array* array_dst);
void argBuffer_delete_array(struct array* arg_array);

#endif