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

#define TRACER_INSTRUCTION_BUFFER_SIZE 1024

struct tracer{
	struct codeMap* 	code_map;
	struct whiteList*	white_list;
	struct traceFiles* 	trace;

	struct instruction* current_instruction;

	struct instruction* buffer;
	uint32_t 			buffer_offset;
};

/* Selector for the analysis routine value:
 * - bit 1: set to 0 -> 0 MEM READ 		set to 1 -> 1 MEM READ
 * - bit 2: set to 0 -> 0 MEM READ 		set to 1 -> 2 MEM READ
 * - bit 3: set to 0 -> 0 MEM WRITE 	set to 1 -> 1 MEM WRITE
 */

#define ANALYSIS_SELECTOR_SET_1MR(s) 	(s) |= 0x00000001
#define ANALYSIS_SELECTOR_SET_2MR(s) 	(s) |= 0x00000003
#define ANALYSIS_SELECTOR_SET_1MW(s)	(s) |= 0x00000004
#define ANALYSIS_SELECTOR_SET_1RR(s)	(s) |= 0x00000008
#define ANALYSIS_SELECTOR_SET_2RR(s) 	(s) |= 0x00000018
#define ANALYSIS_SELECTOR_SET_3RR(s) 	(s) |= 0x00000038

#define ANALYSIS_SELECTOR_NO_ARG		0x00000000
#define ANALYSIS_SELECTOR_1MR			0x00000001
#define ANALYSIS_SELECTOR_2MR 			0x00000003
#define ANALYSIS_SELECTOR_1MW 			0x00000004
#define ANALYSIS_SELECTOR_1MR_1MW 		0x00000005
#define ANALYSIS_SELECTOR_2MR_1MW 		0x00000007
#define ANALYSIS_SELECTOR_1RR 			0x00000008
#define ANALYSIS_SELECTOR_2RR 			0x00000018
#define ANALYSIS_SELECTOR_3RR 			0x00000038
#define ANALYSIS_SELECTOR_1MR_1RR		0x00000009
#define ANALYSIS_SELECTOR_1MR_2RR 		0x00000019
#define ANALYSIS_SELECTOR_1MR_3RR		0x00000039
#define ANALYSIS_SELECTOR_1MW_1RR		0x0000000c
#define ANALYSIS_SELECTOR_1MW_2RR 		0x0000001c
#define ANALYSIS_SELECTOR_1MW_3RR 		0x0000003c
#define ANALYSIS_SELECTOR_1MR_1MW_1RR 	0x0000000d
#define ANALYSIS_SELECTOR_1MR_1MW_2RR 	0x0000001d
#define ANALYSIS_SELECTOR_1MR_1MW_3RR 	0x0000003d

#endif