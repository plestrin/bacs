#ifndef ASSEMBLY_H
#define ASSEMBLY_H

#include <stdint.h>

#include "xed-interface.h"
#include "address.h"
#include "base.h"

struct asmWriter{
	uint32_t 				blockId_generator;
};

#define asmWriter_init(writer) 			(writer)->blockId_generator = 1
#define asmWrite_get_BlockId(writer) 	(writer)->blockId_generator++
#define BLACK_LISTED_ID 				0x00000000
#define UNTRACK_MEM_ACCESS 				0xffffffff

struct asmBlockHeader{
	uint32_t 				id;
	uint32_t 				size;
	uint32_t 				nb_ins;
	uint32_t 				nb_mem_access;
	ADDRESS 				address;
};

struct asmBlock{
	struct asmBlockHeader 	header;
	char 					data[1];
};

struct dynBlock{
	uint32_t 				instruction_count;
	uint64_t 				mem_access_count;
	struct asmBlock* 		block;
};

#define dynBlock_is_valid(dyn_block) ((dyn_block)->block != NULL)
#define dynBlock_is_invalid(dyn_block) ((dyn_block)->block == NULL)
#define dynBlock_set_invalid(dyn_block) (dyn_block)->block = NULL

uint32_t asmBlock_count_nb_ins(struct asmBlock* block);

struct assembly{
	uint32_t 				nb_dyn_instruction;
	uint32_t 				nb_dyn_block;
	struct dynBlock* 		dyn_blocks;

	enum allocationType 	allocation_type;
	void* 					mapping_block;
	size_t 					mapping_size_block;
};

struct instructionIterator{
	xed_decoded_inst_t 		xedd;
	uint32_t 				instruction_index;
	uint32_t 				dyn_block_index;
	uint32_t 				instruction_sub_index;
	uint32_t 				instruction_offset;
	uint8_t 				instruction_size;
	uint8_t 				prev_black_listed;
	ADDRESS 				instruction_address;
};

#define instructionIterator_get_instruction_index(it) ((it)->instruction_index)

int32_t assembly_init(struct assembly* assembly, const uint32_t* buffer_id, size_t buffer_size_id, uint32_t* buffer_block, size_t buffer_size_block, enum allocationType buffer_alloc_block);

int32_t assembly_load_trace(struct assembly* assembly, const char* file_name_id, const char* file_name_block);

int32_t assembly_get_instruction(struct assembly* assembly, struct instructionIterator* it, uint32_t index);
int32_t assembly_get_next_instruction(struct assembly* assembly, struct instructionIterator* it);

int32_t assembly_get_last_instruction(struct asmBlock* block, xed_decoded_inst_t* xedd);

int32_t assembly_check(struct assembly* assembly);

int32_t assembly_extract_segment(struct assembly* assembly_src, struct assembly* assembly_dst, uint32_t offset, uint32_t length, uint64_t* index_mem_access_start, uint64_t* index_mem_access_stop);

int32_t assembly_concat(struct assembly** assembly_src_buffer, uint32_t nb_assembly_src, struct assembly* assembly_dst);

#define assembly_get_nb_instruction(assembly) ((assembly)->nb_dyn_instruction)

void assembly_print(struct assembly* assembly, uint32_t start, uint32_t stop);

void assembly_clean(struct assembly* assembly);

#define assembly_delete(assembly) 																																\
	assembly_clean(assembly); 																																	\
	free(assembly)

#endif