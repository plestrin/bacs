#ifndef ASSEMBLY_H
#define ASSEMBLY_H

#include <stdint.h>

#include "xed-interface.h"
#include "address.h"

struct asmWriter{
	uint32_t 				blockId_generator;
};

#define asmWriter_init(writer) 			(writer)->blockId_generator = 1
#define asmWrite_get_BlockId(writer) 	(writer)->blockId_generator++

struct asmBlockHeader{
	uint32_t 				id;
	uint32_t 				size;
	uint32_t 				nb_ins;
	ADDRESS 				address;
};

struct asmBlock{
	struct asmBlockHeader 	header;
	char 					data[1];
};

struct dynBlock{
	uint32_t 				instruction_count;
	struct asmBlock* 		block;
};

enum assemblyAllocation{
	ASSEMBLYALLOCATION_MALLOC,
	ASSEMBLYALLOCATION_MMAP
};

struct assembly{
	uint32_t 				nb_dyn_instruction;
	uint32_t 				nb_dyn_block;
	struct dynBlock* 		dyn_blocks;

	enum assemblyAllocation allocation_type;
	void* 					mapping_block;
	uint64_t 				mapping_size_block;
};

struct instructionIterator{
	xed_decoded_inst_t 		xedd;
	uint32_t 				instruction_index;
	uint32_t 				dyn_block_index;
	uint32_t 				instruction_sub_index;
	uint32_t 				instruction_offset;
	uint8_t 				instruction_size;
	ADDRESS 				instruction_address;
};

struct assembly* assembly_create(const char* file_name_id, const char* file_name_block);
int32_t assembly_init(struct assembly* assembly, const char* file_name_id, const char* file_name_block);

int32_t assembly_get_instruction(struct assembly* assembly, struct instructionIterator* it, uint32_t index);
int32_t assembly_get_next_instruction(struct assembly* assembly, struct instructionIterator* it);

int32_t assembly_check(struct assembly* assembly);

int32_t assembly_extract_segment(struct assembly* assembly_src, struct assembly* assembly_dst, uint32_t offset, uint32_t length);

#define assembly_get_nb_instruction(assembly) ((assembly)->nb_dyn_instruction)

void assembly_clean(struct assembly* assembly);

#define assembly_delete(assembly) 																																\
	assembly_clean(assembly); 																																	\
	free(assembly)

#endif