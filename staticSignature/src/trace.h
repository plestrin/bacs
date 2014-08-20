#ifndef TRACE_H
#define TRACE_H

#include <stdint.h>

#include "instruction.h"
#include "assembly.h"

enum traceAllocation{
	TRACEALLOCATION_MALLOC,
	TRACEALLOCATION_MMAP
};

struct trace{
	struct instruction* 	instructions;
	struct operand* 		operands;
	uint8_t* 				data;

	uint32_t 				alloc_size_ins;
	uint32_t 				alloc_size_op;
	uint32_t 				alloc_size_data;

	uint32_t 				nb_instruction;

	uint32_t 				reference_count;
	enum traceAllocation 	allocation_type;

	struct assembly 		assembly;
};

struct trace* trace_create(const char* directory_path);
int32_t trace_init(struct trace* trace, const char* directory_path);

void trace_check(struct trace* trace);

void trace_print(struct trace* trace, uint32_t start, uint32_t stop);

int32_t trace_extract_segment(struct trace* trace_src, struct trace* trace_dst, uint32_t offset, uint32_t length);

void trace_clean(struct trace* trace);
void trace_delete(struct trace* trace);

#define trace_get_reference(trace) 					((trace)->reference_count ++)
#define trace_get_ins_operands(trace, index) 		((trace)->operands + (trace)->instructions[(index)].operand_offset)
#define trace_get_ins_op_data(trace, i_ins, i_op)	((trace)->data + (trace)->operands[(trace)->instructions[(i_ins)].operand_offset + (i_op)].data_offset)
#define trace_get_op_data(trace, index) 			((trace)->data + (trace)->operands[(index)].data_offset)

#define trace_get_nb_operand(trace) 				((trace)->alloc_size_op / sizeof(struct operand))

static inline uint32_t trace_get_ins_nb_read_op(struct trace* trace, uint32_t index){
	uint32_t 		nb;
	uint32_t 		i;
	struct operand* operands;

	for (i = 0, nb = 0; i < trace->instructions[index].nb_operand; i++){
		operands = trace_get_ins_operands(trace, index);
		if (OPERAND_IS_READ(operands[i])){
			nb ++;
		}
	}

	return nb;
}

static inline uint32_t trace_get_ins_nb_write_op(struct trace* trace, uint32_t index){
	uint32_t 		nb;
	uint32_t 		i;
	struct operand*	operands;

	for (i = 0, nb = 0; i < trace->instructions[index].nb_operand; i++){
		operands = trace_get_ins_operands(trace, index);
		if (OPERAND_IS_WRITE(operands[i])){
			nb ++;
		}
	}

	return nb;
}

#endif