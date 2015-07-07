#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include <stdint.h>
#include <stdio.h>

#include "address.h"

/* Operand type value:
 * - bit 1: set to 0 -> INVALID 	set to 1 -> VALID
 * - bit 2: set to 0 -> MEM 		set to 1 -> REG
 * - bit 3: set to 0 -> READ 		set to 1 -> WRITE
 * - bit 4: set to 0 -> NO_BASE 	set to 1 -> BASE
 * - bit 5: set to 0 -> NO_INDEX 	set to 1 -> INDEX
 */

#define OPERAND_TYPE_SET_INVALID(type) 	(type) &= 0xfffffffe
#define OPERAND_TYPE_SET_VALID(type) 	(type) |= 0x00000001
#define OPERAND_TYPE_SET_MEM(type)		(type) &= 0xfffffffd
#define OPERAND_TYPE_SET_REG(type)		(type) |= 0x00000002
#define OPERAND_TYPE_SET_READ(type)		(type) &= 0xfffffffb
#define OPERAND_TYPE_SET_WRITE(type)	(type) |= 0x00000004

#define OPERAND_TYPE_IS_INVALID(type) 	(((type) & 0x00000001) == 0x00000000)
#define OPERAND_TYPE_IS_VALID(type) 	(((type) & 0x00000001) == 0x00000001)
#define OPERAND_TYPE_IS_MEM(type) 		(((type) & 0x00000002) == 0x00000000)
#define OPERAND_TYPE_IS_REG(type)		(((type) & 0x00000002) == 0x00000002)
#define OPERAND_TYPE_IS_READ(type)		(((type) & 0x00000004) == 0x00000000)
#define OPERAND_TYPE_IS_WRITE(type) 	(((type) & 0x00000004) == 0x00000004)

#define OPERAND_SET_INVALID(op) 		OPERAND_TYPE_SET_INVALID((op).type)
#define OPERAND_SET_VALID(op) 			OPERAND_TYPE_SET_VALID((op).type)
#define OPERAND_SET_MEM(op)				OPERAND_TYPE_SET_MEM((op).type)
#define OPERAND_SET_REG(op)				OPERAND_TYPE_SET_REG((op).type)
#define OPERAND_SET_READ(op)			OPERAND_TYPE_SET_READ((op).type)
#define OPERAND_SET_WRITE(op)			OPERAND_TYPE_SET_WRITE((op).type)

#define OPERAND_IS_INVALID(op) 			OPERAND_TYPE_IS_INVALID((op).type)
#define OPERAND_IS_VALID(op) 			OPERAND_TYPE_IS_VALID((op).type)
#define OPERAND_IS_MEM(op) 				OPERAND_TYPE_IS_MEM((op).type)
#define OPERAND_IS_REG(op)				OPERAND_TYPE_IS_REG((op).type)
#define OPERAND_IS_READ(op)				OPERAND_TYPE_IS_READ((op).type)
#define OPERAND_IS_WRITE(op) 			OPERAND_TYPE_IS_WRITE((op).type)
#define OPERAND_IS_BASE(op) 			((op).type == OPERAND_REG_READ_BASE)
#define OPERAND_IS_INDEX(op) 			((op).type == OPERAND_REG_READ_INDEX)

enum operandType{
	OPERAND_INVALID 		= 0x00000000,
	OPERAND_MEM_READ		= 0x00000001,
	OPERAND_REG_READ 		= 0x00000003,
	OPERAND_MEM_WRITE		= 0x00000005,
	OPERAND_REG_WRITE		= 0x00000007,
	OPERAND_REG_READ_BASE 	= 0x0000000b,
	OPERAND_REG_READ_INDEX 	= 0x00000013
};

#define NB_REGISTER 20 /* do not forget to update this value */

enum reg{
	REGISTER_INVALID 	= 0x00000000,	/* 0  */
	REGISTER_EAX 		= 0x00000001,	/* 1  */
	REGISTER_AX 		= 0x00000002,	/* 2  */
	REGISTER_AH 		= 0x00000003,	/* 3  */
	REGISTER_AL 		= 0x00000004,	/* 4  */
	REGISTER_EBX 		= 0x00000005,	/* 5  */
	REGISTER_BX 		= 0x00000006,	/* 6  */
	REGISTER_BH 		= 0x00000007,	/* 7  */
	REGISTER_BL 		= 0x00000008,	/* 8  */
	REGISTER_ECX 		= 0x00000009,	/* 9  */
	REGISTER_CX 		= 0x0000000a,	/* 10 */
	REGISTER_CH 		= 0x0000000b,	/* 11 */
	REGISTER_CL 		= 0x0000000c,	/* 12 */
	REGISTER_EDX 		= 0x0000000d,	/* 13 */
	REGISTER_DX 		= 0x0000000e,	/* 14 */
	REGISTER_DH 		= 0x0000000f,	/* 15 */
	REGISTER_DL 		= 0x00000010,	/* 16 */
	REGISTER_ESI 		= 0x00000011,	/* 17 */
	REGISTER_EDI 		= 0x00000012,	/* 18 */
	REGISTER_EBP 		= 0x00000013,	/* 19 */
	REGISTER_ESP 		= 0x00000014 	/* 20 */
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

struct instruction{
	ADDRESS 		pc;
	uint32_t 		opcode;
	uint32_t 		operand_offset;
	uint32_t 		nb_operand;
};

const char* reg_2_string(enum reg reg);

int32_t reg_is_contained_in(enum reg reg1, enum reg reg2);
int8_t reg_get_size(enum reg reg);

#endif