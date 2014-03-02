#ifndef TRACER_H
#define TRACER_H

#include <stdint.h>

#ifdef __linux__

#include "codeMap.h"
#include "instruction.h"

#endif

#ifdef WIN32

#include "../../../shared/codeMap.h"
#include "../../../shared/instruction.h"

#ifndef __func__
#define __func__ __FUNCTION__
#endif

#endif

#include "whiteList.h"

#define TRACER_INSTRUCTION_BUFFER_SIZE 1024 /*tmp*/

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
	traceBuffer_reserve_instruction((trace_buffer), (trace_file), 1) 																\
 																																	\
	(trace_buffer)->buffer_ins[(trace_buffer)->local_offset_ins].pc 				= (pc_); 										\
	(trace_buffer)->buffer_ins[(trace_buffer)->local_offset_ins].opcode 			= (opcode_); 									\
	(trace_buffer)->buffer_ins[(trace_buffer)->local_offset_ins].operand_offset 	= (trace_buffer)->global_offset_op; 			\
	(trace_buffer)->buffer_ins[(trace_buffer)->local_offset_ins].nb_operand 		= (nb_operand_); 								\
 																																	\
	(trace_buffer)->local_offset_ins += 1;

struct tracer{
	struct codeMap* 			code_map;
	struct whiteList*			white_list;
	struct traceFiles* 			trace_file;
	struct traceBuffer*			trace_buffer;
};

/* Selector for the analysis routine value:
 * - bit [0:1] 	set to 0 -> 0 MEM READ 		set to 1 -> 1 MEM READ 			set to 3 -> 2 MEM READ
 * - bit 2 		set to 0 -> 0 MEM WRITE 	set to 1 -> 1 MEM WRITE
 * - bit[3:5]	set to 0 -> 0 REG READ 		set to 1 -> 1 REG READ 			set to 3 -> 2 REG READ 			set to 7 -> 3 REG READ
 * - bit[8:11] 	set to 0 -> 0 REG WRITE 	set to 1 -> 1 REG WRITE 		set to 3 -> 2 REG WRITE 		set to 7 -> 3 REG WRITE 		set to f -> 4 REG WRITE
 */

#define ANALYSIS_SELECTOR_SET_1MR(s) 	(s) |= 0x00000001
#define ANALYSIS_SELECTOR_SET_2MR(s) 	(s) |= 0x00000003
#define ANALYSIS_SELECTOR_SET_1MW(s)	(s) |= 0x00000004
#define ANALYSIS_SELECTOR_SET_1RR(s)	(s) |= 0x00000008
#define ANALYSIS_SELECTOR_SET_2RR(s) 	(s) |= 0x00000018
#define ANALYSIS_SELECTOR_SET_3RR(s) 	(s) |= 0x00000038
#define ANALYSIS_SELECTOR_SET_1RW(s)	(s) |= 0x00000100
#define ANALYSIS_SELECTOR_SET_2RW(s)	(s) |= 0x00000300
#define ANALYSIS_SELECTOR_SET_3RW(s)	(s) |= 0x00000700
#define ANALYSIS_SELECTOR_SET_4RW(s)	(s) |= 0x00000f00

#define ANALYSIS_SELECTOR_NO_ARG			0x00000000
#define ANALYSIS_SELECTOR_1MR				0x00000001
#define ANALYSIS_SELECTOR_2MR 				0x00000003
#define ANALYSIS_SELECTOR_1MW 				0x00000004
#define ANALYSIS_SELECTOR_1MR_1MW 			0x00000005
#define ANALYSIS_SELECTOR_2MR_1MW 			0x00000007
#define ANALYSIS_SELECTOR_1RR 				0x00000008
#define ANALYSIS_SELECTOR_1MR_1RR			0x00000009
#define ANALYSIS_SELECTOR_1MW_1RR			0x0000000c
#define ANALYSIS_SELECTOR_1MR_1MW_1RR 		0x0000000d
#define ANALYSIS_SELECTOR_2RR 				0x00000018
#define ANALYSIS_SELECTOR_1MR_2RR 			0x00000019
#define ANALYSIS_SELECTOR_1MW_2RR 			0x0000001c
#define ANALYSIS_SELECTOR_1MR_1MW_2RR 		0x0000001d
#define ANALYSIS_SELECTOR_3RR 				0x00000038
#define ANALYSIS_SELECTOR_1MR_3RR			0x00000039
#define ANALYSIS_SELECTOR_1MW_3RR 			0x0000003c
#define ANALYSIS_SELECTOR_1MR_1MW_3RR 		0x0000003d
#define ANALYSIS_SELECTOR_1RW 				0x00000100
#define ANALYSIS_SELECTOR_1MR_1RW 			0x00000101
#define ANALYSIS_SELECTOR_1MW_1RW 			0x00000104
#define ANALYSIS_SELECTOR_1RR_1RW			0x00000108
#define ANALYSIS_SELECTOR_1MR_1RR_1RW 		0x00000109
#define ANALYSIS_SELECTOR_2RR_1RW			0x00000118
#define ANALYSIS_SELECTOR_1MR_2RR_1RW 		0x00000119
#define ANALYSIS_SELECTOR_1MR_1MW_2RR_1RW 	0x0000011d /**/
#define ANALYSIS_SELECTOR_3RR_1RW			0x00000138
#define ANALYSIS_SELECTOR_1MR_3RR_1RW 		0x00000139
#define ANALYSIS_SELECTOR_2RW 				0x00000300
#define ANALYSIS_SELECTOR_1MR_2RW 			0x00000301
#define ANALYSIS_SELECTOR_1MW_2RW 			0x00000304
#define ANALYSIS_SELECTOR_1RR_2RW			0x00000308
#define ANALYSIS_SELECTOR_2RR_2RW 			0x00000318
#define ANALYSIS_SELECTOR_3RR_2RW 			0x00000338 /**/
#define ANALYSIS_SELECTOR_3RW 				0x00000700
#define ANALYSIS_SELECTOR_1MR_3RW 			0x00000701
#define ANALYSIS_SELECTOR_1MW_3RW 			0x00000704


#endif