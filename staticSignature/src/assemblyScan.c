#include <stdlib.h>
#include <stdio.h>

#include "assemblyScan.h"
#include "base.h"

#define ASSEMBLYSCAN_NB_MIN_INSTRUCTION 40

static const xed_iclass_enum_t crypto_instruction[] = {
	XED_ICLASS_AESDEC,
	XED_ICLASS_AESDECLAST,
	XED_ICLASS_AESENC,
	XED_ICLASS_AESENCLAST,
	XED_ICLASS_AESIMC,
	XED_ICLASS_AESKEYGENASSIST
};

#define nb_crypto_instruction (sizeof(crypto_instruction) / sizeof(xed_iclass_enum_t))

void assemblyScan_scan(struct assembly* assembly){
	uint32_t 			block_offset;
	struct asmBlock* 	block;
	uint32_t 			nb_instruction;
	xed_decoded_inst_t 	xedd;
	uint8_t* 			ptr;
	uint32_t 			nb_block;

	for (block_offset = 0, nb_block = 0; block_offset != assembly->mapping_size_block; block_offset += sizeof(struct asmBlockHeader) + block->header.size, nb_block ++){
		block = (struct asmBlock*)((char*)assembly->mapping_block + block_offset);
		if (block_offset + block->header.size + sizeof(struct asmBlockHeader) > assembly->mapping_size_block){
			log_err("the last asmBlock is incomplete");
			break;
		}

		for (ptr = asmBlock_search_instruction(block, crypto_instruction, nb_crypto_instruction, &xedd, 0); ptr != NULL; ptr = asmBlock_search_instruction(block, crypto_instruction, nb_crypto_instruction, &xedd, ptr - block->data)){
			printf("  - %s @ 0x%08x\n", xed_iclass_enum_t2str(xed_decoded_inst_get_iclass(&xedd)), block->header.address + (ptr - block->data));
		}
				
		if ((nb_instruction = asmBlock_count_nb_ins(block)) > ASSEMBLYSCAN_NB_MIN_INSTRUCTION){
			printf("  - bbl of %u instruction @ 0x%08x\n", nb_instruction, block->header.address);
		}
	}

	log_info_m("%u block(s) have been scanned", nb_block);
}