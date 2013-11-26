#ifndef ARGBUFFER_H
#define ARGBUFFER_H

#include <stdint.h>

enum argLocationType{
	ARG_LOCATION_MEMORY,
	ARG_LOCATION_REGISTER
};

struct argBuffer{
	enum argLocationType 	location_type;
	union {
		uint64_t 			address;
	} 						location;
	uint32_t 				size;
	char* 					data;
};

void argBuffer_print_raw(struct argBuffer* arg);



#endif