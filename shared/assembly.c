#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <search.h>

#include "assembly.h"
#include "mapFile.h"
#include "array.h"
#include "printBuffer.h"

struct disassembler{
	uint8_t 					xed_init;
	xed_machine_mode_enum_t 	mmode;
    xed_address_width_enum_t 	stack_addr_width;
};

int32_t assembly_compare_asmBlock_id(const struct asmBlock* b1, const struct asmBlock* b2);
static void assembly_clean_tree_node(void* data);

#pragma GCC diagnostic ignored "-Wpedantic" /* ISO C90 forbids specifying subobject to initialize */
struct disassembler disas = {
	.xed_init 			= 0,
	.mmode 				= XED_MACHINE_MODE_LEGACY_32,
	.stack_addr_width 	= XED_ADDRESS_WIDTH_32b
};

struct assembly* assembly_create(const char* file_name_id, const char* file_name_block){
	struct assembly* assembly;
	assembly = (struct assembly*)malloc(sizeof(struct assembly));
	if (assembly != NULL){
		if (assembly_init(assembly, file_name_id, file_name_block)){
			printf("ERROR: in %s, unable to init assembly structure\n", __func__);
			free(assembly);
			assembly = NULL;
		}
	}
	else{
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
	}

	return assembly;
}

int32_t assembly_init(struct assembly* assembly, const char* file_name_id, const char* file_name_block){
	uint64_t 			mapping_size_id;
	uint64_t			mapping_size_block;
	uint32_t* 			mapping_id;
	void* 				mapping_block;
	uint32_t 			i;
	struct array 		asmBlock_array;
	struct asmBlock* 	current_ptr;
	uint32_t 			current_offset;

	if (disas.xed_init == 0){
		xed_tables_init();
		disas.xed_init = 1;
	}

	mapping_id = mapFile_map(file_name_id, &mapping_size_id);
	mapping_block = mapFile_map(file_name_block, &mapping_size_block);

	if (mapping_id == NULL || mapping_block == NULL){
		printf("ERROR: in %s, unable to map files\n", __func__);
		if (mapping_id != NULL){
			munmap(mapping_id, mapping_size_id);
		}
		if (mapping_block != NULL){
			munmap(mapping_block, mapping_size_block);
		}
		return -1;
	}

	assembly->allocation_type 		= ASSEMBLYALLOCATION_MMAP;
	assembly->mapping_block 		= mapping_block;
	assembly->mapping_size_block 	= mapping_size_block;
	assembly->nb_dyn_block 			= mapping_size_id / sizeof(uint32_t);
	assembly->dyn_blocks 			= (struct dynBlock*)malloc(sizeof(struct dynBlock) * assembly->nb_dyn_block);
	if (assembly->dyn_blocks == NULL){
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
		munmap(mapping_id, mapping_size_id);
		munmap(mapping_block, mapping_size_block);
		return -1;
	}

	if (array_init(&asmBlock_array, sizeof(struct asmBlock*))){
		printf("ERROR: in %s, unable to init array\n", __func__);
		free(assembly->dyn_blocks);
		munmap(mapping_id, mapping_size_id);
		munmap(mapping_block, mapping_size_block);
		return -1;
	}

	current_offset = 0;
	current_ptr = (struct asmBlock*)((char*)mapping_block + current_offset);
	while (current_offset != mapping_size_block){
		if (current_offset + current_ptr->header.size + sizeof(struct asmBlockHeader) > mapping_size_block){
			printf("ERROR: in %s, the last asmBlock is incomplete\n", __func__);
			break;
		}
		else{
			if (array_add(&asmBlock_array, &current_ptr) < 0){
				printf("ERROR: in %s, unable to add asmBlock pointer to array\n", __func__);
			}

			current_offset += sizeof(struct asmBlockHeader) + current_ptr->header.size;
			current_ptr = (struct asmBlock*)((char*)mapping_block + current_offset);
		}
	}

	for (i = 0; i < assembly->nb_dyn_block; i++){
		uint32_t 			up = array_get_length(&asmBlock_array);
		uint32_t 			down = 0;
		uint32_t 			idx;
		struct asmBlock* 	idx_block;

		current_ptr = NULL;
		while(down < up){
			idx  = (up + down) / 2;
			idx_block = *(struct asmBlock**)array_get(&asmBlock_array, idx);
			if (mapping_id[i] > idx_block->header.id){
				down = idx + 1;
			}
			else if (mapping_id[i] < idx_block->header.id){
				up = idx;
			}
			else{
				current_ptr = idx_block;
				break;
			}
		}

		if (current_ptr == NULL){
			printf("ERROR: in %s, unable to locate asmBlock %u\n", __func__, mapping_id[i]);
			break;
		}

		if (i == 0){
			assembly->dyn_blocks[i].instruction_count = 0;
		}
		else{
			assembly->dyn_blocks[i].instruction_count = assembly->dyn_blocks[i - 1].instruction_count + assembly->dyn_blocks[i - 1].block->header.nb_ins;
		}
		assembly->dyn_blocks[i].block = current_ptr;
	}

	assembly->nb_dyn_instruction = assembly->dyn_blocks[assembly->nb_dyn_block - 1].instruction_count + assembly->dyn_blocks[assembly->nb_dyn_block - 1].block->header.nb_ins;

	
	array_clean(&asmBlock_array);
	munmap(mapping_id, mapping_size_id);

	return 0;
}

int32_t assembly_get_instruction(struct assembly* assembly, struct instructionIterator* it, uint32_t index){
	uint32_t 			up 		= assembly->nb_dyn_block;
	uint32_t 			down 	= 0;
	uint32_t 			idx;
	uint8_t 			found 	= 0;
	uint32_t 			i;
	xed_error_enum_t 	xed_error;

	while(down < up){
		idx  = (up + down) / 2;
		if (index >= assembly->dyn_blocks[idx].instruction_count + assembly->dyn_blocks[idx].block->header.nb_ins){
			down = idx + 1;
		}
		else if (index < assembly->dyn_blocks[idx].instruction_count){
			up = idx;
		}
		else{
			it->instruction_index 		= index;
			it->dyn_block_index 		= idx;
			it->instruction_sub_index 	= index - assembly->dyn_blocks[idx].instruction_count;
			it->instruction_offset 		= 0;
			found = 1;
			break;
		}
	}

	if (!found){
		printf("ERROR: in %s, unable to locate the dyn block containing the index %u\n", __func__, index);
		return -1;
	}

	for (i = 0; i <= it->instruction_sub_index; i++){
		xed_decoded_inst_zero(&(it->xedd));
		xed_decoded_inst_set_mode(&(it->xedd), disas.mmode, disas.stack_addr_width);
		xed_error = xed_decode(&(it->xedd), (const xed_uint8_t*)(assembly->dyn_blocks[it->dyn_block_index].block->data + it->instruction_offset), (assembly->dyn_blocks[it->dyn_block_index].block->header.size - it->instruction_offset > 15) ? 15 : (assembly->dyn_blocks[it->dyn_block_index].block->header.size - it->instruction_offset));
		
		switch(xed_error){
			case XED_ERROR_NONE : {
				break;
			}
			case XED_ERROR_BUFFER_TOO_SHORT : {
				printf("ERROR: in %s, not enough bytes provided\n", __func__);
				return -1;
			}
			case XED_ERROR_INVALID_FOR_CHIP : {
				printf("ERROR: in %s, the instruction was not valid for the specified chip\n", __func__);
				return -1;
			}
			case XED_ERROR_GENERAL_ERROR : {
				printf("ERROR: in %s, could not decode given input\n", __func__);
				return -1;
			}
			default : {
				printf("ERROR: in %s, unhandled error code: %s\n", __func__, xed_error_enum_t2str(xed_error));
				return -1;
			}
		}

		if (i == it->instruction_sub_index){
			it->instruction_size = xed_decoded_inst_get_length(&(it->xedd));
			it->instruction_address = assembly->dyn_blocks[it->dyn_block_index].block->header.address + it->instruction_offset;
		}
		else{
			it->instruction_offset += xed_decoded_inst_get_length(&(it->xedd));
		}
	}

	return 0;
}

int32_t assembly_get_next_instruction(struct assembly* assembly, struct instructionIterator* it){
	xed_error_enum_t xed_error;

	if (it->instruction_index + 1 >= assembly->dyn_blocks[it->dyn_block_index].instruction_count + assembly->dyn_blocks[it->dyn_block_index].block->header.nb_ins){
		if (it->dyn_block_index + 1 < assembly->nb_dyn_block){
			it->instruction_index 		= it->instruction_index + 1;
			it->dyn_block_index 		= it->dyn_block_index + 1;
			it->instruction_sub_index 	= 0;
			it->instruction_offset 		= 0;
		}
		else{
			printf("ERROR: in %s, the last instruction has been reached\n", __func__);
			return -1;
		}
	}
	else{
		it->instruction_index += 1;
		it->instruction_sub_index += 1;
		it->instruction_offset += it->instruction_size;
	}

	xed_decoded_inst_zero(&(it->xedd));
	xed_decoded_inst_set_mode(&(it->xedd), disas.mmode, disas.stack_addr_width);
	xed_error = xed_decode(&(it->xedd), (const xed_uint8_t*)(assembly->dyn_blocks[it->dyn_block_index].block->data + it->instruction_offset), (assembly->dyn_blocks[it->dyn_block_index].block->header.size - it->instruction_offset > 15) ? 15 : (assembly->dyn_blocks[it->dyn_block_index].block->header.size - it->instruction_offset));
		
	switch(xed_error){
		case XED_ERROR_NONE : {
			break;
		}
		case XED_ERROR_BUFFER_TOO_SHORT : {
			printf("ERROR: in %s, not enough bytes provided: %u\n", __func__, (assembly->dyn_blocks[it->dyn_block_index].block->header.size - it->instruction_offset > 15) ? 15 : (assembly->dyn_blocks[it->dyn_block_index].block->header.size - it->instruction_offset));
			return -1;
		}
		case XED_ERROR_INVALID_FOR_CHIP : {
			printf("ERROR: in %s, the instruction was not valid for the specified chip\n", __func__);
			return -1;
		}
		case XED_ERROR_GENERAL_ERROR : {
			printf("ERROR: in %s, could not decode given input\n", __func__);
			return -1;
		}
		default : {
			printf("ERROR: in %s, unhandled error code: %s\n", __func__, xed_error_enum_t2str(xed_error));
			return -1;
		}
	}

	it->instruction_size = xed_decoded_inst_get_length(&(it->xedd));
	it->instruction_address = assembly->dyn_blocks[it->dyn_block_index].block->header.address + it->instruction_offset;

	return 0;
}

int32_t assembly_check(struct assembly* assembly){
	uint32_t 			block_offset;
	struct asmBlock* 	block;
	uint32_t 			nb_instruction;
	uint32_t 			instruction_offset;
	xed_error_enum_t 	xed_error;
	xed_decoded_inst_t 	xedd;
	int32_t 			result = 0;

	block_offset = 0;
	block = (struct asmBlock*)((char*)assembly->mapping_block + block_offset);
	while (block_offset != assembly->mapping_size_block){
		if (block_offset + block->header.size + sizeof(struct asmBlockHeader) > assembly->mapping_size_block){
			printf("ERROR: in %s, the last asmBlock is incomplete\n", __func__);
			break;
		}
		else{
			nb_instruction 		= 0;
			instruction_offset 	= 0;

			while(instruction_offset != block->header.size){
				xed_decoded_inst_zero(&xedd);
				xed_decoded_inst_set_mode(&xedd, disas.mmode, disas.stack_addr_width);
				xed_error = xed_decode(&xedd, (const xed_uint8_t*)(block->data + instruction_offset), (block->header.size - instruction_offset > 15) ? 15 : (block->header.size - instruction_offset));
				
				switch(xed_error){
					case XED_ERROR_NONE : {
						nb_instruction ++;
						break;
					}
					case XED_ERROR_BUFFER_TOO_SHORT : {
						printf("ERROR: in %s, not enough bytes provided\n", __func__);
						result = -1;
						goto next_block;
					}
					case XED_ERROR_INVALID_FOR_CHIP : {
						printf("ERROR: in %s, the instruction was not valid for the specified chip\n", __func__);
						result = -1;
						goto next_block;
					}
					case XED_ERROR_GENERAL_ERROR : {
						printf("ERROR: in %s, could not decode given input\n", __func__);
						result = -1;
						goto next_block;
					}
					default : {
						#if defined ARCH_32
						printf("ERROR: in %s, unhandled error code: %s - block id: %u - address: 0x%08x ", __func__, xed_error_enum_t2str(xed_error), block->header.id, block->header.address + instruction_offset);
						#elif defined ARCH_64
						#pragma GCC diagnostic ignored "-Wformat" /* ISO C90 does not support the ‘ll’ gnu_printf length modifier */
						printf("ERROR: in %s, unhandled error code: %s - block id: %u - address: 0x%llx ", __func__, xed_error_enum_t2str(xed_error), block->header.id, block->header.address + instruction_offset);
						#else
						#error Please specify an architecture {ARCH_32 or ARCH_64}
						#endif
						printBuffer_raw(stdout, block->data + instruction_offset, (block->header.size - instruction_offset > 15) ? 15 : (block->header.size - instruction_offset));
						printf("\n");

						result = -1;
						goto next_block;
					}
				}
				instruction_offset += xed_decoded_inst_get_length(&xedd);
			}

			if (nb_instruction != block->header.nb_ins){
				printf("ERROR: in %s, basic block contains %u instruction(s), expecting: %u\n", __func__, nb_instruction, block->header.nb_ins);
			}

			next_block:
			block_offset += sizeof(struct asmBlockHeader) + block->header.size;
			block = (struct asmBlock*)((char*)assembly->mapping_block + block_offset);
		}
	}

	return result;
}

int32_t assembly_compare_asmBlock_id(const struct asmBlock* b1, const struct asmBlock* b2){
	if (b1->header.id > b2->header.id){
		return 1;
	}
	else if (b1->header.id < b2->header.id){
		return -1;
	}
	else{
		return 0;
	}
}

#pragma GCC diagnostic ignored "-Wunused-parameter"
static void assembly_clean_tree_node(void* data){
}

int32_t assembly_extract_segment(struct assembly* assembly_src, struct assembly* assembly_dst, uint32_t offset, uint32_t length){
	uint32_t 			up 					= assembly_src->nb_dyn_block;
	uint32_t 			down 				= 0;
	uint32_t 			idx_block_start;
	uint32_t 			idx_block_stop;
	uint32_t 			idx_ins_start;
	uint32_t 			idx_ins_stop;
	uint8_t 			found 				= 0;
	uint32_t 			i;
	uint32_t 			size;
	xed_decoded_inst_t 	xedd;
	xed_error_enum_t 	xed_error;
	uint32_t 			possible_new_id 	= 0;
	struct asmBlock* 	new_block;
	void* 				bintree_root 		= NULL;

	while(down < up){
		idx_block_start  = (up + down) / 2;
		if (offset >= assembly_src->dyn_blocks[idx_block_start].instruction_count + assembly_src->dyn_blocks[idx_block_start].block->header.nb_ins){
			down = idx_block_start + 1;
		}
		else if (offset < assembly_src->dyn_blocks[idx_block_start].instruction_count){
			up = idx_block_start;
		}
		else{
			found = 1;
			break;
		}
	}

	if (!found){
		printf("ERROR: in %s, unable to locate the dyn block containing the index %u\n", __func__, offset);
		return -1;
	}

	if (offset + length > assembly_src->nb_dyn_instruction){
		printf("WARNING: in %s, length is too big (%u) -> cropping\n", __func__, offset + length);
		length = assembly_src->nb_dyn_instruction - offset;
	}

	for (i = 0, idx_ins_start = 0, size = assembly_src->dyn_blocks[idx_block_start].block->header.size; i < offset - assembly_src->dyn_blocks[idx_block_start].instruction_count; i++){
		xed_decoded_inst_zero(&xedd);
		xed_decoded_inst_set_mode(&xedd, disas.mmode, disas.stack_addr_width);
		xed_error = xed_decode(&xedd, (const xed_uint8_t*)(assembly_src->dyn_blocks[idx_block_start].block->data + idx_ins_start), (assembly_src->dyn_blocks[idx_block_start].block->header.size - idx_ins_start > 15) ? 15 :  (assembly_src->dyn_blocks[idx_block_start].block->header.size - idx_ins_start));
		
		switch(xed_error){
			case XED_ERROR_NONE : {
				break;
			}
			case XED_ERROR_BUFFER_TOO_SHORT : {
				printf("ERROR: in %s, not enough bytes provided\n", __func__);
				return -1;
			}
			case XED_ERROR_INVALID_FOR_CHIP : {
				printf("ERROR: in %s, the instruction was not valid for the specified chip\n", __func__);
				return -1;
			}
			case XED_ERROR_GENERAL_ERROR : {
				printf("ERROR: in %s, could not decode given input\n", __func__);
				return -1;
			}
			default : {
				printf("ERROR: in %s, unhandled error code: %s\n", __func__, xed_error_enum_t2str(xed_error));
				return -1;
			}
		}

		idx_ins_start += xed_decoded_inst_get_length(&xedd);

		if (i != offset - assembly_src->dyn_blocks[idx_block_start].instruction_count - 1){
			size = assembly_src->dyn_blocks[idx_block_start].block->header.size - idx_ins_start;
			possible_new_id = (assembly_src->dyn_blocks[idx_block_start].block->header.id >= possible_new_id) ? (assembly_src->dyn_blocks[idx_block_start].block->header.id + 1) : possible_new_id;
		}
	}

	idx_block_stop = idx_block_start;
	while(offset + length >= assembly_src->dyn_blocks[idx_block_stop].instruction_count + assembly_src->dyn_blocks[idx_block_stop].block->header.nb_ins && idx_block_stop < assembly_src->nb_dyn_block){
		size += assembly_src->dyn_blocks[idx_block_stop].block->header.size;
		idx_block_stop ++;
		possible_new_id = (assembly_src->dyn_blocks[idx_block_stop].block->header.id >= possible_new_id) ? (assembly_src->dyn_blocks[idx_block_stop].block->header.id + 1) : possible_new_id;
	}

	for (i = 0, idx_ins_stop = 0; i < offset + length - assembly_src->dyn_blocks[idx_block_stop].instruction_count; i++){
		xed_decoded_inst_zero(&xedd);
		xed_decoded_inst_set_mode(&xedd, disas.mmode, disas.stack_addr_width);
		xed_error = xed_decode(&xedd, (const xed_uint8_t*)(assembly_src->dyn_blocks[idx_block_stop].block->data + idx_ins_stop), (assembly_src->dyn_blocks[idx_block_stop].block->header.size - idx_ins_stop > 15) ? 15 :  (assembly_src->dyn_blocks[idx_block_stop].block->header.size - idx_ins_stop));
		
		switch(xed_error){
			case XED_ERROR_NONE : {
				break;
			}
			case XED_ERROR_BUFFER_TOO_SHORT : {
				printf("ERROR: in %s, not enough bytes provided\n", __func__);
				return -1;
			}
			case XED_ERROR_INVALID_FOR_CHIP : {
				printf("ERROR: in %s, the instruction was not valid for the specified chip\n", __func__);
				return -1;
			}
			case XED_ERROR_GENERAL_ERROR : {
				printf("ERROR: in %s, could not decode given input\n", __func__);
				return -1;
			}
			default : {
				printf("ERROR: in %s, unhandled error code: %s\n", __func__, xed_error_enum_t2str(xed_error));
				return -1;
			}
		}

		idx_ins_stop += xed_decoded_inst_get_length(&xedd);
	}

	if (idx_block_start != idx_block_stop){
		size += idx_ins_stop;
	}
	else{
		size -= (assembly_src->dyn_blocks[idx_block_stop].block->header.size - idx_ins_stop);
	}

	assembly_dst->nb_dyn_instruction 	= length;
	assembly_dst->nb_dyn_block 			= idx_block_stop - idx_block_start + 1 - ((idx_ins_stop == 0) ? 1 : 0);
	assembly_dst->dyn_blocks 			= (struct dynBlock*)malloc(sizeof(struct dynBlock) * assembly_dst->nb_dyn_block);

	assembly_dst->allocation_type 		= ASSEMBLYALLOCATION_MALLOC;
	assembly_dst->mapping_size_block 	= size + sizeof(struct asmBlockHeader) * assembly_dst->nb_dyn_block;
	assembly_dst->mapping_block 		= malloc(assembly_dst->mapping_size_block);

	if (assembly_dst->dyn_blocks == NULL || assembly_dst->mapping_block == NULL){
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
		if (assembly_dst->dyn_blocks != NULL){
			free(assembly_dst->dyn_blocks);
		}
		if ( assembly_dst->mapping_block != NULL){
			free( assembly_dst->mapping_block);
		}

		return -1;
	}

	/* pour le debug */
	printf("{length=%u, offset=%u, bb_start=%u, bb_stop=%u, ins_start=%u, ins_stop=%u, nb_dyn_block=%u, size=%u}\n", length, offset, idx_block_start, idx_block_stop, idx_ins_start, idx_ins_stop, assembly_dst->nb_dyn_block, size);
	
	new_block = (struct asmBlock*)assembly_dst->mapping_block;
	if (idx_block_start == idx_block_stop){
		new_block->header.id 		= possible_new_id ++;
		new_block->header.size 		= size;
		new_block->header.nb_ins 	= length;
		new_block->header.address 	= assembly_src->dyn_blocks[idx_block_start].block->header.address + idx_ins_start;
		memcpy(&(new_block->data), (char*)&(assembly_src->dyn_blocks[idx_block_start].block->data) + idx_ins_start, new_block->header.size);
		size = sizeof(struct asmBlockHeader) + new_block->header.size;

		assembly_dst->dyn_blocks[0].instruction_count = 0;
		assembly_dst->dyn_blocks[0].block = new_block;
	}
	else{
		if (idx_ins_start == 0){
			memcpy(new_block, assembly_src->dyn_blocks[idx_block_start].block, sizeof(struct asmBlockHeader) + assembly_src->dyn_blocks[idx_block_start].block->header.size);
			size = sizeof(struct asmBlockHeader) + assembly_src->dyn_blocks[idx_block_start].block->header.size;
			
			tsearch(new_block, &bintree_root, (int(*)(const void*,const void*))assembly_compare_asmBlock_id);
		}
		else{
			new_block->header.id 		= possible_new_id ++;
			new_block->header.size 		= assembly_src->dyn_blocks[idx_block_start].block->header.size - idx_ins_start;
			new_block->header.nb_ins 	= assembly_src->dyn_blocks[idx_block_start].instruction_count +  assembly_src->dyn_blocks[idx_block_start].block->header.nb_ins - offset;
			new_block->header.address 	= assembly_src->dyn_blocks[idx_block_start].block->header.address + idx_ins_start;
			memcpy(&(new_block->data), (char*)&(assembly_src->dyn_blocks[idx_block_start].block->data) + idx_ins_start, new_block->header.size);
			size = sizeof(struct asmBlockHeader) + new_block->header.size;
		}

		assembly_dst->dyn_blocks[0].instruction_count = 0;
		assembly_dst->dyn_blocks[0].block = new_block;

		if (idx_ins_stop != 0){
			new_block = (struct asmBlock*)((char*)assembly_dst->mapping_block + size);
			new_block->header.id 		= possible_new_id ++;
			new_block->header.size 		= idx_ins_stop;
			new_block->header.nb_ins 	= assembly_src->dyn_blocks[idx_block_stop].instruction_count - ((offset + length) > assembly_src->nb_dyn_instruction) ? assembly_src->nb_dyn_instruction : (offset + length);
			new_block->header.address 	= assembly_src->dyn_blocks[idx_block_stop].block->header.address;
			memcpy(&(new_block->data), &(assembly_src->dyn_blocks[idx_block_start].block->data), idx_ins_stop);
			size += sizeof(struct asmBlockHeader) + new_block->header.size;

			assembly_dst->dyn_blocks[assembly_dst->nb_dyn_block - 1].block = new_block;
		}
	}

	for (i = idx_block_start + 1; i < idx_block_stop; i++){
		struct asmBlock 	compare_block;
		struct asmBlock** 	result_block;

		assembly_dst->dyn_blocks[i - idx_block_start].instruction_count = assembly_dst->dyn_blocks[i - idx_block_start - 1].instruction_count + assembly_dst->dyn_blocks[i - idx_block_start - 1].block->header.nb_ins;

		compare_block.header.id 		= assembly_src->dyn_blocks[i].block->header.id;
		compare_block.header.size 		= assembly_src->dyn_blocks[i].block->header.size;
		compare_block.header.nb_ins 	= assembly_src->dyn_blocks[i].block->header.nb_ins;
		compare_block.header.address 	= assembly_src->dyn_blocks[i].block->header.address;

		result_block = (struct asmBlock**)tsearch(&compare_block, &bintree_root, (int(*)(const void*,const void*))assembly_compare_asmBlock_id);
		if (result_block == NULL){
			printf("ERROR: in %s, unable to search the bin tree\n", __func__);
			return -1;
		}
		else if (*result_block == &compare_block){
			new_block = (struct asmBlock*)((char*)assembly_dst->mapping_block + size);
			memcpy(new_block, assembly_src->dyn_blocks[i].block, sizeof(struct asmBlockHeader) + assembly_src->dyn_blocks[i].block->header.size);
			size += sizeof(struct asmBlockHeader) + assembly_src->dyn_blocks[i].block->header.size;
			*result_block = new_block;
		}
		else{
			new_block = *result_block;
		}
		assembly_dst->dyn_blocks[i - idx_block_start].block = new_block;
	}

	if ((idx_block_start != idx_block_stop) && (idx_ins_stop != 0)){
		assembly_dst->dyn_blocks[assembly_dst->nb_dyn_block - 1].instruction_count = assembly_dst->dyn_blocks[assembly_dst->nb_dyn_block - 2].instruction_count + assembly_dst->dyn_blocks[assembly_dst->nb_dyn_block - 2].block->header.size;
	}

	tdestroy(bintree_root, assembly_clean_tree_node);

	return 0;
}

void assembly_clean(struct assembly* assembly){
	free(assembly->dyn_blocks);

	switch(assembly->allocation_type){
		case ASSEMBLYALLOCATION_MALLOC : {
			free(assembly->mapping_block);
			break;
		}
		case ASSEMBLYALLOCATION_MMAP : {
			munmap(assembly->mapping_block, assembly->mapping_size_block);
			break;
		}
	}
}