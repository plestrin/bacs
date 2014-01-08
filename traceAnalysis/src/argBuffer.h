#ifndef ARGBUFFER_H
#define ARGBUFFER_H

#include <stdint.h>

#include "array.h"
#include "address.h"

#define ARGBUFFER_TAG_LENGTH 				32

#define ARGBUFFER_FRAGMENT_MAX_NB_ELEMENT 	9

#define ARGBUFFER_ACCESS_SIZE_UNDEFINED 	-1

struct argument{
	char 			tag[ARGBUFFER_TAG_LENGTH];
	struct array*	input;
	struct array*	output;
};

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
int32_t argBuffer_search(struct argBuffer* arg, char* buffer, uint32_t buffer_size);

int32_t argBuffer_clone_array(struct array* array_src, struct array* array_dst);
void argBuffer_delete_array(struct array* arg_array);

void argument_fragment_input(struct argument* argument, struct array* array);

#endif