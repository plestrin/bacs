#include <stdlib.h>
#include <stdio.h>

#include "assemblyScan.h"
#include "callGraph.h"
#include "set.h"
#include "array.h"
#include "base.h"

#define ASSEMBLYSCAN_NB_MIN_EXECUTION 	5
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

#define INSTRUCTION_FLAG 0x80000000

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
	XED_ICLASS_SHRD,
	XED_ICLASS_PSRLD,
	XED_ICLASS_VPSRLD,
	XED_ICLASS_SHL,
	XED_ICLASS_SHLD,
	XED_ICLASS_PSLLD,
	XED_ICLASS_VPSLLD,
	XED_ICLASS_XOR,
	XED_ICLASS_PXOR,
	XED_ICLASS_VPXOR,
	XED_ICLASS_XORPD,
	XED_ICLASS_XORPS,
	XED_ICLASS_VXORPD,
	XED_ICLASS_VXORPS
};

#define nb_bitwise_instruction (sizeof(bitwise_instruction) / sizeof(xed_iclass_enum_t))

#define MD5_NB_ELEMENT 		64
#define MD5_NAME 			"MD5"
#define MD5_FLAG 			0x00000001

static const uint32_t MD5[MD5_NB_ELEMENT] = {
	0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee,
	0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
	0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be,
	0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,

	0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa,
	0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
	0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed,
	0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,

	0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c,
	0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
	0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05,
	0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,

	0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039,
	0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
	0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1,
	0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391,
};

#define SHA1_NB_ELEMENT 	9
#define SHA1_NAME 			"SHA1"
#define SHA1_FLAG 			0x00000002

static const uint32_t SHA1[SHA1_NB_ELEMENT] = {
	0x5a827999, 0x6ed9eba1, 0x8f1bbcdc, 0xca62c1d6,
	0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476,
	0xc3d2e1f0
};

#define HAVAL_NB_ELEMENT 	8
#define HAVAL_NAME 			"HAVAL"
#define HAVAL_FLAG 			0x00000004

static const uint32_t HAVAL[HAVAL_NB_ELEMENT] = {
	0x243F6A88, 0x85A308D3, 0x13198A2E, 0x03707344,
	0xA4093822, 0x299F31D0, 0x082EFA98, 0xEC4E6C89
};

#define TEA_NB_ELEMENT 		3
#define TEA_NAME 			"TEA"
#define TEA_FLAG 			0x00000008

static const uint32_t TEA[TEA_NB_ELEMENT] = {
	0x9e3779b9, 0x61c88647, 0xc6ef3720
};

static uint32_t assemblySan_search_cst(struct asmBlock* block, uint32_t verbose){
	uint32_t 				offset;
	xed_decoded_inst_t 		xedd;
	int32_t 				flag;
	const xed_inst_t* 		xi;
	const xed_operand_t* 	xed_op;
	xed_operand_enum_t 		op_name;
	uint32_t 				i;
	uint32_t 				j;
	uint64_t 				value;
	uint32_t 				result;

	for (flag = asmBlock_get_first_instruction(block, &xedd), offset = 0, result = 0; !flag; flag = asmBlock_get_next_instruction(block, &xedd, &offset)){
		xi = xed_decoded_inst_inst(&xedd);
		for (i = 0; i < xed_inst_noperands(xi); i++){
			xed_op = xed_inst_operand(xi, i);
			op_name = xed_operand_name(xed_op);

			if (op_name == XED_OPERAND_IMM0 && xed_decoded_inst_get_immediate_width_bits(&xedd) == 32){
				value = xed_decoded_inst_get_unsigned_immediate(&xedd);
			}
			else if (op_name == XED_OPERAND_AGEN){
				value = xed_decoded_inst_get_memory_displacement(&xedd, 0);
			}
			else{
				continue;
			}

			for (j = 0; j < MD5_NB_ELEMENT; j++){
				if ((uint32_t)value == MD5[j]){
					result |= MD5_FLAG;
					if (verbose){
						printf("  - " MD5_NAME " constant @ 0x%08x\n", block->header.address + offset);
					}
					break;
				}
			}

			for (j = 0; j < SHA1_NB_ELEMENT; j++){
				if ((uint32_t)value == SHA1[j]){
					result |= SHA1_FLAG;
					if (verbose){
						printf("  - " SHA1_NAME " constant @ 0x%08x\n", block->header.address + offset);
					}
					break;
				}
			}

			for (j = 0; j < HAVAL_NB_ELEMENT; j++){
				if ((uint32_t)value == HAVAL[j]){
					result |= HAVAL_FLAG;
					if (verbose){
						printf("  - " HAVAL_NAME " constant @ 0x%08x\n", block->header.address + offset);
					}
					break;
				}
			}

			for (j = 0; j < TEA_NB_ELEMENT; j++){
				if ((uint32_t)value == TEA[j]){
					result |= TEA_FLAG;
					if (verbose){
						printf("  - " TEA_NAME " constant @ 0x%08x\n", block->header.address + offset);
					}
					break;
				}
			}
		}
	}
	if (flag < 0){
		log_err("unable to fetch instruction");
	}

	return result;
}

static uint32_t assemblySan_search_inst(struct asmBlock* block, uint32_t verbose){
	uint8_t* 			ptr;
	xed_decoded_inst_t 	xedd;
	uint32_t 			result;

	for (ptr = asmBlock_search_instruction(block, crypto_instruction, nb_crypto_instruction, &xedd, 0), result = 0; ptr != NULL; ptr = asmBlock_search_instruction(block, crypto_instruction, nb_crypto_instruction, &xedd, ptr + xed_decoded_inst_get_length(&xedd) - block->data)){
		result |= INSTRUCTION_FLAG;
		if (verbose){
			printf("  - %s @ 0x%08x\n", xed_iclass_enum_t2str(xed_decoded_inst_get_iclass(&xedd)), block->header.address + (ptr - block->data));
		}
	}

	return result;
}

struct flagAsmBlock{
	struct asmBlock* 	block;
	uint32_t 			flag;
	uint32_t 			nb_exec;
	uint32_t 			nb_bitw;
	uint32_t 			taken;
};

static struct array* assemblyScan_create_flag_block_array(const struct assembly* assembly, uint32_t verbose){
	uint32_t 			i;
	struct array* 		flag_block_array;
	struct asmBlock* 	block;
	uint32_t 			block_offset;
	struct flagAsmBlock flag_block;
	xed_decoded_inst_t 	xedd;
	uint8_t* 			ptr;

	if ((flag_block_array = array_create(sizeof(struct flagAsmBlock))) == NULL){
		log_err("unable to create array");
		return NULL;
	}

	for (block_offset = 0; block_offset != assembly->mapping_size_block; block_offset += sizeof(struct asmBlockHeader) + block->header.size){
		block = (struct asmBlock*)((char*)assembly->mapping_block + block_offset);
		if (block_offset + block->header.size + sizeof(struct asmBlockHeader) > assembly->mapping_size_block){
			log_err("the last asmBlock is incomplete");
			break;
		}

		flag_block.block 	= block;
		flag_block.flag 	= assemblySan_search_cst(block, verbose) | assemblySan_search_inst(block, verbose);
		flag_block.nb_exec 	= 0;
		flag_block.taken 	= 0; 

		for (flag_block.nb_bitw = 0, ptr = asmBlock_search_instruction(block, bitwise_instruction, nb_bitwise_instruction, &xedd, 0); ptr != NULL; ptr = asmBlock_search_instruction(block, bitwise_instruction, nb_bitwise_instruction, &xedd, ptr + xed_decoded_inst_get_length(&xedd) - block->data)){
			flag_block.nb_bitw ++;
		}

		if (array_add(flag_block_array, &flag_block) < 0){
			log_err("unable to add element to array");
		}
	}

	for (i = 0; i < assembly->nb_dyn_block; i++){
		if (dynBlock_is_valid(assembly->dyn_blocks + i)){
			if (assembly->dyn_blocks[i].block->header.id - FIRST_BLOCK_ID > array_get_length(flag_block_array)){
				log_err_m("block id (%u) is out of bound", assembly->dyn_blocks[i].block->header.id);
			}
			else{
				((struct flagAsmBlock*)array_get(flag_block_array, assembly->dyn_blocks[i].block->header.id - FIRST_BLOCK_ID))->nb_exec ++;
			}
		}
	}

	return flag_block_array;
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

void assemblyScan_scan(const struct assembly* assembly, void* call_graph, struct codeMap* cm, uint32_t filters){
	struct macroBlock{
		uint32_t 	nb_block;
		uint32_t 	nb_ins;
		uint32_t 	nb_exec;
		uint32_t 	nb_bitw;
		uint32_t 	flag;
		ADDRESS 	addr_strt;
		ADDRESS 	addr_end;
	};

	#define macroBlock_init(macro_block, flag_block) 												\
		(macro_block).nb_block 	= 1; 																\
		(macro_block).nb_ins 	= (flag_block)->block->header.nb_ins; 								\
		(macro_block).nb_exec 	= (flag_block)->nb_exec; 											\
		(macro_block).nb_bitw 	= (flag_block)->nb_bitw; 											\
		(macro_block).flag 		= (flag_block)->flag; 												\
		(macro_block).addr_strt = (flag_block)->block->header.address; 								\
		(macro_block).addr_end 	= (macro_block).addr_strt + (flag_block)->block->header.size;

	#define macroBlock_add(macro_block, flag_block) 												\
		(macro_block).nb_block 	+= 1; 																\
		(macro_block).nb_ins 	+= (flag_block)->block->header.nb_ins; 								\
		(macro_block).nb_bitw 	+= (flag_block)->nb_bitw; 											\
		(macro_block).flag 		|= (flag_block)->flag; 												\
		(macro_block).addr_end 	+= (flag_block)->block->header.size;

	uint32_t 				i;
	uint32_t 				j;
	struct array* 			flag_block_array 	= NULL;
	uint32_t* 				address_mapping 	= NULL;
	struct set* 			function_set 		= NULL;
	struct setIterator 		it;
	struct node** 			func_ptr;
	double 					ratio;
	struct flagAsmBlock* 	flag_block;
	struct macroBlock 		macro_block;

	#ifdef VERBOSE
	if (call_graph != NULL){
		log_info("using callGraph to apply function heuristic(s)");
	}
	#endif

	if ((function_set = set_create(sizeof(struct node*), 16)) == NULL){
		log_err("unable to create set");
		goto exit;
	}

	if ((flag_block_array = assemblyScan_create_flag_block_array(assembly, filters & ASSEMBLYSCAN_FILTER_VERBOSE)) == NULL){
		log_err("unable to create bbl array");
		goto exit;
	}

	if ((address_mapping = array_create_mapping(flag_block_array, asmBlock_compare_address)) == NULL){
		log_err("unable to create array mapping");
		goto exit;
	}

	for (i = 0; i < array_get_length(flag_block_array); i++){
		flag_block = (struct flagAsmBlock*)array_get(flag_block_array, address_mapping[i]);
		if (!flag_block->nb_exec || flag_block->taken){
			continue;
		}

		macroBlock_init(macro_block, flag_block)
		flag_block->taken = 1;

		for (j = j + 1; (macro_block.nb_ins % ASSEMBLYSCAN_NB_MAX_INS_PER_BBL) == 0 && j < array_get_length(flag_block_array); j++){
			flag_block = (struct flagAsmBlock*)array_get(flag_block_array, address_mapping[j]);
			if (flag_block->block->header.address > macro_block.addr_end){
				break;
			}
			else if (!flag_block->taken && macro_block.addr_end == flag_block->block->header.address && macro_block.nb_exec == flag_block->nb_exec){
				macroBlock_add(macro_block, flag_block)
				flag_block->taken = 1;
			}
		}

		if (macro_block.nb_ins == 0){
			continue;
		}

		ratio = 100 * (double)macro_block.nb_bitw / (double)macro_block.nb_ins;

		if (!(macro_block.flag & INSTRUCTION_FLAG) && !((filters & ASSEMBLYSCAN_FILTER_CST) && macro_block.flag)){
			if (macro_block.nb_ins < ASSEMBLYSCAN_NB_MIN_INSTRUCTION && (filters & ASSEMBLYSCAN_FILTER_BBL_SIZE)){
				continue;
			}

			if (ratio < ASSEMBLYSCAN_MIN_BIT_WISE_RATIO && (filters & ASSEMBLYSCAN_FILTER_BBL_RATIO)){
				continue;
			}

			if (macro_block.nb_exec < ASSEMBLYSCAN_NB_MIN_EXECUTION && (filters & ASSEMBLYSCAN_FILTER_BBL_EXEC)){
				continue;
			}
		}

		#ifdef COLOR
		if (ratio < 25){
			printf("%3u bbl, %4u ins, [" PRINTF_ADDR " : " PRINTF_ADDR "], R:%5.2f%%", macro_block.nb_block, macro_block.nb_ins, macro_block.addr_strt, macro_block.addr_end, ratio);
		}
		else if (ratio < 50){
			printf("%3u bbl, %4u ins, [" PRINTF_ADDR " : " PRINTF_ADDR "], R:" "\x1b[35m" "%5.2f%%" ANSI_COLOR_RESET, macro_block.nb_block, macro_block.nb_ins, macro_block.addr_strt, macro_block.addr_end, ratio);
		}
		else{
			printf("%3u bbl, %4u ins, [" PRINTF_ADDR " : " PRINTF_ADDR "], R:" "\x1b[1;35m" "%5.2f%%" ANSI_COLOR_RESET, macro_block.nb_block, macro_block.nb_ins, macro_block.addr_strt, macro_block.addr_end, ratio);
		}
		#else
		printf("%3u bbl, %4u ins, [" PRINTF_ADDR " : " PRINTF_ADDR "], R:%5.2f%%", macro_block.nb_block, macro_block.nb_ins, macro_block.addr_strt, macro_block.addr_end, ratio);
		#endif
		
		printf(", E: %6u", macro_block.nb_exec);

		if (cm != NULL){
			struct cm_routine* rtn;

			if ((rtn = codeMap_search_routine(cm, macro_block.addr_strt)) != NULL){
				printf(", RTN:%s+" PRINTF_ADDR_SHORT, rtn->name, macro_block.addr_strt - rtn->address_start);
			}
		}

		if (macro_block.flag & INSTRUCTION_FLAG){
			fputs(" " ANSI_COLOR_BOLD_YELLOW "INST" ANSI_COLOR_RESET, stdout);
		}
		if (macro_block.flag & MD5_FLAG){
			fputs(" " ANSI_COLOR_BOLD_YELLOW MD5_NAME ANSI_COLOR_RESET, stdout);
		}
		if (macro_block.flag & SHA1_FLAG){
			fputs(" " ANSI_COLOR_BOLD_YELLOW SHA1_NAME ANSI_COLOR_RESET, stdout);
		}
		if (macro_block.flag & HAVAL_FLAG){
			fputs(" " ANSI_COLOR_BOLD_YELLOW HAVAL_NAME ANSI_COLOR_RESET, stdout);
		}
		if (macro_block.flag & TEA_FLAG){
			fputs(" " ANSI_COLOR_BOLD_YELLOW TEA_NAME ANSI_COLOR_RESET, stdout);
		}
		fputc('\n', stdout);

		if (call_graph != NULL){
			assemblyScan_get_function(assembly, ((struct flagAsmBlock*)array_get(flag_block_array, address_mapping[i]))->block, call_graph, function_set); /* chaud */
		}
	}

	free(address_mapping);

	log_info_m("%d block(s) have been scanned", array_get_length(flag_block_array));

	for (func_ptr = setIterator_get_first(function_set, &it); func_ptr; func_ptr = setIterator_get_next(&it)){
		if (callGraphNode_is_leaf(call_graph, *func_ptr, assembly)){
			fputs("L:" ANSI_COLOR_BOLD_GREEN "Y" ANSI_COLOR_RESET ", ", stdout);
		}
		else if (!(filters & ASSEMBLYSCAN_FILTER_FUNC_LEAF)){
			fputs("L:" ANSI_COLOR_BOLD_RED "N" ANSI_COLOR_RESET ", ", stdout);
		}
		else{
			continue;
		}
		callGraph_fprint_node(call_graph, *func_ptr, stdout);
		putchar('\n');
	}

	log_info_m("%d dyn function(s) have been scanned", set_get_length(function_set));

	exit:
	if (flag_block_array != NULL){
		array_delete(flag_block_array);
	}
	if (function_set != NULL){
		set_delete(function_set);
	}
}