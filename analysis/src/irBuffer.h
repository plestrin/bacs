#ifndef IRBUFFER_H
#define IRBUFFER_H

#include "ir.h"

struct memParameter{
	ADDRESS 	addr;
	uint32_t 	size;
	uint8_t* 	ptr;
};

struct primitiveParameter{
	uint32_t 				nb_in;
	uint32_t 				nb_ou;
	struct memParameter* 	mem_para_buffer;
};

struct primitiveParameter* ir_search_buffer_signature(struct ir* ir, uint32_t* nb_prim_para);

void bufferSignature_print_buffer(void);

#endif
