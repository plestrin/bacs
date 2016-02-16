#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/stat.h>

#include "memTrace.h"
#include "base.h"

void memAddress_print(const struct memAddress* address){
	if (memAddress_descriptor_is_read(address->descriptor)){
		printf("R:%u:", (address->descriptor >> 8) & 0x000000ff);
	}
	else if (memAddress_descriptor_is_write(address->descriptor)){
		printf("W:%u:", (address->descriptor >> 24) & 0x000000ff);
	}
	else{
		printf("??:");
	}

	printf(PRINTF_ADDR, address->address);
}

int32_t memTrace_is_trace_exist(const char* directory_path, uint32_t pid, uint32_t tid){
	char file_path[PATH_MAX];

	snprintf(file_path, PATH_MAX, "%s/memAddr%u_%u.bin", directory_path, pid, tid);
	if (access(file_path, R_OK)){
		return 0;
	}

	#ifdef VERBOSE
	log_info("found a memory address file");
	#endif

	return 1;
}

struct memTrace* memTrace_create_trace(const char* directory_path, uint32_t pid, uint32_t tid, struct assembly* assembly){
	struct memTrace* 	mem_trace;
	char 				file_path[PATH_MAX];
	struct stat 		sb;

	mem_trace = (struct memTrace*)malloc(sizeof(struct memTrace));
	if (mem_trace == NULL){
		log_err("unable to allocate memory");
		return NULL;
	}

	mem_trace->mapping.buffer 	= NULL;
	mem_trace->mem_addr_buffer 	= NULL;
	mem_trace->nb_mem_addr 		= assembly->dyn_blocks[assembly->nb_dyn_block - 1].mem_access_count;
	mem_trace->allocation_type  = ALLOCATION_MMAP;

	snprintf(file_path, PATH_MAX, "%s/memAddr%u_%u.bin", directory_path, pid, tid);

	mem_trace->file = open(file_path, O_RDONLY);
	if (mem_trace->file == -1){
		log_err_m("unable to open file: \"%s\"", file_path);
		memTrace_delete(mem_trace);
		return NULL;
	}

	if (fstat(mem_trace->file, &sb) < 0){
		log_err("unable to read file size");
		memTrace_delete(mem_trace);
		return NULL;
	}

	if ((uint64_t)sb.st_size != mem_trace->nb_mem_addr * sizeof(struct memAddress)){
		log_err_m("incorrect file size (theoretical size: %lld, practical size: %lld)", mem_trace->nb_mem_addr * sizeof(struct memAddress), sb.st_size);
		memTrace_delete(mem_trace);
		return NULL;
	}

	return mem_trace;
}

struct memTrace* memTrace_create_frag(struct memTrace* master, uint64_t index_mem_start, uint64_t index_mem_stop, struct array* extrude_array){
	uint32_t 					i;
	struct memTrace* 			mem_trace;
	struct memAddress* 			new_mem_addr_buffer;
	struct memAccessExtrude* 	extrude;
	uint64_t 					size;
	uint64_t 					start;

	if (master->file == -1){
		log_err("incorrect argument, extraction from a fragment is not implemented yet");
		return NULL;
	}

	if (index_mem_start > master->nb_mem_addr){
		log_warn("memory index is out of bound");
		index_mem_start = master->nb_mem_addr;
	}

	if (index_mem_stop > master->nb_mem_addr){
		log_warn("memory index is out of bound");
		index_mem_stop = master->nb_mem_addr;
	}

	if (index_mem_start >= index_mem_stop){
		log_err("incorrect memory range");
		return NULL;
	}

	if ((mem_trace = (struct memTrace*)malloc(sizeof(struct memTrace))) == NULL){
		log_err("unable to allocate memory");
		return NULL;
	}

	mem_trace->file = -1;
	mem_trace->allocation_type = ALLOCATION_MMAP;
	mem_trace->nb_mem_addr = index_mem_stop - index_mem_start;

	if ((mem_trace->mem_addr_buffer = mapFile_part(master->file, index_mem_start * sizeof(struct memAddress), mem_trace->nb_mem_addr * sizeof(struct memAddress), &(mem_trace->mapping))) == NULL){
		log_err("unable to map file part");
		free(mem_trace);
		return NULL;
	}

	if (array_get_length(extrude_array)){
		for (i = 0, size = 0; i < array_get_length(extrude_array); i++){
			extrude = (struct memAccessExtrude*)array_get(extrude_array, i);
			size += extrude->index_stop - extrude->index_start;
		}

		new_mem_addr_buffer = (struct memAddress*)malloc((mem_trace->nb_mem_addr - size) * sizeof(struct memAddress));
		if (new_mem_addr_buffer == NULL){
			log_err("unable to allocate memory");
			memTrace_delete(mem_trace);
			return NULL;
		}

		for (i = 0, size = 0, start = 0; i < array_get_length(extrude_array); i++){
			extrude = (struct memAccessExtrude*)array_get(extrude_array, i);

			memcpy(new_mem_addr_buffer + size, mem_trace->mem_addr_buffer + start, sizeof(struct memAddress) * (extrude->index_start - start));
			size += extrude->index_start - start;
			start = extrude->index_stop;
		}

		memcpy(new_mem_addr_buffer + size, mem_trace->mem_addr_buffer + start, sizeof(struct memAddress) * (mem_trace->nb_mem_addr - start));
		size += mem_trace->nb_mem_addr - start;

		mappingDesc_free_mapping(mem_trace->mapping);
		mem_trace->allocation_type 	= ALLOCATION_MALLOC;
		mem_trace->nb_mem_addr 		= size;
		mem_trace->mem_addr_buffer 	= new_mem_addr_buffer;
	}

	return mem_trace;
}

struct memTrace* memTrace_create_concat(struct memTrace** mem_trace_src_buffer, uint32_t nb_mem_trace_src){
	struct memTrace* 	mem_trace;
	uint32_t 			i;
	size_t 				j;

	mem_trace = (struct memTrace*)malloc(sizeof(struct memTrace));
	if (mem_trace == NULL){
		log_err("unable to allocate memory");
		return NULL;
	}

	mem_trace->file = -1;
	mem_trace->allocation_type = ALLOCATION_MALLOC;
	mem_trace->nb_mem_addr = 0;

	for (i = 0; i < nb_mem_trace_src; i++){
		mem_trace->nb_mem_addr += mem_trace_src_buffer[i]->nb_mem_addr;
	}

	mem_trace->mem_addr_buffer = (struct memAddress*)malloc(sizeof(struct memAddress) * mem_trace->nb_mem_addr);
	if (mem_trace->mem_addr_buffer == NULL){
		log_err("unable to allocate memory");
		free(mem_trace);
		return NULL;
	}

	for (i = 0, j = 0; i < nb_mem_trace_src; i++){
		memcpy(mem_trace->mem_addr_buffer + j, mem_trace_src_buffer[i]->mem_addr_buffer, sizeof(struct memAddress) * mem_trace_src_buffer[i]->nb_mem_addr);
		j += mem_trace_src_buffer[i]->nb_mem_addr;
	}

	return mem_trace;
}

int32_t memAddress_buffer_compare(const struct memAddress* buffer1, const struct memAddress* buffer2, uint64_t nb_mem_addr){
	uint64_t i;

	for (i = 0; i < nb_mem_addr; i++){
		if (buffer1[i].descriptor < buffer2[i].descriptor){
			return -1;
		}
		else if (buffer1[i].descriptor > buffer2[i].descriptor){
			return 1;
		}
		else if (buffer1[i].address < buffer2[i].address){
			return -1;
		}
		else if (buffer1[i].address > buffer2[i].address){
			return 1;
		}
	}

	return 0;
}

int32_t memTrace_compare(const struct memTrace* mem_trace1, const struct memTrace* mem_trace2){
	if (mem_trace1->file < mem_trace2->file){
		return -1;
	}
	else if (mem_trace1->file > mem_trace2->file){
		return 1;
	}

	if (mem_trace1->mem_addr_buffer == NULL){
		if (mem_trace2->mem_addr_buffer == NULL){
			return 0;
		}
		else{
			return -1;
		}
	}
	else{
		if (mem_trace2->mem_addr_buffer == NULL){
			return 1;
		}
		else{
			if (mem_trace1->nb_mem_addr < mem_trace2->nb_mem_addr){
				return -1;
			}
			else if (mem_trace1->nb_mem_addr > mem_trace2->nb_mem_addr){
				return 1;
			}
			else{
				return memAddress_buffer_compare(mem_trace1->mem_addr_buffer, mem_trace2->mem_addr_buffer, mem_trace1->nb_mem_addr);
			}
		}
	}
}

void memTrace_clean(struct memTrace* mem_trace){
	if (mem_trace->file != -1){
		close(mem_trace->file);
	}
	if (mem_trace->mem_addr_buffer != NULL){
		switch(mem_trace->allocation_type){
			case ALLOCATION_MALLOC 	: {free(mem_trace->mem_addr_buffer); break;}
			case ALLOCATION_MMAP 	: {mappingDesc_free_mapping(mem_trace->mapping); break;}
		}
	}
}

#define DEFAULT_NB_ADDR_MAP 32768

int32_t memTraceIterator_init(struct memTraceIterator* it, struct memTrace* master, uint64_t mem_addr_index){
	if (mem_addr_index >= master->nb_mem_addr){
		log_err("memory access out of bound");
		return -1;
	}

	it->master 			= master;
	it->mapping.buffer 	= NULL;
	it->mem_addr_index 	= mem_addr_index;

	if (master->file == -1){
		it->mem_addr_buffer 	= master->mem_addr_buffer;
		it->nb_mem_addr 		= (uint32_t)master->nb_mem_addr;
		it->mem_addr_sub_index 	= (uint32_t)mem_addr_index;
	}
	else{
		it->mem_addr_buffer 	= NULL;
	}

	return 0;
}

int32_t memTraceIterator_get_prev_addr(struct memTraceIterator* it, ADDRESS addr){
	if (it->mem_addr_index == 0){
		return 1;
	}

	it->mem_addr_index --;

	restart:

	if (it->mem_addr_buffer == NULL){
		it->nb_mem_addr 		= min(it->mem_addr_index + 1, DEFAULT_NB_ADDR_MAP);
		it->mem_addr_sub_index 	= it->nb_mem_addr;

		if ((it->mem_addr_buffer = mapFile_part(it->master->file, (it->mem_addr_index + 1 - it->nb_mem_addr) * sizeof(struct memAddress), it->nb_mem_addr * sizeof(struct memAddress), &(it->mapping))) == NULL){
			log_err("unable to map file part");
			return -1;
		}
	}

	while (it->mem_addr_sub_index != 0){
		it->mem_addr_sub_index --;
		if (it->mem_addr_buffer[it->mem_addr_sub_index].address == addr){
			return 0;
		}
		if (it->mem_addr_index == 0){
			return 1;
		}
		it->mem_addr_index --;
	}

	mappingDesc_free_mapping(it->mapping);
	it->mem_addr_buffer = NULL;
	it->mapping.buffer = NULL;

	goto restart;
}

int32_t memTraceIterator_get_next_addr(struct memTraceIterator* it, ADDRESS addr){
	if (it->mem_addr_index >= it->master->nb_mem_addr){
		return 1;
	}

	it->mem_addr_index ++;

	restart:

	if (it->mem_addr_buffer == NULL){
		it->nb_mem_addr 		= min(it->mem_addr_index + DEFAULT_NB_ADDR_MAP, it->master->nb_mem_addr) - it->mem_addr_index;
		it->mem_addr_sub_index 	= 0xffffffff;

		if ((it->mem_addr_buffer = mapFile_part(it->master->file, it->mem_addr_index * sizeof(struct memAddress), it->nb_mem_addr * sizeof(struct memAddress), &(it->mapping))) == NULL){
			log_err("unable to map file part");
			return -1;
		}
	}

	while (it->mem_addr_sub_index + 1 < it->nb_mem_addr){
		it->mem_addr_sub_index ++;
		if (it->mem_addr_buffer[it->mem_addr_sub_index].address == addr){
			return 0;
		}
		if (it->mem_addr_index + 1 == it->master->nb_mem_addr){
			return 1;
		}
		it->mem_addr_index ++;
	}

	mappingDesc_free_mapping(it->mapping);
	it->mem_addr_buffer = NULL;
	it->mapping.buffer = NULL;

	goto restart;
}
