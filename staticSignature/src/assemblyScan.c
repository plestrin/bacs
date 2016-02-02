#include <stdlib.h>
#include <stdio.h>

#include "assemblyScan.h"
#include "callGraph.h"
#include "set.h"
#include "array.h"
#include "base.h"

#define ASSEMBLYSCAN_NB_MIN_INSTRUCTION 40
#define ASSEMBLYSCAN_NB_MAX_INS_PER_BBL 70

static const xed_iclass_enum_t crypto_instruction[] = {
	XED_ICLASS_AESDEC,
	XED_ICLASS_AESDECLAST,
	XED_ICLASS_AESENC,
	XED_ICLASS_AESENCLAST,
	XED_ICLASS_AESIMC,
	XED_ICLASS_AESKEYGENASSIST,
	XED_ICLASS_PCLMULQDQ
};

#define nb_crypto_instruction (sizeof(crypto_instruction) / sizeof(xed_iclass_enum_t))

#define ASSEMBLYSCAN_MIN_BIT_WISE_RATIO 25.0

static const xed_iclass_enum_t bitwise_instruction[] = {
	XED_ICLASS_AND,
	XED_ICLASS_PAND,
	XED_ICLASS_VPAND,
	XED_ICLASS_NOT,
	XED_ICLASS_OR,
	XED_ICLASS_POR,
	XED_ICLASS_VPOR,
	XED_ICLASS_ROL,
	XED_ICLASS_ROR,
	XED_ICLASS_SHR,
	XED_ICLASS_SHL,
	XED_ICLASS_PSLLD,
	XED_ICLASS_PSRLD,
	XED_ICLASS_XOR,
	XED_ICLASS_PXOR,
	XED_ICLASS_VPXOR,
	XED_ICLASS_XORPD,
	XED_ICLASS_XORPS,
	XED_ICLASS_VXORPD,
	XED_ICLASS_VXORPS
};

#define nb_bitwise_instruction (sizeof(bitwise_instruction) / sizeof(xed_iclass_enum_t))

static struct array* assemblyScan_create_bbl_array(const struct assembly* assembly){
	struct array* 		bbl_array;
	struct asmBlock* 	block;
	uint32_t 			block_offset;

	bbl_array = array_create(sizeof(struct asmBlock*));
	if (bbl_array == NULL){
		log_err("unable to create array");
		return NULL;
	}

	for (block_offset = 0; block_offset != assembly->mapping_size_block; block_offset += sizeof(struct asmBlockHeader) + block->header.size){
		block = (struct asmBlock*)((char*)assembly->mapping_block + block_offset);
		if (block_offset + block->header.size + sizeof(struct asmBlockHeader) > assembly->mapping_size_block){
			log_err("the last asmBlock is incomplete");
			break;
		}

		if (array_add(bbl_array, &block) < 0){
			log_err("unable to add element to array");
		}
	}

	return bbl_array;
}

static int32_t asmBlock_compare_address(void* arg1, void* arg2){
	struct asmBlock* bbl1 = *(struct asmBlock**)arg1;
	struct asmBlock* bbl2 = *(struct asmBlock**)arg2;

	if (bbl1->header.address < bbl2->header.address){
		return -1;
	}
	else if (bbl1->header.address > bbl2->header.address){
		return 1;
	}
	else{
		if (bbl1->header.nb_ins > bbl2->header.nb_ins){
			return -1;
		}
		else if (bbl1->header.nb_ins < bbl2->header.nb_ins){
			return 1;
		}
		else{
			return 0;
		}
	}
}

static uint32_t assemblyScan_count_specific_instruction(struct asmBlock* block, const xed_iclass_enum_t* buffer, uint32_t buffer_length){
	uint32_t 			result;
	xed_decoded_inst_t 	xedd;
	uint8_t* 			ptr;

	for (result = 0, ptr = asmBlock_search_instruction(block, buffer, buffer_length, &xedd, 0); ptr != NULL; ptr = asmBlock_search_instruction(block, buffer, buffer_length, &xedd, ptr + xed_decoded_inst_get_length(&xedd) - block->data)){
		result ++;
	}

	return result;
}

static uint32_t assemblyScan_is_executed(const struct assembly* assembly, const struct asmBlock* block){
	uint32_t i;

	for (i = 0; i < assembly->nb_dyn_block; i++){
		if (dynBlock_is_valid(assembly->dyn_blocks + i)){
			if (assembly->dyn_blocks[i].block == block){
				return 1;
			}
		}
	}

	return 0;
}

static void assemblyScan_get_function(const struct assembly* assembly, const struct asmBlock* block, struct callGraph* call_graph, struct set* function_set){
	uint32_t 		i;
	struct node* 	function;

	for (i = 0; i < assembly->nb_dyn_block; i++){
		if (dynBlock_is_valid(assembly->dyn_blocks + i)){
			if (assembly->dyn_blocks[i].block == block){
				function = callGraph_get_index(call_graph, assembly->dyn_blocks[i].instruction_count);
				if (function != NULL){
					if (set_add_unique(function_set, &function) < 0){
						log_err("unable to add element to set");
					}
				}
				else{
					log_err_m("no function for index: %u", assembly->dyn_blocks[i].instruction_count);
				}
			}
		}
	}
}

/*
 * List of heuristics
 *	- Special opcode (refer to crypto_instruction)
 * 	1 Large basic blocks (refer to ASSEMBLYSCAN_NB_MIN_INSTRUCTION)
 * 	2 High ratio of bitwise instruction (refer to bitwise_instruction and ASSEMBLYSCAN_MIN_BIT_WISE_RATIO)
 * 	3 Leaf in the callGraph if provided
 */

void assemblyScan_scan(const struct assembly* assembly, void* call_graph, uint32_t filters){
	struct asmBlock* 	block;
	xed_decoded_inst_t 	xedd;
	uint32_t 			i;
	struct array* 		bbl_array;
	uint32_t* 			address_mapping;
	struct set 			function_set;
	struct setIterator 	it;
	struct node** 		func_ptr;

	set_init(&function_set, sizeof(struct node*), 16);

	if (call_graph != NULL){
		log_info("using callGraph to apply function heuristic(s)");
	}

	bbl_array = assemblyScan_create_bbl_array(assembly);
	if (bbl_array == NULL){
		log_err("unable to create bbl array");
		return;
	}

	for (i = 0; i < array_get_length(bbl_array); i++){
		uint8_t* ptr;

		block = *(struct asmBlock**)array_get(bbl_array, i);
		for (ptr = asmBlock_search_instruction(block, crypto_instruction, nb_crypto_instruction, &xedd, 0); ptr != NULL; ptr = asmBlock_search_instruction(block, crypto_instruction, nb_crypto_instruction, &xedd, ptr + xed_decoded_inst_get_length(&xedd) - block->data)){
			printf("  - %s @ 0x%08x\n", xed_iclass_enum_t2str(xed_decoded_inst_get_iclass(&xedd)), block->header.address + (ptr - block->data));
		}
	}

	address_mapping = array_create_mapping(bbl_array, asmBlock_compare_address);
	if (address_mapping == NULL){
		log_err("unable to create array mapping");
	}
	else{
		uint32_t 			nb_bbl;
		uint32_t 			nb_ins;
		uint32_t 			nb_bitwise;
		struct asmBlock* 	next_block;
		ADDRESS 			address;
		double 				ratio;
		uint32_t 			is_executed;

		for (i = 0; i < array_get_length(bbl_array); ){
			block = *(struct asmBlock**)array_get(bbl_array, address_mapping[i]);

			if (block->header.nb_ins > ASSEMBLYSCAN_NB_MIN_INSTRUCTION || !(filters & ASSEMBLYSCAN_FILTER_BBL_SIZE)){
				nb_bbl 		= 1;
				nb_ins 		= block->header.nb_ins;
				address 	=  block->header.address + block->header.size;
				nb_bitwise 	= assemblyScan_count_specific_instruction(block, bitwise_instruction, nb_bitwise_instruction);

				for (next_block = block, i = i + 1; next_block->header.nb_ins == ASSEMBLYSCAN_NB_MAX_INS_PER_BBL && i < array_get_length(bbl_array); i++){
					next_block = *(struct asmBlock**)array_get(bbl_array, address_mapping[i]);
					if (next_block->header.address > address){
						break;
					}
					else if (next_block->header.address < address){
						continue;
					}
					else{
						nb_bbl 		+= 1;
						nb_ins 		+= next_block->header.nb_ins;
						address 	+= next_block->header.size;
						nb_bitwise 	+= assemblyScan_count_specific_instruction(next_block, bitwise_instruction, nb_bitwise_instruction);
					}
				}

				if (nb_bitwise == 0){
					continue;
				}

				ratio = 100 * (double)nb_bitwise / (double)nb_ins;

				if (ratio >= ASSEMBLYSCAN_MIN_BIT_WISE_RATIO || !(filters & ASSEMBLYSCAN_FILTER_BBL_RATIO)){
					is_executed = assemblyScan_is_executed(assembly, block);
					if (is_executed || !(filters & ASSEMBLYSCAN_FILTER_BBL_EXEC)){
						#ifdef COLOR
						if (ratio < 25){
							printf(" - Super block: %3u bbl(s), %4u instruction(s) @ 0x%08x -> 0x%08x %5.2f%%", nb_bbl, nb_ins, block->header.address, address, ratio);
						}
						else if (ratio < 50){
							printf(" - Super block: %3u bbl(s), %4u instruction(s) @ 0x%08x -> 0x%08x " "\x1b[35m" "%5.2f%%" ANSI_COLOR_RESET, nb_bbl, nb_ins, block->header.address, address, ratio);
						}
						else{
							printf(" - Super block: %3u bbl(s), %4u instruction(s) @ 0x%08x -> 0x%08x " "\x1b[1;35m" "%5.2f%%" ANSI_COLOR_RESET, nb_bbl, nb_ins, block->header.address, address, ratio);
						}
						#else
						printf(" - Super block: %3u bbl(s), %4u instruction(s) @ 0x%08x -> 0x%08x %5.2f%%", nb_bbl, nb_ins, block->header.address, address, ratio);
						#endif

						
						if (is_executed){
							puts(" " ANSI_COLOR_BOLD_GREEN "Y" ANSI_COLOR_RESET);
						}
						else{
							puts(" " ANSI_COLOR_BOLD_RED "N" ANSI_COLOR_RESET);
						}

						if (call_graph != NULL){
							assemblyScan_get_function(assembly, block, call_graph, &function_set);
						}
					}
				}
			}
			else{
				i ++;
			}
		}

		free(address_mapping);
	}

	log_info_m("%d block(s) have been scanned", array_get_length(bbl_array));

	for (func_ptr = setIterator_get_first(&function_set, &it); func_ptr; func_ptr = setIterator_get_next(&it)){
		if (callGraphNode_is_leaf(*func_ptr)){
			fputs(" - leaf:" ANSI_COLOR_BOLD_GREEN "Y" ANSI_COLOR_RESET " ", stdout);
		}
		else if (!(filters & ASSEMBLYSCAN_FILTER_FUNC_LEAF)){
			fputs(" - leaf:" ANSI_COLOR_BOLD_RED "N" ANSI_COLOR_RESET " ", stdout);
		}
		callGraph_fprint_node(call_graph, *func_ptr, stdout);
		putchar('\n');
	}

	array_delete(bbl_array);
	set_clean(&function_set);
}