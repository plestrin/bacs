#ifndef ARGBUFFER_H
#define ARGBUFFER_H

#include <stdint.h>

#include "array.h"
#include "address.h"

#define ARGBUFFER_TAG_LENGTH 32

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
	char* 					data;
};

void argBuffer_print_raw(struct argBuffer* arg);

void argBuffer_delete_array(struct array* arg_array);



#endif