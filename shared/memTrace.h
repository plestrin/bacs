#ifndef MEMTRACE_H
#define MEMTRACE_H

#include <stdint.h>

#include "address.h"
#include "mapFile.h"
#include "assembly.h"
#include "base.h"

struct memAddress{
	ADDRESS 	pc;
	uint32_t 	descriptor;
	ADDRESS 	address;
};

#define MEMADDRESS_DESCRIPTOR_CLEAN 	0x00000000
#define MEMADDRESS_DESCRIPTOR_READ_0 	0x00000001
#define MEMADDRESS_DESCRIPTOR_WRITE_0 	0x00010000

#define memAddress_descriptor_is_included(desc1, desc2) (((desc2) & 0x00010000) ? (((desc1) & 0xffff0000) == (desc2)) : (((desc1) & 0x0000ffff) == (desc2)))

#define memAddress_descriptor_set_read(desc, index) 	((desc) |= 0x00000001 | (((index) << 8) & 0x0000ff00))
#define memAddress_descriptor_set_write(desc, index) 	((desc) |= 0x00010000 | ((index) << 24))

#define MEMADDRESS_INVALID 0x00000000

static inline ADDRESS memAddress_get_and_check(struct memAddress* mem_addr, uint32_t descriptor){
	if (mem_addr == NULL){
		return MEMADDRESS_INVALID;
	}

	if (memAddress_descriptor_is_included(mem_addr->descriptor, descriptor)){
		return mem_addr->address;
	}

	log_err_m("incorrect memory access descriptor, get 0x%08x but 0x%08x was expected", mem_addr->descriptor, descriptor);
	return MEMADDRESS_INVALID;
}

static inline ADDRESS memAddress_search_and_get(struct memAddress* mem_addr, uint32_t descriptor, uint32_t length){
	uint32_t i;

	if (mem_addr == NULL){
		return MEMADDRESS_INVALID;
	}

	for (i = 0; i < length; i++){
		if (memAddress_descriptor_is_included(mem_addr[i].descriptor, descriptor)){
			return mem_addr[i].address;
		}
	}

	log_err_m("unable to find memory access: 0x%08x", descriptor);
	return MEMADDRESS_INVALID;
}

struct memTrace{
	int 				file;
	struct mappingDesc 	mapping;
	struct memAddress* 	mem_addr_buffer;
	uint32_t 			nb_mem_addr;
	enum allocationType allocation_type;
};

int32_t memTrace_is_trace_exist(const char* directory_path, uint32_t thread_id);

struct memTrace* memTrace_create_trace(const char* directory_path, uint32_t thread_id, struct assembly* assembly);
struct memTrace* memTrace_create_frag(struct memTrace* master, uint64_t index_mem_start, uint64_t index_mem_stop);
struct memTrace* memTrace_create_concat(struct memTrace** mem_trace_src_buffer, uint32_t nb_mem_trace_src);

void memTrace_clean(struct memTrace* mem_trace);

#define memTrace_delete(mem_trace) 					\
	memTrace_clean(mem_trace); 						\
	free(mem_trace);

#endif