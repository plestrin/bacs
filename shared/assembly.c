#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <search.h>

#include "assembly.h"
#include "memTrace.h"
#include "mapFile.h"
#include "array.h"
#include "printBuffer.h"
#include "base.h"

struct disassembler{
	uint8_t 					xed_init;
	xed_machine_mode_enum_t 	mmode;
    xed_address_width_enum_t 	stack_addr_width;
};

static int32_t assembly_compare_asmBlock_id(const struct asmBlock* b1, const struct asmBlock* b2);
static void assembly_clean_tree_node(void* data);
static uint32_t assembly_get_instruction_nb_mem_access(xed_decoded_inst_t* xedd);

struct disassembler disas = {
	.xed_init 			= 0,
	.mmode 				= XED_MACHINE_MODE_LEGACY_32,
	.stack_addr_width 	= XED_ADDRESS_WIDTH_32b
};

#define disassembler_init() 			\
	if (disas.xed_init == 0){ 			\
		xed_tables_init(); 				\
		disas.xed_init = 1; 			\
	}

uint32_t asmBlock_count_nb_ins(struct asmBlock* block){
	xed_decoded_inst_t 		xedd;
	xed_error_enum_t 		xed_error;
	uint32_t 				offset;
	uint32_t 				result;

	for (offset = 0, result = 0; offset < block->header.size; offset += xed_decoded_inst_get_length(&xedd), result ++){
		xed_decoded_inst_zero(&xedd);
		xed_decoded_inst_set_mode(&xedd, disas.mmode, disas.stack_addr_width);
		if ((xed_error = xed_decode(&xedd, (const xed_uint8_t*)(block->data + offset), min(block->header.size - offset, 15))) != XED_ERROR_NONE){
			log_err_m("xed decode error: %s", xed_error_enum_t2str(xed_error));
			return -1;
		}
	}

	return result;
}

uint8_t* asmBlock_search_instruction(struct asmBlock* block, const xed_iclass_enum_t* buffer, uint32_t buffer_length, xed_decoded_inst_t* xedd, uint32_t offset){
	xed_error_enum_t 	xed_error;
	uint32_t 			result;
	uint32_t 			i;

	for (result = 0; offset < block->header.size; offset += xed_decoded_inst_get_length(xedd), result ++){
		xed_decoded_inst_zero(xedd);
		xed_decoded_inst_set_mode(xedd, disas.mmode, disas.stack_addr_width);
		if ((xed_error = xed_decode(xedd, (const xed_uint8_t*)(block->data + offset), min(block->header.size - offset, 15))) != XED_ERROR_NONE){
			log_err_m("xed decode error: %s", xed_error_enum_t2str(xed_error));
			return NULL;
		}

		for (i = 0; i < buffer_length; i++){
			if (buffer[i] == xed_decoded_inst_get_iclass(xedd)){
				return block->data + offset;
			}
		}
	}

	return NULL;
}

int32_t assembly_load_trace(struct assembly* assembly, const char* file_name_id, const char* file_name_block){
	size_t 		mapping_size_id;
	size_t 		mapping_size_block;
	uint32_t* 	mapping_id;
	void* 		mapping_block;
	int32_t 	result;

	mapping_id = mapFile_map(file_name_id, &mapping_size_id);
	mapping_block = mapFile_map(file_name_block, &mapping_size_block);

	if (mapping_id == NULL || mapping_block == NULL){
		log_err("unable to map files");
		if (mapping_id != NULL){
			munmap(mapping_id, mapping_size_id);
		}
		if (mapping_block != NULL){
			munmap(mapping_block, mapping_size_block);
		}
		return -1;
	}

	result = assembly_init(assembly, mapping_id, mapping_size_id, mapping_block, mapping_size_block, ALLOCATION_MMAP);

	munmap(mapping_id, mapping_size_id);

	return result;
}

int32_t assembly_init(struct assembly* assembly, const uint32_t* buffer_id, size_t buffer_size_id, uint32_t* buffer_block, size_t buffer_size_block, enum allocationType buffer_alloc_block){
	uint32_t 			i;
	uint32_t 			j;
	struct array 		asmBlock_array;
	struct asmBlock* 	current_ptr;
	size_t 				current_offset;
	struct dynBlock* 	dyn_blocks_realloc;

	disassembler_init()

	assembly->allocation_type 		= buffer_alloc_block;
	assembly->mapping_block 		= buffer_block;
	assembly->mapping_size_block 	= buffer_size_block;
	assembly->nb_dyn_block 			= buffer_size_id / sizeof(uint32_t);
	assembly->dyn_blocks 			= (struct dynBlock*)malloc(sizeof(struct dynBlock) * assembly->nb_dyn_block);
	if (assembly->dyn_blocks == NULL){
		log_err("unable to allocate memory");
		switch(buffer_alloc_block){
			case ALLOCATION_MALLOC 	: {free(buffer_block); break;}
			case ALLOCATION_MMAP 	: {munmap(buffer_block, buffer_size_block); break;}
		}
		return -1;
	}

	if (array_init(&asmBlock_array, sizeof(struct asmBlock*))){
		log_err("unable to init array");
		free(assembly->dyn_blocks);
		switch(buffer_alloc_block){
			case ALLOCATION_MALLOC 	: {free(buffer_block); break;}
			case ALLOCATION_MMAP 	: {munmap(buffer_block, buffer_size_block); break;}
		}
		return -1;
	}

	current_offset = 0;
	current_ptr = (struct asmBlock*)((char*)buffer_block + current_offset);
	while (current_offset != buffer_size_block){
		if (current_offset + current_ptr->header.size + sizeof(struct asmBlockHeader) > buffer_size_block){
			log_err("the last asmBlock is incomplete");
			break;
		}
		else{
			if (array_add(&asmBlock_array, &current_ptr) < 0){
				log_err("unable to add asmBlock pointer to array");
			}

			current_offset += sizeof(struct asmBlockHeader) + current_ptr->header.size;
			current_ptr = (struct asmBlock*)((char*)buffer_block + current_offset);
		}
	}

	for (i = 0, j = 0; i < assembly->nb_dyn_block; i++){
		uint32_t 			up = array_get_length(&asmBlock_array);
		uint32_t 			down = 0;
		uint32_t 			idx;
		struct asmBlock* 	idx_block;

		if (buffer_id[i] != BLACK_LISTED_ID){
			current_ptr = NULL;
			while(down < up){
				idx  = (up + down) / 2;
				idx_block = *(struct asmBlock**)array_get(&asmBlock_array, idx);
				if (buffer_id[i] > idx_block->header.id){
					down = idx + 1;
				}
				else if (buffer_id[i] < idx_block->header.id){
					up = idx;
				}
				else{
					current_ptr = idx_block;
					break;
				}
			}

			if (current_ptr == NULL){
				log_err_m("unable to locate asmBlock %u", buffer_id[i]);
				break;
			}

			if (j == 0){
				assembly->dyn_blocks[j].instruction_count = 0;
				assembly->dyn_blocks[j].mem_access_count = 0;
			}
			else if (dynBlock_is_valid(assembly->dyn_blocks + (j - 1))){
				assembly->dyn_blocks[j].instruction_count = assembly->dyn_blocks[j - 1].instruction_count + assembly->dyn_blocks[j - 1].block->header.nb_ins;
				assembly->dyn_blocks[j].mem_access_count = assembly->dyn_blocks[j - 1].mem_access_count + ((assembly->dyn_blocks[j - 1].block->header.nb_mem_access == UNTRACK_MEM_ACCESS) ? 0 : assembly->dyn_blocks[j - 1].block->header.nb_mem_access);
			}
			else if (j > 1){
				assembly->dyn_blocks[j].instruction_count = assembly->dyn_blocks[j - 2].instruction_count + assembly->dyn_blocks[j - 2].block->header.nb_ins;
				assembly->dyn_blocks[j].mem_access_count = assembly->dyn_blocks[j - 2].mem_access_count + ((assembly->dyn_blocks[j - 2].block->header.nb_mem_access == UNTRACK_MEM_ACCESS) ? 0 : assembly->dyn_blocks[j - 2].block->header.nb_mem_access);
			}
			else{
				assembly->dyn_blocks[j].instruction_count = 0;
				assembly->dyn_blocks[j].mem_access_count = 0;
			}
			assembly->dyn_blocks[j].block = current_ptr;
			j ++;
		}
		else if (i == 0 || (i != 0 && buffer_id[i - 1] != BLACK_LISTED_ID)){
			dynBlock_set_invalid(assembly->dyn_blocks + j);
			if (j){
				assembly->dyn_blocks[j].instruction_count = assembly->dyn_blocks[j - 1].instruction_count + assembly->dyn_blocks[j - 1].block->header.nb_ins;
				assembly->dyn_blocks[j].mem_access_count = assembly->dyn_blocks[j - 1].mem_access_count + ((assembly->dyn_blocks[j - 1].block->header.nb_mem_access == UNTRACK_MEM_ACCESS) ? 0 : assembly->dyn_blocks[j - 1].block->header.nb_mem_access);
			}
			else{
				assembly->dyn_blocks[j].instruction_count = 0;
				assembly->dyn_blocks[j].mem_access_count = 0;
			}
			j ++;
		}
	}

	dyn_blocks_realloc = (struct dynBlock*)realloc(assembly->dyn_blocks, sizeof(struct dynBlock) * j);
	if (dyn_blocks_realloc == NULL){
		log_err("unable to realloc memory");
	}
	else{
		assembly->dyn_blocks = dyn_blocks_realloc;
		assembly->nb_dyn_block = j;
	}

	if(dynBlock_is_valid(assembly->dyn_blocks + (assembly->nb_dyn_block - 1))){
		assembly->nb_dyn_instruction = assembly->dyn_blocks[assembly->nb_dyn_block - 1].instruction_count + assembly->dyn_blocks[assembly->nb_dyn_block - 1].block->header.nb_ins;
	}
	else if (assembly->nb_dyn_block > 1){
		assembly->nb_dyn_instruction = assembly->dyn_blocks[assembly->nb_dyn_block - 2].instruction_count + assembly->dyn_blocks[assembly->nb_dyn_block - 2].block->header.nb_ins;
	}
	else{
		assembly->nb_dyn_instruction = 0;
	}

	array_clean(&asmBlock_array);

	return 0;
}

int32_t assembly_get_instruction(struct assembly* assembly, struct instructionIterator* it, uint32_t index){
	uint32_t 			up 		= assembly->nb_dyn_block;
	uint32_t 			down 	= 0;
	uint32_t 			idx;
	uint32_t 			found 	= 0;
	uint32_t 			i;
	xed_error_enum_t 	xed_error;

	while(down < up){
		idx  = (up + down) / 2;
		if (dynBlock_is_invalid(assembly->dyn_blocks + idx)){
			if (idx != down){
				idx --;
			}
			else if (idx + 1 != assembly->nb_dyn_block){
				idx ++;
			}
			else{
				break;
			}
		}
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
			if (it->instruction_sub_index == 0 && idx > 0 && dynBlock_is_invalid(assembly->dyn_blocks + (idx - 1))){
				it->prev_black_listed 	= 1;
			}
			else{
				it->prev_black_listed 	= 0;
			}
			if (assembly->dyn_blocks[it->dyn_block_index].block->header.nb_mem_access == UNTRACK_MEM_ACCESS){
				it->mem_access_valid 	= 0;
			}
			else{
				it->mem_access_valid 	= 1;
			}
			it->mem_access_index 		= assembly->dyn_blocks[idx].mem_access_count;
			found = 1;
			break;
		}
	}

	if (!found){
		log_err_m("unable to locate the dyn block containing the index %u (tot nb instruction: %u)", index, assembly->nb_dyn_instruction);
		return -1;
	}

	for (i = 0; i <= it->instruction_sub_index; i++){
		xed_decoded_inst_zero(&(it->xedd));
		xed_decoded_inst_set_mode(&(it->xedd), disas.mmode, disas.stack_addr_width);
		if ((xed_error = xed_decode(&(it->xedd), (const xed_uint8_t*)(assembly->dyn_blocks[it->dyn_block_index].block->data + it->instruction_offset), min(assembly->dyn_blocks[it->dyn_block_index].block->header.size - it->instruction_offset, 15))) != XED_ERROR_NONE){
			log_err_m("xed decode error: %s", xed_error_enum_t2str(xed_error));
			return -1;
		}

		if (i == it->instruction_sub_index){
			it->instruction_size = xed_decoded_inst_get_length(&(it->xedd));
			it->instruction_address = assembly->dyn_blocks[it->dyn_block_index].block->header.address + it->instruction_offset;
		}
		else{
			it->instruction_offset += xed_decoded_inst_get_length(&(it->xedd));

			if (assembly->dyn_blocks[it->dyn_block_index].block->header.nb_mem_access == UNTRACK_MEM_ACCESS){
				it->mem_access_index += assembly_get_instruction_nb_mem_access(&(it->xedd));
			}
		}
	}

	return 0;
}

int32_t assembly_get_next_instruction(struct assembly* assembly, struct instructionIterator* it){
	xed_error_enum_t xed_error;

	if (it->instruction_index + 1 >= assembly->dyn_blocks[it->dyn_block_index].instruction_count + assembly->dyn_blocks[it->dyn_block_index].block->header.nb_ins){
		if (it->dyn_block_index + 1 < assembly->nb_dyn_block){
			if (dynBlock_is_valid(assembly->dyn_blocks + (it->dyn_block_index + 1))){
				it->instruction_index 		= it->instruction_index + 1;
				it->dyn_block_index 		= it->dyn_block_index + 1;
				it->instruction_sub_index 	= 0;
				it->instruction_offset 		= 0;
				it->prev_black_listed 		= 0;
				if (assembly->dyn_blocks[it->dyn_block_index].block->header.nb_mem_access == UNTRACK_MEM_ACCESS){
					it->mem_access_valid 	= 0;
				}
				else{
					it->mem_access_valid 	= 1;
				}
				it->mem_access_index 		= assembly->dyn_blocks[it->dyn_block_index].mem_access_count;
			}
			else if (it->dyn_block_index + 2 < assembly->nb_dyn_block){
				it->instruction_index 		= it->instruction_index + 1;
				it->dyn_block_index 		= it->dyn_block_index + 2;
				it->instruction_sub_index 	= 0;
				it->instruction_offset 		= 0;
				it->prev_black_listed 		= 1;
				if (assembly->dyn_blocks[it->dyn_block_index].block->header.nb_mem_access == UNTRACK_MEM_ACCESS){
					it->mem_access_valid 	= 0;
				}
				else{
					it->mem_access_valid 	= 1;
				}
				it->mem_access_index 		= assembly->dyn_blocks[it->dyn_block_index].mem_access_count;
			}
			else{
				log_err("the last instruction has been reached");
				return -1;
			}
		}
		else{
			log_err("the last instruction has been reached");
			return -1;
		}
	}
	else{
		it->instruction_index 		= it->instruction_index + 1;
		it->instruction_sub_index 	= it->instruction_sub_index + 1;
		it->instruction_offset 		= it->instruction_offset + it->instruction_size;
		it->prev_black_listed		= 0;

		if (assembly->dyn_blocks[it->dyn_block_index].block->header.nb_mem_access != UNTRACK_MEM_ACCESS){
			it->mem_access_index += assembly_get_instruction_nb_mem_access(&(it->xedd));
		}
	}

	xed_decoded_inst_zero(&(it->xedd));
	xed_decoded_inst_set_mode(&(it->xedd), disas.mmode, disas.stack_addr_width);
	if ((xed_error = xed_decode(&(it->xedd), (const xed_uint8_t*)(assembly->dyn_blocks[it->dyn_block_index].block->data + it->instruction_offset), min(assembly->dyn_blocks[it->dyn_block_index].block->header.size - it->instruction_offset, 15))) != XED_ERROR_NONE){
		log_err_m("xed decode error: %s", xed_error_enum_t2str(xed_error));
		return -1;
	}

	it->instruction_size = xed_decoded_inst_get_length(&(it->xedd));
	it->instruction_address = assembly->dyn_blocks[it->dyn_block_index].block->header.address + it->instruction_offset;

	return 0;
}

int32_t assembly_get_last_instruction(struct asmBlock* block, xed_decoded_inst_t* xedd){
	uint32_t instruction_offset;

	for (instruction_offset = 0; instruction_offset != block->header.size; ){
		xed_decoded_inst_zero(xedd);
		xed_decoded_inst_set_mode(xedd, disas.mmode, disas.stack_addr_width);
		if (xed_decode(xedd, (const xed_uint8_t*)(block->data + instruction_offset), min(block->header.size - instruction_offset, 15)) == XED_ERROR_NONE){
			instruction_offset += xed_decoded_inst_get_length(xedd);
		}
		else{
			log_err_m("unable to decode instruction in block %u. Run \"Check trace\" for more information", block->header.id);
			return -1;
		}
	}

	return 0;
}

int32_t assembly_check(struct assembly* assembly){
	uint32_t 			block_offset;
	struct asmBlock* 	block;
	uint32_t 			block_count 		= 0;
	uint32_t 			nb_instruction;
	uint32_t 			instruction_offset;
	xed_error_enum_t 	xed_error;
	xed_decoded_inst_t 	xedd;
	uint32_t 			nb_mem_access;
	int32_t 			result 				= 0;

	for (block_offset = 0, block_count = 0; block_offset != assembly->mapping_size_block; block_offset += sizeof(struct asmBlockHeader) + block->header.size, block_count++){
		block = (struct asmBlock*)((char*)assembly->mapping_block + block_offset);
		if (block_offset + block->header.size + sizeof(struct asmBlockHeader) > assembly->mapping_size_block){
			log_err("the last asmBlock is incomplete");
			break;
		}

		for (nb_instruction = 0, nb_mem_access = 0, instruction_offset = 0; instruction_offset != block->header.size; nb_instruction ++, instruction_offset += xed_decoded_inst_get_length(&xedd)){
			xed_decoded_inst_zero(&xedd);
			xed_decoded_inst_set_mode(&xedd, disas.mmode, disas.stack_addr_width);
			if ((xed_error = xed_decode(&xedd, (const xed_uint8_t*)(block->data + instruction_offset), min(block->header.size - instruction_offset, 15))) != XED_ERROR_NONE){
				log_err_m("xed decode error: %s", xed_error_enum_t2str(xed_error));
				result = -1;
				break;
			}

			if (block->header.nb_mem_access != UNTRACK_MEM_ACCESS){
				nb_mem_access += assembly_get_instruction_nb_mem_access(&xedd);
			}
		}

		if (nb_instruction != block->header.nb_ins){
			log_err_m("basic block %u contains %u instruction(s), expecting: %u", block_count, nb_instruction, block->header.nb_ins);
		}

		if (block->header.nb_mem_access != UNTRACK_MEM_ACCESS && nb_mem_access != block->header.nb_mem_access){
			log_err_m("basic block %u contains %u memory access(es), expecting: %u", block_count, nb_mem_access, block->header.nb_mem_access);
		}
	}

	#ifdef VERBOSE
	log_info_m("%u basic blocks have been checked", block_count);
	#endif

	return result;
}

static int32_t assembly_compare_asmBlock_id(const struct asmBlock* b1, const struct asmBlock* b2){
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

int32_t assembly_extract_segment(struct assembly* assembly_src, struct assembly* assembly_dst, uint32_t offset, uint32_t length, uint64_t* index_mem_access_start, uint64_t* index_mem_access_stop){
	uint32_t 			up 					= assembly_src->nb_dyn_block;
	uint32_t 			down 				= 0;
	uint32_t 			idx_block_start;
	uint32_t 			idx_block_stop;
	uint32_t 			idx_ins_start;
	uint32_t 			idx_ins_stop;
	uint32_t 			mem_access_start;
	uint32_t 			mem_access_stop;
	uint8_t 			found 				= 0;
	uint32_t 			i;
	uint32_t 			size;
	xed_decoded_inst_t 	xedd;
	xed_error_enum_t 	xed_error;
	uint32_t 			possible_new_id 	= 0;
	struct asmBlock* 	new_block;
	void* 				bintree_root 		= NULL;
	void* 				realloc_mapping;

	while(down < up){
		idx_block_start  = (up + down) / 2;
		if (!dynBlock_is_valid(assembly_src->dyn_blocks + idx_block_start)){
			if (idx_block_start != down){
				idx_block_start --;
			}
			else{
				idx_block_start ++;
			}
		}
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
		log_err_m("unable to locate the dyn block containing the index %u", offset);
		return -1;
	}

	if (assembly_src->dyn_blocks[idx_block_start].block->header.nb_mem_access != UNTRACK_MEM_ACCESS){
		mem_access_start = 0;
	}
	else{
		mem_access_start = UNTRACK_MEM_ACCESS;
	}

	for (i = 0, idx_ins_start = 0, size = assembly_src->dyn_blocks[idx_block_start].block->header.size; i < offset - assembly_src->dyn_blocks[idx_block_start].instruction_count; i++){
		xed_decoded_inst_zero(&xedd);
		xed_decoded_inst_set_mode(&xedd, disas.mmode, disas.stack_addr_width);
		if ((xed_error = xed_decode(&xedd, (const xed_uint8_t*)(assembly_src->dyn_blocks[idx_block_start].block->data + idx_ins_start), min(assembly_src->dyn_blocks[idx_block_start].block->header.size - idx_ins_start, 15))) != XED_ERROR_NONE){
			log_err_m("xed decode error: %s", xed_error_enum_t2str(xed_error));
			return -1;
		}

		idx_ins_start += xed_decoded_inst_get_length(&xedd);

		if (assembly_src->dyn_blocks[idx_block_start].block->header.nb_mem_access != UNTRACK_MEM_ACCESS){
			mem_access_start += assembly_get_instruction_nb_mem_access(&xedd);
		}
	}

	size = assembly_src->dyn_blocks[idx_block_start].block->header.size - idx_ins_start;

	if (offset + length > assembly_src->nb_dyn_instruction){
		log_warn_m("length is too big (%u) -> cropping", offset + length);
		length = assembly_src->nb_dyn_instruction - offset;
	}


	for (idx_block_stop = idx_block_start; idx_block_stop < assembly_src->nb_dyn_block && offset + length > assembly_src->dyn_blocks[idx_block_stop].instruction_count + assembly_src->dyn_blocks[idx_block_stop].block->header.nb_ins; ){
		possible_new_id = max(assembly_src->dyn_blocks[idx_block_stop].block->header.id + 1, possible_new_id);
		size += assembly_src->dyn_blocks[idx_block_stop].block->header.size;
		idx_block_stop ++;

		while (idx_block_stop < assembly_src->nb_dyn_block && !dynBlock_is_valid(assembly_src->dyn_blocks + idx_block_stop)){
			idx_block_stop ++;
		}
	}

	if (idx_block_stop == assembly_src->nb_dyn_block){
		log_err_m("unable to locate the dyn block containing the index %u", offset + length);
		return -1;
	}

	possible_new_id = max(assembly_src->dyn_blocks[idx_block_stop].block->header.id + 1, possible_new_id);

	if (assembly_src->dyn_blocks[idx_block_stop].block->header.nb_mem_access != UNTRACK_MEM_ACCESS){
		mem_access_stop = 0;
	}
	else{
		mem_access_stop = UNTRACK_MEM_ACCESS;
	}

	for (i = 0, idx_ins_stop = 0; i < offset + length - assembly_src->dyn_blocks[idx_block_stop].instruction_count; i++){
		xed_decoded_inst_zero(&xedd);
		xed_decoded_inst_set_mode(&xedd, disas.mmode, disas.stack_addr_width);
		if ((xed_error = xed_decode(&xedd, (const xed_uint8_t*)(assembly_src->dyn_blocks[idx_block_stop].block->data + idx_ins_stop), min(assembly_src->dyn_blocks[idx_block_stop].block->header.size - idx_ins_stop, 15))) != XED_ERROR_NONE){
			log_err_m("xed decode error: %s", xed_error_enum_t2str(xed_error));
			return -1;
		}

		if (assembly_src->dyn_blocks[idx_block_start].block->header.nb_mem_access != UNTRACK_MEM_ACCESS){
			mem_access_stop += assembly_get_instruction_nb_mem_access(&xedd);
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

	assembly_dst->allocation_type 		= ALLOCATION_MALLOC;
	assembly_dst->mapping_size_block 	= size + sizeof(struct asmBlockHeader) * assembly_dst->nb_dyn_block;
	assembly_dst->mapping_block 		= malloc(assembly_dst->mapping_size_block);

	if (assembly_dst->dyn_blocks == NULL || assembly_dst->mapping_block == NULL){
		log_err("unable to allocate memory");
		if (assembly_dst->dyn_blocks != NULL){
			free(assembly_dst->dyn_blocks);
		}
		if (assembly_dst->mapping_block != NULL){
			free(assembly_dst->mapping_block);
		}

		return -1;
	}

	new_block = (struct asmBlock*)assembly_dst->mapping_block;
	if (idx_block_start == idx_block_stop){
		new_block->header.id 				= possible_new_id ++;
		new_block->header.size 				= size;
		new_block->header.nb_ins 			= length;
		if (assembly_src->dyn_blocks[idx_block_stop].block->header.nb_mem_access == UNTRACK_MEM_ACCESS){
			new_block->header.nb_mem_access = UNTRACK_MEM_ACCESS;
		}
		else{
			new_block->header.nb_mem_access = mem_access_stop - mem_access_start;
		}
		new_block->header.address 			= assembly_src->dyn_blocks[idx_block_start].block->header.address + idx_ins_start;
		memcpy(&(new_block->data), (char*)&(assembly_src->dyn_blocks[idx_block_start].block->data) + idx_ins_start, new_block->header.size);
		size = sizeof(struct asmBlockHeader) + new_block->header.size;

		assembly_dst->dyn_blocks[0].instruction_count = 0;
		assembly_dst->dyn_blocks[0].mem_access_count = 0;
		assembly_dst->dyn_blocks[0].block = new_block;
	}
	else{
		if (idx_ins_start == 0){
			memcpy(new_block, assembly_src->dyn_blocks[idx_block_start].block, sizeof(struct asmBlockHeader) + assembly_src->dyn_blocks[idx_block_start].block->header.size);
			size = sizeof(struct asmBlockHeader) + assembly_src->dyn_blocks[idx_block_start].block->header.size;

			tsearch(new_block, &bintree_root, (int(*)(const void*,const void*))assembly_compare_asmBlock_id);
		}
		else{
			new_block->header.id 				= possible_new_id ++;
			new_block->header.size 				= assembly_src->dyn_blocks[idx_block_start].block->header.size - idx_ins_start;
			new_block->header.nb_ins 			= assembly_src->dyn_blocks[idx_block_start].instruction_count +  assembly_src->dyn_blocks[idx_block_start].block->header.nb_ins - offset;
			if (assembly_src->dyn_blocks[idx_block_start].block->header.nb_mem_access == UNTRACK_MEM_ACCESS){
				new_block->header.nb_mem_access = UNTRACK_MEM_ACCESS;
			}
			else{
				new_block->header.nb_mem_access = assembly_src->dyn_blocks[idx_block_start].block->header.nb_mem_access - mem_access_start;
			}
			new_block->header.address 			= assembly_src->dyn_blocks[idx_block_start].block->header.address + idx_ins_start;
			memcpy(&(new_block->data), (char*)&(assembly_src->dyn_blocks[idx_block_start].block->data) + idx_ins_start, new_block->header.size);
			size = sizeof(struct asmBlockHeader) + new_block->header.size;
		}

		assembly_dst->dyn_blocks[0].instruction_count = 0;
		assembly_dst->dyn_blocks[0].mem_access_count = 0;
		assembly_dst->dyn_blocks[0].block = new_block;

		if (idx_ins_stop != 0){
			new_block = (struct asmBlock*)((char*)assembly_dst->mapping_block + size);
			new_block->header.id 				= possible_new_id ++;
			new_block->header.size 				= idx_ins_stop;
			new_block->header.nb_ins 			= (((offset + length) > assembly_src->nb_dyn_instruction) ? assembly_src->nb_dyn_instruction : (offset + length)) - assembly_src->dyn_blocks[idx_block_stop].instruction_count;
			if (assembly_src->dyn_blocks[idx_block_stop].block->header.nb_mem_access == UNTRACK_MEM_ACCESS){
				new_block->header.nb_mem_access = UNTRACK_MEM_ACCESS;
			}
			else{
				new_block->header.nb_mem_access = mem_access_stop;
			}
			new_block->header.address 			= assembly_src->dyn_blocks[idx_block_stop].block->header.address;
			memcpy(&(new_block->data), &(assembly_src->dyn_blocks[idx_block_stop].block->data), idx_ins_stop);
			size += sizeof(struct asmBlockHeader) + new_block->header.size;

			assembly_dst->dyn_blocks[assembly_dst->nb_dyn_block - 1].block = new_block;
		}
	}

	for (i = idx_block_start + 1; i < idx_block_stop; i++){
		struct asmBlock 	compare_block;
		struct asmBlock** 	result_block;

		if (dynBlock_is_valid(assembly_src->dyn_blocks + i)){
			if (dynBlock_is_valid(assembly_dst->dyn_blocks + (i - idx_block_start - 1))){
				assembly_dst->dyn_blocks[i - idx_block_start].instruction_count = assembly_dst->dyn_blocks[i - idx_block_start - 1].instruction_count + assembly_dst->dyn_blocks[i - idx_block_start - 1].block->header.nb_ins;
				assembly_dst->dyn_blocks[i - idx_block_start].mem_access_count = assembly_dst->dyn_blocks[i - idx_block_start - 1].mem_access_count + ((assembly_dst->dyn_blocks[i - idx_block_start - 1].block->header.nb_mem_access == UNTRACK_MEM_ACCESS) ? 0 : assembly_dst->dyn_blocks[i - idx_block_start - 1].block->header.nb_mem_access);
			}
			else{
				assembly_dst->dyn_blocks[i - idx_block_start].instruction_count = assembly_dst->dyn_blocks[i - idx_block_start - 2].instruction_count + assembly_dst->dyn_blocks[i - idx_block_start - 2].block->header.nb_ins;
				assembly_dst->dyn_blocks[i - idx_block_start].mem_access_count = assembly_dst->dyn_blocks[i - idx_block_start - 2].mem_access_count + ((assembly_dst->dyn_blocks[i - idx_block_start - 2].block->header.nb_mem_access == UNTRACK_MEM_ACCESS) ? 0 : assembly_dst->dyn_blocks[i - idx_block_start - 2].block->header.nb_mem_access);
			}
			compare_block.header.id 			= assembly_src->dyn_blocks[i].block->header.id;
			compare_block.header.size 			= assembly_src->dyn_blocks[i].block->header.size;
			compare_block.header.nb_ins 		= assembly_src->dyn_blocks[i].block->header.nb_ins;
			compare_block.header.nb_mem_access 	= assembly_src->dyn_blocks[i].block->header.nb_mem_access;
			compare_block.header.address 		= assembly_src->dyn_blocks[i].block->header.address;

			result_block = (struct asmBlock**)tsearch(&compare_block, &bintree_root, (int(*)(const void*,const void*))assembly_compare_asmBlock_id);
			if (result_block == NULL){
				log_err("unable to search the bin tree");
				return -1;
			}
			else if (*result_block == &compare_block){
				new_block = (struct asmBlock*)((char*)assembly_dst->mapping_block + size);
				memcpy(new_block, assembly_src->dyn_blocks[i].block, sizeof(struct asmBlockHeader) + assembly_src->dyn_blocks[i].block->header.size);
				size += sizeof(struct asmBlockHeader) + assembly_src->dyn_blocks[i].block->header.size;
				*result_block = new_block;
			}
			assembly_dst->dyn_blocks[i - idx_block_start].block = *result_block;
		}
		else{
			dynBlock_set_invalid(assembly_dst->dyn_blocks + (i - idx_block_start));
		}
	}

	if ((idx_block_start != idx_block_stop) && (idx_ins_stop != 0)){
		if (dynBlock_is_valid(assembly_dst->dyn_blocks + (assembly_dst->nb_dyn_block - 2))){
			assembly_dst->dyn_blocks[assembly_dst->nb_dyn_block - 1].instruction_count = assembly_dst->dyn_blocks[assembly_dst->nb_dyn_block - 2].instruction_count + assembly_dst->dyn_blocks[assembly_dst->nb_dyn_block - 2].block->header.nb_ins;
			assembly_dst->dyn_blocks[assembly_dst->nb_dyn_block - 1].mem_access_count = assembly_dst->dyn_blocks[assembly_dst->nb_dyn_block - 2].mem_access_count + ((assembly_dst->dyn_blocks[assembly_dst->nb_dyn_block - 2].block->header.nb_mem_access == UNTRACK_MEM_ACCESS) ? 0 : assembly_dst->dyn_blocks[assembly_dst->nb_dyn_block - 2].block->header.nb_mem_access);
		}
		else{
			assembly_dst->dyn_blocks[assembly_dst->nb_dyn_block - 1].instruction_count = assembly_dst->dyn_blocks[assembly_dst->nb_dyn_block - 3].instruction_count + assembly_dst->dyn_blocks[assembly_dst->nb_dyn_block - 3].block->header.nb_ins;
			assembly_dst->dyn_blocks[assembly_dst->nb_dyn_block - 1].mem_access_count = assembly_dst->dyn_blocks[assembly_dst->nb_dyn_block - 3].mem_access_count + ((assembly_dst->dyn_blocks[assembly_dst->nb_dyn_block - 3].block->header.nb_mem_access == UNTRACK_MEM_ACCESS) ? 0 : assembly_dst->dyn_blocks[assembly_dst->nb_dyn_block - 3].block->header.nb_mem_access);
		}
	}

	tdestroy(bintree_root, assembly_clean_tree_node);

	realloc_mapping = realloc(assembly_dst->mapping_block, size);
	if (realloc_mapping == NULL){
		log_err("unable to realloc memory");
	}
	else if (realloc_mapping != assembly_dst->mapping_block){
		for (i = 0; i < assembly_dst->nb_dyn_block; i++){
			if (dynBlock_is_valid(assembly_dst->dyn_blocks + i)){
				assembly_dst->dyn_blocks[i].block = (struct asmBlock*)((char*)realloc_mapping + (uint32_t)((char*)assembly_dst->dyn_blocks[i].block - (char*)assembly_dst->mapping_block));
			}
		}
		assembly_dst->mapping_block = realloc_mapping;
	}
	assembly_dst->mapping_size_block = size;

	if (index_mem_access_start != NULL){
		*index_mem_access_start = assembly_src->dyn_blocks[idx_block_start].mem_access_count + mem_access_start;
	}
	if (index_mem_access_stop != NULL){
		*index_mem_access_stop = assembly_src->dyn_blocks[idx_block_stop].mem_access_count + mem_access_stop;
	}

	return 0;
}

int32_t assembly_concat(struct assembly** assembly_src_buffer, uint32_t nb_assembly_src, struct assembly* assembly_dst){
	uint32_t i;
	uint32_t j;
	uint32_t nb_dyn_instruction 	= 0;
	uint32_t nb_dyn_block 			= 0;
	uint32_t size 					= 0;
	uint32_t id_generator 			= 1;
	uint32_t mapping_block_offset;

	for (i = 0; i < nb_assembly_src; i++){
		nb_dyn_instruction 	+= assembly_src_buffer[i]->nb_dyn_instruction;
		nb_dyn_block		+= assembly_src_buffer[i]->nb_dyn_block;
		size 				+= assembly_src_buffer[i]->mapping_size_block;
	}

	assembly_dst->nb_dyn_instruction 	= nb_dyn_instruction;
	assembly_dst->nb_dyn_block 			= nb_dyn_block;
	assembly_dst->dyn_blocks 			= (struct dynBlock*)malloc(sizeof(struct dynBlock) * assembly_dst->nb_dyn_block);

	assembly_dst->allocation_type 		= ALLOCATION_MALLOC;
	assembly_dst->mapping_size_block 	= size;
	assembly_dst->mapping_block 		= malloc(assembly_dst->mapping_size_block);

	if (assembly_dst->dyn_blocks == NULL || assembly_dst->mapping_block == NULL){
		log_err("unable to allocate memory");
		if (assembly_dst->dyn_blocks != NULL){
			free(assembly_dst->dyn_blocks);
		}
		if (assembly_dst->mapping_block != NULL){
			free(assembly_dst->mapping_block);
		}

		return -1;
	}

	nb_dyn_instruction 	= 0;
	nb_dyn_block 		= 0;
	size 				= 0;

	for (i = 0; i < nb_assembly_src; i++){
		memcpy((char*)assembly_dst->mapping_block + size, assembly_src_buffer[i]->mapping_block, assembly_src_buffer[i]->mapping_size_block);
		mapping_block_offset = size;

		while(mapping_block_offset - size < assembly_src_buffer[i]->mapping_size_block){
			((struct asmBlock*)((char*)assembly_dst->mapping_block + mapping_block_offset))->header.id = id_generator ++;
			mapping_block_offset += ((struct asmBlock*)((char*)assembly_dst->mapping_block + mapping_block_offset))->header.size + sizeof(struct asmBlockHeader);
		}

		for (j = 0; j < assembly_src_buffer[i]->nb_dyn_block; j++){
			assembly_dst->dyn_blocks[nb_dyn_block + j].instruction_count = nb_dyn_instruction + assembly_src_buffer[i]->dyn_blocks[j].instruction_count;
			if (dynBlock_is_valid(assembly_src_buffer[i]->dyn_blocks + j)){
				assembly_dst->dyn_blocks[nb_dyn_block + j].block = (struct asmBlock*)((char*)assembly_dst->mapping_block + size + ((char*)assembly_src_buffer[i]->dyn_blocks[j].block - (char*)assembly_src_buffer[i]->mapping_block));
			}
			else{
				assembly_dst->dyn_blocks[nb_dyn_block + j].block = NULL;
			}

			if (nb_dyn_block + j == 0){
				assembly_dst->dyn_blocks[nb_dyn_block + j].mem_access_count = 0;	
			}
			else if (dynBlock_is_valid(assembly_dst->dyn_blocks + nb_dyn_block + j - 1)){
				assembly_dst->dyn_blocks[nb_dyn_block + j].mem_access_count = assembly_dst->dyn_blocks[nb_dyn_block + j - 1].mem_access_count + ((assembly_dst->dyn_blocks[nb_dyn_block + j - 1].block->header.nb_mem_access == UNTRACK_MEM_ACCESS) ? 0 : assembly_dst->dyn_blocks[nb_dyn_block + j - 1].block->header.nb_mem_access);
			}
			else if (nb_dyn_block + j > 1){
				assembly_dst->dyn_blocks[nb_dyn_block + j].mem_access_count = assembly_dst->dyn_blocks[nb_dyn_block + j - 2].mem_access_count + ((assembly_dst->dyn_blocks[nb_dyn_block + j - 2].block->header.nb_mem_access == UNTRACK_MEM_ACCESS) ? 0 : assembly_dst->dyn_blocks[nb_dyn_block + j - 2].block->header.nb_mem_access);
			}
			else{
				assembly_dst->dyn_blocks[nb_dyn_block + j].mem_access_count = 0;
			}
		}

		nb_dyn_instruction 	+= assembly_src_buffer[i]->nb_dyn_instruction;
		nb_dyn_block 		+= assembly_src_buffer[i]->nb_dyn_block;
		size 				+= assembly_src_buffer[i]->mapping_size_block;
	}

	return 0;
}

static uint32_t assembly_get_instruction_nb_mem_access(xed_decoded_inst_t* xedd){
	uint32_t 				nb_mem_access;
	uint32_t 				i;
	const xed_inst_t* 		xi;
	const xed_operand_t* 	xed_op;

	if (xed_decoded_inst_get_iclass(xedd) == XED_ICLASS_NOP){
		return 0;
	}

	xi = xed_decoded_inst_inst(xedd);
	for (i = 0, nb_mem_access = 0; i < xed_inst_noperands(xi); i++){
		xed_op = xed_inst_operand(xi, i);
		switch (xed_operand_name(xed_op)){
			case XED_OPERAND_MEM0 	:
			case XED_OPERAND_MEM1 	: {
				nb_mem_access ++;
				break;
			}
			case XED_OPERAND_AGEN 	:
			case XED_OPERAND_IMM0 	:
			case XED_OPERAND_REG0 	:
			case XED_OPERAND_REG1 	:
			case XED_OPERAND_REG2 	:
			case XED_OPERAND_REG3 	:
			case XED_OPERAND_REG4 	:
			case XED_OPERAND_REG5 	:
			case XED_OPERAND_REG6 	:
			case XED_OPERAND_REG7 	:
			case XED_OPERAND_REG8 	:
			case XED_OPERAND_RELBR 	:
			case XED_OPERAND_BASE0 	:
			case XED_OPERAND_BASE1 	: {
				break;
			}
			default 				: {
				log_err_m("operand type not supported: %s for instruction %s", xed_operand_enum_t2str(xed_operand_name(xed_op)), xed_iclass_enum_t2str(xed_decoded_inst_get_iclass(xedd)));
				break;
			}
		}
	}

	return nb_mem_access;
}

static void assembly_print_opcode_hex(uint8_t* buffer, uint32_t size, uint32_t padding_size){
	uint32_t i;

	for (i = 0; i < padding_size; i++){
		if (i < size){
			printf("%02x ", buffer[i] & 0x000000ff);
		}
		else{
			fputs("   ", stdout);
		}
	}
}

void assembly_print(struct assembly* assembly, uint32_t start, uint32_t stop, void* mem_trace){
	#define ASSEMBLY_PRINT_PADDING_INDEX 	"10"
	#define ASSEMBLY_PRINT_PADDING_OPCODE 	8
	#define ASSEMBLY_PRINT_PADDING_STRING 	"40"
	#define ASSEMBLY_PRINT_BUFFER_SIZE 		256

	uint32_t 					i;
	struct instructionIterator 	it;
	char 						buffer[ASSEMBLY_PRINT_BUFFER_SIZE];
	xed_print_info_t 			print_info;
	uint32_t 					prev_dyn_block_index;
	uint32_t 					j;
	uint32_t 					nb_mem_access;

	if (assembly_get_instruction(assembly, &it, start)){
		log_err_m("unable to fetch instruction %u from the assembly", start);
		return;
	}

	if (it.prev_black_listed){
		printf("[...]\n");
	}

	xed_init_print_info(&print_info);
	print_info.blen 					= ASSEMBLY_PRINT_BUFFER_SIZE;
	print_info.buf  					= buffer;
	print_info.context  				= NULL;
	print_info.disassembly_callback		= NULL;
	print_info.format_options_valid 	= 0;
	print_info.p 						= &(it.xedd);
	print_info.runtime_address 			= it.instruction_address;
	print_info.syntax 					= XED_SYNTAX_INTEL;

	if (xed_format_generic(&print_info)){
		printf("0x%08x * %-" ASSEMBLY_PRINT_PADDING_INDEX "u", it.instruction_address, it.instruction_index);
		assembly_print_opcode_hex(assembly->dyn_blocks[it.dyn_block_index].block->data + it.instruction_offset, it.instruction_size, ASSEMBLY_PRINT_PADDING_OPCODE);
		printf("%-" ASSEMBLY_PRINT_PADDING_STRING "s", buffer);

		if (mem_trace != NULL && ((struct memTrace*)mem_trace)->mem_addr_buffer != NULL && (nb_mem_access = assembly_get_instruction_nb_mem_access(&(it.xedd))) > 0){
			for (j = 0; j < nb_mem_access ; j++){
				memAddress_print(((struct memTrace*)mem_trace)->mem_addr_buffer + it.mem_access_index + j);
				if (it.instruction_address != ((struct memTrace*)mem_trace)->mem_addr_buffer[it.mem_access_index + j].pc){
					log_err("memory access has a different pc address");
				}
			}
		}
			
		putchar('\n');
	}
	else{
		log_err("xed_format_generic returns an error code");
	}

	prev_dyn_block_index = it.dyn_block_index;

	for (i = start + 1; i < stop && i < assembly_get_nb_instruction(assembly); i++){
		if (assembly_get_next_instruction(assembly, &it)){
			log_err_m("unable to fetch next instruction %u from the assembly", i);
			break;
		}

		if (it.prev_black_listed){
			printf("[...]\n");
		}

		xed_init_print_info(&print_info);
		print_info.blen 					= ASSEMBLY_PRINT_BUFFER_SIZE;
		print_info.buf  					= buffer;
		print_info.context  				= NULL;
		print_info.disassembly_callback		= NULL;
		print_info.format_options_valid 	= 0;
		print_info.p 						= &(it.xedd);
		print_info.runtime_address 			= it.instruction_address;
		print_info.syntax 					= XED_SYNTAX_INTEL;

		if (xed_format_generic(&print_info)){
			printf("0x%08x %c %-" ASSEMBLY_PRINT_PADDING_INDEX "u", it.instruction_address, (prev_dyn_block_index != it.dyn_block_index) ? '*' : ' ', it.instruction_index);
			assembly_print_opcode_hex(assembly->dyn_blocks[it.dyn_block_index].block->data + it.instruction_offset, it.instruction_size, ASSEMBLY_PRINT_PADDING_OPCODE);
			printf("%-" ASSEMBLY_PRINT_PADDING_STRING "s", buffer);

			if (mem_trace != NULL && ((struct memTrace*)mem_trace)->mem_addr_buffer != NULL && (nb_mem_access = assembly_get_instruction_nb_mem_access(&(it.xedd))) > 0){
				for (j = 0; j < nb_mem_access ; j++){
					memAddress_print(((struct memTrace*)mem_trace)->mem_addr_buffer + it.mem_access_index + j);
					if (it.instruction_address != ((struct memTrace*)mem_trace)->mem_addr_buffer[it.mem_access_index + j].pc){
						log_err("memory access has a different pc address");
					}
				}
			}

			putchar('\n');
		}
		else{
			log_err("xed_format_generic returns an error code");
		}

		prev_dyn_block_index = it.dyn_block_index;
	}
}

static int32_t assembly_assert_asmBlock(const struct asmBlock* block, const uint8_t* buffer, const uint8_t* valid, uint32_t size){
	uint32_t i;

	if (block->header.size != size){
		return 0;
	}

	for (i = 0; i < size; i++){
		if (valid[i]){
			if (block->data[i] != buffer[i]){
				return 0;
			}
		}
	}

	return 1;
}

#define SIZE_BBL_1_LINUX_1 12
static const uint8_t buffer_bbl_1_linux_1[SIZE_BBL_1_LINUX_1] = {0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00};
static const uint8_t valid_bbl_1_linux_1[SIZE_BBL_1_LINUX_1] = {1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0};

#define SIZE_BBL_2_LINUX_1 10
static const uint8_t buffer_bbl_2_linux_1[SIZE_BBL_2_LINUX_1] = {0x68, 0x00, 0x00, 0x00, 0x00, 0xe9, 0x00, 0x00, 0x00, 0x00};
static const uint8_t valid_bbl_2_linux_1[SIZE_BBL_2_LINUX_1] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0};

#define SIZE_BBL_3_LINUX_1 6
static const uint8_t buffer_bbl_3_linux_1[SIZE_BBL_3_LINUX_1] = {0xff, 0x00, 0x00, 0x00, 0x00, 0x00};
static const uint8_t valid_bbl_3_linux_1[SIZE_BBL_3_LINUX_1] = {1, 0, 0, 0, 0, 0};

#define SIZE_BBL_1_LINUX_2 6
static const uint8_t buffer_bbl_1_linux_2[SIZE_BBL_1_LINUX_2] = {0xff, 0x00, 0x00, 0x00, 0x00, 0x00};
static const uint8_t valid_bbl_1_linux_2[SIZE_BBL_1_LINUX_2] = {1, 0, 0, 0, 0, 0};

#define SIZE_MAGIC_BLOCK 6
static const uint8_t magic_block[SIZE_MAGIC_BLOCK] = {0x81, 0xc4, 0x04, 0x00, 0x00, 0x00};

int32_t assembly_filter_blacklisted_function_call(struct assembly* assembly, struct array** extrude_array){
	union asmBlockMaintainer{
		size_t 					size;
		struct asmBlock* 		ptr;
	};				

	uint32_t 					i;
	xed_decoded_inst_t 			xedd;
	uint32_t 					offset;
	union asmBlockMaintainer* 	asm_block_maintainer;
	struct asmBlock* 			asm_block_cursor;
	uint32_t 					nb_asm_block;
	struct memAccessExtrude		extrude;
	size_t 						new_mapping_size_block;
	void* 						new_mapping_block;
	size_t 						disp;
	uint32_t 					modify = 0;
	uint32_t 					mem_access_delete_count;

	*extrude_array = array_create(sizeof(struct memAccessExtrude));
	if (*extrude_array == NULL){
		log_err("unable to create array");
		return -1;
	}

	for (i = 0, offset = 0, mem_access_delete_count = 0; i < assembly->nb_dyn_block; i++){
		if (dynBlock_is_invalid(assembly->dyn_blocks + i) &&  i + 1 < assembly->nb_dyn_block){

			/* LINUX - resolve @ plt */
			if (i - offset >= 4){
				if (dynBlock_is_valid(assembly->dyn_blocks + i - offset - 1) && dynBlock_is_valid(assembly->dyn_blocks + i - offset - 2) && dynBlock_is_valid(assembly->dyn_blocks + i - offset - 3) && dynBlock_is_valid(assembly->dyn_blocks + i - offset - 4)){
					if (assembly->dyn_blocks[i - offset - 1].block->header.nb_ins == 2 && assembly->dyn_blocks[i - offset - 2].block->header.nb_ins == 2 && assembly->dyn_blocks[i - offset - 3].block->header.nb_ins == 1){
						assembly_get_last_instruction(assembly->dyn_blocks[i - offset - 4].block, &xedd);
						if (xed_decoded_inst_get_iclass(&xedd) == XED_ICLASS_CALL_NEAR){
							if (assembly_assert_asmBlock(assembly->dyn_blocks[i - offset - 1].block, buffer_bbl_1_linux_1, valid_bbl_1_linux_1, SIZE_BBL_1_LINUX_1) && assembly_assert_asmBlock(assembly->dyn_blocks[i - offset - 2].block, buffer_bbl_2_linux_1, valid_bbl_2_linux_1, SIZE_BBL_2_LINUX_1) && assembly_assert_asmBlock(assembly->dyn_blocks[i - offset - 3].block, buffer_bbl_3_linux_1, valid_bbl_3_linux_1, SIZE_BBL_3_LINUX_1)){
								log_info_m("found LINUX black listed function call @ %u, formatting", assembly->dyn_blocks[i - offset - 3].instruction_count - 1);

								extrude.index_start = mem_access_delete_count + assembly->dyn_blocks[i - offset - 3].mem_access_count; 
								extrude.index_stop = mem_access_delete_count + assembly->dyn_blocks[i - offset - 1].mem_access_count + assembly->dyn_blocks[i - offset - 1].block->header.nb_mem_access;

								if (array_add(*extrude_array, &extrude) < 0){
									log_err("unable to add element to array");
								}
								else{
									mem_access_delete_count += extrude.index_stop - extrude.index_start;
								}

								if (dynBlock_is_invalid(assembly->dyn_blocks + i + 1) || assembly->dyn_blocks[i + 1].block->header.address != assembly->dyn_blocks[i - offset - 4].block->header.address + assembly->dyn_blocks[i - offset - 4].block->header.size){
									log_warn("black listed function call has not returned, ESP value could be incorrect");

									assembly->dyn_blocks[i - offset - 3].instruction_count 	= assembly->dyn_blocks[i - offset - 4].instruction_count + assembly->dyn_blocks[i - offset - 4].block->header.nb_ins;
									assembly->dyn_blocks[i - offset - 3].mem_access_count 	= assembly->dyn_blocks[i - offset - 4].mem_access_count + assembly->dyn_blocks[i - offset - 4].block->header.nb_mem_access;
									assembly->dyn_blocks[i - offset - 3].block 				= NULL;

									offset += 3;
								}
								else{
									assembly->dyn_blocks[i - offset - 3].instruction_count 	= assembly->dyn_blocks[i - offset - 4].instruction_count + assembly->dyn_blocks[i - offset - 4].block->header.nb_ins;
									assembly->dyn_blocks[i - offset - 3].mem_access_count 	= assembly->dyn_blocks[i - offset - 4].mem_access_count + assembly->dyn_blocks[i - offset - 4].block->header.nb_mem_access;
									assembly->dyn_blocks[i - offset - 3].block 				= NULL;

									assembly->dyn_blocks[i - offset - 2].instruction_count 	= assembly->dyn_blocks[i - offset - 3].instruction_count;
									assembly->dyn_blocks[i - offset - 2].mem_access_count 	= assembly->dyn_blocks[i - offset - 3].mem_access_count;
									assembly->dyn_blocks[i - offset - 2].block 				= (void*)(-1);

									offset += 2;
								}

								modify = 1;
								continue;
							}
						}
					}
				}
			}

			/* LINUX - already resolved call */
			if (i - offset >= 2){
				if (dynBlock_is_valid(assembly->dyn_blocks + i - offset - 1) && dynBlock_is_valid(assembly->dyn_blocks + i - offset - 2)){
					if (assembly->dyn_blocks[i - offset - 1].block->header.nb_ins == 1){
						assembly_get_last_instruction(assembly->dyn_blocks[i - offset - 2].block, &xedd);
						if (xed_decoded_inst_get_iclass(&xedd) == XED_ICLASS_CALL_NEAR){
							if (assembly_assert_asmBlock(assembly->dyn_blocks[i - offset - 1].block, buffer_bbl_1_linux_2, valid_bbl_1_linux_2, SIZE_BBL_1_LINUX_2)){
								log_info_m("found LINUX black listed function call @ %u, formatting", assembly->dyn_blocks[i - offset - 1].instruction_count - 1);

								extrude.index_start = mem_access_delete_count + assembly->dyn_blocks[i - offset - 1].mem_access_count; 
								extrude.index_stop = mem_access_delete_count + assembly->dyn_blocks[i - offset - 1].mem_access_count + assembly->dyn_blocks[i - offset - 1].block->header.nb_mem_access;

								if (array_add(*extrude_array, &extrude) < 0){
									log_err("unable to add element to array");
								}
								else{
									mem_access_delete_count += extrude.index_stop - extrude.index_start;
								}

								if (dynBlock_is_invalid(assembly->dyn_blocks + i + 1) || assembly->dyn_blocks[i + 1].block->header.address != assembly->dyn_blocks[i - offset - 2].block->header.address + assembly->dyn_blocks[i - offset - 2].block->header.size){
									log_warn("black listed function call has not returned, ESP value could be incorrect");

									assembly->dyn_blocks[i - offset - 1].instruction_count 	= assembly->dyn_blocks[i - offset - 2].instruction_count + assembly->dyn_blocks[i - offset - 2].block->header.nb_ins;
									assembly->dyn_blocks[i - offset - 1].mem_access_count 	= assembly->dyn_blocks[i - offset - 2].mem_access_count + assembly->dyn_blocks[i - offset - 2].block->header.nb_mem_access;
									assembly->dyn_blocks[i - offset - 1].block 				= NULL;

									offset += 1;
								}
								else{
									assembly->dyn_blocks[i - offset - 1].instruction_count 	= assembly->dyn_blocks[i - offset - 2].instruction_count + assembly->dyn_blocks[i - offset - 2].block->header.nb_ins;
									assembly->dyn_blocks[i - offset - 1].mem_access_count 	= assembly->dyn_blocks[i - offset - 2].mem_access_count + assembly->dyn_blocks[i - offset - 2].block->header.nb_mem_access;
									assembly->dyn_blocks[i - offset - 1].block 				= NULL;

									assembly->dyn_blocks[i - offset - 0].instruction_count 	= assembly->dyn_blocks[i - offset - 1].instruction_count;
									assembly->dyn_blocks[i - offset - 0].mem_access_count 	= assembly->dyn_blocks[i - offset - 1].mem_access_count;
									assembly->dyn_blocks[i - offset - 0].block 				= (void*)(-1);
								}

								modify = 1;
								continue;
							}
						}
					}
				}
			}

			log_warn_m("unable to format black listed call @ %u", (i - offset > 0) ? assembly->dyn_blocks[i - offset - 1].instruction_count + assembly->dyn_blocks[i - offset - 1].block->header.nb_ins : 0);
		}
		else{
			if (i - offset == 0){
				assembly->dyn_blocks[i - offset].instruction_count 		= 0;
				assembly->dyn_blocks[i - offset].mem_access_count 		= 0;
				assembly->dyn_blocks[i - offset].block 					= assembly->dyn_blocks[i].block;
			}
			else if (dynBlock_is_invalid(assembly->dyn_blocks + i - offset - 1)){
				if (i - offset == 1){
					assembly->dyn_blocks[i - offset].instruction_count 	= 0;
					assembly->dyn_blocks[i - offset].mem_access_count 	= 0;
					assembly->dyn_blocks[i - offset].block 				= assembly->dyn_blocks[i].block;
				}
				else if (assembly->dyn_blocks[i - offset - 2].block == (void*)(-1)){
					assembly->dyn_blocks[i - offset].instruction_count 	= assembly->dyn_blocks[i - offset - 2].instruction_count + 1;
					assembly->dyn_blocks[i - offset].mem_access_count 	= assembly->dyn_blocks[i - offset - 2].mem_access_count;
					assembly->dyn_blocks[i - offset].block 				= assembly->dyn_blocks[i].block;
				}
				else{
					assembly->dyn_blocks[i - offset].instruction_count 	= assembly->dyn_blocks[i - offset - 2].instruction_count + assembly->dyn_blocks[i - offset - 2].block->header.nb_ins;
					assembly->dyn_blocks[i - offset].mem_access_count 	= assembly->dyn_blocks[i - offset - 2].mem_access_count + assembly->dyn_blocks[i - offset - 2].block->header.nb_mem_access;
					assembly->dyn_blocks[i - offset].block 				= assembly->dyn_blocks[i].block;
				}
			}
			else if (assembly->dyn_blocks[i - offset - 1].block == (void*)(-1)){
				assembly->dyn_blocks[i - offset].instruction_count 		= assembly->dyn_blocks[i - offset - 1].instruction_count + 1;
				assembly->dyn_blocks[i - offset].mem_access_count 		= assembly->dyn_blocks[i - offset - 1].mem_access_count;
				assembly->dyn_blocks[i - offset].block 					= assembly->dyn_blocks[i].block;
			}
			else{
				assembly->dyn_blocks[i - offset].instruction_count 		= assembly->dyn_blocks[i - offset - 1].instruction_count + assembly->dyn_blocks[i - offset - 1].block->header.nb_ins;
				assembly->dyn_blocks[i - offset].mem_access_count 		= assembly->dyn_blocks[i - offset - 1].mem_access_count + assembly->dyn_blocks[i - offset - 1].block->header.nb_mem_access;
				assembly->dyn_blocks[i - offset].block 					= assembly->dyn_blocks[i].block;
			}
		}
	}

	if (offset){
		assembly->nb_dyn_block -= offset;
		assembly->dyn_blocks = (struct dynBlock*)realloc(assembly->dyn_blocks, sizeof(struct dynBlock) * assembly->nb_dyn_block);
		if (assembly->dyn_blocks == NULL){
			log_err("unable to realloc memory");
			return -1;
		}
	}

	if (modify){
		for (asm_block_cursor = (struct asmBlock*)(assembly->mapping_block), nb_asm_block = 0; (char*)asm_block_cursor != (char*)(assembly->mapping_block) + assembly->mapping_size_block; ){
			nb_asm_block ++;
			asm_block_cursor->header.id = nb_asm_block;
			asm_block_cursor = (struct asmBlock*)((char*)asm_block_cursor + asm_block_cursor->header.size + sizeof(struct asmBlockHeader));
		}

		asm_block_maintainer = (union asmBlockMaintainer*)calloc(nb_asm_block , sizeof(union asmBlockMaintainer));
		if (asm_block_maintainer == NULL){
			log_err("unable to allocate memory");
			return -1;
		}

		for (i = 0; i < assembly->nb_dyn_block; i++){
			if (dynBlock_is_valid(assembly->dyn_blocks + i) && assembly->dyn_blocks[i].block != (void*)(-1)){
				asm_block_maintainer[assembly->dyn_blocks[i].block->header.id - 1].size = assembly->dyn_blocks[i].block->header.size + sizeof(struct asmBlockHeader);
			}
		}

		for (i = 0, new_mapping_size_block = sizeof(struct asmBlockHeader) + SIZE_MAGIC_BLOCK; i < nb_asm_block; i++){
			new_mapping_size_block += asm_block_maintainer[i].size;
		}

		new_mapping_block = malloc(new_mapping_size_block);
		if (new_mapping_block == NULL){
			log_err("unable to allocate memory");
			free(asm_block_maintainer);
			return -1;
		}

		for (asm_block_cursor = (struct asmBlock*)(assembly->mapping_block), disp = 0, i = 0; (char*)asm_block_cursor != (char*)(assembly->mapping_block) + assembly->mapping_size_block; asm_block_cursor = (struct asmBlock*)((char*)asm_block_cursor + asm_block_cursor->header.size + sizeof(struct asmBlockHeader)), i++){
			if (asm_block_maintainer[i].size){
				memcpy((char*)new_mapping_block + disp, asm_block_cursor, sizeof(struct asmBlockHeader) + asm_block_cursor->header.size);
				asm_block_maintainer[i].ptr = (struct asmBlock*)((char*)new_mapping_block + disp);
				disp += sizeof(struct asmBlockHeader) + asm_block_cursor->header.size;
			}
			else{
				asm_block_maintainer[i].ptr = NULL;
			}
		}
		asm_block_cursor = (struct asmBlock*)((char*)new_mapping_block + disp);

		asm_block_cursor->header.id 			= nb_asm_block + 1;
		asm_block_cursor->header.size 			= SIZE_MAGIC_BLOCK;
		asm_block_cursor->header.nb_ins 		= 1;
		asm_block_cursor->header.nb_mem_access 	= 0;
		asm_block_cursor->header.address 		= 0;
		memcpy(asm_block_cursor->data, magic_block, SIZE_MAGIC_BLOCK);

		for (i = 0, assembly->nb_dyn_instruction = 0; i < assembly->nb_dyn_block; i++){
			if (dynBlock_is_valid(assembly->dyn_blocks + i)){
				if (assembly->dyn_blocks[i].block != (void*)(-1)){
					assembly->dyn_blocks[i].block = asm_block_maintainer[assembly->dyn_blocks[i].block->header.id - 1].ptr;
				}
				else{
					assembly->dyn_blocks[i].block = asm_block_cursor;
				}
				assembly->nb_dyn_instruction += assembly->dyn_blocks[i].block->header.nb_ins;
			}
		}

		free(asm_block_maintainer);

		switch(assembly->allocation_type){
			case ALLOCATION_MALLOC : {
				free(assembly->mapping_block);
				break;
			}
			case ALLOCATION_MMAP : {
				munmap(assembly->mapping_block, assembly->mapping_size_block);
				break;
			}
		}

		assembly->allocation_type 		= ALLOCATION_MALLOC;
		assembly->mapping_block 		= new_mapping_block;
		assembly->mapping_size_block 	= new_mapping_size_block;
	}

	return 0;
}

void assembly_locate_opcode(struct assembly* assembly, const uint8_t* opcode, size_t opcode_length){
	uint32_t 			block_offset;
	struct asmBlock* 	block;
	uint8_t* 			ptr;
	size_t 				offset;
	uint32_t 			instruction_offset;
	xed_error_enum_t 	xed_error;
	xed_decoded_inst_t 	xedd;

	for (block_offset = 0; block_offset != assembly->mapping_size_block; block_offset += sizeof(struct asmBlockHeader) + block->header.size){
		block = (struct asmBlock*)((char*)assembly->mapping_block + block_offset);
		if (block_offset + block->header.size + sizeof(struct asmBlockHeader) > assembly->mapping_size_block){
			log_err("the last asmBlock is incomplete");
			break;
		}

		if (block->header.size >= opcode_length){
			if ((ptr = memmem(block->data, block->header.size, opcode, opcode_length)) != NULL){
				offset = ptr - block->data;

				for (instruction_offset = 0; ; instruction_offset += xed_decoded_inst_get_length(&xedd)){
					xed_decoded_inst_zero(&xedd);
					xed_decoded_inst_set_mode(&xedd, disas.mmode, disas.stack_addr_width);
					if ((xed_error = xed_decode(&xedd, (const xed_uint8_t*)(block->data + instruction_offset), min(block->header.size - instruction_offset, 15))) != XED_ERROR_NONE){
						log_err_m("xed decode error: %s", xed_error_enum_t2str(xed_error));
						break;
					}

					
					if (instruction_offset + xed_decoded_inst_get_length(&xedd) > offset){
						break;
					}
				}

				printf("  - found opcode in trace @ address: 0x%08x\n", block->header.address + instruction_offset);
			}
		}
	}
}

void assembly_clean(struct assembly* assembly){
	if (assembly->dyn_blocks != NULL){
		free(assembly->dyn_blocks);
		assembly->dyn_blocks = NULL;
	}

	if (assembly->mapping_block != NULL){
		switch(assembly->allocation_type){
			case ALLOCATION_MALLOC : {
				free(assembly->mapping_block);
				break;
			}
			case ALLOCATION_MMAP : {
				munmap(assembly->mapping_block, assembly->mapping_size_block);
				break;
			}
		}
		assembly->mapping_block = NULL;
	}
}