#ifndef MEMACCESS_H
#define MEMACCESS_H

#include <stdint.h>

#include "address.h"
#include "array.h"

struct memAccess{
	uint32_t 	order;
	uint32_t 	value;
	ADDRESS 	address;
	uint8_t 	size;
	uint32_t 	opcode;
};

void memAccess_print(struct memAccess* mem_access, int nb_mem_access);

int32_t memAccess_extract_arg_adjacent_read(struct array* array, struct memAccess* mem_access, int nb_mem_access, void* frag);
int32_t memAccess_extract_arg_adjacent_write(struct array* array, struct memAccess* mem_access, int nb_mem_access, void* frag);

int32_t memAccess_extract_arg_adjacent_size_read(struct array* array, struct memAccess* mem_access, int nb_mem_access, void* frag);
int32_t memAccess_extract_arg_adjacent_size_write(struct array* array, struct memAccess* mem_access, int nb_mem_access, void* frag);

int32_t memAccess_extract_arg_adjacent_size_opcode_read(struct array* array, struct memAccess* mem_access, int nb_mem_access, void* frag);
int32_t memAccess_extract_arg_adjacent_size_opcode_write(struct array* array, struct memAccess* mem_access, int nb_mem_access, void* frag);

int32_t memAccess_extract_arg_loop_adjacent_size_opcode_read(struct array* array, struct memAccess* mem_access, int nb_mem_access, void* frag);
int32_t memAccess_extract_arg_loop_adjacent_size_opcode_write(struct array* array, struct memAccess* mem_access, int nb_mem_access, void* frag);

#endif