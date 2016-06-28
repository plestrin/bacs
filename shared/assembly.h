#ifndef ASSEMBLY_H
#define ASSEMBLY_H

#include <stdint.h>

#include "address.h"

struct asmWriter{
	uint32_t 				blockId_generator;
};

#define FIRST_BLOCK_ID 					1
#define asmWriter_init(writer) 			(writer)->blockId_generator = FIRST_BLOCK_ID
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

#ifndef __PIN__

#include "xed-interface.h"
#include "array.h"
#include "base.h"

struct asmBlock{
	struct asmBlockHeader 	header;
	uint8_t 				data[1];
};

uint32_t asmBlock_count_nb_ins(struct asmBlock* block);
uint8_t* asmBlock_search_instruction(struct asmBlock* block, const xed_iclass_enum_t* buffer, uint32_t buffer_length, xed_decoded_inst_t* xedd, uint32_t offset);
uint8_t* asmBlock_search_opcode(struct asmBlock* block, const uint8_t* opcode, size_t opcode_length, size_t offset);
int32_t asmBlock_get_first_instruction(struct asmBlock* block, xed_decoded_inst_t* xedd);
int32_t asmBlock_get_next_instruction(struct asmBlock* block, xed_decoded_inst_t* xedd, uint32_t* offset);
int32_t asmBlock_get_last_instruction(struct asmBlock* block, xed_decoded_inst_t* xedd);

struct dynBlock{
	uint32_t 				instruction_count;
	uint64_t 				mem_access_count;
	struct asmBlock* 		block;
};

#define dynBlock_is_valid(dyn_block) ((dyn_block)->block != NULL)
#define dynBlock_is_invalid(dyn_block) ((dyn_block)->block == NULL)
#define dynBlock_set_invalid(dyn_block) (dyn_block)->block = NULL

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
	uint32_t 				instruction_size;
	uint8_t 				prev_black_listed;
	uint8_t 				mem_access_valid;
	uint64_t 				mem_access_index;
	ADDRESS 				instruction_address;
};

#define instructionIterator_get_instruction_index(it) ((it)->instruction_index)
#define instructionIterator_is_mem_addr_valid(it) ((it)->mem_access_valid == 1)
#define instructionIterator_get_mem_addr_index(it) ((it)->mem_access_index)

int32_t assembly_init(struct assembly* assembly, const uint32_t* buffer_id, size_t buffer_size_id, uint32_t* buffer_block, size_t buffer_size_block, enum allocationType buffer_alloc_block);

int32_t assembly_load_trace(struct assembly* assembly, const char* file_name_id, const char* file_name_block);

int32_t assembly_get_first_instruction(const struct assembly* assembly, struct instructionIterator* it);
int32_t assembly_get_first_pc(const struct assembly* assembly, struct instructionIterator* it, ADDRESS pc);
int32_t assembly_get_instruction(const struct assembly* assembly, struct instructionIterator* it, uint32_t ins_index);
int32_t assembly_get_address(const struct assembly* assembly, struct instructionIterator* it, uint64_t mem_index);
int32_t assembly_get_next_instruction(const struct assembly* assembly, struct instructionIterator* it);
int32_t assembly_get_next_block(const struct assembly* assembly, struct instructionIterator* it);
int32_t assembly_get_next_pc(const struct assembly* assembly, struct instructionIterator* it);

int32_t assembly_get_dyn_block_ins(const struct assembly* assembly, uint32_t ins_index, uint32_t* result);
int32_t assembly_get_dyn_block_mem(const struct assembly* assembly, uint64_t mem_index, uint32_t* result);

int32_t assembly_check(struct assembly* assembly);

int32_t assembly_extract_segment(struct assembly* assembly_src, struct assembly* assembly_dst, uint32_t offset, uint32_t length, uint64_t* index_mem_access_start, uint64_t* index_mem_access_stop);

int32_t assembly_concat(struct assembly** assembly_src_buffer, uint32_t nb_assembly_src, struct assembly* assembly_dst);

#define assembly_get_nb_instruction(assembly) ((assembly)->nb_dyn_instruction)

void assembly_print_all(struct assembly* assembly, uint32_t start, uint32_t stop, void* mem_trace);
void assembly_print_ins(struct assembly* assembly, struct instructionIterator* it, void* mem_access_buffer);

struct memAccessExtrude{
	uint64_t index_start;
	uint64_t index_stop;
};

int32_t assembly_filter_blacklisted_function_call(struct assembly* assembly, struct array** extrude_array);

void assembly_locate_opcode(struct assembly* assembly, const uint8_t* opcode, size_t opcode_length);

int32_t assembly_search_sub_sequence(const struct assembly* assembly_ext, const struct assembly* assembly_inn, struct instructionIterator* it);

int32_t assembly_compare(const struct assembly* assembly1, const struct assembly* assembly2);

void assembly_clean(struct assembly* assembly);

#define assembly_delete(assembly) 				\
	assembly_clean(assembly); 					\
	free(assembly)

#endif

#endif