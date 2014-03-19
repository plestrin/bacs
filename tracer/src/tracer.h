#ifndef TRACER_H
#define TRACER_H

#include <stdint.h>

#include "codeMap.h"
#include "instruction.h"
#include "whiteList.h"

#define TRACERBUFFER_SIZE_INS 		 	1024
#define TRACERBUFFER_SIZE_OP 			2048
#define TRACERBUFFER_SIZE_DATA 			4096
#define TRACERBUFFER_NB_PENDING_WRITE	4


struct tracePendingWrite{
	union {
		ADDRESS 				address;
		enum reg 				reg;
	}							location;
	uint8_t 					size;
};

struct traceBuffer{
	struct instruction 			buffer_ins[TRACERBUFFER_SIZE_INS];
	struct operand 				buffer_op[TRACERBUFFER_SIZE_OP];
	uint8_t 					buffer_data[TRACERBUFFER_SIZE_DATA];

	uint32_t 					local_offset_ins;
	uint32_t 					local_offset_op;
	uint32_t 					local_offset_data;

	uint32_t 					global_offset_op;
	uint32_t 					global_offset_data;

	struct tracePendingWrite 	pending_write[TRACERBUFFER_NB_PENDING_WRITE];
};

#define traceBuffer_reserve_instruction(trace_buffer, trace_file, nb_instruction) 													\
	if ((trace_buffer)->local_offset_ins + (nb_instruction) >= TRACERBUFFER_SIZE_INS){ 												\
		traceFiles_flush_instruction((trace_file), (trace_buffer)->buffer_ins, (trace_buffer)->local_offset_ins); 					\
		(trace_buffer)->local_offset_ins = 0; 																						\
	}

#define traceBuffer_reserve_operand(trace_buffer, trace_file, nb_operand) 															\
	if ((trace_buffer)->local_offset_op + (nb_operand) >= TRACERBUFFER_SIZE_OP){ 													\
		traceFiles_flush_operand((trace_file), (trace_buffer)->buffer_op, (trace_buffer)->local_offset_op); 						\
		(trace_buffer)->local_offset_op = 0; 																						\
	}

#define traceBuffer_reserve_data(trace_buffer, trace_file, nb_data) 																\
	if ((trace_buffer)->local_offset_data + (nb_data) >= TRACERBUFFER_SIZE_DATA){ 													\
		traceFiles_flush_data((trace_file), (trace_buffer)->buffer_data, (trace_buffer)->local_offset_data); 						\
		(trace_buffer)->local_offset_data = 0; 																						\
	}

#define traceBuffer_flush(trace_buffer, trace_file) 																				\
	if ((trace_buffer)->local_offset_ins != 0){ 																					\
		traceFiles_flush_instruction((trace_file), (trace_buffer)->buffer_ins, (trace_buffer)->local_offset_ins); 					\
		(trace_buffer)->local_offset_ins = 0; 																						\
	} 																																\
	if ((trace_buffer)->local_offset_op != 0){ 																						\
		traceFiles_flush_operand((trace_file), (trace_buffer)->buffer_op, (trace_buffer)->local_offset_op); 						\
		(trace_buffer)->local_offset_op = 0; 																						\
	} 																																\
	if ((trace_buffer)->local_offset_data != 0){ 																					\
		traceFiles_flush_data((trace_file), (trace_buffer)->buffer_data, (trace_buffer)->local_offset_data); 						\
		(trace_buffer)->local_offset_data = 0; 																						\
	}

#define traceBuffer_commit_operand(trace_buffer, size) 																				\
	(trace_buffer)->local_offset_op 	+= 1; 																						\
	(trace_buffer)->local_offset_data 	+= (size); 																					\
	(trace_buffer)->global_offset_op 	+= 1; 																						\
	(trace_buffer)->global_offset_data 	+= (size);

#define traceBuffer_add_instruction(trace_buffer, trace_file, pc_, opcode_, nb_operand_) 											\
	traceBuffer_reserve_operand((trace_buffer), (trace_file), (nb_operand_))														\
	traceBuffer_reserve_instruction((trace_buffer), (trace_file), 1) 																\
 																																	\
	(trace_buffer)->buffer_ins[(trace_buffer)->local_offset_ins].pc 				= (pc_); 										\
	(trace_buffer)->buffer_ins[(trace_buffer)->local_offset_ins].opcode 			= (opcode_); 									\
	(trace_buffer)->buffer_ins[(trace_buffer)->local_offset_ins].operand_offset 	= (trace_buffer)->global_offset_op; 			\
	(trace_buffer)->buffer_ins[(trace_buffer)->local_offset_ins].nb_operand 		= (nb_operand_); 								\
 																																	\
	(trace_buffer)->local_offset_ins += 1;

#define traceBuffer_add_read_register_operand(trace_buffer, regDesc, value)															\
	(trace_buffer)->buffer_op[(trace_buffer)->local_offset_op].type 			= OPERAND_REG_READ;									\
	(trace_buffer)->buffer_op[(trace_buffer)->local_offset_op].location.reg 	= ANALYSIS_REGISTER_DESCRIPTOR_GET_REG(regDesc);	\
	(trace_buffer)->buffer_op[(trace_buffer)->local_offset_op].size 			= ANALYSIS_REGISTER_DESCRIPTOR_GET_SIZE(regDesc); 	\
	(trace_buffer)->buffer_op[(trace_buffer)->local_offset_op].data_offset 		= (trace_buffer)->global_offset_data; 				\
																																	\
	*(uint32_t*)((trace_buffer)->buffer_data + (trace_buffer)->local_offset_data) = (value); 										\
																																	\
	traceBuffer_commit_operand((trace_buffer), ANALYSIS_REGISTER_DESCRIPTOR_GET_SIZE(regDesc))

struct tracer{
	struct codeMap* 			code_map;
	struct whiteList*			white_list;
	struct traceFiles* 			trace_file;
	struct traceBuffer*			trace_buffer;
};

#define ANALYSIS_REGISTER_READ_STD 		0x00
#define ANALYSIS_REGISTER_READ_BASE 	0x01
#define ANALYSIS_REGISTER_READ_INDEX 	0x02

/* Register descriptor for the analysis routine
 * - bit [0 :15] 	: enum reg
 * - bit [16:23] 	: register size
 * - bit [24:31] 	: register access mode (STD = 0x00, BASE = 0x01, INDEX = 0x02)
 */

#define ANALYSIS_PACK_REGISTER_DESCRIPTOR(reg, size, type) 	(((reg) & 0x0000ffff) | (((size) & 0x000000ff) << 16) | (((type) & 0x00000003) << 24))
#define ANALYSIS_REGISTER_DESCRIPTOR_GET_REG(desc)			((enum reg)((desc) & 0x0000ffff))
#define ANALYSIS_REGISTER_DESCRIPTOR_GET_SIZE(desc)			(((desc) >> 16) & 0x000000ff)
#define ANALYSIS_REGISTER_DESCRIPTOR_GET_TYPE(desc) 		(((desc) >> 24) & 0x00000003)

#define ANALYSIS_MAX_OPERAND_MEM_READ 	2
#define ANALYSIS_MAX_OPERAND_MEM_WRITE 	1
#define ANALYSIS_MAX_OPERAND_REG_READ 	3
#define ANALYSIS_MAX_OPERAND_REG_WRITE 	4

/* Selector for the analysis routine value:
 * - bit [0 :7 ] 	: number of memory read
 * - bit [8 :15] 	: number of memory write
 * - bit [16:23]	: number of register read
 * - bit [24:31] 	: number of regsiter write
 */

#define ANALYSIS_SELECTOR_INC_MR(s) 		(s) += 0x00000001
#define ANALYSIS_SELECTOR_INC_MW(s) 		(s) += 0x00000100
#define ANALYSIS_SELECTOR_INC_RR(s) 		(s) += 0x00010000
#define ANALYSIS_SELECTOR_INC_RW(s) 		(s) += 0x01000000

#define ANALYSIS_SELECTOR_GET_MR_COUNT(s)	((s) & 0x000000ff)
#define ANALYSIS_SELECTOR_GET_MW_COUNT(s)	(((s) >> 8) & 0x000000ff)
#define ANALYSIS_SELECTOR_GET_RR_COUNT(s)	(((s) >> 16) & 0x000000ff)
#define ANALYSIS_SELECTOR_GET_RW_COUNT(s)	((s) >> 24)

#define ANALYSIS_SELECTOR_NO_OPERAND		0x00000000
#define ANALYSIS_SELECTOR_1MR				0x00000001
#define ANALYSIS_SELECTOR_2MR 				0x00000002
#define ANALYSIS_SELECTOR_1MW 				0x00000100
#define ANALYSIS_SELECTOR_1MR_1MW 			0x00000101
#define ANALYSIS_SELECTOR_2MR_1MW 			0x00000102
#define ANALYSIS_SELECTOR_1RR 				0x00010000
#define ANALYSIS_SELECTOR_1MR_1RR			0x00010001
#define ANALYSIS_SELECTOR_1MW_1RR			0x00010100
#define ANALYSIS_SELECTOR_1MR_1MW_1RR 		0x00010101
#define ANALYSIS_SELECTOR_2RR 				0x00020000
#define ANALYSIS_SELECTOR_1MR_2RR 			0x00020001
#define ANALYSIS_SELECTOR_1MW_2RR 			0x00020100
#define ANALYSIS_SELECTOR_1MR_1MW_2RR 		0x00020101
#define ANALYSIS_SELECTOR_3RR 				0x00030000
#define ANALYSIS_SELECTOR_1MR_3RR			0x00030001
#define ANALYSIS_SELECTOR_1MW_3RR 			0x00030100
#define ANALYSIS_SELECTOR_1MR_1MW_3RR 		0x00030101
#define ANALYSIS_SELECTOR_1RW 				0x01000000
#define ANALYSIS_SELECTOR_1MR_1RW 			0x01000001
#define ANALYSIS_SELECTOR_1MW_1RW 			0x01000100
#define ANALYSIS_SELECTOR_1RR_1RW			0x01010000
#define ANALYSIS_SELECTOR_1MR_1RR_1RW 		0x01010001
#define ANALYSIS_SELECTOR_2RR_1RW			0x01020000
#define ANALYSIS_SELECTOR_1MR_2RR_1RW 		0x01020001
#define ANALYSIS_SELECTOR_1MR_1MW_2RR_1RW 	0x01020101 /**/
#define ANALYSIS_SELECTOR_3RR_1RW			0x01030000
#define ANALYSIS_SELECTOR_1MR_3RR_1RW 		0x01030001
#define ANALYSIS_SELECTOR_2RW 				0x02000000
#define ANALYSIS_SELECTOR_1MR_2RW 			0x02000001
#define ANALYSIS_SELECTOR_1MW_2RW 			0x02000100
#define ANALYSIS_SELECTOR_1RR_2RW			0x02010000
#define ANALYSIS_SELECTOR_2RR_2RW 			0x02020000
#define ANALYSIS_SELECTOR_3RR_2RW 			0x02030000 /**/
#define ANALYSIS_SELECTOR_3RW 				0x03000000
#define ANALYSIS_SELECTOR_1MR_3RW 			0x03000001
#define ANALYSIS_SELECTOR_1MW_3RW 			0x03000100


#endif