#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include <stdint.h>
#include <stdio.h>

#include "include/xed-iclass-enum.h"
#include "address.h"

#if defined __linux__
#include "multiColumn.h"
#elif defined WIN32
#include "../misc/multiColumn.h"
#endif

#define INSTRUCTION_MAX_NB_DATA 5 /* Do not update this value without taking care of the tracer */

/* Data type value:
 * - bit 1: set to 0 -> INVALID 	set to 1 -> VALID
 * - bit 2: set to 0 -> MEM 		set to 1 -> REG
 * - bit 3: set to 0 -> READ 		set to 1 -> WRITE
 */

#define INSTRUCTION_DATA_TYPE_SET_INVALID(type) (type) &= 0xfffffffe
#define INSTRUCTION_DATA_TYPE_SET_VALID(type) 	(type) |= 0x00000001
#define INSTRUCTION_DATA_TYPE_SET_MEM(type)		(type) &= 0xfffffffd
#define INSTRUCTION_DATA_TYPE_SET_REG(type)		(type) |= 0x00000002
#define INSTRUCTION_DATA_TYPE_SET_READ(type)	(type) &= 0xfffffffb
#define INSTRUCTION_DATA_TYPE_SET_WRITE(type)	(type) |= 0x00000004

#define INSTRUCTION_DATA_TYPE_IS_INVALID(type) 	(((type) & 0x00000001) == 0x00000000)
#define INSTRUCTION_DATA_TYPE_IS_VALID(type) 	(((type) & 0x00000001) == 0x00000001)
#define INSTRUCTION_DATA_TYPE_IS_MEM(type) 		(((type) & 0x00000002) == 0x00000000)
#define INSTRUCTION_DATA_TYPE_IS_REG(type)		(((type) & 0x00000002) == 0x00000002)
#define INSTRUCTION_DATA_TYPE_IS_READ(type)		(((type) & 0x00000004) == 0x00000000)
#define INSTRUCTION_DATA_TYPE_IS_WRITE(type) 	(((type) & 0x00000004) == 0x00000004)

enum operandType{
	OPERAND_INVALID 	= 0x00000000,
	OPERAND_MEM_READ	= 0x00000001,
	OPERAND_REG_READ 	= 0x00000003,
	OPERAND_MEM_WRITE	= 0x00000005,
	OPERAND_REG_WRITE	= 0x00000007
};

#define NB_REGISTER 19 /* do not forget to update this value */

enum reg{
	REGISTER_INVALID 	= 0x00000000,
	REGISTER_EAX 		= 0x00000001,
	REGISTER_AX 		= 0x00000002,
	REGISTER_AH 		= 0x00000003,
	REGISTER_AL 		= 0x00000004,
	REGISTER_EBX 		= 0x00000005,
	REGISTER_BX 		= 0x00000006,
	REGISTER_BH 		= 0x00000007,
	REGISTER_BL 		= 0x00000008,
	REGISTER_ECX 		= 0x00000009,
	REGISTER_CX 		= 0x0000000a,
	REGISTER_CH 		= 0x0000000b,
	REGISTER_CL 		= 0x0000000c,
	REGISTER_EDX 		= 0x0000000d,
	REGISTER_DX 		= 0x0000000e,
	REGISTER_DH 		= 0x0000000f,
	REGISTER_DL 		= 0x00000010,
	REGISTER_ESI 		= 0x00000011,
	REGISTER_EDI 		= 0x00000012,
	REGISTER_EBP 		= 0x00000013
};

struct operand{
	enum operandType 	type;
	union {
		ADDRESS 		address;
		enum reg 		reg;
	}					location;
	uint8_t 			size;
	uint32_t 			data_offset;
};

/* a supprimer */
struct insData{
	enum operandType 	type;
	union {
		ADDRESS 		address;
		enum reg 		reg;
	}					location;
	uint32_t 			value;
	uint8_t 			size;
};

/* a supprimer */
struct instruction{
	ADDRESS 		pc;
	uint32_t 		opcode;
	struct insData 	data[INSTRUCTION_MAX_NB_DATA];
	/* offset vers le tablau de truc uint32_t operand_offset*/
	/* uint32_t nb_operand */
};

struct _instruction{ /*tmp*/
	ADDRESS 		pc;
	uint32_t 		opcode;
	uint32_t 		operand_offset;
	uint32_t 		nb_operand;
};

struct multiColumnPrinter* instruction_init_multiColumnPrinter();
void instruction_print(struct multiColumnPrinter* printer, struct instruction *ins);

int32_t instruction_compare_pc(struct instruction* ins1, struct instruction* ins2);

void instruction_flush_tracer_buffer(FILE* file, struct instruction* buffer, uint32_t nb_instruction);

const char* instruction_opcode_2_string(uint32_t opcode);
const char* reg_2_string(enum reg reg);


#endif