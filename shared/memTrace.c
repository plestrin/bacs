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
	snprintf(file_path, PATH_MAX, "%s/memValu%u_%u.bin", directory_path, pid, tid);
	if (!access(file_path, R_OK)){
		log_info("found memory address & value files");
	}
	else{
		log_info("found a memory address file");
	}
	#endif

	return 1;
}

struct memTrace* memTrace_create_trace(const char* directory_path, uint32_t pid, uint32_t tid, struct assembly* assembly){
	struct memTrace* 	mem_trace;
	char 				file_path[PATH_MAX];
	struct stat 		sb;

	mem_trace = malloc(sizeof(struct memTrace));
	if (mem_trace == NULL){
		log_err("unable to allocate memory");
		return NULL;
	}

	mem_trace->mem_addr_buffer 	= NULL;
	mem_trace->mem_valu_buffer 	= NULL;
	mem_trace->nb_mem 			= assembly->dyn_blocks[assembly->nb_dyn_block - 1].mem_access_count;
	mem_trace->allocation_type 	= ALLOCATION_MMAP;

	snprintf(file_path, PATH_MAX, "%s/memAddr%u_%u.bin", directory_path, pid, tid);

	mem_trace->file_addr = open(file_path, O_RDONLY);
	if (mem_trace->file_addr == -1){
		log_err_m("unable to open file: \"%s\"", file_path);
		memTrace_delete(mem_trace);
		return NULL;
	}

	if (fstat(mem_trace->file_addr, &sb) < 0){
		log_err("unable to read file size");
		memTrace_delete(mem_trace);
		return NULL;
	}

	if ((uint64_t)sb.st_size != mem_trace->nb_mem * sizeof(struct memAddress)){
		log_err_m("incorrect file size (theoretical size: %lld, practical size: %lld)", mem_trace->nb_mem * sizeof(struct memAddress), sb.st_size);
		memTrace_delete(mem_trace);
		return NULL;
	}

	snprintf(file_path, PATH_MAX, "%s/memValu%u_%u.bin", directory_path, pid, tid);
	if (!access(file_path, R_OK)){
		mem_trace->file_valu = open(file_path, O_RDONLY);
		if (mem_trace->file_valu == -1){
			log_err_m("unable to open file: \"%s\"", file_path);
		}
		else{
			if (fstat(mem_trace->file_valu, &sb) < 0){
				log_err("unable to read file size");
				close(mem_trace->file_valu);
				mem_trace->file_valu = -1;
			}
			else{
				if ((uint64_t)sb.st_size != mem_trace->nb_mem * MEMVALUE_PADDING){
					log_err_m("incorrect file size (theoretical size: %lld, practical size: %lld)", mem_trace->nb_mem * MEMVALUE_PADDING, sb.st_size);
					close(mem_trace->file_valu);
					mem_trace->file_valu = -1;
				}
			}
		}
	}
	else{
		mem_trace->file_valu = -1;
	}

	return mem_trace;
}

struct memTrace* memTrace_create_frag(struct memTrace* master, uint64_t index_mem_start, uint64_t index_mem_stop, struct array* extrude_array){
	uint32_t 					i;
	struct memTrace* 			mem_trace;
	struct memAddress* 			new_mem_addr_buffer;
	uint8_t* 					new_mem_valu_buffer = NULL;
	struct memAccessExtrude* 	extrude;
	uint64_t 					size;
	uint64_t 					start;

	if (master->file_addr == -1){
		log_err("incorrect argument, extraction from a fragment is not implemented yet");
		return NULL;
	}

	if (index_mem_start > master->nb_mem){
		log_warn("memory index is out of bound");
		index_mem_start = master->nb_mem;
	}

	if (index_mem_stop > master->nb_mem){
		log_warn("memory index is out of bound");
		index_mem_stop = master->nb_mem;
	}

	if (index_mem_start >= index_mem_stop){
		log_err("incorrect memory range");
		return NULL;
	}

	if ((mem_trace = malloc(sizeof(struct memTrace))) == NULL){
		log_err("unable to allocate memory");
		return NULL;
	}

	mem_trace->file_addr = -1;
	mem_trace->file_valu = -1;
	mem_trace->allocation_type = ALLOCATION_MMAP;
	mem_trace->nb_mem = index_mem_stop - index_mem_start;

	if ((mem_trace->mem_addr_buffer = mapFile_part(master->file_addr, index_mem_start * sizeof(struct memAddress), mem_trace->nb_mem * sizeof(struct memAddress), &(mem_trace->mapping_addr))) == NULL){
		log_err("unable to map file part");
		free(mem_trace);
		return NULL;
	}

	if (master->file_valu != -1){
		if ((mem_trace->mem_valu_buffer = mapFile_part(master->file_valu, index_mem_start * MEMVALUE_PADDING, mem_trace->nb_mem * MEMVALUE_PADDING, &(mem_trace->mapping_valu))) == NULL){
			log_err("unable to map file part");
		}
	}
	else{
		mem_trace->mem_valu_buffer = NULL;
	}

	if (array_get_length(extrude_array)){
		for (i = 0, size = 0; i < array_get_length(extrude_array); i++){
			extrude = (struct memAccessExtrude*)array_get(extrude_array, i);
			size += extrude->index_stop - extrude->index_start;
		}

		new_mem_addr_buffer = malloc((mem_trace->nb_mem - size) * sizeof(struct memAddress));
		if (new_mem_addr_buffer == NULL){
			log_err("unable to allocate memory");
			memTrace_delete(mem_trace);
			return NULL;
		}

		if (mem_trace->mem_valu_buffer != NULL){
			new_mem_valu_buffer = malloc((mem_trace->nb_mem - size) * MEMVALUE_PADDING);
			if (new_mem_valu_buffer == NULL){
				log_err("unable to allocate memory");
				mappingDesc_free_mapping(mem_trace->mapping_valu);
				mem_trace->mem_valu_buffer = NULL;
			}
		}

		for (i = 0, size = 0, start = 0; i < array_get_length(extrude_array); i++){
			extrude = (struct memAccessExtrude*)array_get(extrude_array, i);

			memcpy(new_mem_addr_buffer + size, mem_trace->mem_addr_buffer + start, sizeof(struct memAddress) * (extrude->index_start - start));
			if (new_mem_valu_buffer != NULL){
				memcpy(new_mem_valu_buffer + (size * MEMVALUE_PADDING), mem_trace->mem_valu_buffer + (start * MEMVALUE_PADDING), MEMVALUE_PADDING * (extrude->index_start - start));
			}
			size += extrude->index_start - start;
			start = extrude->index_stop;
		}

		memcpy(new_mem_addr_buffer + size, mem_trace->mem_addr_buffer + start, sizeof(struct memAddress) * (mem_trace->nb_mem - start));
		if (new_mem_valu_buffer != NULL){
			memcpy(new_mem_valu_buffer + (size * MEMVALUE_PADDING), mem_trace->mem_valu_buffer + (start * MEMVALUE_PADDING), MEMVALUE_PADDING * (mem_trace->nb_mem - start));
		}
		size += mem_trace->nb_mem - start;

		mappingDesc_free_mapping(mem_trace->mapping_addr);
		mem_trace->allocation_type 	= ALLOCATION_MALLOC;
		mem_trace->nb_mem 			= size;
		mem_trace->mem_addr_buffer 	= new_mem_addr_buffer;
		if (new_mem_valu_buffer != NULL){
			mappingDesc_free_mapping(mem_trace->mapping_valu);
			mem_trace->mem_valu_buffer 	= new_mem_valu_buffer;
		}
	}

	return mem_trace;
}

struct memTrace* memTrace_create_concat(struct memTrace** mem_trace_src_buffer, uint32_t nb_mem_trace_src){
	struct memTrace* 	mem_trace;
	uint32_t 			i;
	size_t 				j;
	uint32_t 			copy_mem_valu_buffer;

	mem_trace = malloc(sizeof(struct memTrace));
	if (mem_trace == NULL){
		log_err("unable to allocate memory");
		return NULL;
	}

	mem_trace->file_addr = -1;
	mem_trace->file_valu = -1;
	mem_trace->allocation_type = ALLOCATION_MALLOC;
	mem_trace->nb_mem = 0;

	for (i = 0, copy_mem_valu_buffer = 1; i < nb_mem_trace_src; i++){
		mem_trace->nb_mem += mem_trace_src_buffer[i]->nb_mem;
		if (mem_trace_src_buffer[i]->mem_valu_buffer == NULL){
			copy_mem_valu_buffer = 0;
		}
	}

	mem_trace->mem_addr_buffer = malloc(sizeof(struct memAddress) * mem_trace->nb_mem);
	if (mem_trace->mem_addr_buffer == NULL){
		log_err("unable to allocate memory");
		free(mem_trace);
		return NULL;
	}

	if (copy_mem_valu_buffer){
		mem_trace->mem_valu_buffer = malloc(MEMVALUE_PADDING * mem_trace->nb_mem);
		if (mem_trace->mem_valu_buffer == NULL){
			log_err("unable to allocate memory");
		}
	}
	else{
		mem_trace->mem_valu_buffer = NULL;
	}

	for (i = 0, j = 0; i < nb_mem_trace_src; i++){
		memcpy(mem_trace->mem_addr_buffer + j, mem_trace_src_buffer[i]->mem_addr_buffer, sizeof(struct memAddress) * mem_trace_src_buffer[i]->nb_mem);
		if (mem_trace->mem_valu_buffer != NULL){
			memcpy(mem_trace->mem_valu_buffer + (j * MEMVALUE_PADDING), mem_trace_src_buffer[i]->mem_valu_buffer, MEMVALUE_PADDING * mem_trace_src_buffer[i]->nb_mem);
		}
		j += mem_trace_src_buffer[i]->nb_mem;
	}

	return mem_trace;
}

int32_t memAddress_buffer_compare(const struct memAddress* buffer1, const struct memAddress* buffer2, uint64_t nb_mem){
	uint64_t i;

	for (i = 0; i < nb_mem; i++){
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
	if (mem_trace1->file_addr < mem_trace2->file_addr){
		return -1;
	}
	else if (mem_trace1->file_addr > mem_trace2->file_addr){
		return 1;
	}
	if (mem_trace1->file_valu < mem_trace2->file_valu){
		return -1;
	}
	else if (mem_trace1->file_valu > mem_trace2->file_valu){
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
			if (mem_trace1->nb_mem < mem_trace2->nb_mem){
				return -1;
			}
			else if (mem_trace1->nb_mem > mem_trace2->nb_mem){
				return 1;
			}
			else{
				return memAddress_buffer_compare(mem_trace1->mem_addr_buffer, mem_trace2->mem_addr_buffer, mem_trace1->nb_mem);
			}
		}
	}
}

void memTrace_clean(struct memTrace* mem_trace){
	if (mem_trace->file_addr != -1){
		close(mem_trace->file_addr);
	}
	if (mem_trace->file_valu != -1){
		close(mem_trace->file_valu);
	}
	if (mem_trace->mem_addr_buffer != NULL){
		switch (mem_trace->allocation_type){
			case ALLOCATION_MALLOC 	: {
				free(mem_trace->mem_addr_buffer);
				if (mem_trace->mem_valu_buffer != NULL){
					free(mem_trace->mem_valu_buffer);
				}
				break;
			}
			case ALLOCATION_MMAP 	: {
				mappingDesc_free_mapping(mem_trace->mapping_addr);
				if (mem_trace->mem_valu_buffer != NULL){
					mappingDesc_free_mapping(mem_trace->mapping_valu);
				}
				break;
			}
		}
	}
}

#define DEFAULT_NB_ADDR_MAP 32768

int32_t memTraceIterator_init(struct memTraceIterator* it, struct memTrace* master, uint64_t mem_addr_index){
	if (mem_addr_index >= master->nb_mem){
		log_err("memory access out of bound");
		return -1;
	}

	it->master 			= master;
	it->mapping.buffer 	= NULL;
	it->mem_addr_index 	= mem_addr_index;

	if (master->file_addr == -1){
		it->mem_addr_buffer 	= master->mem_addr_buffer;
		it->nb_mem_addr 		= (uint32_t)master->nb_mem;
		it->mem_addr_sub_index 	= (uint32_t)mem_addr_index;
	}
	else{
		it->mem_addr_buffer 	= NULL;
	}

	return 0;
}

int32_t memTraceIterator_get_prev_addr(struct memTraceIterator* it, ADDRESS addr){
	if (!it->mem_addr_index){
		return 1;
	}

	it->mem_addr_index --;

	restart:

	if (it->mem_addr_buffer == NULL){
		it->nb_mem_addr 		= min(it->mem_addr_index + 1, DEFAULT_NB_ADDR_MAP);
		it->mem_addr_sub_index 	= it->nb_mem_addr;

		if ((it->mem_addr_buffer = mapFile_part(it->master->file_addr, (it->mem_addr_index + 1 - it->nb_mem_addr) * sizeof(struct memAddress), it->nb_mem_addr * sizeof(struct memAddress), &(it->mapping))) == NULL){
			log_err("unable to map file part");
			return -1;
		}
	}

	while (it->mem_addr_sub_index){
		it->mem_addr_sub_index --;
		if (it->mem_addr_buffer[it->mem_addr_sub_index].address == addr){
			return 0;
		}
		if (!it->mem_addr_index){
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
	if (it->mem_addr_index >= it->master->nb_mem){
		return 1;
	}

	it->mem_addr_index ++;

	restart:

	if (it->mem_addr_buffer == NULL){
		it->nb_mem_addr 		= min(it->mem_addr_index + DEFAULT_NB_ADDR_MAP, it->master->nb_mem) - it->mem_addr_index;
		it->mem_addr_sub_index 	= 0xffffffff;

		if ((it->mem_addr_buffer = mapFile_part(it->master->file_addr, it->mem_addr_index * sizeof(struct memAddress), it->nb_mem_addr * sizeof(struct memAddress), &(it->mapping))) == NULL){
			log_err("unable to map file part");
			return -1;
		}
	}

	while (it->mem_addr_sub_index + 1 < it->nb_mem_addr){
		it->mem_addr_sub_index ++;
		if (it->mem_addr_buffer[it->mem_addr_sub_index].address == addr){
			return 0;
		}
		if (it->mem_addr_index + 1 == it->master->nb_mem){
			return 1;
		}
		it->mem_addr_index ++;
	}

	mappingDesc_free_mapping(it->mapping);
	it->mem_addr_buffer = NULL;
	it->mapping.buffer = NULL;

	goto restart;
}
