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
	uint32_t 			i;
	int64_t 			nb_mem_access;
	struct stat 		sb;

	mem_trace = (struct memTrace*)malloc(sizeof(struct memTrace));
	if (mem_trace == NULL){
		log_err("unable to allocate memory");
		return NULL;
	}

	mem_trace->mem_addr_buffer = NULL;
	mem_trace->allocation_type = ALLOCATION_MMAP;

	snprintf(file_path, PATH_MAX, "%s/memAddr%u_%u.bin", directory_path, pid, tid);
	mem_trace->file = open(file_path, O_RDONLY);
	if (mem_trace->file == -1){
		log_err_m("unable to open file: \"%s\"", file_path);
		free(mem_trace);
		return NULL;
	}

	for (i = 0, nb_mem_access = 0; i < assembly->nb_dyn_block; i++){
		if (dynBlock_is_valid(assembly->dyn_blocks + i) && assembly->dyn_blocks[i].block->header.nb_mem_access != UNTRACK_MEM_ACCESS){
			nb_mem_access += assembly->dyn_blocks[i].block->header.nb_mem_access;
		}
	}

	if (fstat(mem_trace->file, &sb) < 0){
		log_err("unable to read file size");
		memTrace_delete(mem_trace);
		return NULL;
	}

	if (sb.st_size != nb_mem_access * sizeof(struct memAddress)){
		log_err("incorrect file size");
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

	mem_trace = (struct memTrace*)malloc(sizeof(struct memTrace));
	if (mem_trace == NULL){
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
