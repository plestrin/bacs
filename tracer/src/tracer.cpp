#include <iostream>
#include <fstream>
#include <string.h>
#include <stdlib.h>
#include "pin.H"

#include "tracer.h"
#include "traceFiles.h"

#define DEFAULT_TRACE_FILE_NAME 		"trace"
#define DEFAULT_WHITE_LIST_FILE_NAME	""

struct tracer	tracer;

KNOB<string> 	knob_trace(KNOB_MODE_WRITEONCE, "pintool", "o", DEFAULT_TRACE_FILE_NAME, "Specify a directory to write trace results");
KNOB<string> 	knob_white_list(KNOB_MODE_WRITEONCE, "pintool", "w", DEFAULT_WHITE_LIST_FILE_NAME, "(Optional) Shared library white list. Specify file name");

static inline uint8_t pintool_monitor_REG(REG reg){
	if (reg == REG_EAX ||
		reg == REG_AX  ||
		reg == REG_AH  ||
		reg == REG_AL  ||
		reg == REG_EBX ||
		reg == REG_BX  ||
		reg == REG_BH  ||
		reg == REG_BL  ||
		reg == REG_ECX ||
		reg == REG_CX  ||
		reg == REG_CH  ||
		reg == REG_CL  ||
		reg == REG_EDX ||
		reg == REG_DX  ||
		reg == REG_DH  ||
		reg == REG_DL  ||
		reg == REG_ESI ||
		reg == REG_EDI ||
		reg == REG_EBP){
		return 1;
	}
	return 0;
}

static inline uint32_t pintool_REG_size(REG reg){
	if (reg == REG_EAX || reg == REG_EBX || reg == REG_ECX || reg == REG_EDX || reg == REG_ESI || reg == REG_EDI || reg == REG_EBP){
		return 4;
	}
	else if (reg == REG_AX || reg == REG_BX || reg == REG_CX || reg == REG_DX){
		return 2;
	}
	else if (reg == REG_AH || reg == REG_AL || reg == REG_BH || reg == REG_BL || reg == REG_CH || reg == REG_CL || reg == REG_DH || reg == REG_DL){
		return 1;
	}
	else{
		printf("ERROR: in %s, unknown register (%s) by default size is set to 1\n", __func__, REG_StringShort(reg).c_str());
	}

	return 1;
}

static inline enum reg pintool_REG_2_reg(REG reg){
	switch(reg){
	case REG_EAX : {return REGISTER_EAX;}
	case REG_AX  : {return REGISTER_AX;}
	case REG_AH  : {return REGISTER_AH;}
	case REG_AL  : {return REGISTER_AL;}
	case REG_EBX : {return REGISTER_EBX;}
	case REG_BX  : {return REGISTER_BX;}
	case REG_BH  : {return REGISTER_BH;}
	case REG_BL  : {return REGISTER_BL;}
	case REG_ECX : {return REGISTER_ECX;}
	case REG_CX  : {return REGISTER_CX;}
	case REG_CH  : {return REGISTER_CH;}
	case REG_CL  : {return REGISTER_CL;}
	case REG_EDX : {return REGISTER_EDX;}
	case REG_DX  : {return REGISTER_DX;}
	case REG_DH  : {return REGISTER_DH;}
	case REG_DL  : {return REGISTER_DL;}
	case REG_ESI : {return REGISTER_ESI;}
	case REG_EDI : {return REGISTER_EDI;}
	case REG_EBP : {return REGISTER_EBP;}
	default : {printf("ERROR: in %s, this register (%s) is meant to be monitored\n", __func__, REG_StringShort(reg).c_str()); break;}
	}

	return REGISTER_INVALID;
}


/* ===================================================================== */
/* Analysis function(s) 	                                             */
/* ===================================================================== */

void pintool_instruction_analysis_no_arg(ADDRINT pc, UINT32 opcode){
	traceBuffer_add_instruction(tracer.trace_buffer, tracer.trace_file, pc, opcode, 0)
}

void pintool_instruction_analysis_1read_mem(ADDRINT pc, UINT32 opcode, ADDRINT address, UINT32 size){
	traceBuffer_reserve_operand(tracer.trace_buffer, tracer.trace_file, 1)
	traceBuffer_reserve_data(tracer.trace_buffer, tracer.trace_file, size)

	traceBuffer_add_instruction(tracer.trace_buffer, tracer.trace_file, pc, opcode, 1)

	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].type 				= OPERAND_MEM_READ;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].location.address 	= address;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].size 				= size;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].data_offset 		= tracer.trace_buffer->global_offset_data;

	PIN_SafeCopy(tracer.trace_buffer->buffer_data + tracer.trace_buffer->local_offset_data, (void*)address, size);

	traceBuffer_commit_operand(tracer.trace_buffer, size)
}

void pintool_instruction_analysis_1write_mem_p1(ADDRINT pc, UINT32 opcode, ADDRINT address, UINT32 size){
	traceBuffer_reserve_operand(tracer.trace_buffer, tracer.trace_file, 1)
	traceBuffer_reserve_data(tracer.trace_buffer, tracer.trace_file, size)

	traceBuffer_add_instruction(tracer.trace_buffer, tracer.trace_file, pc, opcode, 1)

	tracer.trace_buffer->pending_write[0].location.address 									= address;
	tracer.trace_buffer->pending_write[0].size 												= size;
}

void pintool_instruction_analysis_1write_mem_1read_mem_p1(ADDRINT pc, UINT32 opcode, ADDRINT address_write, UINT32 size_write, ADDRINT address_read, UINT32 size_read){
	traceBuffer_reserve_operand(tracer.trace_buffer, tracer.trace_file, 2)
	traceBuffer_reserve_data(tracer.trace_buffer, tracer.trace_file, size_write + size_read)

	traceBuffer_add_instruction(tracer.trace_buffer, tracer.trace_file, pc, opcode, 2)

	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].type 				= OPERAND_MEM_READ;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].location.address 	= address_read;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].size 				= size_read;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].data_offset 		= tracer.trace_buffer->global_offset_data;

	PIN_SafeCopy(tracer.trace_buffer->buffer_data + tracer.trace_buffer->local_offset_data, (void*)address_read, size_read);

	traceBuffer_commit_operand(tracer.trace_buffer, size_read)

	tracer.trace_buffer->pending_write[0].location.address 									= address_write;
	tracer.trace_buffer->pending_write[0].size 												= size_write;
}

void pintool_instruction_analysis_1read_reg(ADDRINT pc, UINT32 opcode, UINT32 name, UINT32 value, UINT32 size){
	traceBuffer_reserve_operand(tracer.trace_buffer, tracer.trace_file, 1)
	traceBuffer_reserve_data(tracer.trace_buffer, tracer.trace_file, 4)

	traceBuffer_add_instruction(tracer.trace_buffer, tracer.trace_file, pc, opcode, 1)

	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].type 				= OPERAND_REG_READ;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].location.reg 		= (enum reg)name;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].size 				= size;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].data_offset 		= tracer.trace_buffer->global_offset_data;

	*(uint32_t*)(tracer.trace_buffer->buffer_data + tracer.trace_buffer->local_offset_data) = value;

	traceBuffer_commit_operand(tracer.trace_buffer, size)
}

void pintool_instruction_analysis_1read_mem_1read_reg(ADDRINT pc, UINT32 opcode, ADDRINT address_mem, UINT32 size_mem, UINT32 name_reg, UINT32 value_reg, UINT32 size_reg){
	traceBuffer_reserve_operand(tracer.trace_buffer, tracer.trace_file, 2)
	traceBuffer_reserve_data(tracer.trace_buffer, tracer.trace_file, size_mem + 4)

	traceBuffer_add_instruction(tracer.trace_buffer, tracer.trace_file, pc, opcode, 2)

	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].type 				= OPERAND_MEM_READ;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].location.address 	= address_mem;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].size 				= size_mem;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].data_offset 		= tracer.trace_buffer->global_offset_data;

	PIN_SafeCopy(tracer.trace_buffer->buffer_data + tracer.trace_buffer->local_offset_data, (void*)address_mem, size_mem);
	
	traceBuffer_commit_operand(tracer.trace_buffer, size_mem)

	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].type 				= OPERAND_REG_READ;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].location.reg 		= (enum reg)name_reg;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].size 				= size_reg;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].data_offset 		= tracer.trace_buffer->global_offset_data;

	*(uint32_t*)(tracer.trace_buffer->buffer_data + tracer.trace_buffer->local_offset_data) = value_reg;

	traceBuffer_commit_operand(tracer.trace_buffer, size_reg)
}

void pintool_instruction_analysis_1write_mem_1read_reg_p1(ADDRINT pc, UINT32 opcode, ADDRINT address_mem, UINT32 size_mem, UINT32 name_reg, UINT32 value_reg, UINT32 size_reg){
	traceBuffer_reserve_operand(tracer.trace_buffer, tracer.trace_file, 2)
	traceBuffer_reserve_data(tracer.trace_buffer, tracer.trace_file, size_mem + 4)

	traceBuffer_add_instruction(tracer.trace_buffer, tracer.trace_file, pc, opcode, 2)

	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].type 				= OPERAND_REG_READ;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].location.reg 		= (enum reg)name_reg;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].size 				= size_reg;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].data_offset 		= tracer.trace_buffer->global_offset_data;

	*(uint32_t*)(tracer.trace_buffer->buffer_data + tracer.trace_buffer->local_offset_data) = value_reg;

	traceBuffer_commit_operand(tracer.trace_buffer, size_reg)

	tracer.trace_buffer->pending_write[0].location.address 									= address_mem;
	tracer.trace_buffer->pending_write[0].size 												= size_mem;
}

void pintool_instruction_analysis_1write_mem_1read_mem_1read_reg_p1(ADDRINT pc, UINT32 opcode, ADDRINT address_mem_write, UINT32 size_mem_write, ADDRINT address_mem_read, UINT32 size_mem_read, UINT32 name_reg, UINT32 value_reg, UINT32 size_reg){
	traceBuffer_reserve_operand(tracer.trace_buffer, tracer.trace_file, 3)
	traceBuffer_reserve_data(tracer.trace_buffer, tracer.trace_file, size_mem_write + size_mem_read + 4)

	traceBuffer_add_instruction(tracer.trace_buffer, tracer.trace_file, pc, opcode, 3)

	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].type 				= OPERAND_MEM_READ;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].location.address 	= address_mem_read;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].size 				= size_mem_read;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].data_offset 		= tracer.trace_buffer->global_offset_data;

	PIN_SafeCopy(tracer.trace_buffer->buffer_data + tracer.trace_buffer->local_offset_data, (void*)address_mem_read, size_mem_read);
	
	traceBuffer_commit_operand(tracer.trace_buffer, size_mem_read)

	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].type 				= OPERAND_REG_READ;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].location.reg 		= (enum reg)name_reg;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].size 				= size_reg;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].data_offset 		= tracer.trace_buffer->global_offset_data;

	*(uint32_t*)(tracer.trace_buffer->buffer_data + tracer.trace_buffer->local_offset_data) = value_reg;

	traceBuffer_commit_operand(tracer.trace_buffer, size_reg)

	tracer.trace_buffer->pending_write[0].location.address 									= address_mem_write;
	tracer.trace_buffer->pending_write[0].size 												= size_mem_write;
}

void pintool_instruction_analysis_2read_reg(ADDRINT pc, UINT32 opcode, UINT32 name1, UINT32 value1, UINT32 size1, UINT32 name2, UINT32 value2, UINT32 size2){
	traceBuffer_reserve_operand(tracer.trace_buffer, tracer.trace_file, 2)
	traceBuffer_reserve_data(tracer.trace_buffer, tracer.trace_file, 8)

	traceBuffer_add_instruction(tracer.trace_buffer, tracer.trace_file, pc, opcode, 2)

	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].type 				= OPERAND_REG_READ;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].location.reg 		= (enum reg)name1;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].size 				= size1;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].data_offset 		= tracer.trace_buffer->global_offset_data;

	*(uint32_t*)(tracer.trace_buffer->buffer_data + tracer.trace_buffer->local_offset_data) = value1;

	traceBuffer_commit_operand(tracer.trace_buffer, size1)

	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].type 				= OPERAND_REG_READ;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].location.reg 		= (enum reg)name2;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].size 				= size2;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].data_offset 		= tracer.trace_buffer->global_offset_data;

	*(uint32_t*)(tracer.trace_buffer->buffer_data + tracer.trace_buffer->local_offset_data) = value2;

	traceBuffer_commit_operand(tracer.trace_buffer, size2)
}

void pintool_instruction_analysis_1read_mem_2read_reg(ADDRINT pc, UINT32 opcode, ADDRINT address_mem, UINT32 size_mem, UINT32 name_reg1, UINT32 value_reg1, UINT32 size_reg1, UINT32 name_reg2, UINT32 value_reg2, UINT32 size_reg2){
	traceBuffer_reserve_operand(tracer.trace_buffer, tracer.trace_file, 3)
	traceBuffer_reserve_data(tracer.trace_buffer, tracer.trace_file, size_mem + 8)

	traceBuffer_add_instruction(tracer.trace_buffer, tracer.trace_file, pc, opcode, 3)

	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].type 				= OPERAND_MEM_READ;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].location.address 	= address_mem;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].size 				= size_mem;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].data_offset 		= tracer.trace_buffer->global_offset_data;

	PIN_SafeCopy(tracer.trace_buffer->buffer_data + tracer.trace_buffer->local_offset_data, (void*)address_mem, size_mem);
	
	traceBuffer_commit_operand(tracer.trace_buffer, size_mem)

	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].type 				= OPERAND_REG_READ;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].location.reg 		= (enum reg)name_reg1;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].size 				= size_reg1;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].data_offset 		= tracer.trace_buffer->global_offset_data;

	*(uint32_t*)(tracer.trace_buffer->buffer_data + tracer.trace_buffer->local_offset_data) = value_reg1;

	traceBuffer_commit_operand(tracer.trace_buffer, size_reg1)

	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].type 				= OPERAND_REG_READ;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].location.reg 		= (enum reg)name_reg2;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].size 				= size_reg2;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].data_offset 		= tracer.trace_buffer->global_offset_data;

	*(uint32_t*)(tracer.trace_buffer->buffer_data + tracer.trace_buffer->local_offset_data) = value_reg2;

	traceBuffer_commit_operand(tracer.trace_buffer, size_reg2)
}

void pintool_instruction_analysis_1write_mem_2read_reg_p1(ADDRINT pc, UINT32 opcode, ADDRINT address_mem, UINT32 size_mem, UINT32 name_reg1, UINT32 value_reg1, UINT32 size_reg1, UINT32 name_reg2, UINT32 value_reg2, UINT32 size_reg2){
	traceBuffer_reserve_operand(tracer.trace_buffer, tracer.trace_file, 3)
	traceBuffer_reserve_data(tracer.trace_buffer, tracer.trace_file, size_mem + 8)

	traceBuffer_add_instruction(tracer.trace_buffer, tracer.trace_file, pc, opcode, 3)

	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].type 				= OPERAND_REG_READ;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].location.reg 		= (enum reg)name_reg1;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].size 				= size_reg1;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].data_offset 		= tracer.trace_buffer->global_offset_data;

	*(uint32_t*)(tracer.trace_buffer->buffer_data + tracer.trace_buffer->local_offset_data) = value_reg1;

	traceBuffer_commit_operand(tracer.trace_buffer, size_reg1)

	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].type 				= OPERAND_REG_READ;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].location.reg 		= (enum reg)name_reg2;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].size 				= size_reg2;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].data_offset 		= tracer.trace_buffer->global_offset_data;

	*(uint32_t*)(tracer.trace_buffer->buffer_data + tracer.trace_buffer->local_offset_data) = value_reg2;

	traceBuffer_commit_operand(tracer.trace_buffer, size_reg2)

	tracer.trace_buffer->pending_write[0].location.address 									= address_mem;
	tracer.trace_buffer->pending_write[0].size 												= size_mem;
}

void pintool_instruction_analysis_1write_mem_1read_mem_2read_reg_p1(ADDRINT pc, UINT32 opcode, ADDRINT address_mem_write, UINT32 size_mem_write, ADDRINT address_mem_read, UINT32 size_mem_read, UINT32 name_reg1, UINT32 value_reg1, UINT32 size_reg1, UINT32 name_reg2, UINT32 value_reg2, UINT32 size_reg2){
	traceBuffer_reserve_operand(tracer.trace_buffer, tracer.trace_file, 4)
	traceBuffer_reserve_data(tracer.trace_buffer, tracer.trace_file, size_mem_write + size_mem_read + 8)

	traceBuffer_add_instruction(tracer.trace_buffer, tracer.trace_file, pc, opcode, 4)

	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].type 				= OPERAND_MEM_READ;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].location.address 	= address_mem_read;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].size 				= size_mem_read;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].data_offset 		= tracer.trace_buffer->global_offset_data;

	PIN_SafeCopy(tracer.trace_buffer->buffer_data + tracer.trace_buffer->local_offset_data, (void*)address_mem_read, size_mem_read);
	
	traceBuffer_commit_operand(tracer.trace_buffer, size_mem_read)

	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].type 				= OPERAND_REG_READ;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].location.reg 		= (enum reg)name_reg1;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].size 				= size_reg1;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].data_offset 		= tracer.trace_buffer->global_offset_data;

	*(uint32_t*)(tracer.trace_buffer->buffer_data + tracer.trace_buffer->local_offset_data) = value_reg1;

	traceBuffer_commit_operand(tracer.trace_buffer, size_reg1)

	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].type 				= OPERAND_REG_READ;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].location.reg 		= (enum reg)name_reg2;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].size 				= size_reg2;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].data_offset 		= tracer.trace_buffer->global_offset_data;

	*(uint32_t*)(tracer.trace_buffer->buffer_data + tracer.trace_buffer->local_offset_data) = value_reg2;

	traceBuffer_commit_operand(tracer.trace_buffer, size_reg2)

	tracer.trace_buffer->pending_write[0].location.address 									= address_mem_write;
	tracer.trace_buffer->pending_write[0].size 												= size_mem_write;
}

void pintool_instruction_analysis_1write_mem_3read_reg_p1(ADDRINT pc, UINT32 opcode, ADDRINT address_mem, UINT32 size_mem, UINT32 name_reg1, UINT32 value_reg1, UINT32 size_reg1, UINT32 name_reg2, UINT32 value_reg2, UINT32 size_reg2, UINT32 name_reg3, UINT32 value_reg3, UINT32 size_reg3){
	traceBuffer_reserve_operand(tracer.trace_buffer, tracer.trace_file, 4)
	traceBuffer_reserve_data(tracer.trace_buffer, tracer.trace_file, size_mem + 12)

	traceBuffer_add_instruction(tracer.trace_buffer, tracer.trace_file, pc, opcode, 4)

	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].type 				= OPERAND_REG_READ;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].location.reg 		= (enum reg)name_reg1;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].size 				= size_reg1;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].data_offset 		= tracer.trace_buffer->global_offset_data;

	*(uint32_t*)(tracer.trace_buffer->buffer_data + tracer.trace_buffer->local_offset_data) = value_reg1;

	traceBuffer_commit_operand(tracer.trace_buffer, size_reg1)

	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].type 				= OPERAND_REG_READ;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].location.reg 		= (enum reg)name_reg2;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].size 				= size_reg2;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].data_offset 		= tracer.trace_buffer->global_offset_data;

	*(uint32_t*)(tracer.trace_buffer->buffer_data + tracer.trace_buffer->local_offset_data) = value_reg2;

	traceBuffer_commit_operand(tracer.trace_buffer, size_reg2)

	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].type 				= OPERAND_REG_READ;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].location.reg 		= (enum reg)name_reg3;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].size 				= size_reg3;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].data_offset 		= tracer.trace_buffer->global_offset_data;

	*(uint32_t*)(tracer.trace_buffer->buffer_data + tracer.trace_buffer->local_offset_data) = value_reg3;

	traceBuffer_commit_operand(tracer.trace_buffer, size_reg3)

	tracer.trace_buffer->pending_write[0].location.address 									= address_mem;
	tracer.trace_buffer->pending_write[0].size 												= size_mem;
}

void pintool_instruction_analysis_1write_reg_p1(ADDRINT pc, UINT32 opcode, UINT32 name, UINT32 size){
	traceBuffer_reserve_operand(tracer.trace_buffer, tracer.trace_file, 1)
	traceBuffer_reserve_data(tracer.trace_buffer, tracer.trace_file, 4)

	traceBuffer_add_instruction(tracer.trace_buffer, tracer.trace_file, pc, opcode, 1)

	tracer.trace_buffer->pending_write[0].location.reg 										= (enum reg)name;
	tracer.trace_buffer->pending_write[0].size 												= size;
}

void pintool_instruction_analysis_1write_reg_1read_mem_p1(ADDRINT pc, UINT32 opcode, UINT32 name_reg, UINT32 size_reg, ADDRINT address_mem, UINT32 size_mem){
	traceBuffer_reserve_operand(tracer.trace_buffer, tracer.trace_file, 2)
	traceBuffer_reserve_data(tracer.trace_buffer, tracer.trace_file, size_mem + 4)

	traceBuffer_add_instruction(tracer.trace_buffer, tracer.trace_file, pc, opcode, 2)

	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].type 				= OPERAND_MEM_READ;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].location.address 	= address_mem;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].size 				= size_mem;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].data_offset 		= tracer.trace_buffer->global_offset_data;

	PIN_SafeCopy(tracer.trace_buffer->buffer_data + tracer.trace_buffer->local_offset_data, (void*)address_mem, size_mem);
	
	traceBuffer_commit_operand(tracer.trace_buffer, size_mem)

	tracer.trace_buffer->pending_write[0].location.reg 										= (enum reg)name_reg;
	tracer.trace_buffer->pending_write[0].size 												= size_reg;
}

void pintool_instruction_analysis_1write_reg_1read_reg_p1(ADDRINT pc, UINT32 opcode, UINT32 name_write, UINT32 size_write, UINT32 name_read, UINT32 value_read, UINT32 size_read){
	traceBuffer_reserve_operand(tracer.trace_buffer, tracer.trace_file, 2)
	traceBuffer_reserve_data(tracer.trace_buffer, tracer.trace_file, 8)

	traceBuffer_add_instruction(tracer.trace_buffer, tracer.trace_file, pc, opcode, 2)

	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].type 				= OPERAND_REG_READ;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].location.reg 		= (enum reg)name_read;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].size 				= size_read;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].data_offset 		= tracer.trace_buffer->global_offset_data;

	*(uint32_t*)(tracer.trace_buffer->buffer_data + tracer.trace_buffer->local_offset_data) = value_read;

	traceBuffer_commit_operand(tracer.trace_buffer, size_read)

	tracer.trace_buffer->pending_write[0].location.reg 										= (enum reg)name_write;
	tracer.trace_buffer->pending_write[0].size 												= size_write;
}

void pintool_instruction_analysis_1write_reg_1read_mem_1read_reg_p1(ADDRINT pc, UINT32 opcode, UINT32 name_reg_write, UINT32 size_reg_write, ADDRINT address_mem, UINT32 size_mem, UINT32 name_reg_read, UINT32 value_reg_read, UINT32 size_reg_read){
	traceBuffer_reserve_operand(tracer.trace_buffer, tracer.trace_file, 3)
	traceBuffer_reserve_data(tracer.trace_buffer, tracer.trace_file, size_mem + 8)

	traceBuffer_add_instruction(tracer.trace_buffer, tracer.trace_file, pc, opcode, 3)

	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].type 				= OPERAND_MEM_READ;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].location.address 	= address_mem;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].size 				= size_mem;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].data_offset 		= tracer.trace_buffer->global_offset_data;

	PIN_SafeCopy(tracer.trace_buffer->buffer_data + tracer.trace_buffer->local_offset_data, (void*)address_mem, size_mem);
	
	traceBuffer_commit_operand(tracer.trace_buffer, size_mem)

	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].type 				= OPERAND_REG_READ;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].location.reg 		= (enum reg)name_reg_read;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].size 				= size_reg_read;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].data_offset 		= tracer.trace_buffer->global_offset_data;

	*(uint32_t*)(tracer.trace_buffer->buffer_data + tracer.trace_buffer->local_offset_data) = value_reg_read;

	traceBuffer_commit_operand(tracer.trace_buffer, size_reg_read)

	tracer.trace_buffer->pending_write[0].location.reg 										= (enum reg)name_reg_write;
	tracer.trace_buffer->pending_write[0].size 												= size_reg_write;
}

void pintool_instruction_analysis_1write_reg_2read_reg_p1(ADDRINT pc, UINT32 opcode, UINT32 name_write, UINT32 size_write, UINT32 name_read1, UINT32 value_read1, UINT32 size_read1, UINT32 name_read2, UINT32 value_read2, UINT32 size_read2){
	traceBuffer_reserve_operand(tracer.trace_buffer, tracer.trace_file, 3)
	traceBuffer_reserve_data(tracer.trace_buffer, tracer.trace_file, 12)

	traceBuffer_add_instruction(tracer.trace_buffer, tracer.trace_file, pc, opcode, 3)

	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].type 				= OPERAND_REG_READ;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].location.reg 		= (enum reg)name_read1;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].size 				= size_read1;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].data_offset 		= tracer.trace_buffer->global_offset_data;

	*(uint32_t*)(tracer.trace_buffer->buffer_data + tracer.trace_buffer->local_offset_data) = value_read1;

	traceBuffer_commit_operand(tracer.trace_buffer, size_read1)

	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].type 				= OPERAND_REG_READ;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].location.reg 		= (enum reg)name_read2;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].size 				= size_read2;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].data_offset 		= tracer.trace_buffer->global_offset_data;

	*(uint32_t*)(tracer.trace_buffer->buffer_data + tracer.trace_buffer->local_offset_data) = value_read2;

	traceBuffer_commit_operand(tracer.trace_buffer, size_read2)

	tracer.trace_buffer->pending_write[0].location.reg 										= (enum reg)name_write;
	tracer.trace_buffer->pending_write[0].size 												= size_write;
}

void pintool_instruction_analysis_1write_reg_1read_mem_2read_reg_p1(ADDRINT pc, UINT32 opcode, UINT32 name_reg_write, UINT32 size_reg_write, ADDRINT address_mem, UINT32 size_mem, UINT32 name_reg_read1, UINT32 value_reg_read1, UINT32 size_reg_read1, UINT32 name_reg_read2, UINT32 value_reg_read2, UINT32 size_reg_read2){
	traceBuffer_reserve_operand(tracer.trace_buffer, tracer.trace_file, 4)
	traceBuffer_reserve_data(tracer.trace_buffer, tracer.trace_file, size_mem + 12)

	traceBuffer_add_instruction(tracer.trace_buffer, tracer.trace_file, pc, opcode, 4)

	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].type 				= OPERAND_MEM_READ;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].location.address 	= address_mem;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].size 				= size_mem;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].data_offset 		= tracer.trace_buffer->global_offset_data;

	PIN_SafeCopy(tracer.trace_buffer->buffer_data + tracer.trace_buffer->local_offset_data, (void*)address_mem, size_mem);
	
	traceBuffer_commit_operand(tracer.trace_buffer, size_mem)

	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].type 				= OPERAND_REG_READ;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].location.reg 		= (enum reg)name_reg_read1;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].size 				= size_reg_read1;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].data_offset 		= tracer.trace_buffer->global_offset_data;

	*(uint32_t*)(tracer.trace_buffer->buffer_data + tracer.trace_buffer->local_offset_data) = value_reg_read1;

	traceBuffer_commit_operand(tracer.trace_buffer, size_reg_read1)

	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].type 				= OPERAND_REG_READ;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].location.reg 		= (enum reg)name_reg_read2;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].size 				= size_reg_read2;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].data_offset 		= tracer.trace_buffer->global_offset_data;

	*(uint32_t*)(tracer.trace_buffer->buffer_data + tracer.trace_buffer->local_offset_data) = value_reg_read2;

	traceBuffer_commit_operand(tracer.trace_buffer, size_reg_read2)

	tracer.trace_buffer->pending_write[0].location.reg 										= (enum reg)name_reg_write;
	tracer.trace_buffer->pending_write[0].size 												= size_reg_write;
}

void pintool_instruction_analysis_1write_mem_1write_reg_1read_mem_2read_reg_p1(ADDRINT pc, UINT32 opcode, ADDRINT address_mem_write, UINT32 size_mem_write, UINT32 name_reg_write, UINT32 size_reg_write, ADDRINT address_mem_read, UINT32 size_mem_read, UINT32 name_reg_read1, UINT32 value_reg_read1, UINT32 size_reg_read1, UINT32 name_reg_read2, UINT32 value_reg_read2, UINT32 size_reg_read2){
	traceBuffer_reserve_operand(tracer.trace_buffer, tracer.trace_file, 5)
	traceBuffer_reserve_data(tracer.trace_buffer, tracer.trace_file, size_mem_write + size_mem_read + 12)

	traceBuffer_add_instruction(tracer.trace_buffer, tracer.trace_file, pc, opcode, 5)

	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].type 				= OPERAND_MEM_READ;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].location.address 	= address_mem_read;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].size 				= size_mem_read;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].data_offset 		= tracer.trace_buffer->global_offset_data;

	PIN_SafeCopy(tracer.trace_buffer->buffer_data + tracer.trace_buffer->local_offset_data, (void*)address_mem_read, size_mem_read);
	
	traceBuffer_commit_operand(tracer.trace_buffer, size_mem_read)

	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].type 				= OPERAND_REG_READ;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].location.reg 		= (enum reg)name_reg_read1;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].size 				= size_reg_read1;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].data_offset 		= tracer.trace_buffer->global_offset_data;

	*(uint32_t*)(tracer.trace_buffer->buffer_data + tracer.trace_buffer->local_offset_data) = value_reg_read1;

	traceBuffer_commit_operand(tracer.trace_buffer, size_reg_read1)

	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].type 				= OPERAND_REG_READ;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].location.reg 		= (enum reg)name_reg_read2;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].size 				= size_reg_read2;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].data_offset 		= tracer.trace_buffer->global_offset_data;

	*(uint32_t*)(tracer.trace_buffer->buffer_data + tracer.trace_buffer->local_offset_data) = value_reg_read2;

	traceBuffer_commit_operand(tracer.trace_buffer, size_reg_read2)

	tracer.trace_buffer->pending_write[0].location.address 									= address_mem_write;
	tracer.trace_buffer->pending_write[0].size 												= size_mem_write;

	tracer.trace_buffer->pending_write[1].location.reg 										= (enum reg)name_reg_write;
	tracer.trace_buffer->pending_write[1].size 												= size_reg_write;
}

void pintool_instruction_analysis_1write_reg_1read_mem_3read_reg_p1(ADDRINT pc, UINT32 opcode, UINT32 name_reg_write, UINT32 size_reg_write, ADDRINT address_mem, UINT32 size_mem, UINT32 name_reg_read1, UINT32 value_reg_read1, UINT32 size_reg_read1, UINT32 name_reg_read2, UINT32 value_reg_read2, UINT32 size_reg_read2, UINT32 name_reg_read3, UINT32 value_reg_read3, UINT32 size_reg_read3){
	traceBuffer_reserve_operand(tracer.trace_buffer, tracer.trace_file, 5)
	traceBuffer_reserve_data(tracer.trace_buffer, tracer.trace_file, size_mem + 16)

	traceBuffer_add_instruction(tracer.trace_buffer, tracer.trace_file, pc, opcode, 5)

	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].type 				= OPERAND_MEM_READ;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].location.address 	= address_mem;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].size 				= size_mem;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].data_offset 		= tracer.trace_buffer->global_offset_data;

	PIN_SafeCopy(tracer.trace_buffer->buffer_data + tracer.trace_buffer->local_offset_data, (void*)address_mem, size_mem);
	
	traceBuffer_commit_operand(tracer.trace_buffer, size_mem)

	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].type 				= OPERAND_REG_READ;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].location.reg 		= (enum reg)name_reg_read1;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].size 				= size_reg_read1;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].data_offset 		= tracer.trace_buffer->global_offset_data;

	*(uint32_t*)(tracer.trace_buffer->buffer_data + tracer.trace_buffer->local_offset_data) = value_reg_read1;

	traceBuffer_commit_operand(tracer.trace_buffer, size_reg_read1)

	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].type 				= OPERAND_REG_READ;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].location.reg 		= (enum reg)name_reg_read2;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].size 				= size_reg_read2;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].data_offset 		= tracer.trace_buffer->global_offset_data;

	*(uint32_t*)(tracer.trace_buffer->buffer_data + tracer.trace_buffer->local_offset_data) = value_reg_read2;

	traceBuffer_commit_operand(tracer.trace_buffer, size_reg_read2)

	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].type 				= OPERAND_REG_READ;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].location.reg 		= (enum reg)name_reg_read3;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].size 				= size_reg_read3;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].data_offset 		= tracer.trace_buffer->global_offset_data;

	*(uint32_t*)(tracer.trace_buffer->buffer_data + tracer.trace_buffer->local_offset_data) = value_reg_read3;

	traceBuffer_commit_operand(tracer.trace_buffer, size_reg_read3)

	tracer.trace_buffer->pending_write[0].location.reg 										= (enum reg)name_reg_write;
	tracer.trace_buffer->pending_write[0].size 												= size_reg_write;
}

void pintool_instruction_analysis_2write_reg_1read_reg_p1(ADDRINT pc, UINT32 opcode, UINT32 name_write1, UINT32 size_write1, UINT32 name_write2, UINT32 size_write2, UINT32 name_read, UINT32 value_read, UINT32 size_read){
	traceBuffer_reserve_operand(tracer.trace_buffer, tracer.trace_file, 3)
	traceBuffer_reserve_data(tracer.trace_buffer, tracer.trace_file, 12)

	traceBuffer_add_instruction(tracer.trace_buffer, tracer.trace_file, pc, opcode, 3)

	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].type 				= OPERAND_REG_READ;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].location.reg 		= (enum reg)name_read;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].size 				= size_read;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].data_offset 		= tracer.trace_buffer->global_offset_data;

	*(uint32_t*)(tracer.trace_buffer->buffer_data + tracer.trace_buffer->local_offset_data) = value_read;

	traceBuffer_commit_operand(tracer.trace_buffer, size_read)

	tracer.trace_buffer->pending_write[0].location.reg 										= (enum reg)name_write1;
	tracer.trace_buffer->pending_write[0].size 												= size_write1;

	tracer.trace_buffer->pending_write[1].location.reg 										= (enum reg)name_write2;
	tracer.trace_buffer->pending_write[1].size 												= size_write2;
}

void pintool_instruction_analysis_2write_reg_2read_reg_p1(ADDRINT pc, UINT32 opcode, UINT32 name_write1, UINT32 size_write1, UINT32 name_write2, UINT32 size_write2, UINT32 name_read1, UINT32 value_read1, UINT32 size_read1, UINT32 name_read2, UINT32 value_read2, UINT32 size_read2){
	traceBuffer_reserve_operand(tracer.trace_buffer, tracer.trace_file, 4)
	traceBuffer_reserve_data(tracer.trace_buffer, tracer.trace_file, 16)

	traceBuffer_add_instruction(tracer.trace_buffer, tracer.trace_file, pc, opcode, 4)

	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].type 				= OPERAND_REG_READ;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].location.reg 		= (enum reg)name_read1;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].size 				= size_read1;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].data_offset 		= tracer.trace_buffer->global_offset_data;

	*(uint32_t*)(tracer.trace_buffer->buffer_data + tracer.trace_buffer->local_offset_data) = value_read1;

	traceBuffer_commit_operand(tracer.trace_buffer, size_read1)

	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].type 				= OPERAND_REG_READ;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].location.reg 		= (enum reg)name_read2;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].size 				= size_read2;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].data_offset 		= tracer.trace_buffer->global_offset_data;

	*(uint32_t*)(tracer.trace_buffer->buffer_data + tracer.trace_buffer->local_offset_data) = value_read2;

	traceBuffer_commit_operand(tracer.trace_buffer, size_read2)

	tracer.trace_buffer->pending_write[0].location.reg 										= (enum reg)name_write1;
	tracer.trace_buffer->pending_write[0].size 												= size_write1;

	tracer.trace_buffer->pending_write[1].location.reg 										= (enum reg)name_write2;
	tracer.trace_buffer->pending_write[1].size 												= size_write2;
}

void pintool_instruction_analysis_2write_reg_3read_reg_p1(ADDRINT pc, UINT32 opcode, UINT32 name_write1, UINT32 size_write1, UINT32 name_write2, UINT32 size_write2, UINT32 name_read1, UINT32 value_read1, UINT32 size_read1, UINT32 name_read2, UINT32 value_read2, UINT32 size_read2, UINT32 name_read3, UINT32 value_read3, UINT32 size_read3){
	traceBuffer_reserve_operand(tracer.trace_buffer, tracer.trace_file, 5)
	traceBuffer_reserve_data(tracer.trace_buffer, tracer.trace_file, 20)

	traceBuffer_add_instruction(tracer.trace_buffer, tracer.trace_file, pc, opcode, 5)

	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].type 				= OPERAND_REG_READ;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].location.reg 		= (enum reg)name_read1;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].size 				= size_read1;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].data_offset 		= tracer.trace_buffer->global_offset_data;

	*(uint32_t*)(tracer.trace_buffer->buffer_data + tracer.trace_buffer->local_offset_data) = value_read1;

	traceBuffer_commit_operand(tracer.trace_buffer, size_read1)

	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].type 				= OPERAND_REG_READ;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].location.reg 		= (enum reg)name_read2;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].size 				= size_read2;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].data_offset 		= tracer.trace_buffer->global_offset_data;

	*(uint32_t*)(tracer.trace_buffer->buffer_data + tracer.trace_buffer->local_offset_data) = value_read2;

	traceBuffer_commit_operand(tracer.trace_buffer, size_read2)

	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].type 				= OPERAND_REG_READ;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].location.reg 		= (enum reg)name_read3;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].size 				= size_read3;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].data_offset 		= tracer.trace_buffer->global_offset_data;

	*(uint32_t*)(tracer.trace_buffer->buffer_data + tracer.trace_buffer->local_offset_data) = value_read3;

	traceBuffer_commit_operand(tracer.trace_buffer, size_read3)

	tracer.trace_buffer->pending_write[0].location.reg 										= (enum reg)name_write1;
	tracer.trace_buffer->pending_write[0].size 												= size_write1;

	tracer.trace_buffer->pending_write[1].location.reg 										= (enum reg)name_write2;
	tracer.trace_buffer->pending_write[1].size 												= size_write2;
}

void pintool_instruction_analysis_1write_mem_Xread_p2(){
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].type 				= OPERAND_MEM_WRITE;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].location.address 	= tracer.trace_buffer->pending_write[0].location.address;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].size 				= tracer.trace_buffer->pending_write[0].size;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].data_offset 		= tracer.trace_buffer->global_offset_data;

	PIN_SafeCopy(tracer.trace_buffer->buffer_data + tracer.trace_buffer->local_offset_data, (void*)tracer.trace_buffer->pending_write[0].location.address, tracer.trace_buffer->pending_write[0].size);

	traceBuffer_commit_operand(tracer.trace_buffer, tracer.trace_buffer->pending_write[0].size)
}

void pintool_instruction_analysis_1write_reg_Xread_p2(UINT32 value){
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].type 				= OPERAND_REG_WRITE;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].location.reg 		= tracer.trace_buffer->pending_write[0].location.reg;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].size 				= tracer.trace_buffer->pending_write[0].size;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].data_offset 		= tracer.trace_buffer->global_offset_data;

	*(uint32_t*)(tracer.trace_buffer->buffer_data + tracer.trace_buffer->local_offset_data) = value;

	traceBuffer_commit_operand(tracer.trace_buffer, tracer.trace_buffer->pending_write[0].size)
}

void pintool_instruction_analysis_1write_mem_1write_reg_Xread_p2(UINT32 value){
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].type 				= OPERAND_MEM_WRITE;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].location.address 	= tracer.trace_buffer->pending_write[0].location.address;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].size 				= tracer.trace_buffer->pending_write[0].size;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].data_offset 		= tracer.trace_buffer->global_offset_data;

	PIN_SafeCopy(tracer.trace_buffer->buffer_data + tracer.trace_buffer->local_offset_data, (void*)tracer.trace_buffer->pending_write[0].location.address, tracer.trace_buffer->pending_write[0].size);

	traceBuffer_commit_operand(tracer.trace_buffer, tracer.trace_buffer->pending_write[0].size)

	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].type 				= OPERAND_REG_WRITE;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].location.reg 		= tracer.trace_buffer->pending_write[1].location.reg;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].size 				= tracer.trace_buffer->pending_write[1].size;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].data_offset 		= tracer.trace_buffer->global_offset_data;

	*(uint32_t*)(tracer.trace_buffer->buffer_data + tracer.trace_buffer->local_offset_data) = value;

	traceBuffer_commit_operand(tracer.trace_buffer, tracer.trace_buffer->pending_write[1].size)
}

void pintool_instruction_analysis_2write_reg_Xread_p2(UINT32 value1, UINT32 value2){
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].type 				= OPERAND_REG_WRITE;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].location.reg 		= tracer.trace_buffer->pending_write[0].location.reg;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].size 				= tracer.trace_buffer->pending_write[0].size;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].data_offset 		= tracer.trace_buffer->global_offset_data;

	*(uint32_t*)(tracer.trace_buffer->buffer_data + tracer.trace_buffer->local_offset_data) = value1;

	traceBuffer_commit_operand(tracer.trace_buffer, tracer.trace_buffer->pending_write[0].size)

	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].type 				= OPERAND_REG_WRITE;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].location.reg 		= tracer.trace_buffer->pending_write[1].location.reg;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].size 				= tracer.trace_buffer->pending_write[1].size;
	tracer.trace_buffer->buffer_op[tracer.trace_buffer->local_offset_op].data_offset 		= tracer.trace_buffer->global_offset_data;

	*(uint32_t*)(tracer.trace_buffer->buffer_data + tracer.trace_buffer->local_offset_data) = value2;

	traceBuffer_commit_operand(tracer.trace_buffer, tracer.trace_buffer->pending_write[1].size)
}

void pintool_routine_analysis(void* cm_routine_ptr){
	struct cm_routine* routine = (struct cm_routine*)cm_routine_ptr;
	
	CODEMAP_INCREMENT_ROUTINE_EXE(routine);
}


/* ===================================================================== */
/* Instrumentation function                                              */
/* ===================================================================== */

void pintool_instrumentation_ins(INS instruction, void* arg){
	void (*ins_insertCall)		(INS ins, IPOINT ipoint, AFUNPTR funptr, ...);
	IPOINT 						ipoint_p2 	= IPOINT_BEFORE;
	uint32_t 					selector 	= ANALYSIS_SELECTOR_NO_ARG;
	uint32_t 					i;
	uint8_t 					j;
	REG 						current_reg;
	REG 						read_reg1 	= REG_INVALID_;
	REG 						read_reg2 	= REG_INVALID_;
	REG 						read_reg3 	= REG_INVALID_;
	REG 						write_reg1 	= REG_INVALID_;
	REG 						write_reg2 	= REG_INVALID_;
	

	if (codeMap_is_instruction_whiteListed(tracer.code_map, (unsigned long)INS_Address(instruction)) == CODEMAP_NOT_WHITELISTED){

		if (INS_IsPredicated(instruction)){
			ins_insertCall = INS_InsertPredicatedCall;
		}
		else{
			ins_insertCall = INS_InsertCall;
		}
		
		if (INS_HasFallThrough(instruction)){
			ipoint_p2 = IPOINT_AFTER;
		}
		else if (INS_IsBranchOrCall(instruction)){
			ipoint_p2 = IPOINT_TAKEN_BRANCH;
		}

		if (INS_IsMemoryRead(instruction)){
			ANALYSIS_SELECTOR_SET_1MR(selector);
			if (INS_HasMemoryRead2(instruction)){
				ANALYSIS_SELECTOR_SET_2MR(selector);
			}
		}

		if (INS_IsMemoryWrite(instruction)){
			ANALYSIS_SELECTOR_SET_1MW(selector);
		}

		i = 0;
		j = 0;

		current_reg = INS_RegR(instruction, i);
		while(REG_valid(current_reg)){
			if (pintool_monitor_REG(current_reg)){
				j ++;
				if (j == 1){
					read_reg1 = current_reg;
					ANALYSIS_SELECTOR_SET_1RR(selector);
				}
				else if (j == 2){
					read_reg2 = current_reg;
					ANALYSIS_SELECTOR_SET_2RR(selector);
				}
				else if (j == 3){
					read_reg3 = current_reg;
					ANALYSIS_SELECTOR_SET_3RR(selector);
				}
				else{
					printf("ERROR: in %s, max read register is reached\n", __func__);
				}
			}
			i++;
			current_reg = INS_RegR(instruction, i);
		}

		i = 0;
		j = 0;

		current_reg = INS_RegW(instruction, i);
		while(REG_valid(current_reg)){
			if (pintool_monitor_REG(current_reg)){
				j ++;
				if (j == 1){
					write_reg1 = current_reg;
					ANALYSIS_SELECTOR_SET_1RW(selector);
				}
				else if (j == 2){
					write_reg2 = current_reg;
					ANALYSIS_SELECTOR_SET_2RW(selector);
				}
				else if (j == 3){
					ANALYSIS_SELECTOR_SET_3RW(selector);
				}
				else if (j == 4){
					ANALYSIS_SELECTOR_SET_4RW(selector);
				}
				else{
					printf("ERROR: in %s, max read register is reached\n", __func__);
				}
			}
			i++;
			current_reg = INS_RegW(instruction, i);
		}

		switch(selector){
		case ANALYSIS_SELECTOR_NO_ARG			: {
			ins_insertCall(instruction, IPOINT_BEFORE, AFUNPTR(pintool_instruction_analysis_no_arg), 
				IARG_INST_PTR,									/* pc 					*/
				IARG_UINT32, INS_Opcode(instruction), 			/* opcode 				*/
				IARG_END);
			break;
		}
		case ANALYSIS_SELECTOR_1MR				: {
			ins_insertCall(instruction, IPOINT_BEFORE, AFUNPTR(pintool_instruction_analysis_1read_mem),
				IARG_INST_PTR,									/* pc 					*/
				IARG_UINT32, INS_Opcode(instruction),			/* opcode 				*/
				IARG_MEMORYREAD_EA,								/* @ MR1 				*/
				IARG_MEMORYREAD_SIZE,							/* size MR1 			*/
				IARG_END);
			break;
		}
		case ANALYSIS_SELECTOR_2MR 				: {
			printf("ERROR: in %s, this case (2MR) is not supported\n", __func__);
			break;
		}
		case ANALYSIS_SELECTOR_1MW 				: {
			ins_insertCall(instruction, IPOINT_BEFORE, AFUNPTR(pintool_instruction_analysis_1write_mem_p1),
				IARG_INST_PTR,									/* pc 					*/
				IARG_UINT32, INS_Opcode(instruction),			/* opcode 				*/
				IARG_MEMORYWRITE_EA,							/* @ MW1 				*/
				IARG_MEMORYWRITE_SIZE,							/* size MW1 			*/
				IARG_END);
			ins_insertCall(instruction, ipoint_p2, AFUNPTR(pintool_instruction_analysis_1write_mem_Xread_p2), IARG_END);
			break;
		}
		case ANALYSIS_SELECTOR_1MR_1MW 			: {
			ins_insertCall(instruction, IPOINT_BEFORE, AFUNPTR(pintool_instruction_analysis_1write_mem_1read_mem_p1), 
				IARG_INST_PTR, 									/* pc 					*/
				IARG_UINT32, INS_Opcode(instruction),			/* opcode 				*/
				IARG_MEMORYWRITE_EA, 							/* @ MW1 				*/
				IARG_MEMORYWRITE_SIZE,							/* size MW1 			*/
				IARG_MEMORYREAD_EA, 							/* @ MR1 				*/
				IARG_MEMORYREAD_SIZE,							/* size MR1 			*/
				IARG_END);
			ins_insertCall(instruction, ipoint_p2, AFUNPTR(pintool_instruction_analysis_1write_mem_Xread_p2), IARG_END);
			break;
		}
		case ANALYSIS_SELECTOR_2MR_1MW			: {
			printf("ERROR: in %s, this case (2MR_1MW) is not supported\n", __func__);
			break;
		}
		case ANALYSIS_SELECTOR_1RR 				: {
			ins_insertCall(instruction, IPOINT_BEFORE, AFUNPTR(pintool_instruction_analysis_1read_reg),
				IARG_INST_PTR,									/* pc 					*/
				IARG_UINT32, INS_Opcode(instruction),			/* opcode 				*/
				IARG_UINT32, pintool_REG_2_reg(read_reg1),		/* RR1 name 			*/
				IARG_REG_VALUE, read_reg1,						/* RR1 value 			*/
				IARG_UINT32, pintool_REG_size(read_reg1),		/* RR1 size 			*/
				IARG_END);
			break;
		}
		case ANALYSIS_SELECTOR_1MR_1RR			: {
			ins_insertCall(instruction, IPOINT_BEFORE, AFUNPTR(pintool_instruction_analysis_1read_mem_1read_reg),
				IARG_INST_PTR,									/* pc 					*/
				IARG_UINT32, INS_Opcode(instruction),			/* opcode 				*/
				IARG_MEMORYREAD_EA,								/* @ MR1 				*/
				IARG_MEMORYREAD_SIZE,							/* size MR1 			*/
				IARG_UINT32, pintool_REG_2_reg(read_reg1),		/* RR1 name 			*/
				IARG_REG_VALUE, read_reg1,						/* RR1 value 			*/
				IARG_UINT32, pintool_REG_size(read_reg1),		/* RR1 size 			*/
				IARG_END);
			break;
		}
		case ANALYSIS_SELECTOR_1MW_1RR			: {
			ins_insertCall(instruction, IPOINT_BEFORE, AFUNPTR(pintool_instruction_analysis_1write_mem_1read_reg_p1),
				IARG_INST_PTR,									/* pc 					*/
				IARG_UINT32, INS_Opcode(instruction),			/* opcode 				*/
				IARG_MEMORYWRITE_EA,							/* @ MW1 				*/
				IARG_MEMORYWRITE_SIZE,							/* size MW1 			*/
				IARG_UINT32, pintool_REG_2_reg(read_reg1),		/* RR1 name 			*/
				IARG_REG_VALUE, read_reg1,						/* RR1 value 			*/
				IARG_UINT32, pintool_REG_size(read_reg1),		/* RR1 size 			*/
				IARG_END);
			ins_insertCall(instruction, ipoint_p2, AFUNPTR(pintool_instruction_analysis_1write_mem_Xread_p2), IARG_END);
			break;
		}
		case ANALYSIS_SELECTOR_1MR_1MW_1RR 		: {
			ins_insertCall(instruction, IPOINT_BEFORE, AFUNPTR(pintool_instruction_analysis_1write_mem_1read_mem_1read_reg_p1),
				IARG_INST_PTR,									/* pc 					*/
				IARG_UINT32, INS_Opcode(instruction),			/* opcode 				*/
				IARG_MEMORYWRITE_EA,							/* @ MW1 				*/
				IARG_MEMORYWRITE_SIZE,							/* size MW1 			*/
				IARG_MEMORYREAD_EA,								/* @ MR1 				*/
				IARG_MEMORYREAD_SIZE,							/* size MR1 			*/
				IARG_UINT32, pintool_REG_2_reg(read_reg1),		/* RR1 name 			*/
				IARG_REG_VALUE, read_reg1,						/* RR1 value 			*/
				IARG_UINT32, pintool_REG_size(read_reg1),		/* RR1 size 			*/
				IARG_END);
			ins_insertCall(instruction, ipoint_p2, AFUNPTR(pintool_instruction_analysis_1write_mem_Xread_p2), IARG_END);
			break;
		}
		case ANALYSIS_SELECTOR_2RR 				: {
			ins_insertCall(instruction, IPOINT_BEFORE, AFUNPTR(pintool_instruction_analysis_1read_reg),
				IARG_INST_PTR,									/* pc 					*/
				IARG_UINT32, INS_Opcode(instruction),			/* opcode 				*/
				IARG_UINT32, pintool_REG_2_reg(read_reg1),		/* RR1 name 			*/
				IARG_REG_VALUE, read_reg1,						/* RR1 value 			*/
				IARG_UINT32, pintool_REG_size(read_reg1),		/* RR1 size 			*/
				IARG_UINT32, pintool_REG_2_reg(read_reg2),		/* RR2 name 			*/
				IARG_REG_VALUE, read_reg2,						/* RR2 value 			*/
				IARG_UINT32, pintool_REG_size(read_reg2),		/* RR2 size 			*/
				IARG_END);
			break;
		}
		case ANALYSIS_SELECTOR_1MR_2RR 			: {
			ins_insertCall(instruction, IPOINT_BEFORE, AFUNPTR(pintool_instruction_analysis_1read_mem_2read_reg),
				IARG_INST_PTR,									/* pc 					*/
				IARG_UINT32, INS_Opcode(instruction),			/* opcode 				*/
				IARG_MEMORYREAD_EA,								/* @ MR1 				*/
				IARG_MEMORYREAD_SIZE,							/* size MR1 			*/
				IARG_UINT32, pintool_REG_2_reg(read_reg1),		/* RR1 name 			*/
				IARG_REG_VALUE, read_reg1,						/* RR1 value 			*/
				IARG_UINT32, pintool_REG_size(read_reg1),		/* RR1 size 			*/
				IARG_UINT32, pintool_REG_2_reg(read_reg2),		/* RR2 name 			*/
				IARG_REG_VALUE, read_reg2,						/* RR2 value 			*/
				IARG_UINT32, pintool_REG_size(read_reg2),		/* RR2 size 			*/
				IARG_END);
			break;
		}
		case ANALYSIS_SELECTOR_1MW_2RR 			: {
			ins_insertCall(instruction, IPOINT_BEFORE, AFUNPTR(pintool_instruction_analysis_1write_mem_2read_reg_p1),
				IARG_INST_PTR,									/* pc 					*/
				IARG_UINT32, INS_Opcode(instruction),			/* opcode 				*/
				IARG_MEMORYWRITE_EA,							/* @ MW1 				*/
				IARG_MEMORYWRITE_SIZE,							/* size MW1 			*/
				IARG_UINT32, pintool_REG_2_reg(read_reg1),		/* RR1 name 			*/
				IARG_REG_VALUE, read_reg1,						/* RR1 value 			*/
				IARG_UINT32, pintool_REG_size(read_reg1),		/* RR1 size 			*/
				IARG_UINT32, pintool_REG_2_reg(read_reg2),		/* RR2 name 			*/
				IARG_REG_VALUE, read_reg2,						/* RR2 value 			*/
				IARG_UINT32, pintool_REG_size(read_reg2),		/* RR2 size 			*/
				IARG_END);
			ins_insertCall(instruction, ipoint_p2, AFUNPTR(pintool_instruction_analysis_1write_mem_Xread_p2), IARG_END);
			break;
		}
		case ANALYSIS_SELECTOR_1MR_1MW_2RR 		: {
			ins_insertCall(instruction, IPOINT_BEFORE, AFUNPTR(pintool_instruction_analysis_1write_mem_1read_mem_2read_reg_p1),
				IARG_INST_PTR,									/* pc 					*/
				IARG_UINT32, INS_Opcode(instruction),			/* opcode 				*/
				IARG_MEMORYWRITE_EA,							/* @ MW1 				*/
				IARG_MEMORYWRITE_SIZE,							/* size MW1 			*/
				IARG_MEMORYREAD_EA,								/* @ MR1 				*/
				IARG_MEMORYREAD_SIZE,							/* size MR1 			*/
				IARG_UINT32, pintool_REG_2_reg(read_reg1),		/* RR1 name 			*/
				IARG_REG_VALUE, read_reg1,						/* RR1 value 			*/
				IARG_UINT32, pintool_REG_size(read_reg1),		/* RR1 size 			*/
				IARG_UINT32, pintool_REG_2_reg(read_reg2),		/* RR2 name 			*/
				IARG_REG_VALUE, read_reg2,						/* RR2 value 			*/
				IARG_UINT32, pintool_REG_size(read_reg2),		/* RR2 size 			*/
				IARG_END);
			ins_insertCall(instruction, ipoint_p2, AFUNPTR(pintool_instruction_analysis_1write_mem_Xread_p2), IARG_END);
			break;
		}
		case ANALYSIS_SELECTOR_3RR 				: {
			printf("ERROR: in %s, this case (3RR) is not supported\n", __func__);
			break;
		}
		case ANALYSIS_SELECTOR_1MR_3RR			: {
			printf("ERROR: in %s, this case (1MR_3RR) is not supported\n", __func__);
			break;
		}
		case ANALYSIS_SELECTOR_1MW_3RR 			: {
			ins_insertCall(instruction, IPOINT_BEFORE, AFUNPTR(pintool_instruction_analysis_1write_mem_3read_reg_p1),
				IARG_INST_PTR,									/* pc 					*/
				IARG_UINT32, INS_Opcode(instruction),			/* opcode 				*/
				IARG_MEMORYWRITE_EA,							/* @ MW1 				*/
				IARG_MEMORYWRITE_SIZE,							/* size MW1 			*/
				IARG_UINT32, pintool_REG_2_reg(read_reg1),		/* RR1 name 			*/
				IARG_REG_VALUE, read_reg1,						/* RR1 value 			*/
				IARG_UINT32, pintool_REG_size(read_reg1),		/* RR1 size 			*/
				IARG_UINT32, pintool_REG_2_reg(read_reg2),		/* RR2 name 			*/
				IARG_REG_VALUE, read_reg2,						/* RR2 value 			*/
				IARG_UINT32, pintool_REG_size(read_reg2),		/* RR2 size 			*/
				IARG_UINT32, pintool_REG_2_reg(read_reg3),		/* RR3 name 			*/
				IARG_REG_VALUE, read_reg3,						/* RR3 value 			*/
				IARG_UINT32, pintool_REG_size(read_reg3),		/* RR3 size 			*/
				IARG_END);
			ins_insertCall(instruction, ipoint_p2, AFUNPTR(pintool_instruction_analysis_1write_mem_Xread_p2), IARG_END);
			break;
		}
		case ANALYSIS_SELECTOR_1MR_1MW_3RR 		: {
			printf("ERROR: in %s, this case (1MR_1MW_3RR) is not supported\n", __func__);
			break;
		}
		case ANALYSIS_SELECTOR_1RW 				: {
			ins_insertCall(instruction, IPOINT_BEFORE, AFUNPTR(pintool_instruction_analysis_1write_reg_p1),
				IARG_INST_PTR,									/* pc 					*/
				IARG_UINT32, INS_Opcode(instruction),			/* opcode 				*/
				IARG_UINT32, pintool_REG_2_reg(write_reg1),		/* RW1 name 			*/
				IARG_UINT32, pintool_REG_size(write_reg1),		/* RW1 size 			*/
				IARG_END);
			ins_insertCall(instruction, ipoint_p2, AFUNPTR(pintool_instruction_analysis_1write_reg_Xread_p2),
				IARG_REG_VALUE, write_reg1,						/* RW1 value 			*/
				IARG_END);
			break;
		}
		case ANALYSIS_SELECTOR_1MR_1RW 			: {
			ins_insertCall(instruction, IPOINT_BEFORE, AFUNPTR(pintool_instruction_analysis_1write_reg_1read_mem_p1),
				IARG_INST_PTR,									/* pc 					*/
				IARG_UINT32, INS_Opcode(instruction),			/* opcode 				*/
				IARG_UINT32, pintool_REG_2_reg(write_reg1),		/* RW1 name 			*/
				IARG_UINT32, pintool_REG_size(write_reg1),		/* RW1 size 			*/
				IARG_MEMORYREAD_EA,								/* @ MR1  				*/
				IARG_MEMORYREAD_SIZE,							/* MR1 size 			*/
				IARG_END);
			ins_insertCall(instruction, ipoint_p2, AFUNPTR(pintool_instruction_analysis_1write_reg_Xread_p2),
				IARG_REG_VALUE, write_reg1,						/* RW1 value 			*/
				IARG_END);
			break;
		}
		case ANALYSIS_SELECTOR_1MW_1RW 			: {
			printf("ERROR: in %s, this case (1MW_1RW) is not supported\n", __func__);
			break;
		}
		case ANALYSIS_SELECTOR_1RR_1RW			: {
			ins_insertCall(instruction, IPOINT_BEFORE, AFUNPTR(pintool_instruction_analysis_1write_reg_1read_reg_p1),
				IARG_INST_PTR,									/* pc 					*/
				IARG_UINT32, INS_Opcode(instruction),			/* opcode 				*/
				IARG_UINT32, pintool_REG_2_reg(write_reg1),		/* RW1 name 			*/
				IARG_UINT32, pintool_REG_size(write_reg1),		/* RW1 size 			*/
				IARG_UINT32, pintool_REG_2_reg(read_reg1),		/* RR1 name 			*/
				IARG_REG_VALUE, read_reg1,						/* RR1 value 			*/
				IARG_UINT32, pintool_REG_size(read_reg1),		/* RR1 size 			*/
				IARG_END);
			ins_insertCall(instruction, ipoint_p2, AFUNPTR(pintool_instruction_analysis_1write_reg_Xread_p2),
				IARG_REG_VALUE, write_reg1,						/* RW1 value 			*/
				IARG_END);
			break;
		}
		case ANALYSIS_SELECTOR_1MR_1RR_1RW 		: {
			ins_insertCall(instruction, IPOINT_BEFORE, AFUNPTR(pintool_instruction_analysis_1write_reg_1read_mem_1read_reg_p1),
				IARG_INST_PTR,									/* pc 					*/
				IARG_UINT32, INS_Opcode(instruction),			/* opcode 				*/
				IARG_UINT32, pintool_REG_2_reg(write_reg1),		/* RW1 name 			*/
				IARG_UINT32, pintool_REG_size(write_reg1),		/* RW1 size 			*/
				IARG_MEMORYREAD_EA,								/* @ MR1  				*/
				IARG_MEMORYREAD_SIZE,							/* MR1 size 			*/
				IARG_UINT32, pintool_REG_2_reg(read_reg1),		/* RR1 name 			*/
				IARG_REG_VALUE, read_reg1,						/* RR1 value 			*/
				IARG_UINT32, pintool_REG_size(read_reg1),		/* RR1 size 			*/
				IARG_END);
			ins_insertCall(instruction, ipoint_p2, AFUNPTR(pintool_instruction_analysis_1write_reg_Xread_p2),
				IARG_REG_VALUE, write_reg1,						/* RW1 value 			*/
				IARG_END);
			break;
		}
		case ANALYSIS_SELECTOR_2RR_1RW			: {
			ins_insertCall(instruction, IPOINT_BEFORE, AFUNPTR(pintool_instruction_analysis_1write_reg_2read_reg_p1),
				IARG_INST_PTR,									/* pc 					*/
				IARG_UINT32, INS_Opcode(instruction),			/* opcode 				*/
				IARG_UINT32, pintool_REG_2_reg(write_reg1),		/* RW1 name 			*/
				IARG_UINT32, pintool_REG_size(write_reg1),		/* RW1 size 			*/
				IARG_UINT32, pintool_REG_2_reg(read_reg1),		/* RR1 name 			*/
				IARG_REG_VALUE, read_reg1,						/* RR1 value 			*/
				IARG_UINT32, pintool_REG_size(read_reg1),		/* RR1 size 			*/
				IARG_UINT32, pintool_REG_2_reg(read_reg2),		/* RR2 name 			*/
				IARG_REG_VALUE, read_reg2,						/* RR2 value 			*/
				IARG_UINT32, pintool_REG_size(read_reg2),		/* RR2 size 			*/
				IARG_END);
			ins_insertCall(instruction, ipoint_p2, AFUNPTR(pintool_instruction_analysis_1write_reg_Xread_p2),
				IARG_REG_VALUE, write_reg1,						/* RW1 value 			*/
				IARG_END);
			break;
		}
		case ANALYSIS_SELECTOR_1MR_2RR_1RW 		: {
			ins_insertCall(instruction, IPOINT_BEFORE, AFUNPTR(pintool_instruction_analysis_1write_reg_1read_mem_2read_reg_p1),
				IARG_INST_PTR,									/* pc 					*/
				IARG_UINT32, INS_Opcode(instruction),			/* opcode 				*/
				IARG_UINT32, pintool_REG_2_reg(write_reg1),		/* RW1 name 			*/
				IARG_UINT32, pintool_REG_size(write_reg1),		/* RW1 size 			*/
				IARG_MEMORYREAD_EA,								/* @ MR1  				*/
				IARG_MEMORYREAD_SIZE,							/* MR1 size 			*/
				IARG_UINT32, pintool_REG_2_reg(read_reg1),		/* RR1 name 			*/
				IARG_REG_VALUE, read_reg1,						/* RR1 value 			*/
				IARG_UINT32, pintool_REG_size(read_reg1),		/* RR1 size 			*/
				IARG_UINT32, pintool_REG_2_reg(read_reg2),		/* RR2 name 			*/
				IARG_REG_VALUE, read_reg2,						/* RR2 value 			*/
				IARG_UINT32, pintool_REG_size(read_reg2),		/* RR2 size 			*/
				IARG_END);
			ins_insertCall(instruction, ipoint_p2, AFUNPTR(pintool_instruction_analysis_1write_reg_Xread_p2),
				IARG_REG_VALUE, write_reg1,						/* RW1 value 			*/
				IARG_END);
			break; 
		}
		#if 0
		case ANALYSIS_SELECTOR_1MR_1MW_2RR_1RW 	: {
			ins_insertCall(instruction, IPOINT_BEFORE, AFUNPTR(pintool_instruction_analysis_1write_mem_1write_reg_1read_mem_2read_reg_p1),
				IARG_INST_PTR,									/* pc 					*/
				IARG_UINT32, INS_Opcode(instruction),			/* opcode 				*/
				IARG_MEMORYWRITE_EA,							/* @ MW1  				*/
				IARG_MEMORYWRITE_SIZE,							/* MW1 size 			*/
				IARG_UINT32, pintool_REG_2_reg(write_reg1),		/* RW1 name 			*/
				IARG_UINT32, pintool_REG_size(write_reg1),		/* RW1 size 			*/
				IARG_MEMORYREAD_EA,								/* @ MR1  				*/
				IARG_MEMORYREAD_SIZE,							/* MR1 size 			*/
				IARG_UINT32, pintool_REG_2_reg(read_reg1),		/* RR1 name 			*/
				IARG_REG_VALUE, read_reg1,						/* RR1 value 			*/
				IARG_UINT32, pintool_REG_size(read_reg1),		/* RR1 size 			*/
				IARG_UINT32, pintool_REG_2_reg(read_reg2),		/* RR2 name 			*/
				IARG_REG_VALUE, read_reg2,						/* RR2 value 			*/
				IARG_UINT32, pintool_REG_size(read_reg2),		/* RR2 size 			*/
				IARG_END);
			ins_insertCall(instruction, ipoint_p2, AFUNPTR(pintool_instruction_analysis_1write_mem_1write_reg_Xread_p2),
				IARG_REG_VALUE, write_reg1,						/* RW1 value 			*/
				IARG_END);
			break; 
		}
		#endif
		case ANALYSIS_SELECTOR_3RR_1RW			: {
			printf("ERROR: in %s, this case (3RR_1RW) is not supported\n", __func__);
			break;
		}
		case ANALYSIS_SELECTOR_1MR_3RR_1RW 		: {
			ins_insertCall(instruction, IPOINT_BEFORE, AFUNPTR(pintool_instruction_analysis_1write_reg_1read_mem_3read_reg_p1),
				IARG_INST_PTR,									/* pc 					*/
				IARG_UINT32, INS_Opcode(instruction),			/* opcode 				*/
				IARG_UINT32, pintool_REG_2_reg(write_reg1),		/* RW1 name 			*/
				IARG_UINT32, pintool_REG_size(write_reg1),		/* RW1 size 			*/
				IARG_MEMORYREAD_EA,								/* @ MR1  				*/
				IARG_MEMORYREAD_SIZE,							/* MR1 size 			*/
				IARG_UINT32, pintool_REG_2_reg(read_reg1),		/* RR1 name 			*/
				IARG_REG_VALUE, read_reg1,						/* RR1 value 			*/
				IARG_UINT32, pintool_REG_size(read_reg1),		/* RR1 size 			*/
				IARG_UINT32, pintool_REG_2_reg(read_reg2),		/* RR2 name 			*/
				IARG_REG_VALUE, read_reg2,						/* RR2 value 			*/
				IARG_UINT32, pintool_REG_size(read_reg2),		/* RR2 size 			*/
				IARG_UINT32, pintool_REG_2_reg(read_reg3),		/* RR3 name 			*/
				IARG_REG_VALUE, read_reg3,						/* RR3 value 			*/
				IARG_UINT32, pintool_REG_size(read_reg3),		/* RR3 size 			*/
				IARG_END);
			ins_insertCall(instruction, ipoint_p2, AFUNPTR(pintool_instruction_analysis_1write_reg_Xread_p2),
				IARG_REG_VALUE, write_reg1,						/* RW1 value 			*/
				IARG_END);
			break;
		}
		case ANALYSIS_SELECTOR_2RW 				: {
			printf("ERROR: in %s, this case (2RW) is not supported\n", __func__);
			break;
		}
		case ANALYSIS_SELECTOR_1MR_2RW 			: {
			printf("ERROR: in %s, this case (1MR_2RW) is not supported\n", __func__);
			break;
		}
		case ANALYSIS_SELECTOR_1MW_2RW 			: {
			printf("ERROR: in %s, this case (1MW_2RW) is not supported\n", __func__);
			break;
		}
		case ANALYSIS_SELECTOR_1RR_2RW 			: {
			ins_insertCall(instruction, IPOINT_BEFORE, AFUNPTR(pintool_instruction_analysis_2write_reg_1read_reg_p1),
				IARG_INST_PTR,									/* pc 					*/
				IARG_UINT32, INS_Opcode(instruction),			/* opcode 				*/
				IARG_UINT32, pintool_REG_2_reg(write_reg1),		/* RW1 name 			*/
				IARG_UINT32, pintool_REG_size(write_reg1),		/* RW1 size 			*/
				IARG_UINT32, pintool_REG_2_reg(write_reg2),		/* RW2 name 			*/
				IARG_UINT32, pintool_REG_size(write_reg2),		/* RW2 size 			*/
				IARG_UINT32, pintool_REG_2_reg(read_reg1),		/* RR1 name 			*/
				IARG_REG_VALUE, read_reg1,						/* RR1 value 			*/
				IARG_UINT32, pintool_REG_size(read_reg1),		/* RR1 size 			*/
				IARG_END);
			ins_insertCall(instruction, ipoint_p2, AFUNPTR(pintool_instruction_analysis_2write_reg_Xread_p2),
				IARG_REG_VALUE, write_reg1,						/* RW1 value 			*/
				IARG_REG_VALUE, write_reg2,						/* RW2 value 			*/
				IARG_END);
			break;
		}
		case ANALYSIS_SELECTOR_2RR_2RW 			: {
			ins_insertCall(instruction, IPOINT_BEFORE, AFUNPTR(pintool_instruction_analysis_2write_reg_2read_reg_p1),
				IARG_INST_PTR,									/* pc 					*/
				IARG_UINT32, INS_Opcode(instruction),			/* opcode 				*/
				IARG_UINT32, pintool_REG_2_reg(write_reg1),		/* RW1 name 			*/
				IARG_UINT32, pintool_REG_size(write_reg1),		/* RW1 size 			*/
				IARG_UINT32, pintool_REG_2_reg(write_reg2),		/* RW2 name 			*/
				IARG_UINT32, pintool_REG_size(write_reg2),		/* RW2 size 			*/
				IARG_UINT32, pintool_REG_2_reg(read_reg1),		/* RR1 name 			*/
				IARG_REG_VALUE, read_reg1,						/* RR1 value 			*/
				IARG_UINT32, pintool_REG_size(read_reg1),		/* RR1 size 			*/
				IARG_UINT32, pintool_REG_2_reg(read_reg2),		/* RR2 name 			*/
				IARG_REG_VALUE, read_reg2,						/* RR2 value 			*/
				IARG_UINT32, pintool_REG_size(read_reg2),		/* RR2 size 			*/
				IARG_END);
			ins_insertCall(instruction, ipoint_p2, AFUNPTR(pintool_instruction_analysis_2write_reg_Xread_p2),
				IARG_REG_VALUE, write_reg1,						/* RW1 value 			*/
				IARG_REG_VALUE, write_reg2,						/* RW2 value 			*/
				IARG_END);
			break;
		}
		#if 0
		case ANALYSIS_SELECTOR_3RR_2RW 			: {
			ins_insertCall(instruction, IPOINT_BEFORE, AFUNPTR(pintool_instruction_analysis_2write_reg_3read_reg_p1),
				IARG_INST_PTR,									/* pc 					*/
				IARG_UINT32, INS_Opcode(instruction),			/* opcode 				*/
				IARG_UINT32, pintool_REG_2_reg(write_reg1),		/* RW1 name 			*/
				IARG_UINT32, pintool_REG_size(write_reg1),		/* RW1 size 			*/
				IARG_UINT32, pintool_REG_2_reg(write_reg2),		/* RW2 name 			*/
				IARG_UINT32, pintool_REG_size(write_reg2),		/* RW2 size 			*/
				IARG_UINT32, pintool_REG_2_reg(read_reg1),		/* RR1 name 			*/
				IARG_REG_VALUE, read_reg1,						/* RR1 value 			*/
				IARG_UINT32, pintool_REG_size(read_reg1),		/* RR1 size 			*/
				IARG_UINT32, pintool_REG_2_reg(read_reg2),		/* RR2 name 			*/
				IARG_REG_VALUE, read_reg2,						/* RR2 value 			*/
				IARG_UINT32, pintool_REG_size(read_reg2),		/* RR2 size 			*/
				IARG_UINT32, pintool_REG_2_reg(read_reg3),		/* RR3 name 			*/
				IARG_REG_VALUE, read_reg3,						/* RR3 value 			*/
				IARG_UINT32, pintool_REG_size(read_reg3),		/* RR3 size 			*/
				IARG_END);
			ins_insertCall(instruction, ipoint_p2, AFUNPTR(pintool_instruction_analysis_2write_reg_Xread_p2),
				IARG_REG_VALUE, write_reg1,						/* RW1 value 			*/
				IARG_REG_VALUE, write_reg2,						/* RW2 value 			*/
				IARG_END);
			break;
		}
		#endif
		case ANALYSIS_SELECTOR_3RW 				: {
			printf("ERROR: in %s, this case (3RW) is not supported\n", __func__);
			break;
		}
		case ANALYSIS_SELECTOR_1MR_3RW 			: {
			printf("ERROR: in %s, this case (1MR_3RW) is not supported\n", __func__);
			break;
		}
		case ANALYSIS_SELECTOR_1MW_3RW 			: {
			printf("ERROR: in %s, this case (1MW_3RW) is not supported\n", __func__);
			break;
		}
		default 								: {
			printf("ERROR: in %s, invalid analysis selector: 0x%08x ins: %s\n", __func__, selector, INS_Mnemonic(instruction).c_str());
			ins_insertCall(instruction, IPOINT_BEFORE, AFUNPTR(pintool_instruction_analysis_no_arg), 
				IARG_INST_PTR,									/* pc 					*/
				IARG_UINT32, INS_Opcode(instruction), 			/* opcode 				*/
				IARG_END);
			break;
		}
		}
	}
}

void pintool_instrumentation_img(IMG image, void* val){
	SEC 				section;
	RTN 				routine;
	struct cm_routine*	cm_rtn;
	char				white_listed;

	white_listed = (whiteList_search(tracer.white_list, IMG_Name(image).c_str()) == 0)?CODEMAP_WHITELISTED:CODEMAP_NOT_WHITELISTED;
	if (codeMap_add_image(tracer.code_map, IMG_LowAddress(image), IMG_HighAddress(image), IMG_Name(image).c_str(), white_listed)){
		printf("ERROR: in %s, unable to add image to code map structure\n", __func__);
	}
	else{
		for (section= IMG_SecHead(image); SEC_Valid(section); section = SEC_Next(section)){
			if (SEC_IsExecutable(section) && SEC_Mapped(section)){
				if (codeMap_add_section(tracer.code_map, SEC_Address(section), SEC_Address(section) + SEC_Size(section), SEC_Name(section).c_str())){
					printf("ERROR: in %s, unable to add section to code map structure\n", __func__);
					break;
				}
				else{
					for (routine= SEC_RtnHead(section); RTN_Valid(routine); routine = RTN_Next(routine)){
						white_listed |= (whiteList_search(tracer.white_list, RTN_Name(routine).c_str()) == 0)?CODEMAP_WHITELISTED:CODEMAP_NOT_WHITELISTED;
						cm_rtn = codeMap_add_routine(tracer.code_map, RTN_Address(routine), RTN_Address(routine) + RTN_Size(routine), RTN_Name(routine).c_str(), white_listed);
						if (cm_rtn == NULL){
							printf("ERROR: in %s, unable to add routine to code map structure\n", __func__);
							break;
						}
						else{
							RTN_Open(routine);
							RTN_InsertCall(routine, IPOINT_BEFORE, (AFUNPTR)pintool_routine_analysis, IARG_PTR, cm_rtn, IARG_END);
							RTN_Close(routine);
						}
					}
				}
			}
		}
	}
}


/* ===================================================================== */
/* Init function                                                    	 */
/* ===================================================================== */

int pintool_init(const char* trace_dir_name, const char* white_list_file_name){
	tracer.trace_file = traceFiles_create(trace_dir_name);
	if (tracer.trace_file == NULL){
		printf("ERROR: in %s, unable to create trace file\n", __func__);
		goto fail;
	}

	tracer.code_map = codeMap_create();
	if (tracer.code_map == NULL){
		printf("ERROR: in %s, unable to create code map\n", __func__);
		goto fail;
	}

	if (white_list_file_name != NULL && strcmp(white_list_file_name, "NULL")){
		tracer.white_list = whiteList_create(white_list_file_name);
		if (tracer.white_list == NULL){
			printf("ERROR: in %s, unable to create shared library white list\n", __func__);
			goto fail;
		}
	}
	else{
		tracer.white_list = NULL;
	}
	
	tracer.trace_buffer = (struct traceBuffer*)malloc(sizeof(struct traceBuffer));
	if (tracer.trace_buffer == NULL){
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
		goto fail;
	}

	tracer.trace_buffer->local_offset_ins 	= 0;
	tracer.trace_buffer->local_offset_op 	= 0;
	tracer.trace_buffer->local_offset_data 	= 0;
	tracer.trace_buffer->global_offset_op 	= 0;
	tracer.trace_buffer->global_offset_data = 0;

	return 0;

	fail:

	if (tracer.trace_file != NULL){
		traceFiles_delete(tracer.trace_file);
	}
	if (tracer.code_map != NULL){
		codeMap_delete(tracer.code_map);
	}
	if (tracer.white_list != NULL){
		whiteList_delete(tracer.white_list);
	}

	return -1;
}


/* ===================================================================== */
/* Cleanup function                                                 	 */
/* ===================================================================== */

void pintool_clean(INT32 code, void* arg){
	traceBuffer_flush(tracer.trace_buffer, tracer.trace_file);
	free(tracer.trace_buffer);

	traceFiles_print_codeMap(tracer.trace_file, tracer.code_map);
	traceFiles_delete(tracer.trace_file);

	codeMap_delete(tracer.code_map);

	whiteList_delete(tracer.white_list);
}


/* ===================================================================== */
/* Main                                                                  */
/* ===================================================================== */

int main(int argc, char * argv[]){
	PIN_InitSymbols();
	
	if (PIN_Init(argc, argv)){
		printf("ERROR: in %s, unable to init PIN\n", __func__);
		return -1;
	}

	if (pintool_init(knob_trace.Value().c_str(), knob_white_list.Value().c_str())){
		printf("ERROR: in %s, unable to init the tool\n", __func__);
		printf("%s",  KNOB_BASE::StringKnobSummary().c_str());
		return -1;
	}

	IMG_AddInstrumentFunction(pintool_instrumentation_img, NULL);
	INS_AddInstrumentFunction(pintool_instrumentation_ins, NULL);
	PIN_AddFiniFunction(pintool_clean, NULL);
	
	PIN_StartProgram();

	return 0;
}
