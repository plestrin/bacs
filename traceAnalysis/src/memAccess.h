#ifndef MEMACCESS_H
#define MEMACCESS_H

#include <stdint.h>

#include "argSet.h"
#include "address.h"
#include "array.h"

#define MEMACCESS_UNDEF_GROUP 0

struct memAccess{
	uint32_t 	order;
	uint32_t 	value;
	ADDRESS 	address;
	uint8_t 	size;
	uint32_t 	opcode;
	uint32_t 	group;
};

void memAccess_print(struct memAccess* mem_access, int nb_mem_access);

int32_t memAccess_extract_arg_adjacent_size_read(struct argSet* set, struct memAccess* mem_access, int nb_mem_access, void* frag);

int32_t memAccess_extract_arg_adjacent_size_opcode_read(struct argSet* set, struct memAccess* mem_access, int nb_mem_access, void* frag);

int32_t memAccess_extract_arg_loop_adjacent_size_opcode_read(struct argSet* set, struct memAccess* mem_access, int nb_mem_access, void* frag);

int32_t memAccess_extract_arg_large_write(struct argSet* set, struct memAccess* mem_access, int nb_mem_access, void* frag);

#endif