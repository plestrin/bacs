#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "irImporterAsm.h"
#include "irRenameEngine.h"
#include "irConcat.h"
#include "base.h"

static enum irOpcode xedOpcode_2_irOpcode(xed_iclass_enum_t xed_opcode);
static enum irRegister xedRegister_2_irRegister(xed_reg_enum_t xed_reg);

#define IRIMPORTERASM_MAX_INPUT_OPERAND 	3
#define IRIMPORTERASM_MAX_RISC_INS 			64

static const enum irDependenceType dependence_label_table[NB_IR_OPCODE - 1][IRIMPORTERASM_MAX_INPUT_OPERAND] = {
	{IR_DEPENDENCE_TYPE_DIRECT, 	IR_DEPENDENCE_TYPE_DIRECT, 		IR_DEPENDENCE_TYPE_DIRECT}, 	/* 0  IR_ADC 		*/
	{IR_DEPENDENCE_TYPE_DIRECT, 	IR_DEPENDENCE_TYPE_DIRECT, 		IR_DEPENDENCE_TYPE_DIRECT}, 	/* 1  IR_ADD 		*/
	{IR_DEPENDENCE_TYPE_DIRECT, 	IR_DEPENDENCE_TYPE_DIRECT, 		IR_DEPENDENCE_TYPE_DIRECT}, 	/* 2  IR_AND 		*/
	{IR_DEPENDENCE_TYPE_DIRECT, 	IR_DEPENDENCE_TYPE_DIRECT, 		IR_DEPENDENCE_TYPE_DIRECT}, 	/* 3  IR_CMOV 		*/
	{IR_DEPENDENCE_TYPE_DIVISOR, 	IR_DEPENDENCE_TYPE_DIRECT, 		IR_DEPENDENCE_TYPE_DIRECT}, 	/* 4  IR_DIVQ 		*/
	{IR_DEPENDENCE_TYPE_DIVISOR, 	IR_DEPENDENCE_TYPE_DIRECT, 		IR_DEPENDENCE_TYPE_DIRECT}, 	/* 5  IR_DIVR 		*/
	{IR_DEPENDENCE_TYPE_DIVISOR, 	IR_DEPENDENCE_TYPE_DIRECT, 		IR_DEPENDENCE_TYPE_DIRECT}, 	/* 6  IR_IDIV 		*/
	{IR_DEPENDENCE_TYPE_DIRECT, 	IR_DEPENDENCE_TYPE_DIRECT, 		IR_DEPENDENCE_TYPE_DIRECT}, 	/* 7  IR_IMUL 		*/
	{IR_DEPENDENCE_TYPE_DIRECT, 	IR_DEPENDENCE_TYPE_DIRECT, 		IR_DEPENDENCE_TYPE_DIRECT}, 	/* 8  IR_LEA 		*/
	{IR_DEPENDENCE_TYPE_DIRECT, 	IR_DEPENDENCE_TYPE_DIRECT, 		IR_DEPENDENCE_TYPE_DIRECT}, 	/* 9  IR_MOV 		*/
	{IR_DEPENDENCE_TYPE_DIRECT, 	IR_DEPENDENCE_TYPE_DIRECT, 		IR_DEPENDENCE_TYPE_DIRECT}, 	/* 10 IR_MOVZX 		*/
	{IR_DEPENDENCE_TYPE_DIRECT, 	IR_DEPENDENCE_TYPE_DIRECT, 		IR_DEPENDENCE_TYPE_DIRECT}, 	/* 11 IR_MUL 		*/
	{IR_DEPENDENCE_TYPE_DIRECT, 	IR_DEPENDENCE_TYPE_DIRECT, 		IR_DEPENDENCE_TYPE_DIRECT}, 	/* 12 IR_NEG 		*/
	{IR_DEPENDENCE_TYPE_DIRECT, 	IR_DEPENDENCE_TYPE_DIRECT, 		IR_DEPENDENCE_TYPE_DIRECT}, 	/* 13 IR_NOT 		*/
	{IR_DEPENDENCE_TYPE_DIRECT, 	IR_DEPENDENCE_TYPE_DIRECT, 		IR_DEPENDENCE_TYPE_DIRECT}, 	/* 14 IR_OR 		*/
	{IR_DEPENDENCE_TYPE_DIRECT, 	IR_DEPENDENCE_TYPE_DIRECT, 		IR_DEPENDENCE_TYPE_DIRECT}, 	/* 15 IR_PART1_8 	*/
	{IR_DEPENDENCE_TYPE_DIRECT, 	IR_DEPENDENCE_TYPE_DIRECT, 		IR_DEPENDENCE_TYPE_DIRECT}, 	/* 16 IR_PART2_8 	*/
	{IR_DEPENDENCE_TYPE_DIRECT, 	IR_DEPENDENCE_TYPE_DIRECT, 		IR_DEPENDENCE_TYPE_DIRECT}, 	/* 17 IR_PART1_16 	*/
	{IR_DEPENDENCE_TYPE_DIRECT, 	IR_DEPENDENCE_TYPE_SHIFT_DISP, 	IR_DEPENDENCE_TYPE_DIRECT}, 	/* 18 IR_ROL 		*/
	{IR_DEPENDENCE_TYPE_DIRECT, 	IR_DEPENDENCE_TYPE_SHIFT_DISP, 	IR_DEPENDENCE_TYPE_DIRECT}, 	/* 19 IR_ROR 		*/
	{IR_DEPENDENCE_TYPE_DIRECT, 	IR_DEPENDENCE_TYPE_SHIFT_DISP, 	IR_DEPENDENCE_TYPE_DIRECT}, 	/* 20 IR_SHL 		*/
	{IR_DEPENDENCE_TYPE_DIRECT, 	IR_DEPENDENCE_TYPE_ROUND_OFF, 	IR_DEPENDENCE_TYPE_SHIFT_DISP}, /* 21 IR_SHLD 		*/
	{IR_DEPENDENCE_TYPE_DIRECT, 	IR_DEPENDENCE_TYPE_SHIFT_DISP, 	IR_DEPENDENCE_TYPE_DIRECT}, 	/* 22 IR_SHR 		*/
	{IR_DEPENDENCE_TYPE_DIRECT, 	IR_DEPENDENCE_TYPE_ROUND_OFF, 	IR_DEPENDENCE_TYPE_SHIFT_DISP}, /* 23 IR_SHRD 		*/
	{IR_DEPENDENCE_TYPE_DIRECT, 	IR_DEPENDENCE_TYPE_SUBSTITUTE, 	IR_DEPENDENCE_TYPE_DIRECT}, 	/* 24 IR_SUB 		*/
	{IR_DEPENDENCE_TYPE_DIRECT, 	IR_DEPENDENCE_TYPE_DIRECT, 		IR_DEPENDENCE_TYPE_DIRECT}, 	/* 25 IR_XOR 		*/
	{IR_DEPENDENCE_TYPE_DIRECT, 	IR_DEPENDENCE_TYPE_DIRECT, 		IR_DEPENDENCE_TYPE_DIRECT}, 	/* 26 IR_LOAD 		*/
	{IR_DEPENDENCE_TYPE_DIRECT, 	IR_DEPENDENCE_TYPE_DIRECT, 		IR_DEPENDENCE_TYPE_DIRECT}, 	/* 27 IR_STORE 		*/
	{IR_DEPENDENCE_TYPE_DIRECT, 	IR_DEPENDENCE_TYPE_DIRECT, 		IR_DEPENDENCE_TYPE_DIRECT}, 	/* 28 IR_JOKER 		*/
};

static const uint8_t sign_extand_table[NB_IR_OPCODE - 1] = {
	1, /* 0  IR_ADD 		*/
	1, /* 1  IR_ADD 		*/
	1, /* 2  IR_AND 		*/
	0, /* 3  IR_CMOV 		*/
	0, /* 4  IR_DIVQ 		*/
	0, /* 5  IR_DIVR 		*/
	0, /* 6  IR_IDIV 		*/
	1, /* 7  IR_IMUL 		*/
	0, /* 8  IR_LEA 		*/
	0, /* 9  IR_MOV 		*/
	0, /* 10 IR_MOVZX 		*/
	0, /* 11 IR_MUL 		*/
	0, /* 12 IR_NEG 		*/
	0, /* 13 IR_NOT 		*/
	1, /* 14 IR_OR 			*/
	0, /* 15 IR_PART1_8 	*/
	0, /* 16 IR_PART2_8 	*/
	0, /* 17 IR_PART1_16 	*/
	0, /* 18 IR_ROL 		*/
	0, /* 19 IR_ROR 		*/
	0, /* 20 IR_SHL 		*/
	0, /* 21 IR_SHLD 		*/
	0, /* 22 IR_SHR 		*/
	0, /* 23 IR_SHRD 		*/
	1, /* 24 IR_SUB 		*/
	1, /* 25 IR_XOR 		*/
	0, /* 26 IR_LOAD 		*/
	0, /* 27 IR_STORE 		*/
	0  /* 28 IR_JOKER 		*/
};

static int32_t xed_reg_get_simd_base(xed_reg_enum_t reg){
	switch(reg){
		case XED_REG_MMX0 	: {return 8;}
		case XED_REG_MMX1 	: {return 9;}
		case XED_REG_MMX2 	: {return 10;}
		case XED_REG_MMX3 	: {return 11;}
		case XED_REG_MMX4 	: {return 12;}
		case XED_REG_MMX5 	: {return 13;}
		case XED_REG_MMX6 	: {return 14;}
		case XED_REG_MMX7 	: {return 15;}
		case XED_REG_XMM0 	: {return 0;}
		case XED_REG_XMM1 	: {return 1;}
		case XED_REG_XMM2 	: {return 2;}
		case XED_REG_XMM3 	: {return 3;}
		case XED_REG_XMM4 	: {return 4;}
		case XED_REG_XMM5 	: {return 5;}
		case XED_REG_XMM6 	: {return 6;}
		case XED_REG_XMM7 	: {return 7;}
		case XED_REG_YMM0 	: {return 0;}
		case XED_REG_YMM1 	: {return 1;}
		case XED_REG_YMM2 	: {return 2;}
		case XED_REG_YMM3 	: {return 3;}
		case XED_REG_YMM4 	: {return 4;}
		case XED_REG_YMM5 	: {return 5;}
		case XED_REG_YMM6 	: {return 6;}
		case XED_REG_YMM7 	: {return 7;}
		default 			: {break;}
	}

	log_err_m("this SIMD register (%s) is not handled yet", xed_reg_enum_t2str(reg));
	return -1;
}

static const uint32_t ir_register_simd_base[80] = {
	0 , /* IR_REG_XMM1_1 */
	0 , /* IR_REG_XMM1_2 */
	0 , /* IR_REG_XMM1_3 */
	0 , /* IR_REG_XMM1_4 */
	1 , /* IR_REG_XMM2_1 */
	1 , /* IR_REG_XMM2_2 */
	1 , /* IR_REG_XMM2_3 */
	1 , /* IR_REG_XMM2_4 */
	2 , /* IR_REG_XMM3_1 */
	2 , /* IR_REG_XMM3_2 */
	2 , /* IR_REG_XMM3_3 */
	2 , /* IR_REG_XMM3_4 */
	3 , /* IR_REG_XMM4_1 */
	3 , /* IR_REG_XMM4_2 */
	3 , /* IR_REG_XMM4_3 */
	3 , /* IR_REG_XMM4_4 */
	4 , /* IR_REG_XMM5_1 */
	4 , /* IR_REG_XMM5_2 */
	4 , /* IR_REG_XMM5_3 */
	4 , /* IR_REG_XMM5_4 */
	5 , /* IR_REG_XMM6_1 */
	5 , /* IR_REG_XMM6_2 */
	5 , /* IR_REG_XMM6_3 */
	5 , /* IR_REG_XMM6_4 */
	6 , /* IR_REG_XMM7_1 */
	6 , /* IR_REG_XMM7_2 */
	6 , /* IR_REG_XMM7_3 */
	6 , /* IR_REG_XMM7_4 */
	7 , /* IR_REG_XMM8_1 */
	7 , /* IR_REG_XMM8_2 */
	7 , /* IR_REG_XMM8_3 */
	7 , /* IR_REG_XMM8_4 */
	8 , /* IR_REG_MMX1_1 */
	8 , /* IR_REG_MMX1_2 */
	9 , /* IR_REG_MMX2_1 */
	9 , /* IR_REG_MMX2_2 */
	10, /* IR_REG_MMX3_1 */
	10, /* IR_REG_MMX3_2 */
	11, /* IR_REG_MMX4_1 */
	11, /* IR_REG_MMX4_2 */
	12, /* IR_REG_MMX5_1 */
	12, /* IR_REG_MMX5_2 */
	13, /* IR_REG_MMX6_1 */
	13, /* IR_REG_MMX6_2 */
	14, /* IR_REG_MMX7_1 */
	14, /* IR_REG_MMX7_2 */
	15, /* IR_REG_MMX8_1 */
	15, /* IR_REG_MMX8_2 */
	0 , /* IR_REG_YMM1_5 */
	0 , /* IR_REG_YMM1_6 */
	0 , /* IR_REG_YMM1_7 */
	0 , /* IR_REG_YMM1_8 */
	1 , /* IR_REG_YMM2_5 */
	1 , /* IR_REG_YMM2_6 */
	1 , /* IR_REG_YMM2_7 */
	1 , /* IR_REG_YMM2_8 */
	2 , /* IR_REG_YMM3_5 */
	2 , /* IR_REG_YMM3_6 */
	2 , /* IR_REG_YMM3_7 */
	2 , /* IR_REG_YMM3_8 */
	3 , /* IR_REG_YMM4_5 */
	3 , /* IR_REG_YMM4_6 */
	3 , /* IR_REG_YMM4_7 */
	3 , /* IR_REG_YMM4_8 */
	4 , /* IR_REG_YMM5_5 */
	4 , /* IR_REG_YMM5_6 */
	4 , /* IR_REG_YMM5_7 */
	4 , /* IR_REG_YMM5_8 */
	5 , /* IR_REG_YMM6_5 */
	5 , /* IR_REG_YMM6_6 */
	5 , /* IR_REG_YMM6_7 */
	5 , /* IR_REG_YMM6_8 */
	6 , /* IR_REG_YMM7_5 */
	6 , /* IR_REG_YMM7_6 */
	6 , /* IR_REG_YMM7_7 */
	6 , /* IR_REG_YMM7_8 */
	7 , /* IR_REG_YMM8_5 */
	7 , /* IR_REG_YMM8_6 */
	7 , /* IR_REG_YMM8_7 */
	7 , /* IR_REG_YMM8_8 */
};

#define ir_reg_get_simd_base(reg) ir_register_simd_base[(reg) - IR_REG_XMM1_1]

#define IR_REGISTER_NB_SIMD_BASE 16
#define IR_REGISTER_NB_MAX_ENTRY_PER_SIMD_BASE 8

static const enum irRegister ir_register_simd_frag[IR_REGISTER_NB_SIMD_BASE][IR_REGISTER_NB_MAX_ENTRY_PER_SIMD_BASE] = {
	{IR_REG_XMM1_1, IR_REG_XMM1_2, IR_REG_XMM1_3, IR_REG_XMM1_4, IR_REG_YMM1_5, IR_REG_YMM1_6, IR_REG_YMM1_7, IR_REG_YMM1_8}, 	/* 0  */
	{IR_REG_XMM2_1, IR_REG_XMM2_2, IR_REG_XMM2_3, IR_REG_XMM2_4, IR_REG_YMM2_5, IR_REG_YMM2_6, IR_REG_YMM2_7, IR_REG_YMM2_8}, 	/* 1  */
	{IR_REG_XMM3_1, IR_REG_XMM3_2, IR_REG_XMM3_3, IR_REG_XMM3_4, IR_REG_YMM3_5, IR_REG_YMM3_6, IR_REG_YMM3_7, IR_REG_YMM3_8}, 	/* 2  */
	{IR_REG_XMM4_1, IR_REG_XMM4_2, IR_REG_XMM4_3, IR_REG_XMM4_4, IR_REG_YMM4_5, IR_REG_YMM4_6, IR_REG_YMM4_7, IR_REG_YMM4_8}, 	/* 3  */
	{IR_REG_XMM5_1, IR_REG_XMM5_2, IR_REG_XMM5_3, IR_REG_XMM5_4, IR_REG_YMM5_5, IR_REG_YMM5_6, IR_REG_YMM5_7, IR_REG_YMM5_8}, 	/* 4  */
	{IR_REG_XMM6_1, IR_REG_XMM6_2, IR_REG_XMM6_3, IR_REG_XMM6_4, IR_REG_YMM6_5, IR_REG_YMM6_6, IR_REG_YMM6_7, IR_REG_YMM6_8}, 	/* 5  */
	{IR_REG_XMM7_1, IR_REG_XMM7_2, IR_REG_XMM7_3, IR_REG_XMM7_4, IR_REG_YMM7_5, IR_REG_YMM7_6, IR_REG_YMM7_7, IR_REG_YMM7_8}, 	/* 6  */
	{IR_REG_XMM8_1, IR_REG_XMM8_2, IR_REG_XMM8_3, IR_REG_XMM8_4, IR_REG_YMM8_5, IR_REG_YMM8_6, IR_REG_YMM8_7, IR_REG_YMM8_8}, 	/* 7  */
	{IR_REG_MMX1_1, IR_REG_MMX1_2, IR_REG_MMX1_2, IR_REG_MMX1_2, IR_REG_MMX1_2, IR_REG_MMX1_2, IR_REG_MMX1_2, IR_REG_MMX1_2}, 	/* 8  */
	{IR_REG_MMX2_1, IR_REG_MMX2_2, IR_REG_MMX2_2, IR_REG_MMX2_2, IR_REG_MMX2_2, IR_REG_MMX2_2, IR_REG_MMX2_2, IR_REG_MMX2_2}, 	/* 9  */
	{IR_REG_MMX3_1, IR_REG_MMX3_2, IR_REG_MMX3_2, IR_REG_MMX3_2, IR_REG_MMX3_2, IR_REG_MMX3_2, IR_REG_MMX3_2, IR_REG_MMX3_2}, 	/* 10 */
	{IR_REG_MMX4_1, IR_REG_MMX4_2, IR_REG_MMX4_2, IR_REG_MMX4_2, IR_REG_MMX4_2, IR_REG_MMX4_2, IR_REG_MMX4_2, IR_REG_MMX4_2}, 	/* 11 */
	{IR_REG_MMX5_1, IR_REG_MMX5_2, IR_REG_MMX5_2, IR_REG_MMX5_2, IR_REG_MMX5_2, IR_REG_MMX5_2, IR_REG_MMX5_2, IR_REG_MMX5_2}, 	/* 12 */
	{IR_REG_MMX6_1, IR_REG_MMX6_2, IR_REG_MMX6_2, IR_REG_MMX6_2, IR_REG_MMX6_2, IR_REG_MMX6_2, IR_REG_MMX6_2, IR_REG_MMX6_2}, 	/* 13 */
	{IR_REG_MMX7_1, IR_REG_MMX7_2, IR_REG_MMX7_2, IR_REG_MMX7_2, IR_REG_MMX7_2, IR_REG_MMX7_2, IR_REG_MMX7_2, IR_REG_MMX7_2}, 	/* 14 */
	{IR_REG_MMX8_1, IR_REG_MMX8_2, IR_REG_MMX8_2, IR_REG_MMX8_2, IR_REG_MMX8_2, IR_REG_MMX8_2, IR_REG_MMX8_2, IR_REG_MMX8_2} 	/* 15 */
};

struct memOperand{
	xed_reg_enum_t 	base;
	xed_reg_enum_t 	index;
	uint8_t 		scale;
	uint32_t 		disp;
	ADDRESS 		con_addr;
};

static struct node* memOperand_build_address(struct irRenameEngine* engine, struct memOperand* mem, uint32_t instruction_index, uint32_t dst);

enum asmOperandType{
	ASM_OPERAND_IMM,
	ASM_OPERAND_REG,
	ASM_OPERAND_MEM
};

#define ASM_OPERAND_ROLE_READ_1 	0x00000001
#define ASM_OPERAND_ROLE_READ_2 	0x00000002
#define ASM_OPERAND_ROLE_READ_3 	0x00000004
#define ASM_OPERAND_ROLE_WRITE_1 	0x00000100
#define ASM_OPERAND_ROLE_WRITE_2 	0x00000200
#define ASM_OPERAND_ROLE_READ_ALL 	0x000000ff
#define ASM_OPERAND_ROLE_WRITE_ALL 	0x0000ff00
#define ASM_OPERAND_ROLE_ALL 		0x0000ffff

#define ASM_OPERAND_ROLE_IS_READ(role) 			((role) & ASM_OPERAND_ROLE_READ_ALL)
#define ASM_OPERAND_ROLE_IS_WRITE(role) 		((role) & ASM_OPERAND_ROLE_WRITE_ALL)
#define ASM_OPERAND_ROLE_IS_READ_INDEX(role, index) 	((role) & (0x00000001 << ((index) & 0x0000007))) 
#define ASM_OPERAND_ROLE_IS_WRITE_INDEX(role, index) 	((role) & (0x00000100 << ((index) & 0x0000007)))
#define ASM_OPERAND_ROLE_SET_FRAG(role, frag) 	((role) | ((frag) << 24))
#define ASM_OPERAND_ROLE_GET_FRAG(role) 		((role) >> 24)

struct asmOperand{
	uint16_t 				size;
	uint32_t 				instruction_index;
	struct node*			variable;
	enum asmOperandType 	type;
	union{
		enum irRegister 	reg;
		uint64_t 			imm;
		struct memOperand	mem;
	} 						operand_type;
};

static void asmOperand_decode(struct instructionIterator* it, struct asmOperand* operand_buffer, uint8_t max_nb_operand, uint32_t selector, uint8_t* nb_operand_, struct memAddress* mem_addr);
static void asmOperand_decode_simd(struct instructionIterator* it, struct asmOperand* operand_buffer, uint8_t max_nb_operand, uint32_t selector, uint8_t* nb_operand_, struct memAddress* mem_addr);

static void asmOperand_fetch_input(struct irRenameEngine* engine, struct asmOperand* operand, uint32_t dst);
static void asmOperand_fetch_output(struct irRenameEngine* engine, struct asmOperand* operand, enum irOpcode opcode, uint32_t dst);

static void asmOperand_sign_extend(struct asmOperand* operand, uint16_t size);

struct asmRiscIns{
	enum irOpcode 			opcode;
	uint8_t 				nb_input_operand;
	struct asmOperand 		input_operand[IRIMPORTERASM_MAX_INPUT_OPERAND];
	struct asmOperand 		output_operand;
};

static int32_t asmRisc_process(struct irRenameEngine* engine, struct asmRiscIns* risc, uint32_t dst);

static int32_t asmRisc_process_special_cmov(struct irRenameEngine* engine, struct asmRiscIns* risc, uint32_t dst);
static int32_t asmRisc_process_special_lea(struct irRenameEngine* engine, struct asmRiscIns* risc, uint32_t dst);
static int32_t asmRisc_process_special_mov(struct irRenameEngine* engine, struct asmRiscIns* risc, uint32_t dst);

struct asmCiscIns{
	uint8_t 				valid;
	uint8_t 				nb_ins;
	struct asmRiscIns 		ins[IRIMPORTERASM_MAX_RISC_INS];
};

#define ASMCISCINS_IS_VALID(cisc) 		((cisc).valid)
#define ASMCISCINS_SET_VALID(cisc) 		((cisc).valid = 1)
#define ASMCISCINS_SET_INVALID(cisc) 	((cisc).valid = 0)

static void cisc_decode_special_call(struct instructionIterator* it, struct asmCiscIns* cisc, struct memAddress* mem_addr);
static void cisc_decode_special_dec(struct instructionIterator* it, struct asmCiscIns* cisc, struct memAddress* mem_addr);
static void cisc_decode_special_div(struct instructionIterator* it, struct asmCiscIns* cisc, struct memAddress* mem_addr);
static void cisc_decode_special_inc(struct instructionIterator* it, struct asmCiscIns* cisc, struct memAddress* mem_addr);
static void cisc_decode_special_leave(struct instructionIterator* it, struct asmCiscIns* cisc, struct memAddress* mem_addr);
static void cisc_decode_special_movsd(struct instructionIterator* it, struct asmCiscIns* cisc, struct memAddress* mem_addr);
static void cisc_decode_special_pop(struct instructionIterator* it, struct asmCiscIns* cisc, struct memAddress* mem_addr);
static void cisc_decode_special_push(struct instructionIterator* it, struct asmCiscIns* cisc, struct memAddress* mem_addr);
static void cisc_decode_special_ret(const struct assembly* assembly, struct instructionIterator* it, struct asmCiscIns* cisc);
static void cisc_decode_special_setxx(struct instructionIterator* it, struct asmCiscIns* cisc, struct memAddress* mem_addr);
static void cisc_decode_special_stosd(struct instructionIterator* it, struct asmCiscIns* cisc, struct memAddress* mem_addr);
static void cisc_decode_special_xchg(struct instructionIterator* it, struct asmCiscIns* cisc, struct memAddress* mem_addr);

static inline uint32_t simd_get_nb_frag(const struct assembly* assembly, const struct instructionIterator* it){
	if (assembly->dyn_blocks[it->dyn_block_index].block->data[it->instruction_offset] == 0x66){
		return 4;
	}
	else{
		return 2;
	}
}

static void cisc_decode_generic_simd(struct instructionIterator* it, struct asmCiscIns* cisc, struct memAddress* mem_addr, uint32_t nb_frag);
static void cisc_decode_special_movd(const struct assembly* assembly, struct instructionIterator* it, struct asmCiscIns* cisc, struct memAddress* mem_addr);
static void cisc_decode_special_movq(const struct assembly* assembly, struct instructionIterator* it, struct asmCiscIns* cisc, struct memAddress* mem_addr);
static void cisc_decode_special_movsd_xmm(struct instructionIterator* it, struct asmCiscIns* cisc, struct memAddress* mem_addr);
static void cisc_decode_special_pinsrw(struct instructionIterator* it, struct asmCiscIns* cisc, struct memAddress* mem_addr);
static void cisc_decode_special_pshufd(struct instructionIterator* it, struct asmCiscIns* cisc, struct memAddress* mem_addr);
static void cisc_decode_special_pslld(const struct assembly* assembly, struct instructionIterator* it, struct asmCiscIns* cisc, struct memAddress* mem_addr);
static void cisc_decode_special_psrld(const struct assembly* assembly, struct instructionIterator* it, struct asmCiscIns* cisc, struct memAddress* mem_addr);
static void cisc_decode_special_punpckldq(const struct assembly* assembly, struct instructionIterator* it, struct asmCiscIns* cisc, struct memAddress* mem_addr);

static void cisc_decode_generic_vex(struct instructionIterator* it, struct asmCiscIns* cisc, struct memAddress* mem_addr);
static void cisc_decode_special_vzeroall(struct instructionIterator* it, struct asmCiscIns* cisc);

#define irImporterAsm_get_mem_trace(trace, ins_it) (((trace) != NULL && instructionIterator_is_mem_addr_valid(ins_it)) ? ((trace)->mem_addr_buffer + instructionIterator_get_mem_addr_index(ins_it)) : NULL)

static void irImporter_handle_instruction(const struct assembly* assembly, struct irRenameEngine* engine, struct instructionIterator* it, struct memAddress* mem_addr){
	struct asmCiscIns 	cisc;
	uint32_t 			i;
	int32_t 			error_code;

	ASMCISCINS_SET_INVALID(cisc);

	/* for stdcall and cdecl eax is caller-saved so it's safe to erase */
	if (it->instruction_index > 0 && it->prev_black_listed){
		#ifdef VERBOSE
		log_info_m("assuming stdcall/cdecl @ %u", it->instruction_index);
		#endif
		irRenameEngine_clear_eax_std_call(engine);
	}

	switch(xed_decoded_inst_get_iclass(&(it->xedd))){
		case XED_ICLASS_BSWAP 		: {break;}
		case XED_ICLASS_CALL_NEAR 	: {
			cisc_decode_special_call(it, &cisc, mem_addr);
			irRenameEngine_increment_call_stack(engine)
			break;
		}
		case XED_ICLASS_CDQ 		: {
			log_warn("no translation for CDQ");
			break;
		}
		case XED_ICLASS_CLD 		: {break;}
		case XED_ICLASS_CMP 		: {break;}
		case XED_ICLASS_DEC 		: {
			cisc_decode_special_dec(it, &cisc, mem_addr);
			break;
		}
		case XED_ICLASS_DIV 		: {
			cisc_decode_special_div(it, &cisc, mem_addr);
			break;
		}
		case XED_ICLASS_EMMS 		: {break;}
		case XED_ICLASS_INC 		: {
			cisc_decode_special_inc(it, &cisc, mem_addr);
			break;
		}
		case XED_ICLASS_JB 			: {break;}
		case XED_ICLASS_JBE 		: {break;}
		case XED_ICLASS_JL  		: {break;}
		case XED_ICLASS_JLE 		: {break;}
		case XED_ICLASS_JMP 		: {break;}
		case XED_ICLASS_JNB 		: {break;}
		case XED_ICLASS_JNBE 		: {break;}
		case XED_ICLASS_JNL 		: {break;}
		case XED_ICLASS_JNLE 		: {break;}
		case XED_ICLASS_JNS 		: {break;}
		case XED_ICLASS_JNZ 		: {break;}
		case XED_ICLASS_JS 			: {break;}
		case XED_ICLASS_JZ 			: {break;}
		case XED_ICLASS_LEAVE 		: {
			cisc_decode_special_leave(it, &cisc, mem_addr);
			break;
		}
		case XED_ICLASS_LFENCE 		: {break;}
		case XED_ICLASS_MOVAPS 		: {
			cisc_decode_generic_simd(it, &cisc, mem_addr, 4);
			break;
		}
		case XED_ICLASS_MOVD 		: {
			cisc_decode_special_movd(assembly, it, &cisc, mem_addr);
			break;
		}
		case XED_ICLASS_MOVDQA 		: {
			cisc_decode_generic_simd(it, &cisc, mem_addr, 4);
			break;
		}
		case XED_ICLASS_MOVDQU 		: {
			cisc_decode_generic_simd(it, &cisc, mem_addr, 4);
			break;
		}
		case XED_ICLASS_MOVQ 		: {
			cisc_decode_special_movq(assembly, it, &cisc, mem_addr);
			break;
		}
		case XED_ICLASS_MOVSD 			: {
			cisc_decode_special_movsd(it, &cisc, mem_addr);
			break;
		}
		case XED_ICLASS_MOVSD_XMM 		: {
			cisc_decode_special_movsd_xmm(it, &cisc, mem_addr);
			break;
		}
		case XED_ICLASS_MOVUPS 		: {
			cisc_decode_generic_simd(it, &cisc, mem_addr, 4);
			break;
		}
		case XED_ICLASS_NOP 		: {break;}
		case XED_ICLASS_PADDD 		: {
			cisc_decode_generic_simd(it, &cisc, mem_addr, simd_get_nb_frag(assembly, it));
			break;
		}
		case XED_ICLASS_PAND 		: {
			cisc_decode_generic_simd(it, &cisc, mem_addr, simd_get_nb_frag(assembly, it));
			break;
		}
		case XED_ICLASS_PINSRW 		: {
			cisc_decode_special_pinsrw(it, &cisc, mem_addr);
			break;
		}
		case XED_ICLASS_POR 		: {
			cisc_decode_generic_simd(it, &cisc, mem_addr, simd_get_nb_frag(assembly, it));
			break;
		}
		case XED_ICLASS_POP 		: {
			cisc_decode_special_pop(it, &cisc, mem_addr);
			break;
		}
		case XED_ICLASS_PSHUFD 		: {
			cisc_decode_special_pshufd(it, &cisc, mem_addr);
			break;
		}
		case XED_ICLASS_PSLLD 		: {
			cisc_decode_special_pslld(assembly, it, &cisc, mem_addr);
			break;
		}
		case XED_ICLASS_PSRLD 		: {
			cisc_decode_special_psrld(assembly, it, &cisc, mem_addr);
			break;
		}
		case XED_ICLASS_PUNPCKLDQ 	: {
			cisc_decode_special_punpckldq(assembly, it, &cisc, mem_addr);
			break;
		}
		case XED_ICLASS_PUSH 		: {
			cisc_decode_special_push(it, &cisc, mem_addr);
			break;
		}
		case XED_ICLASS_PXOR 		: {
			cisc_decode_generic_simd(it, &cisc, mem_addr, simd_get_nb_frag(assembly, it));
			break;
		}
		case XED_ICLASS_RET_FAR 	: {
			cisc_decode_special_ret(assembly, it, &cisc);
			irRenameEngine_decrement_call_stack(engine)
			break;
		}
		case XED_ICLASS_RET_NEAR 	: {
			cisc_decode_special_ret(assembly, it, &cisc);
			irRenameEngine_decrement_call_stack(engine)
			break;
		}
		case XED_ICLASS_SETB 	:
		case XED_ICLASS_SETBE 	:
		case XED_ICLASS_SETL 	:
		case XED_ICLASS_SETLE 	:
		case XED_ICLASS_SETNB 	:
		case XED_ICLASS_SETNBE 	:
		case XED_ICLASS_SETNL 	:
		case XED_ICLASS_SETNLE 	:
		case XED_ICLASS_SETNO 	:
		case XED_ICLASS_SETNP 	:
		case XED_ICLASS_SETNS 	:
		case XED_ICLASS_SETNZ 	:
		case XED_ICLASS_SETO 	:
		case XED_ICLASS_SETP 	:
		case XED_ICLASS_SETS 	:
		case XED_ICLASS_SETZ 	: {
			cisc_decode_special_setxx(it, &cisc, mem_addr);
			break;
		}
		case XED_ICLASS_STOSD 		: {
			cisc_decode_special_stosd(it, &cisc, mem_addr);
			break;
		}
		case XED_ICLASS_TEST 		: {break;}
		case XED_ICLASS_XCHG 		: {
			cisc_decode_special_xchg(it, &cisc, mem_addr);
			break;
		}
		case XED_ICLASS_VPADDD 		: {
			cisc_decode_generic_vex(it, &cisc, mem_addr);
			break;
		}
		case XED_ICLASS_VMOVDQA 	: {
			cisc_decode_generic_vex(it, &cisc, mem_addr);
			break;
		}
		case XED_ICLASS_VMOVDQU 	: {
			cisc_decode_generic_vex(it, &cisc, mem_addr);
			break;
		}
		case XED_ICLASS_VPOR 		: {
			cisc_decode_generic_vex(it, &cisc, mem_addr);
			break;
		}
		case XED_ICLASS_VPXOR 		: {
			cisc_decode_generic_vex(it, &cisc, mem_addr);
			break;
		}
		case XED_ICLASS_VZEROALL 	: {
			cisc_decode_special_vzeroall(it, &cisc);
			break;
		}
		case XED_ICLASS_XORPS 		: {
			cisc_decode_generic_simd(it, &cisc, mem_addr, 4);
			break;
		}
		default :{
			cisc.nb_ins = 1;
			cisc.ins[0].opcode = xedOpcode_2_irOpcode(xed_decoded_inst_get_iclass(&(it->xedd)));
			if (cisc.ins[0].opcode != IR_INVALID){
				ASMCISCINS_SET_VALID(cisc);
				asmOperand_decode(it, cisc.ins[0].input_operand, IRIMPORTERASM_MAX_INPUT_OPERAND, ASM_OPERAND_ROLE_READ_ALL, &(cisc.ins[0].nb_input_operand), mem_addr);
				asmOperand_decode(it, &(cisc.ins[0].output_operand), 1, ASM_OPERAND_ROLE_WRITE_ALL, NULL, mem_addr);
			}
			else{
				log_err_m("unable to convert instruction @ %u", it->instruction_index);
			}
		}
	}

	if (ASMCISCINS_IS_VALID(cisc)){
		for (i = 0; i < cisc.nb_ins; i++){
			switch(cisc.ins[i].opcode){
				case IR_CMOV 	: {
					error_code = asmRisc_process_special_cmov(engine, cisc.ins + i, irRenameEngine_get_call_id(engine));
					break;
				}
				case IR_LEA 	: {
					error_code = asmRisc_process_special_lea(engine, cisc.ins + i, irRenameEngine_get_call_id(engine));
					break;
				}
				case IR_MOV 	: {
					error_code = asmRisc_process_special_mov(engine, cisc.ins + i, irRenameEngine_get_call_id(engine));
					break;
				}
				default 				: {
					error_code = asmRisc_process(engine, cisc.ins + i, irRenameEngine_get_call_id(engine));
				}
			}
			if (error_code){
				log_err_m("error code returned after processing risc %u @ %u", i, it->instruction_index);
			}
		}
	}
}

static void irImporter_handle_irComponent(struct ir* ir, struct irRenameEngine* engine, struct irComponent* ir_component){
	if (ir_concat(ir, ir_component->ir, engine)){
		log_err("unable to concat IR");
	}
}

int32_t irImporterAsm_import(struct ir* ir, const struct assembly* assembly, struct memTrace* mem_trace){
	struct instructionIterator 	it;
	struct irRenameEngine 		engine;

	irRenameEngine_init(&engine, ir);

	if (assembly_get_first_instruction(assembly, &it)){
		log_err("unable to fetch first instruction from the assembly");
		return -1;
	}

	for (; ; ){
		irImporter_handle_instruction(assembly, &engine, &it, irImporterAsm_get_mem_trace(mem_trace, &it));
		if (instructionIterator_get_instruction_index(&it) ==  assembly_get_nb_instruction(assembly) - 1){
			break;
		}
		else{
			if (assembly_get_next_instruction(assembly, &it)){
				log_err("unable to fetch next instruction from the assembly");
				return -1;
			}
		}
	}

	irRenameEngine_tag_final_node(&engine);

	return 0;
}

int32_t irImporterAsm_import_compound(struct ir* ir, const struct assembly* assembly, struct memTrace* mem_trace, struct irComponent** ir_component_buffer, uint32_t nb_ir_component){
	struct instructionIterator 	it;
	struct irRenameEngine 		engine;
	uint32_t 					i;

	irRenameEngine_init(&engine, ir);

	if (assembly_get_first_instruction(assembly, &it)){
		log_err("unable to fetch first instruction from the assembly");
		return -1;
	}

	for (i = 0; ; ){
		if (i < nb_ir_component && instructionIterator_get_instruction_index(&it) == ir_component_buffer[i]->instruction_start){
			irImporter_handle_irComponent(ir, &engine, ir_component_buffer[i]);
			if (ir_component_buffer[i]->instruction_stop == assembly_get_nb_instruction(assembly)){
				break;
			}
			else{
				if (assembly_get_instruction(assembly, &it, ir_component_buffer[i]->instruction_stop)){
					log_err_m("unable to fetch instruction %u from the assembly", ir_component_buffer[i]->instruction_stop);
					return -1;
				}
				i ++;
			}
		}
		else{
			irImporter_handle_instruction(assembly, &engine, &it, irImporterAsm_get_mem_trace(mem_trace, &it));
			if (instructionIterator_get_instruction_index(&it) ==  assembly_get_nb_instruction(assembly) - 1){
				break;
			}
			else{
				if (assembly_get_next_instruction(assembly, &it)){
					log_err("unable to fetch next instruction from the assembly");
					return -1;
				}
			}	
		}
	}

	irRenameEngine_tag_final_node(&engine);

	#ifdef IR_FULL_CHECK
	ir_check_memory(ir);
	#endif

	return 0;
}

static void asmOperand_decode(struct instructionIterator* it, struct asmOperand* operand_buffer, uint8_t max_nb_operand, uint32_t selector, uint8_t* nb_operand_, struct memAddress* mem_addr){
	uint32_t 				i;
	uint8_t 				nb_memops 		= 0;
	const xed_inst_t* 		xi 				= xed_decoded_inst_inst(&(it->xedd));
	const xed_operand_t* 	xed_op;
	xed_operand_enum_t 		op_name;
	uint32_t 				nb_read 		= 0;
	uint32_t 				nb_write 		= 0;
	uint32_t 				nb_operand 		= 0;
	uint32_t 				nb_read_mem 	= 0;
	uint32_t 				nb_write_mem 	= 0;

	for (i = 0; i < xed_inst_noperands(xi); i++){
		xed_op = xed_inst_operand(xi, i);
		op_name = xed_operand_name(xed_op);

		if ((xed_operand_read(xed_op) && ASM_OPERAND_ROLE_IS_READ_INDEX(selector, nb_read)) || (xed_operand_written(xed_op) && ASM_OPERAND_ROLE_IS_WRITE_INDEX(selector, nb_write))){
			switch(op_name){
				case XED_OPERAND_AGEN 	:
				case XED_OPERAND_MEM0 	:
				case XED_OPERAND_MEM1 	: {
					uint64_t disp;
					uint32_t mem_descriptor = MEMADDRESS_DESCRIPTOR_CLEAN;

					if (max_nb_operand == nb_operand){
						log_err_m("the max number of operand has been reached: %u for instruction %s", max_nb_operand, xed_iclass_enum_t2str(xed_decoded_inst_get_iclass(&(it->xedd))));
						goto exit;
					}

					operand_buffer[nb_operand].size 						= xed_decoded_inst_get_memory_operand_length(&(it->xedd), nb_memops) * 8;
					operand_buffer[nb_operand].instruction_index 			= it->instruction_index;
					operand_buffer[nb_operand].variable 					= NULL;
					operand_buffer[nb_operand].type 						= ASM_OPERAND_MEM;
					operand_buffer[nb_operand].operand_type.mem.base 		= xed_decoded_inst_get_base_reg(&(it->xedd), nb_memops);
					operand_buffer[nb_operand].operand_type.mem.index 		= xed_decoded_inst_get_index_reg(&(it->xedd), nb_memops);
					operand_buffer[nb_operand].operand_type.mem.scale 		= xed_decoded_inst_get_scale(&(it->xedd), nb_memops);
					
					disp = xed_decoded_inst_get_memory_displacement(&(it->xedd), nb_memops);
					if (disp){
						switch(xed_decoded_inst_get_memory_displacement_width(&(it->xedd), nb_memops)){
							case 1 : {
								disp |= ~(((disp >> 7) & 0x0000000000000001ULL) + 0xffffffffffffffffULL) & 0xffffffffffffff00ULL;
								break;
							}
							case 2 : {
								disp |= ~(((disp >> 15) & 0x0000000000000001ULL) + 0xffffffffffffffffULL) & 0xffffffffffff0000ULL;
								break;
							}
						}
					}
					operand_buffer[nb_operand].operand_type.mem.disp 		= disp;

					if (xed_operand_read(xed_op)){
						memAddress_descriptor_set_read(mem_descriptor, nb_read_mem);
					}
					else{
						memAddress_descriptor_set_write(mem_descriptor, nb_write_mem);
					}

					operand_buffer[nb_operand].operand_type.mem.con_addr 	= (op_name != XED_OPERAND_AGEN) ? memAddress_get_and_check((mem_addr != NULL) ? (mem_addr + nb_read_mem + nb_write_mem) : NULL, mem_descriptor) : MEMADDRESS_INVALID;

					nb_operand ++;
					break;
				}
				case XED_OPERAND_IMM0 	: {
					if (max_nb_operand == nb_operand){
						log_err_m("the max number of operand has been reached: %u for instruction %s", max_nb_operand, xed_iclass_enum_t2str(xed_decoded_inst_get_iclass(&(it->xedd))));
						goto exit;
					}

					operand_buffer[nb_operand].size 					= xed_decoded_inst_get_immediate_width_bits(&(it->xedd));
					operand_buffer[nb_operand].instruction_index 		= it->instruction_index;
					operand_buffer[nb_operand].variable 				= NULL;
					operand_buffer[nb_operand].type 					= ASM_OPERAND_IMM;
					operand_buffer[nb_operand].operand_type.imm 		= xed_decoded_inst_get_unsigned_immediate(&(it->xedd));

					nb_operand ++;
					break;
				}
				case XED_OPERAND_REG0 	:
				case XED_OPERAND_REG1 	:
				case XED_OPERAND_REG2 	:
				case XED_OPERAND_REG3 	:
				case XED_OPERAND_REG4 	:
				case XED_OPERAND_REG5 	:
				case XED_OPERAND_REG6 	:
				case XED_OPERAND_REG7 	:
				case XED_OPERAND_REG8 	: {
					xed_reg_enum_t reg;

					reg = xed_decoded_inst_get_reg(&(it->xedd), op_name);
					if (reg == XED_REG_EFLAGS){
						break;
					}

					if (max_nb_operand == nb_operand){
						log_err_m("the max number of operand has been reached: %u for instruction %s", max_nb_operand, xed_iclass_enum_t2str(xed_decoded_inst_get_iclass(&(it->xedd))));
						goto exit;
					}
					
					operand_buffer[nb_operand].size 					= irRegister_get_size(xedRegister_2_irRegister(reg));
					operand_buffer[nb_operand].instruction_index 		= it->instruction_index;
					operand_buffer[nb_operand].variable 				= NULL;
					operand_buffer[nb_operand].type 					= ASM_OPERAND_REG;
					operand_buffer[nb_operand].operand_type.reg 		= xedRegister_2_irRegister(reg);

					nb_operand ++;
					break;
				}
				case XED_OPERAND_BASE0 : {
					/* IGNORE: this is ESP for a stack instruction (example: PUSH) */
					break;
				}
				case XED_OPERAND_BASE1 : {
					/* IGNORE: this is ESP for stack instruction (example: PUSH) */
					break;
				}
				default : {
					log_err_m("operand type not supported: %s for instruction %s", xed_operand_enum_t2str(op_name), xed_iclass_enum_t2str(xed_decoded_inst_get_iclass(&(it->xedd))));
					break;
				}
			}
		}

		if (xed_operand_read(xed_op)){
			nb_read ++;
			if (op_name == XED_OPERAND_MEM0 || op_name == XED_OPERAND_MEM1){
				nb_read_mem ++;
			}
		}
		if (xed_operand_written(xed_op)){
			nb_write ++;
			if (op_name == XED_OPERAND_MEM0 || op_name == XED_OPERAND_MEM1){
				nb_write_mem ++;
			}
		}
		if (op_name == XED_OPERAND_AGEN || op_name == XED_OPERAND_MEM0 || op_name == XED_OPERAND_MEM1){
			nb_memops ++;
		}
	}

	exit:
	if (nb_operand == 0){
		log_warn("decode 0 operand");
	}
	if (nb_operand_ != NULL){
		*nb_operand_ = nb_operand;
	}
}

static void asmOperand_decode_simd(struct instructionIterator* it, struct asmOperand* operand_buffer, uint8_t max_nb_operand, uint32_t selector, uint8_t* nb_operand_, struct memAddress* mem_addr){
	uint32_t 				i;
	uint8_t 				nb_memops 		= 0;
	const xed_inst_t* 		xi 				= xed_decoded_inst_inst(&(it->xedd));
	const xed_operand_t* 	xed_op;
	xed_operand_enum_t 		op_name;
	uint32_t 				nb_read 		= 0;
	uint32_t 				nb_write 		= 0;
	uint32_t 				nb_operand 		= 0;
	uint32_t 				nb_read_mem 	= 0;
	uint32_t 				nb_write_mem 	= 0;

	for (i = 0; i < xed_inst_noperands(xi); i++){
		xed_op = xed_inst_operand(xi, i);
		op_name = xed_operand_name(xed_op);

		if ((xed_operand_read(xed_op) && ASM_OPERAND_ROLE_IS_READ_INDEX(selector, nb_read)) || (xed_operand_written(xed_op) && ASM_OPERAND_ROLE_IS_WRITE_INDEX(selector, nb_write))){
			switch(op_name){
				case XED_OPERAND_MEM0 	:
				case XED_OPERAND_MEM1 	: {
					uint64_t disp;
					uint32_t mem_descriptor = MEMADDRESS_DESCRIPTOR_CLEAN;

					if (max_nb_operand == nb_operand){
						log_err_m("the max number of operand has been reached: %u for instruction %s", max_nb_operand, xed_iclass_enum_t2str(xed_decoded_inst_get_iclass(&(it->xedd))));
						goto exit;
					}

					if (xed_decoded_inst_get_memory_operand_length(&(it->xedd), nb_memops) < ASM_OPERAND_ROLE_GET_FRAG(selector) * 4){
						log_err_m("incorrect memory access size for SMID instruction: %u", xed_decoded_inst_get_memory_operand_length(&(it->xedd), nb_memops));
					}

					operand_buffer[nb_operand].size 						= 32;
					operand_buffer[nb_operand].instruction_index 			= it->instruction_index;
					operand_buffer[nb_operand].variable 					= NULL;
					operand_buffer[nb_operand].type 						= ASM_OPERAND_MEM;
					operand_buffer[nb_operand].operand_type.mem.base 		= xed_decoded_inst_get_base_reg(&(it->xedd), nb_memops);
					operand_buffer[nb_operand].operand_type.mem.index 		= xed_decoded_inst_get_index_reg(&(it->xedd), nb_memops);
					operand_buffer[nb_operand].operand_type.mem.scale 		= xed_decoded_inst_get_scale(&(it->xedd), nb_memops);
					
					disp = xed_decoded_inst_get_memory_displacement(&(it->xedd), nb_memops);
					if (disp){
						switch(xed_decoded_inst_get_memory_displacement_width(&(it->xedd), nb_memops)){
							case 1 : {
								disp |= ~(((disp >> 7) & 0x0000000000000001ULL) + 0xffffffffffffffffULL) & 0xffffffffffffff00ULL;
								break;
							}
							case 2 : {
								disp |= ~(((disp >> 15) & 0x0000000000000001ULL) + 0xffffffffffffffffULL) & 0xffffffffffff0000ULL;
								break;
							}
						}
					}
					operand_buffer[nb_operand].operand_type.mem.disp 		= disp + ASM_OPERAND_ROLE_GET_FRAG(selector) * 4;

					if (xed_operand_read(xed_op)){
						memAddress_descriptor_set_read(mem_descriptor, nb_read_mem);
					}
					else{
						memAddress_descriptor_set_write(mem_descriptor, nb_write_mem);
					}

					operand_buffer[nb_operand].operand_type.mem.con_addr 	= memAddress_get_and_check((mem_addr != NULL) ? (mem_addr + nb_read_mem + nb_write_mem) : NULL, mem_descriptor);
					if (operand_buffer[nb_operand].operand_type.mem.con_addr != MEMADDRESS_INVALID){
						operand_buffer[nb_operand].operand_type.mem.con_addr += ASM_OPERAND_ROLE_GET_FRAG(selector) * 4;
					}

					nb_operand ++;
					break;
				}
				case XED_OPERAND_IMM0 	: {
					log_err("immediate operands are not supported yet");
					goto exit;
				}
				case XED_OPERAND_REG0 	:
				case XED_OPERAND_REG1 	:
				case XED_OPERAND_REG2 	:
				case XED_OPERAND_REG3 	:
				case XED_OPERAND_REG4 	:
				case XED_OPERAND_REG5 	:
				case XED_OPERAND_REG6 	:
				case XED_OPERAND_REG7 	:
				case XED_OPERAND_REG8 	: {
					xed_reg_enum_t 	reg;
					int32_t 		simd_base;

					reg = xed_decoded_inst_get_reg(&(it->xedd), op_name);
					if (reg == XED_REG_EFLAGS){
						break;
					}

					if (max_nb_operand == nb_operand){
						log_err_m("the max number of operand has been reached: %u for instruction %s", max_nb_operand, xed_iclass_enum_t2str(xed_decoded_inst_get_iclass(&(it->xedd))));
						goto exit;
					}

					if ((simd_base = xed_reg_get_simd_base(reg)) == -1){
						log_err_m("register %s is not an SIMD register", xed_reg_enum_t2str(reg));
						goto exit;
					}
					
					operand_buffer[nb_operand].size 					= 32;
					operand_buffer[nb_operand].instruction_index 		= it->instruction_index;
					operand_buffer[nb_operand].variable 				= NULL;
					operand_buffer[nb_operand].type 					= ASM_OPERAND_REG;
					operand_buffer[nb_operand].operand_type.reg 		= ir_register_simd_frag[simd_base][ASM_OPERAND_ROLE_GET_FRAG(selector)];

					nb_operand ++;
					break;
				}
				default : {
					log_err_m("operand type not supported: %s for instruction %s", xed_operand_enum_t2str(op_name), xed_iclass_enum_t2str(xed_decoded_inst_get_iclass(&(it->xedd))));
					break;
				}
			}
		}

		if (xed_operand_read(xed_op)){
			nb_read ++;
			if (op_name == XED_OPERAND_MEM0 || op_name == XED_OPERAND_MEM1){
				nb_read_mem ++;
			}
		}
		if (xed_operand_written(xed_op)){
			nb_write ++;
			if (op_name == XED_OPERAND_MEM0 || op_name == XED_OPERAND_MEM1){
				nb_write_mem ++;
			}
		}
		if (op_name == XED_OPERAND_AGEN || op_name == XED_OPERAND_MEM0 || op_name == XED_OPERAND_MEM1){
			nb_memops ++;
		}
	}

	exit:
	if (nb_operand == 0){
		log_warn("decode 0 operand");
	}
	if (nb_operand_ != NULL){
		*nb_operand_ = nb_operand;
	}
}

static void asmOperand_fetch_input(struct irRenameEngine* engine, struct asmOperand* operand, uint32_t dst){
	switch(operand->type){
		case ASM_OPERAND_IMM : {
			operand->variable = ir_add_immediate(engine->ir, operand->size, operand->operand_type.imm);
			if (operand->variable == NULL){
				log_err("unable to add immediate to IR");
			}
			break;
		}
		case ASM_OPERAND_REG : {
			operand->variable = irRenameEngine_get_register_ref(engine, operand->operand_type.reg, operand->instruction_index, dst);
			if (operand->variable == NULL){
				log_err("unable to register reference from the renaming engine");
			}
			break;
		}
		case ASM_OPERAND_MEM : {
			struct node* address;

			address = memOperand_build_address(engine, &(operand->operand_type.mem), operand->instruction_index, dst);
			if (address != NULL){
				operand->variable = ir_add_in_mem_(engine->ir, operand->instruction_index, operand->size, address, irRenameEngine_get_mem_order(engine), operand->operand_type.mem.con_addr);
				if (operand->variable == NULL){
					log_err("unable to add memory load to IR");
				}
				else{
					irRenameEngine_set_mem_order(engine, operand->variable);
				}
			}
			else{
				log_err("unable to build memory address");
			}
			break;
		}
	}
}

static void asmOperand_fetch_output(struct irRenameEngine* engine, struct asmOperand* operand, enum irOpcode opcode, uint32_t dst){
	switch(operand->type){
		case ASM_OPERAND_REG 	: {
			operand->variable = ir_add_inst(engine->ir, operand->instruction_index, operand->size, opcode, dst);
			if (operand->variable == NULL){
				log_err("unable to add operation to IR");
			}
			else{
				irRenameEngine_set_register_ref(engine, operand->operand_type.reg, operand->variable);
			}
			break;
		}
		case ASM_OPERAND_MEM 	: {
			struct node* address;
			struct node* mem_write;

			address = memOperand_build_address(engine, &(operand->operand_type.mem), operand->instruction_index, dst);
			if (address != NULL){
				operand->variable = ir_add_inst(engine->ir, operand->instruction_index, operand->size, opcode, dst);
				if (operand->variable != NULL){
					mem_write = ir_add_out_mem_(engine->ir, operand->instruction_index, operand->size, address, irRenameEngine_get_mem_order(engine), operand->operand_type.mem.con_addr);
					if (mem_write != NULL){
						irRenameEngine_set_mem_order(engine, mem_write);
						if (ir_add_dependence(engine->ir, operand->variable, mem_write, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
							log_err("unable to add output to add dependence to IR");
						}
					}
					else{
						log_err("unable to add memory write to IR");
					}
				}
				else{
					log_err("unable to add operation to IR");
				}
			}
			else{
				log_err("unable to build memory address");
			}
			break;
		}
		default 				: {
			log_err("this case is not supposed to happen (IMM read operand)");
		}
	}
}

static void asmOperand_sign_extend(struct asmOperand* operand, uint16_t size){
	if (operand->type == ASM_OPERAND_IMM && operand->size < size){
		switch(operand->size){
			case 8 : {
				operand->operand_type.imm |= ~(((operand->operand_type.imm >> 7) & 0x0000000000000001ULL) + 0xffffffffffffffffULL) & 0xffffffffffffff00ULL;
				break;
			}
			case 16 : {
				operand->operand_type.imm |= ~(((operand->operand_type.imm >> 15) & 0x0000000000000001ULL) + 0xffffffffffffffffULL) & 0xffffffffffff0000ULL;
				break;
			}
		}
		operand->size = size;
	}
}

static struct node* memOperand_build_address(struct irRenameEngine* engine, struct memOperand* mem, uint32_t instruction_index, uint32_t dst){
	struct node* 	base 		= NULL;
	struct node* 	index 		= NULL;
	struct node* 	disp 		= NULL;
	struct node* 	scale 		= NULL;
	uint8_t 		nb_operand 	= 0;
	struct node* 	operands[3];
	struct node* 	address 	= NULL;

	if (mem->base != XED_REG_INVALID){
		base = irRenameEngine_get_register_ref(engine, xedRegister_2_irRegister(mem->base), instruction_index, dst);
		if (base == NULL){
			log_err("unable to get register reference from the renaming engine");
		}
		else{
			operands[nb_operand] = base;
			nb_operand ++;
		}
	}

	if (mem->scale > 1){
		scale = ir_add_immediate(engine->ir, 8, __builtin_ffs(mem->scale) - 1);
		if (scale == NULL){
			log_err("unable to add immediate to IR");
		}
	}

	if (mem->index != XED_REG_INVALID){
		index = irRenameEngine_get_register_ref(engine, xedRegister_2_irRegister(mem->index), instruction_index, dst);
		if (index == NULL){
			log_err("unable to get register reference from the renaming engine");
		}
		else{
			if (scale == NULL){
				operands[nb_operand] = index;
				nb_operand ++;
			}
			else{
				struct node* shl;

				shl = ir_add_inst(engine->ir, IR_OPERATION_INDEX_ADDRESS, 32, IR_SHL, dst);
				if (shl != NULL){
					if (ir_add_dependence(engine->ir, index, shl, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
						log_err("unable to add dependence between IR nodes");
					}
					if (ir_add_dependence(engine->ir, scale, shl, IR_DEPENDENCE_TYPE_SHIFT_DISP) == NULL){
						log_err("unable to add dependence between IR nodes");
					}

					operands[nb_operand] = shl;
					nb_operand ++;
				}
				else{
					log_err("unable to add operation to IR");
				}
			}
		}
	}

	if (mem->disp){
		disp = ir_add_immediate(engine->ir, 32, mem->disp);
		if (disp == NULL){
			log_err("unable to add immediate to IR");
		}
		else{
			operands[nb_operand] = disp;
			nb_operand ++;
		}
	}

	switch(nb_operand){
		case 1 : {
			address = operands[0];
			break;
		}
		case 2 : {
			address = ir_add_inst(engine->ir, IR_OPERATION_INDEX_ADDRESS, 32, IR_ADD, dst);
			if (address != NULL){
				if (ir_add_dependence(engine->ir, operands[0], address, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
					log_err("unable to add dependence between IR nodes");
				}
				if (ir_add_dependence(engine->ir, operands[1], address, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
					log_err("unable to add dependence between IR nodes");
				}
			}
			else{
				log_err("unable to add operation to IR");
			}
			break;
		}
		case 3 : {
			address = ir_add_inst(engine->ir, IR_OPERATION_INDEX_ADDRESS, 32, IR_ADD, dst);
			if (address != NULL){
				if (ir_add_dependence(engine->ir, operands[0], address, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
					log_err("unable to add dependence between IR nodes");
				}
				if (ir_add_dependence(engine->ir, operands[1], address, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
					log_err("unable to add dependence between IR nodes");
				}
				if (ir_add_dependence(engine->ir, operands[2], address, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
					log_err("unable to add dependence between IR nodes");
				}
			}
			else{
				log_err("unable to add operation to IR");
			}
			break;
		}
		default : {
			log_err_m("incorrect number of operand(s): %u", nb_operand);
		}
	}

	return address;
}

static int32_t asmRisc_process(struct irRenameEngine* engine, struct asmRiscIns* risc, uint32_t dst){
	uint8_t i;
	int32_t error_code = 0;

	if (sign_extand_table[risc->opcode]){
		for (i = 0; i < risc->nb_input_operand; i++){
			asmOperand_sign_extend(risc->input_operand + i, risc->output_operand.size);
		}
	}

	for (i = 0; i < risc->nb_input_operand; i++){
		asmOperand_fetch_input(engine, risc->input_operand + i, dst);
	}
	asmOperand_fetch_output(engine, &(risc->output_operand), risc->opcode, dst);

	if (risc->output_operand.variable != NULL){
		for (i = 0; i < risc->nb_input_operand; i++){
			if (risc->input_operand[i].variable != NULL){
				if (ir_add_dependence(engine->ir, risc->input_operand[i].variable, risc->output_operand.variable, dependence_label_table[risc->opcode][i]) == NULL){
					log_err("unable to add output to add dependence to IR");
					error_code = -1;
				}
			}
		}
	}

	return error_code;
}

static int32_t asmRisc_process_special_cmov(struct irRenameEngine* engine, struct asmRiscIns* risc, uint32_t dst){
	int32_t error_code = 0;

	log_warn("using CMOV glue instruction since runtime behavior is unknown");

	if (risc->nb_input_operand == 1){
		memcpy(risc->input_operand + 1, &(risc->output_operand), sizeof(struct asmOperand));
		risc->nb_input_operand ++;
	}
	if (risc->nb_input_operand != 2){
		log_err("incorrect number of input operand");
		return -1;
	}

	asmOperand_fetch_input(engine, risc->input_operand + 0, dst);
	asmOperand_fetch_input(engine, risc->input_operand + 1, dst);

	asmOperand_fetch_output(engine, &(risc->output_operand), risc->opcode, dst);

	if (risc->output_operand.variable == NULL){
		log_err("unable to fetch output operand");
		return -1;
	}

	if (risc->input_operand[0].variable != NULL){
		if (ir_add_dependence(engine->ir, risc->input_operand[0].variable, risc->output_operand.variable, dependence_label_table[risc->opcode][0]) == NULL){
			log_err("unable to add output to add dependence to IR");
			error_code = -1;
		}
	}
	else{
		log_err("unable to fetch first input operand");
		error_code = -1;
	}

	if (risc->input_operand[1].variable != NULL){
		if (ir_add_dependence(engine->ir, risc->input_operand[1].variable, risc->output_operand.variable, dependence_label_table[risc->opcode][1]) == NULL){
			log_err("unable to add output to add dependence to IR");
			error_code = -1;
		}
	}
	else{
		log_err("unable to fetch second input operand");
		error_code = -1;
	}

	return error_code;
}

static int32_t asmRisc_process_special_lea(struct irRenameEngine* engine, struct asmRiscIns* risc, uint32_t dst){
	struct node* 	address;
	struct node* 	mem_write;
	int32_t 		error_code = 0;

	if (risc->nb_input_operand != 1 || risc->input_operand[0].type != ASM_OPERAND_MEM){
		log_err("incorrect type or number of input operand");
		return -1;
	}

	risc->input_operand[0].variable = memOperand_build_address(engine, &(risc->input_operand[0].operand_type.mem), risc->input_operand[0].instruction_index, dst);
	if (risc->input_operand[0].variable == NULL){
		log_err("unable to build memory address");
		return -1;
	}

	switch(risc->output_operand.type){
		case ASM_OPERAND_REG 	: {
			irRenameEngine_set_register_ref(engine, risc->output_operand.operand_type.reg, risc->input_operand[0].variable);
			break;
		}
		case ASM_OPERAND_MEM 	: {
			address = memOperand_build_address(engine, &(risc->output_operand.operand_type.mem), risc->output_operand.instruction_index, dst);
			if (address != NULL){
				mem_write = ir_add_out_mem_(engine->ir, risc->output_operand.instruction_index, risc->output_operand.size, address, irRenameEngine_get_mem_order(engine), risc->output_operand.operand_type.mem.con_addr);
				if (mem_write != NULL){
					irRenameEngine_set_mem_order(engine, mem_write);
					if (ir_add_dependence(engine->ir, risc->input_operand[0].variable, mem_write, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
						log_err("unable to add dependence to IR");
						error_code = -1;
					}
				}
				else{
					log_err("unable to add memory write to IR");
					error_code = -1;
				}
			}
			else{
				log_err("unable to build memory address");
				error_code = -1;
			}
			break;
		}
		default 				: {
			log_err("this case is not supposed to happen");
			error_code = -1;
		}
	}

	return error_code;
}

static int32_t asmRisc_process_special_mov(struct irRenameEngine* engine, struct asmRiscIns* risc, uint32_t dst){
	struct node* 	address;
	struct node* 	mem_write;
	int32_t 		error_code = 0;

	if (risc->nb_input_operand != 1){
		log_err("incorrect number of input operand");
		return -1;
	}

	asmOperand_fetch_input(engine, risc->input_operand, dst);
	if (risc->input_operand[0].variable == NULL){
		log_err("input variable is NULL");
		return -1;
	}

	switch(risc->output_operand.type){
		case ASM_OPERAND_REG 	: {
			irRenameEngine_set_register_ref(engine, risc->output_operand.operand_type.reg, risc->input_operand[0].variable);
			break;
		}
		case ASM_OPERAND_MEM 	: {
			address = memOperand_build_address(engine, &(risc->output_operand.operand_type.mem), risc->output_operand.instruction_index, dst);
			if (address != NULL){
				mem_write = ir_add_out_mem_(engine->ir, risc->output_operand.instruction_index, risc->output_operand.size, address, irRenameEngine_get_mem_order(engine), risc->output_operand.operand_type.mem.con_addr);
				if (mem_write != NULL){
					irRenameEngine_set_mem_order(engine, mem_write);
					if (ir_add_dependence(engine->ir, risc->input_operand[0].variable, mem_write, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
						log_err("unable to add dependence to IR");
						error_code = -1;
					}
				}
				else{
					log_err("unable to add memory write to IR");
					error_code = -1;
				}
			}
			else{
				log_err("unable to build memory address");
				error_code = -1;
			}
			break;
		}
		default 				: {
			log_err("this case is not supposed to happen");
			error_code = -1;
		}
	}

	return error_code;
}

static enum irOpcode xedOpcode_2_irOpcode(xed_iclass_enum_t xed_opcode){
	switch (xed_opcode){
		case XED_ICLASS_ADC 		: {return IR_ADC;}
		case XED_ICLASS_ADD 		: {return IR_ADD;}
		case XED_ICLASS_AND 		: {return IR_AND;}
		case XED_ICLASS_CMOVB 		: {return IR_CMOV;}
		case XED_ICLASS_CMOVBE 		: {return IR_CMOV;}
		case XED_ICLASS_CMOVL 		: {return IR_CMOV;}
		case XED_ICLASS_CMOVLE 		: {return IR_CMOV;}
		case XED_ICLASS_CMOVNB 		: {return IR_CMOV;}
		case XED_ICLASS_CMOVNBE 	: {return IR_CMOV;}
		case XED_ICLASS_CMOVNL 		: {return IR_CMOV;}
		case XED_ICLASS_CMOVNLE 	: {return IR_CMOV;}
		case XED_ICLASS_CMOVNO 		: {return IR_CMOV;}
		case XED_ICLASS_CMOVNP 		: {return IR_CMOV;}
		case XED_ICLASS_CMOVNS 		: {return IR_CMOV;}
		case XED_ICLASS_CMOVNZ 		: {return IR_CMOV;}
		case XED_ICLASS_CMOVO 		: {return IR_CMOV;}
		case XED_ICLASS_CMOVP 		: {return IR_CMOV;}
		case XED_ICLASS_CMOVS 		: {return IR_CMOV;}
		case XED_ICLASS_CMOVZ 		: {return IR_CMOV;}
		case XED_ICLASS_IDIV		: {return IR_IDIV;}
		case XED_ICLASS_IMUL 		: {return IR_IMUL;}
		case XED_ICLASS_LEA 		: {return IR_LEA;}
		case XED_ICLASS_MOV 		: {return IR_MOV;}
		case XED_ICLASS_MOVAPS 		: {return IR_MOV;}
		case XED_ICLASS_MOVDQA 		: {return IR_MOV;}
		case XED_ICLASS_MOVDQU 		: {return IR_MOV;}
		case XED_ICLASS_MOVQ 		: {return IR_MOV;}
		case XED_ICLASS_MOVUPS 		: {return IR_MOV;}
		case XED_ICLASS_MOVZX 		: {return IR_MOVZX;}
		case XED_ICLASS_MUL 		: {return IR_MUL;}
		case XED_ICLASS_NEG 		: {return IR_NEG;}
		case XED_ICLASS_NOT 		: {return IR_NOT;}
		case XED_ICLASS_OR 			: {return IR_OR;}
		case XED_ICLASS_PADDD 		: {return IR_ADD;}
		case XED_ICLASS_PAND 		: {return IR_AND;}
		case XED_ICLASS_POR 		: {return IR_OR;}
		case XED_ICLASS_PXOR 		: {return IR_XOR;}
		case XED_ICLASS_ROL 		: {return IR_ROL;}
		case XED_ICLASS_ROR 		: {return IR_ROR;}
		case XED_ICLASS_SHL 		: {return IR_SHL;}
		case XED_ICLASS_SHLD 		: {return IR_SHLD;}
		case XED_ICLASS_SHR 		: {return IR_SHR;}
		case XED_ICLASS_SHRD 		: {return IR_SHRD;}
		case XED_ICLASS_SUB 		: {return IR_SUB;}
		case XED_ICLASS_VPADDD 		: {return IR_ADD;}
		case XED_ICLASS_VMOVDQA 	: {return IR_MOV;}
		case XED_ICLASS_VMOVDQU 	: {return IR_MOV;}
		case XED_ICLASS_VPOR 		: {return IR_OR;}
		case XED_ICLASS_VPXOR 		: {return IR_XOR;}
		case XED_ICLASS_XOR 		: {return IR_XOR;}
		case XED_ICLASS_XORPS 		: {return IR_XOR;}
		default : {
			log_err_m("this instruction (%s) cannot be translated into IR Opcode", xed_iclass_enum_t2str(xed_opcode));
			return IR_INVALID;
		}
	}
}

static enum irRegister xedRegister_2_irRegister(xed_reg_enum_t xed_reg){
	switch(xed_reg){
		case XED_REG_AL 	: {return IR_REG_AL;}
		case XED_REG_CL 	: {return IR_REG_CL;}
		case XED_REG_DL 	: {return IR_REG_DL;}
		case XED_REG_BL 	: {return IR_REG_BL;}
		case XED_REG_AH 	: {return IR_REG_AH;}
		case XED_REG_CH 	: {return IR_REG_CH;}
		case XED_REG_DH 	: {return IR_REG_DH;}
		case XED_REG_BH 	: {return IR_REG_BH;}
		case XED_REG_AX 	: {return IR_REG_AX;}
		case XED_REG_CX 	: {return IR_REG_CX;}
		case XED_REG_DX 	: {return IR_REG_DX;}
		case XED_REG_BX 	: {return IR_REG_BX;}
		case XED_REG_SP 	: {return IR_REG_SP;}
		case XED_REG_BP 	: {return IR_REG_BP;}
		case XED_REG_SI 	: {return IR_REG_SI;}
		case XED_REG_DI 	: {return IR_REG_DI;}
		case XED_REG_EAX 	: {return IR_REG_EAX;}
		case XED_REG_ECX 	: {return IR_REG_ECX;}
		case XED_REG_EDX 	: {return IR_REG_EDX;}
		case XED_REG_EBX 	: {return IR_REG_EBX;}
		case XED_REG_ESP 	: {return IR_REG_ESP;}
		case XED_REG_EBP 	: {return IR_REG_EBP;}
		case XED_REG_ESI 	: {return IR_REG_ESI;}
		case XED_REG_EDI 	: {return IR_REG_EDI;}
		default : {
			log_err_m("this register (%s) cannot be translated into IR register", xed_reg_enum_t2str(xed_reg));
			return IR_REG_EAX;
		}
	}
}

#define asmRisc_set_reg_cst(risc, reg_, size_, imm_) 										\
	{ 																						\
		(risc)->opcode 										= IR_MOV; 						\
		(risc)->nb_input_operand 							= 1; 							\
		(risc)->input_operand[0].size 						= (size_); 						\
		(risc)->input_operand[0].instruction_index 			= it->instruction_index; 		\
		(risc)->input_operand[0].variable 					= NULL; 						\
		(risc)->input_operand[0].type 						= ASM_OPERAND_IMM; 				\
		(risc)->input_operand[0].operand_type.imm 			= (imm_); 						\
		(risc)->output_operand.size 						= (size_); 						\
		(risc)->output_operand.instruction_index 			= it->instruction_index; 		\
		(risc)->output_operand.variable 					= NULL; 						\
		(risc)->output_operand.type 						= ASM_OPERAND_REG; 				\
		(risc)->output_operand.operand_type.reg 			= (reg_); 						\
	}

#define asmRisc_set_mem_cst(risc, base_, index_, scale_, disp_, con_addr_, size_, imm_) 	\
	{ 																						\
		(risc)->opcode 										= IR_MOV; 						\
		(risc)->nb_input_operand 							= 1; 							\
		(risc)->input_operand[0].size 						= (size_); 						\
		(risc)->input_operand[0].instruction_index 			= it->instruction_index; 		\
		(risc)->input_operand[0].variable 					= NULL; 						\
		(risc)->input_operand[0].type 						= ASM_OPERAND_IMM; 				\
		(risc)->input_operand[0].operand_type.imm 			= (imm_); 						\
		(risc)->output_operand.size 						= (size_); 						\
		(risc)->output_operand.instruction_index 			= it->instruction_index; 		\
		(risc)->output_operand.variable 					= NULL; 						\
		(risc)->output_operand.type 						= ASM_OPERAND_MEM; 				\
		(risc)->output_operand.operand_type.mem.base 		= (base_); 						\
		(risc)->output_operand.operand_type.mem.index 		= (index_); 					\
		(risc)->output_operand.operand_type.mem.scale 		= (scale_); 					\
		(risc)->output_operand.operand_type.mem.disp 		= (disp_); 						\
		(risc)->output_operand.operand_type.mem.con_addr 	= (con_addr_); 					\
	}

static void cisc_decode_special_call(struct instructionIterator* it, struct asmCiscIns* cisc, struct memAddress* mem_addr){
	cisc->valid 											= 1;
	cisc->nb_ins 											= 2;

	cisc->ins[0].opcode 									= IR_SUB;
	cisc->ins[0].nb_input_operand 							= 2;
	cisc->ins[0].input_operand[0].size 						= 32;
	cisc->ins[0].input_operand[0].instruction_index 		= it->instruction_index;
	cisc->ins[0].input_operand[0].variable 					= NULL;
	cisc->ins[0].input_operand[0].type 						= ASM_OPERAND_REG;
	cisc->ins[0].input_operand[0].operand_type.reg 			= IR_REG_ESP;
	cisc->ins[0].input_operand[1].size 						= 32;
	cisc->ins[0].input_operand[1].instruction_index 		= it->instruction_index;
	cisc->ins[0].input_operand[1].variable 					= NULL;
	cisc->ins[0].input_operand[1].type 						= ASM_OPERAND_IMM;
	cisc->ins[0].input_operand[1].operand_type.imm 			= 4;
	cisc->ins[0].output_operand.size 						= 32;
	cisc->ins[0].output_operand.instruction_index 			= it->instruction_index;
	cisc->ins[0].output_operand.variable 					= NULL;
	cisc->ins[0].output_operand.type 						= ASM_OPERAND_REG;
	cisc->ins[0].output_operand.operand_type.reg 			= IR_REG_ESP;

	cisc->ins[1].opcode 									= IR_MOV;
	cisc->ins[1].nb_input_operand 							= 1;
	cisc->ins[1].input_operand[0].size 						= 32;
	cisc->ins[1].input_operand[0].instruction_index 		= it->instruction_index;
	cisc->ins[1].input_operand[0].variable 					= NULL;
	cisc->ins[1].input_operand[0].type 						= ASM_OPERAND_IMM;
	cisc->ins[1].input_operand[0].operand_type.imm 			= it->instruction_address;
	cisc->ins[1].output_operand.size 						= 32;
	cisc->ins[1].output_operand.instruction_index 			= it->instruction_index;
	cisc->ins[1].output_operand.variable 					= NULL;
	cisc->ins[1].output_operand.type 						= ASM_OPERAND_MEM;
	cisc->ins[1].output_operand.operand_type.mem.base 		= XED_REG_ESP;
	cisc->ins[1].output_operand.operand_type.mem.index 		= XED_REG_INVALID;
	cisc->ins[1].output_operand.operand_type.mem.scale 		= 1;
	cisc->ins[1].output_operand.operand_type.mem.disp 		= 0;
	cisc->ins[1].output_operand.operand_type.mem.con_addr 	= memAddress_search_and_get(mem_addr, MEMADDRESS_DESCRIPTOR_WRITE_0, 2);

}

static void cisc_decode_special_dec(struct instructionIterator* it, struct asmCiscIns* cisc, struct memAddress* mem_addr){
	cisc->valid 											= 1;
	cisc->nb_ins 											= 1;

	cisc->ins[0].opcode 									= IR_SUB;
	cisc->ins[0].nb_input_operand 							= 2;
	asmOperand_decode(it, cisc->ins[0].input_operand, 1, ASM_OPERAND_ROLE_READ_1, NULL, mem_addr);
	cisc->ins[0].input_operand[1].size 						= cisc->ins[0].input_operand[0].size;
	cisc->ins[0].input_operand[1].instruction_index 		= it->instruction_index;
	cisc->ins[0].input_operand[1].variable 					= NULL;
	cisc->ins[0].input_operand[1].type 						= ASM_OPERAND_IMM;
	cisc->ins[0].input_operand[1].operand_type.imm 			= 1;
	asmOperand_decode(it, &(cisc->ins[0].output_operand), 1, ASM_OPERAND_ROLE_WRITE_1, NULL, mem_addr);
}

static void cisc_decode_special_div(struct instructionIterator* it, struct asmCiscIns* cisc, struct memAddress* mem_addr){
	cisc->valid 											= 1;
	cisc->nb_ins 											= 3;

	cisc->ins[0].opcode 									= IR_DIVQ;
	asmOperand_decode(it, cisc->ins[0].input_operand, IRIMPORTERASM_MAX_INPUT_OPERAND, ASM_OPERAND_ROLE_READ_ALL, &(cisc->ins[0].nb_input_operand), mem_addr);
	asmOperand_decode(it, &(cisc->ins[0].output_operand), 1, ASM_OPERAND_ROLE_WRITE_1, NULL, mem_addr);
	cisc->ins[0].output_operand.type 						= ASM_OPERAND_REG;
	cisc->ins[0].output_operand.operand_type.reg 			= IR_REG_TMP;

	cisc->ins[1].opcode 									= IR_DIVR;
	asmOperand_decode(it, cisc->ins[1].input_operand, IRIMPORTERASM_MAX_INPUT_OPERAND, ASM_OPERAND_ROLE_READ_ALL, &(cisc->ins[1].nb_input_operand), mem_addr);
	asmOperand_decode(it, &(cisc->ins[1].output_operand), 1, ASM_OPERAND_ROLE_WRITE_2, NULL, mem_addr);

	cisc->ins[2].opcode 									= IR_MOV;
	cisc->ins[2].nb_input_operand 							= 1;
	cisc->ins[2].input_operand[0].size 						= cisc->ins[0].output_operand.size;
	cisc->ins[2].input_operand[0].instruction_index 		= it->instruction_index;
	cisc->ins[2].input_operand[0].variable 					= NULL;
	cisc->ins[2].input_operand[0].type 						= ASM_OPERAND_REG;
	cisc->ins[2].input_operand[0].operand_type.reg 			= IR_REG_TMP;
	asmOperand_decode(it, &(cisc->ins[2].output_operand), 1, ASM_OPERAND_ROLE_WRITE_1, NULL, mem_addr);
}

static void cisc_decode_special_inc(struct instructionIterator* it, struct asmCiscIns* cisc, struct memAddress* mem_addr){
	cisc->valid 											= 1;
	cisc->nb_ins 											= 1;

	cisc->ins[0].opcode 									= IR_ADD;
	cisc->ins[0].nb_input_operand 							= 2;
	asmOperand_decode(it, cisc->ins[0].input_operand, 1, ASM_OPERAND_ROLE_READ_1, NULL, mem_addr);
	cisc->ins[0].input_operand[1].size 						= cisc->ins[0].input_operand[0].size;
	cisc->ins[0].input_operand[1].instruction_index 		= it->instruction_index;
	cisc->ins[0].input_operand[1].variable 					= NULL;
	cisc->ins[0].input_operand[1].type 						= ASM_OPERAND_IMM;
	cisc->ins[0].input_operand[1].operand_type.imm 			= 1;
	asmOperand_decode(it, &(cisc->ins[0].output_operand), 1, ASM_OPERAND_ROLE_WRITE_1, NULL, mem_addr);
}

static void cisc_decode_special_leave(struct instructionIterator* it, struct asmCiscIns* cisc, struct memAddress* mem_addr){
	cisc->valid 											= 1;
	cisc->nb_ins 											= 3;

	cisc->ins[0].opcode 									= IR_MOV;
	cisc->ins[0].nb_input_operand 							= 1;
	cisc->ins[0].input_operand[0].size 						= 32;
	cisc->ins[0].input_operand[0].instruction_index 		= it->instruction_index;
	cisc->ins[0].input_operand[0].variable 					= NULL;
	cisc->ins[0].input_operand[0].type 						= ASM_OPERAND_REG;
	cisc->ins[0].input_operand[0].operand_type.reg 			= IR_REG_EBP;
	cisc->ins[0].output_operand.size 						= 32;
	cisc->ins[0].output_operand.instruction_index 			= it->instruction_index;
	cisc->ins[0].output_operand.variable 					= NULL;
	cisc->ins[0].output_operand.type 						= ASM_OPERAND_REG;
	cisc->ins[0].output_operand.operand_type.reg 			= IR_REG_ESP;

	cisc->ins[1].opcode 									= IR_MOV;
	cisc->ins[1].nb_input_operand 							= 1;
	cisc->ins[1].input_operand[0].size 						= 32;
	cisc->ins[1].input_operand[0].instruction_index 		= it->instruction_index;
	cisc->ins[1].input_operand[0].variable 					= NULL;
	cisc->ins[1].input_operand[0].type 						= ASM_OPERAND_MEM;
	cisc->ins[1].input_operand[0].operand_type.mem.base 	= XED_REG_ESP;
	cisc->ins[1].input_operand[0].operand_type.mem.index 	= XED_REG_INVALID;
	cisc->ins[1].input_operand[0].operand_type.mem.scale 	= 1;
	cisc->ins[1].input_operand[0].operand_type.mem.disp 	= 0;
	cisc->ins[1].input_operand[0].operand_type.mem.con_addr = memAddress_get_and_check(mem_addr, MEMADDRESS_DESCRIPTOR_READ_0);
	cisc->ins[1].output_operand.size 						= 32;
	cisc->ins[1].output_operand.instruction_index 			= it->instruction_index;
	cisc->ins[1].output_operand.variable 					= NULL;
	cisc->ins[1].output_operand.type 						= ASM_OPERAND_REG;
	cisc->ins[1].output_operand.operand_type.reg 			= IR_REG_EBP;

	cisc->ins[2].opcode 									= IR_ADD;
	cisc->ins[2].nb_input_operand 							= 2;
	cisc->ins[2].input_operand[0].size 						= 32;
	cisc->ins[2].input_operand[0].instruction_index 		= it->instruction_index;
	cisc->ins[2].input_operand[0].variable 					= NULL;
	cisc->ins[2].input_operand[0].type 						= ASM_OPERAND_REG;
	cisc->ins[2].input_operand[0].operand_type.reg 			= IR_REG_ESP;
	cisc->ins[2].input_operand[1].size 						= 32;
	cisc->ins[2].input_operand[1].instruction_index 		= it->instruction_index;
	cisc->ins[2].input_operand[1].variable 					= NULL;
	cisc->ins[2].input_operand[1].type 						= ASM_OPERAND_IMM;
	cisc->ins[2].input_operand[1].operand_type.imm 			= 4;
	cisc->ins[2].output_operand.size 						= 32;
	cisc->ins[2].output_operand.instruction_index 			= it->instruction_index;
	cisc->ins[2].output_operand.variable 					= NULL;
	cisc->ins[2].output_operand.type 						= ASM_OPERAND_REG;
	cisc->ins[2].output_operand.operand_type.reg 			= IR_REG_ESP;
}

static void cisc_decode_special_movsd(struct instructionIterator* it, struct asmCiscIns* cisc, struct memAddress* mem_addr){
	log_warn("unknown value for the direction flag");

	cisc->valid 											= 1;
	cisc->nb_ins 											= 4;

	cisc->ins[0].opcode 									= IR_MOV;
	cisc->ins[0].nb_input_operand 							= 1;
	cisc->ins[0].input_operand[0].size 						= 32;
	cisc->ins[0].input_operand[0].instruction_index 		= it->instruction_index;
	cisc->ins[0].input_operand[0].variable 					= NULL;
	cisc->ins[0].input_operand[0].type 						= ASM_OPERAND_MEM;
	cisc->ins[0].input_operand[0].operand_type.mem.base 	= XED_REG_ESI;
	cisc->ins[0].input_operand[0].operand_type.mem.index 	= XED_REG_INVALID;
	cisc->ins[0].input_operand[0].operand_type.mem.scale 	= 1;
	cisc->ins[0].input_operand[0].operand_type.mem.disp 	= 0;
	cisc->ins[0].input_operand[0].operand_type.mem.con_addr = memAddress_search_and_get(mem_addr, MEMADDRESS_DESCRIPTOR_READ_0, 2);
	cisc->ins[0].output_operand.size 						= 32;
	cisc->ins[0].output_operand.instruction_index 			= it->instruction_index;
	cisc->ins[0].output_operand.variable 					= NULL;
	cisc->ins[0].output_operand.type 						= ASM_OPERAND_MEM;
	cisc->ins[0].output_operand.operand_type.mem.base 		= XED_REG_EDI;
	cisc->ins[0].output_operand.operand_type.mem.index 		= XED_REG_INVALID;
	cisc->ins[0].output_operand.operand_type.mem.scale 		= 1;
	cisc->ins[0].output_operand.operand_type.mem.disp 		= 0;
	cisc->ins[0].output_operand.operand_type.mem.con_addr 	= memAddress_search_and_get(mem_addr, MEMADDRESS_DESCRIPTOR_WRITE_0, 2);

	cisc->ins[1].opcode 									= IR_CMOV;
	cisc->ins[1].nb_input_operand 							= 2;
	cisc->ins[1].input_operand[0].size 						= 32;
	cisc->ins[1].input_operand[0].instruction_index 		= it->instruction_index;
	cisc->ins[1].input_operand[0].variable 					= NULL;
	cisc->ins[1].input_operand[0].type 						= ASM_OPERAND_IMM;
	cisc->ins[1].input_operand[0].operand_type.imm 			= 0x0000000000000004ULL;
	cisc->ins[1].input_operand[1].size 						= 32;
	cisc->ins[1].input_operand[1].instruction_index 		= it->instruction_index;
	cisc->ins[1].input_operand[1].variable 					= NULL;
	cisc->ins[1].input_operand[1].type 						= ASM_OPERAND_IMM;
	cisc->ins[1].input_operand[1].operand_type.imm 			= 0xfffffffffffffffcULL;
	cisc->ins[1].output_operand.size 						= 32;
	cisc->ins[1].output_operand.instruction_index 			= it->instruction_index;
	cisc->ins[1].output_operand.variable 					= NULL;
	cisc->ins[1].output_operand.type 						= ASM_OPERAND_REG;
	cisc->ins[1].output_operand.operand_type.reg 			= IR_REG_TMP;

	cisc->ins[2].opcode 									= IR_ADD;
	cisc->ins[2].nb_input_operand 							= 2;
	cisc->ins[2].input_operand[0].size 						= 32;
	cisc->ins[2].input_operand[0].instruction_index 		= it->instruction_index;
	cisc->ins[2].input_operand[0].variable 					= NULL;
	cisc->ins[2].input_operand[0].type 						= ASM_OPERAND_REG;
	cisc->ins[2].input_operand[0].operand_type.reg 			= IR_REG_ESI;
	cisc->ins[2].input_operand[1].size 						= 32;
	cisc->ins[2].input_operand[1].instruction_index 		= it->instruction_index;
	cisc->ins[2].input_operand[1].variable 					= NULL;
	cisc->ins[2].input_operand[1].type 						= ASM_OPERAND_REG;
	cisc->ins[2].input_operand[1].operand_type.reg 			= IR_REG_TMP;
	cisc->ins[2].output_operand.size 						= 32;
	cisc->ins[2].output_operand.instruction_index 			= it->instruction_index;
	cisc->ins[2].output_operand.variable 					= NULL;
	cisc->ins[2].output_operand.type 						= ASM_OPERAND_REG;
	cisc->ins[2].output_operand.operand_type.reg 			= IR_REG_ESI;

	cisc->ins[3].opcode 									= IR_ADD;
	cisc->ins[3].nb_input_operand 							= 2;
	cisc->ins[3].input_operand[0].size 						= 32;
	cisc->ins[3].input_operand[0].instruction_index 		= it->instruction_index;
	cisc->ins[3].input_operand[0].variable 					= NULL;
	cisc->ins[3].input_operand[0].type 						= ASM_OPERAND_REG;
	cisc->ins[3].input_operand[0].operand_type.reg 			= IR_REG_EDI;
	cisc->ins[3].input_operand[1].size 						= 32;
	cisc->ins[3].input_operand[1].instruction_index 		= it->instruction_index;
	cisc->ins[3].input_operand[1].variable 					= NULL;
	cisc->ins[3].input_operand[1].type 						= ASM_OPERAND_REG;
	cisc->ins[3].input_operand[1].operand_type.reg 			= IR_REG_TMP;
	cisc->ins[3].output_operand.size 						= 32;
	cisc->ins[3].output_operand.instruction_index 			= it->instruction_index;
	cisc->ins[3].output_operand.variable 					= NULL;
	cisc->ins[3].output_operand.type 						= ASM_OPERAND_REG;
	cisc->ins[3].output_operand.operand_type.reg 			= IR_REG_EDI;
}

static void cisc_decode_special_pop(struct instructionIterator* it, struct asmCiscIns* cisc, struct memAddress* mem_addr){
	cisc->valid 											= 1;
	cisc->nb_ins 											= 2;

	cisc->ins[0].opcode 									= IR_MOV;
	cisc->ins[0].nb_input_operand 							= 1;
	asmOperand_decode(it, &(cisc->ins[0].output_operand), 1, ASM_OPERAND_ROLE_WRITE_1, NULL, mem_addr);
	cisc->ins[0].input_operand[0].size 						= cisc->ins[0].output_operand.size;
	cisc->ins[0].input_operand[0].instruction_index 		= it->instruction_index;
	cisc->ins[0].input_operand[0].variable 					= NULL;
	cisc->ins[0].input_operand[0].type 						= ASM_OPERAND_MEM;
	cisc->ins[0].input_operand[0].operand_type.mem.base 	= XED_REG_ESP;
	cisc->ins[0].input_operand[0].operand_type.mem.index 	= XED_REG_INVALID;
	cisc->ins[0].input_operand[0].operand_type.mem.scale 	= 1;
	cisc->ins[0].input_operand[0].operand_type.mem.disp 	= 0;
	cisc->ins[0].input_operand[0].operand_type.mem.con_addr = memAddress_search_and_get(mem_addr, MEMADDRESS_DESCRIPTOR_READ_0, 2);

	cisc->ins[1].opcode 									= IR_ADD;
	cisc->ins[1].nb_input_operand 							= 2;
	cisc->ins[1].input_operand[0].size 						= 32;
	cisc->ins[1].input_operand[0].instruction_index 		= it->instruction_index;
	cisc->ins[1].input_operand[0].variable 					= NULL;
	cisc->ins[1].input_operand[0].type 						= ASM_OPERAND_REG;
	cisc->ins[1].input_operand[0].operand_type.reg 			= IR_REG_ESP;
	cisc->ins[1].input_operand[1].size 						= 32;
	cisc->ins[1].input_operand[1].instruction_index 		= it->instruction_index;
	cisc->ins[1].input_operand[1].variable 					= NULL;
	cisc->ins[1].input_operand[1].type 						= ASM_OPERAND_IMM;
	cisc->ins[1].input_operand[1].operand_type.imm 			= cisc->ins[0].output_operand.size / 8;
	cisc->ins[1].output_operand.size 						= 32;
	cisc->ins[1].output_operand.instruction_index 			= it->instruction_index;
	cisc->ins[1].output_operand.variable 					= NULL;
	cisc->ins[1].output_operand.type 						= ASM_OPERAND_REG;
	cisc->ins[1].output_operand.operand_type.reg 			= IR_REG_ESP;
}

static void cisc_decode_special_push(struct instructionIterator* it, struct asmCiscIns* cisc, struct memAddress* mem_addr){
	cisc->valid 											= 1;
	cisc->nb_ins 											= 2;

	cisc->ins[0].opcode 									= IR_MOV;
	cisc->ins[0].nb_input_operand 							= 1;
	asmOperand_decode(it, cisc->ins[0].input_operand, 1, ASM_OPERAND_ROLE_READ_1, NULL, mem_addr);
	asmOperand_sign_extend(cisc->ins[0].input_operand, 32);
	cisc->ins[0].output_operand.size 						= cisc->ins[0].input_operand[0].size;
	cisc->ins[0].output_operand.instruction_index 			= it->instruction_index;
	cisc->ins[0].output_operand.variable 					= NULL;
	cisc->ins[0].output_operand.type 						= ASM_OPERAND_MEM;
	cisc->ins[0].output_operand.operand_type.mem.base 		= XED_REG_ESP;
	cisc->ins[0].output_operand.operand_type.mem.index 		= XED_REG_INVALID;
	cisc->ins[0].output_operand.operand_type.mem.scale 		= 1;
	cisc->ins[0].output_operand.operand_type.mem.disp 		= -(cisc->ins[0].input_operand[0].size / 8);
	cisc->ins[0].output_operand.operand_type.mem.con_addr 	= memAddress_search_and_get(mem_addr, MEMADDRESS_DESCRIPTOR_WRITE_0, 2);

	cisc->ins[1].opcode 									= IR_SUB;
	cisc->ins[1].nb_input_operand 							= 2;
	cisc->ins[1].input_operand[0].size 						= 32;
	cisc->ins[1].input_operand[0].instruction_index 		= it->instruction_index;
	cisc->ins[1].input_operand[0].variable 					= NULL;
	cisc->ins[1].input_operand[0].type 						= ASM_OPERAND_REG;
	cisc->ins[1].input_operand[0].operand_type.reg 			= IR_REG_ESP;
	cisc->ins[1].input_operand[1].size 						= 32;
	cisc->ins[1].input_operand[1].instruction_index 		= it->instruction_index;
	cisc->ins[1].input_operand[1].variable 					= NULL;
	cisc->ins[1].input_operand[1].type 						= ASM_OPERAND_IMM;
	cisc->ins[1].input_operand[1].operand_type.imm 			= cisc->ins[0].input_operand[0].size / 8;
	cisc->ins[1].output_operand.size 						= 32;
	cisc->ins[1].output_operand.instruction_index 			= it->instruction_index;
	cisc->ins[1].output_operand.variable 					= NULL;
	cisc->ins[1].output_operand.type 						= ASM_OPERAND_REG;
	cisc->ins[1].output_operand.operand_type.reg 			= IR_REG_ESP;
}

static void cisc_decode_special_setxx(struct instructionIterator* it, struct asmCiscIns* cisc, struct memAddress* mem_addr){
	cisc->valid 											= 1;
	cisc->nb_ins 											= 1;

	cisc->ins[0].opcode 									= IR_CMOV;
	cisc->ins[0].nb_input_operand 							= 2;
	cisc->ins[0].input_operand[0].size 						= 8;
	cisc->ins[0].input_operand[0].instruction_index 		= it->instruction_index;
	cisc->ins[0].input_operand[0].variable 					= NULL;
	cisc->ins[0].input_operand[0].type 						= ASM_OPERAND_IMM;
	cisc->ins[0].input_operand[0].operand_type.imm 			= 0;
	cisc->ins[0].input_operand[1].size 						= 8;
	cisc->ins[0].input_operand[1].instruction_index 		= it->instruction_index;
	cisc->ins[0].input_operand[1].variable 					= NULL;
	cisc->ins[0].input_operand[1].type 						= ASM_OPERAND_IMM;
	cisc->ins[0].input_operand[1].operand_type.imm 			= 1;
	asmOperand_decode(it, &(cisc->ins[0].output_operand), 1, ASM_OPERAND_ROLE_WRITE_1, NULL, mem_addr);
}

static void cisc_decode_special_stosd(struct instructionIterator* it, struct asmCiscIns* cisc, struct memAddress* mem_addr){
	log_warn("unknown value for the direction flag");

	cisc->valid 											= 1;
	cisc->nb_ins 											= 3;

	cisc->ins[0].opcode 									= IR_MOV;
	cisc->ins[0].nb_input_operand 							= 1;
	cisc->ins[0].input_operand[0].size 						= 32;
	cisc->ins[0].input_operand[0].instruction_index 		= it->instruction_index;
	cisc->ins[0].input_operand[0].variable 					= NULL;
	cisc->ins[0].input_operand[0].type 						= ASM_OPERAND_REG;
	cisc->ins[0].input_operand[0].operand_type.reg 			= IR_REG_EAX;
	cisc->ins[0].output_operand.size 						= 32;
	cisc->ins[0].output_operand.instruction_index 			= it->instruction_index;
	cisc->ins[0].output_operand.variable 					= NULL;
	cisc->ins[0].output_operand.type 						= ASM_OPERAND_MEM;
	cisc->ins[0].output_operand.operand_type.mem.base 		= XED_REG_EDI;
	cisc->ins[0].output_operand.operand_type.mem.index 		= XED_REG_INVALID;
	cisc->ins[0].output_operand.operand_type.mem.scale 		= 1;
	cisc->ins[0].output_operand.operand_type.mem.disp 		= 0;
	cisc->ins[0].output_operand.operand_type.mem.con_addr 	= memAddress_get_and_check(mem_addr, MEMADDRESS_DESCRIPTOR_WRITE_0);

	cisc->ins[1].opcode 									= IR_CMOV;
	cisc->ins[1].nb_input_operand 							= 2;
	cisc->ins[1].input_operand[0].size 						= 32;
	cisc->ins[1].input_operand[0].instruction_index 		= it->instruction_index;
	cisc->ins[1].input_operand[0].variable 					= NULL;
	cisc->ins[1].input_operand[0].type 						= ASM_OPERAND_IMM;
	cisc->ins[1].input_operand[0].operand_type.imm 			= 0x0000000000000004ULL;
	cisc->ins[1].input_operand[1].size 						= 32;
	cisc->ins[1].input_operand[1].instruction_index 		= it->instruction_index;
	cisc->ins[1].input_operand[1].variable 					= NULL;
	cisc->ins[1].input_operand[1].type 						= ASM_OPERAND_IMM;
	cisc->ins[1].input_operand[1].operand_type.imm 			= 0xfffffffffffffffcULL;
	cisc->ins[1].output_operand.size 						= 32;
	cisc->ins[1].output_operand.instruction_index 			= it->instruction_index;
	cisc->ins[1].output_operand.variable 					= NULL;
	cisc->ins[1].output_operand.type 						= ASM_OPERAND_REG;
	cisc->ins[1].output_operand.operand_type.reg 			= IR_REG_TMP;

	cisc->ins[2].opcode 									= IR_ADD;
	cisc->ins[2].nb_input_operand 							= 2;
	cisc->ins[2].input_operand[0].size 						= 32;
	cisc->ins[2].input_operand[0].instruction_index 		= it->instruction_index;
	cisc->ins[2].input_operand[0].variable 					= NULL;
	cisc->ins[2].input_operand[0].type 						= ASM_OPERAND_REG;
	cisc->ins[2].input_operand[0].operand_type.reg 			= IR_REG_EDI;
	cisc->ins[2].input_operand[1].size 						= 32;
	cisc->ins[2].input_operand[1].instruction_index 		= it->instruction_index;
	cisc->ins[2].input_operand[1].variable 					= NULL;
	cisc->ins[2].input_operand[1].type 						= ASM_OPERAND_REG;
	cisc->ins[2].input_operand[1].operand_type.reg 			= IR_REG_TMP;
	cisc->ins[2].output_operand.size 						= 32;
	cisc->ins[2].output_operand.instruction_index 			= it->instruction_index;
	cisc->ins[2].output_operand.variable 					= NULL;
	cisc->ins[2].output_operand.type 						= ASM_OPERAND_REG;
	cisc->ins[2].output_operand.operand_type.reg 			= IR_REG_EDI;
}

static void cisc_decode_special_ret(const struct assembly* assembly, struct instructionIterator* it, struct asmCiscIns* cisc){
	cisc->valid 											= 1;
	cisc->nb_ins 											= 1;

	cisc->ins[0].opcode 									= IR_ADD;
	cisc->ins[0].nb_input_operand 							= 2;
	cisc->ins[0].input_operand[0].size 						= 32;
	cisc->ins[0].input_operand[0].instruction_index 		= it->instruction_index;
	cisc->ins[0].input_operand[0].variable 					= NULL;
	cisc->ins[0].input_operand[0].type 						= ASM_OPERAND_REG;
	cisc->ins[0].input_operand[0].operand_type.reg 			= IR_REG_ESP;
	cisc->ins[0].input_operand[1].size 						= 32;
	cisc->ins[0].input_operand[1].instruction_index 		= it->instruction_index;
	cisc->ins[0].input_operand[1].variable 					= NULL;
	cisc->ins[0].input_operand[1].type 						= ASM_OPERAND_IMM;
	if (it->instruction_size == 3){
		cisc->ins[0].input_operand[1].operand_type.imm 		= 4 + ((uint64_t)assembly->dyn_blocks[it->dyn_block_index].block->data[it->instruction_offset + 1] << 8) + assembly->dyn_blocks[it->dyn_block_index].block->data[it->instruction_offset + 2];
	}
	else{
		cisc->ins[0].input_operand[1].operand_type.imm 		= 4;
	}
	cisc->ins[0].output_operand.size 						= 32;
	cisc->ins[0].output_operand.instruction_index 			= it->instruction_index;
	cisc->ins[0].output_operand.variable 					= NULL;
	cisc->ins[0].output_operand.type 						= ASM_OPERAND_REG;
	cisc->ins[0].output_operand.operand_type.reg 			= IR_REG_ESP;
}

static void cisc_decode_special_xchg(struct instructionIterator* it, struct asmCiscIns* cisc, struct memAddress* mem_addr){
	cisc->valid 											= 1;
	cisc->nb_ins 											= 3;

	cisc->ins[0].opcode 									= IR_MOV;
	cisc->ins[0].nb_input_operand 							= 1;
	asmOperand_decode(it, cisc->ins[0].input_operand, 1, ASM_OPERAND_ROLE_WRITE_1, NULL, mem_addr);
	cisc->ins[0].output_operand.size 						= cisc->ins[0].input_operand[0].size;
	cisc->ins[0].output_operand.instruction_index 			= it->instruction_index;
	cisc->ins[0].output_operand.variable 					= NULL;
	cisc->ins[0].output_operand.type 						= ASM_OPERAND_REG;
	cisc->ins[0].output_operand.operand_type.reg 			= IR_REG_TMP;

	cisc->ins[1].opcode 									= IR_MOV;
	cisc->ins[1].nb_input_operand 							= 1;
	asmOperand_decode(it, cisc->ins[1].input_operand, 1, ASM_OPERAND_ROLE_WRITE_2, NULL, mem_addr);
	memcpy(&(cisc->ins[1].output_operand), cisc->ins[0].input_operand, sizeof(struct asmOperand));

	cisc->ins[2].opcode 									= IR_MOV;
	cisc->ins[2].nb_input_operand 							= 1;
	cisc->ins[2].input_operand[0].size 						= cisc->ins[0].input_operand[0].size;
	cisc->ins[2].input_operand[0].instruction_index 		= it->instruction_index;
	cisc->ins[2].input_operand[0].variable 					= NULL;
	cisc->ins[2].input_operand[0].type 						= ASM_OPERAND_REG;
	cisc->ins[2].input_operand[0].operand_type.reg 			= IR_REG_TMP;
	memcpy(&(cisc->ins[2].output_operand), cisc->ins[1].input_operand, sizeof(struct asmOperand));
}

static void cisc_decode_generic_simd(struct instructionIterator* it, struct asmCiscIns* cisc, struct memAddress* mem_addr, uint32_t nb_frag){
	uint32_t 			i;
	enum irOpcode 		opcode;
	struct asmRiscIns* 	ins;

	nb_frag = min(nb_frag, IRIMPORTERASM_MAX_RISC_INS);
	opcode = xedOpcode_2_irOpcode(xed_decoded_inst_get_iclass(&(it->xedd)));

	cisc->valid 											= 1;
	cisc->nb_ins 											= nb_frag;

	for (i = 0; i < nb_frag; i++){
		ins = cisc->ins + i;

		ins->opcode = opcode;
		asmOperand_decode_simd(it, ins->input_operand, IRIMPORTERASM_MAX_INPUT_OPERAND, ASM_OPERAND_ROLE_SET_FRAG(ASM_OPERAND_ROLE_READ_ALL, i), &(ins->nb_input_operand), mem_addr);
		asmOperand_decode_simd(it, &(ins->output_operand), 1, ASM_OPERAND_ROLE_SET_FRAG(ASM_OPERAND_ROLE_WRITE_ALL, i), NULL, mem_addr);
	}
}

static void cisc_decode_special_movd(const struct assembly* assembly, struct instructionIterator* it, struct asmCiscIns* cisc, struct memAddress* mem_addr){
	if (assembly->dyn_blocks[it->dyn_block_index].block->data[it->instruction_offset] == 0x66 && assembly->dyn_blocks[it->dyn_block_index].block->data[it->instruction_offset + 2] == 0x6e){
		cisc->valid 										= 1;
		cisc->nb_ins 										= 4;

		cisc->ins[0].opcode 								= IR_MOV;
		cisc->ins[0].nb_input_operand 						= 1;
		asmOperand_decode(it, cisc->ins[0].input_operand, IRIMPORTERASM_MAX_INPUT_OPERAND, ASM_OPERAND_ROLE_READ_1, NULL, mem_addr);
		asmOperand_decode_simd(it, &(cisc->ins[0].output_operand), 1, ASM_OPERAND_ROLE_SET_FRAG(ASM_OPERAND_ROLE_WRITE_ALL, 0), NULL, mem_addr);

		cisc->ins[1].opcode 								= IR_MOV;
		cisc->ins[1].nb_input_operand 						= 1;
		cisc->ins[1].input_operand[0].size 					= 32;
		cisc->ins[1].input_operand[0].instruction_index 	= it->instruction_index;
		cisc->ins[1].input_operand[0].variable 				= NULL;
		cisc->ins[1].input_operand[0].type 					= ASM_OPERAND_IMM;
		cisc->ins[1].input_operand[0].operand_type.imm 		= 0;
		asmOperand_decode_simd(it, &(cisc->ins[1].output_operand), 1, ASM_OPERAND_ROLE_SET_FRAG(ASM_OPERAND_ROLE_WRITE_ALL, 1), NULL, mem_addr);

		cisc->ins[2].opcode 								= IR_MOV;
		cisc->ins[2].nb_input_operand 						= 1;
		cisc->ins[2].input_operand[0].size 					= 32;
		cisc->ins[2].input_operand[0].instruction_index 	= it->instruction_index;
		cisc->ins[2].input_operand[0].variable 				= NULL;
		cisc->ins[2].input_operand[0].type 					= ASM_OPERAND_IMM;
		cisc->ins[2].input_operand[0].operand_type.imm 		= 0;
		asmOperand_decode_simd(it, &(cisc->ins[2].output_operand), 1, ASM_OPERAND_ROLE_SET_FRAG(ASM_OPERAND_ROLE_WRITE_ALL, 2), NULL, mem_addr);

		cisc->ins[3].opcode 								= IR_MOV;
		cisc->ins[3].nb_input_operand 						= 1;
		cisc->ins[3].input_operand[0].size 					= 32;
		cisc->ins[3].input_operand[0].instruction_index 	= it->instruction_index;
		cisc->ins[3].input_operand[0].variable 				= NULL;
		cisc->ins[3].input_operand[0].type 					= ASM_OPERAND_IMM;
		cisc->ins[3].input_operand[0].operand_type.imm 		= 0;
		asmOperand_decode_simd(it, &(cisc->ins[3].output_operand), 1, ASM_OPERAND_ROLE_SET_FRAG(ASM_OPERAND_ROLE_WRITE_ALL, 3), NULL, mem_addr);
	}
	else if (assembly->dyn_blocks[it->dyn_block_index].block->data[it->instruction_offset + 1] == 0x6e){
		cisc->valid 										= 1;
		cisc->nb_ins 										= 2;

		cisc->ins[0].opcode 								= IR_MOV;
		cisc->ins[0].nb_input_operand 						= 1;
		asmOperand_decode(it, cisc->ins[0].input_operand, IRIMPORTERASM_MAX_INPUT_OPERAND, ASM_OPERAND_ROLE_READ_1, NULL, mem_addr);
		asmOperand_decode_simd(it, &(cisc->ins[0].output_operand), 1, ASM_OPERAND_ROLE_SET_FRAG(ASM_OPERAND_ROLE_WRITE_ALL, 0), NULL, mem_addr);

		cisc->ins[1].opcode 								= IR_MOV;
		cisc->ins[1].nb_input_operand 						= 1;
		cisc->ins[1].input_operand[0].size 					= 32;
		cisc->ins[1].input_operand[0].instruction_index 	= it->instruction_index;
		cisc->ins[1].input_operand[0].variable 				= NULL;
		cisc->ins[1].input_operand[0].type 					= ASM_OPERAND_IMM;
		cisc->ins[1].input_operand[0].operand_type.imm 		= 0;
		asmOperand_decode_simd(it, &(cisc->ins[1].output_operand), 1, ASM_OPERAND_ROLE_SET_FRAG(ASM_OPERAND_ROLE_WRITE_ALL, 1), NULL, mem_addr);
	}
	else{
		cisc->valid 										= 1;
		cisc->nb_ins 										= 1;

		cisc->ins[0].opcode 								= IR_MOV;
		cisc->ins[0].nb_input_operand 						= 1;
		asmOperand_decode_simd(it, cisc->ins[0].input_operand, 1, ASM_OPERAND_ROLE_SET_FRAG(ASM_OPERAND_ROLE_READ_1, 0), NULL, mem_addr);
		asmOperand_decode(it, &(cisc->ins[0].output_operand), 1, ASM_OPERAND_ROLE_WRITE_ALL, NULL, mem_addr);
	}
}

static void cisc_decode_special_movq(const struct assembly* assembly, struct instructionIterator* it, struct asmCiscIns* cisc, struct memAddress* mem_addr){
	if (assembly->dyn_blocks[it->dyn_block_index].block->data[it->instruction_offset] == 0x0f){
		cisc_decode_generic_simd(it, cisc, mem_addr, 2);
	}
	else{
		cisc->valid 										= 1;
		cisc->nb_ins 										= 2;

		cisc->ins[0].opcode 								= IR_MOV;
		cisc->ins[0].nb_input_operand 						= 1;
		asmOperand_decode_simd(it, cisc->ins[0].input_operand, 1, ASM_OPERAND_ROLE_SET_FRAG(ASM_OPERAND_ROLE_READ_1, 0), NULL, mem_addr);
		asmOperand_decode_simd(it, &(cisc->ins[0].output_operand), 1, ASM_OPERAND_ROLE_SET_FRAG(ASM_OPERAND_ROLE_WRITE_1, 0), NULL, mem_addr);

		cisc->ins[1].opcode 								= IR_MOV;
		cisc->ins[1].nb_input_operand 						= 1;
		asmOperand_decode_simd(it, cisc->ins[1].input_operand, 1, ASM_OPERAND_ROLE_SET_FRAG(ASM_OPERAND_ROLE_READ_1, 1), NULL, mem_addr);
		asmOperand_decode_simd(it, &(cisc->ins[1].output_operand), 1, ASM_OPERAND_ROLE_SET_FRAG(ASM_OPERAND_ROLE_WRITE_1, 1), NULL, mem_addr);

		if (cisc->ins[0].output_operand.type == ASM_OPERAND_REG){
			cisc->nb_ins 									= 4;

			cisc->ins[2].opcode 							= IR_MOV;
			cisc->ins[2].nb_input_operand 					= 1;
			cisc->ins[2].input_operand[0].size 				= 32;
			cisc->ins[2].input_operand[0].instruction_index = it->instruction_index;
			cisc->ins[2].input_operand[0].variable 			= NULL;
			cisc->ins[2].input_operand[0].type 				= ASM_OPERAND_IMM;
			cisc->ins[2].input_operand[0].operand_type.imm 	= 0x0000000000000000ULL;
			asmOperand_decode_simd(it, &(cisc->ins[2].output_operand), 1, ASM_OPERAND_ROLE_SET_FRAG(ASM_OPERAND_ROLE_WRITE_1, 2), NULL, mem_addr);

			cisc->ins[3].opcode 							= IR_MOV;
			cisc->ins[3].nb_input_operand 					= 1;
			cisc->ins[3].input_operand[0].size 				= 32;
			cisc->ins[3].input_operand[0].instruction_index = it->instruction_index;
			cisc->ins[3].input_operand[0].variable 			= NULL;
			cisc->ins[3].input_operand[0].type 				= ASM_OPERAND_IMM;
			cisc->ins[3].input_operand[0].operand_type.imm 	= 0x0000000000000000ULL;
			asmOperand_decode_simd(it, &(cisc->ins[3].output_operand), 1, ASM_OPERAND_ROLE_SET_FRAG(ASM_OPERAND_ROLE_WRITE_1, 3), NULL, mem_addr);
		}
	}
}

static void cisc_decode_special_movsd_xmm(struct instructionIterator* it, struct asmCiscIns* cisc, struct memAddress* mem_addr){
	cisc->valid 										= 1;
	cisc->nb_ins 										= 2;

	cisc->ins[0].opcode 								= IR_MOV;
	cisc->ins[0].nb_input_operand 						= 1;
	asmOperand_decode_simd(it, cisc->ins[0].input_operand, 1, ASM_OPERAND_ROLE_SET_FRAG(ASM_OPERAND_ROLE_READ_1, 0), NULL, mem_addr);
	asmOperand_decode_simd(it, &(cisc->ins[0].output_operand), 1, ASM_OPERAND_ROLE_SET_FRAG(ASM_OPERAND_ROLE_WRITE_1, 0), NULL, mem_addr);

	cisc->ins[1].opcode 								= IR_MOV;
	cisc->ins[1].nb_input_operand 						= 1;
	asmOperand_decode_simd(it, cisc->ins[1].input_operand, 1, ASM_OPERAND_ROLE_SET_FRAG(ASM_OPERAND_ROLE_READ_1, 1), NULL, mem_addr);
	asmOperand_decode_simd(it, &(cisc->ins[1].output_operand), 1, ASM_OPERAND_ROLE_SET_FRAG(ASM_OPERAND_ROLE_WRITE_1, 1), NULL, mem_addr);

	if (cisc->ins[0].input_operand[0].type == ASM_OPERAND_MEM){
		cisc->nb_ins 									= 4;

		cisc->ins[2].opcode 							= IR_MOV;
		cisc->ins[2].nb_input_operand 					= 1;
		cisc->ins[2].input_operand[0].size 				= 32;
		cisc->ins[2].input_operand[0].instruction_index = it->instruction_index;
		cisc->ins[2].input_operand[0].variable 			= NULL;
		cisc->ins[2].input_operand[0].type 				= ASM_OPERAND_IMM;
		cisc->ins[2].input_operand[0].operand_type.imm 	= 0x0000000000000000ULL;
		asmOperand_decode_simd(it, &(cisc->ins[2].output_operand), 1, ASM_OPERAND_ROLE_SET_FRAG(ASM_OPERAND_ROLE_WRITE_1, 2), NULL, mem_addr);

		cisc->ins[3].opcode 							= IR_MOV;
		cisc->ins[3].nb_input_operand 					= 1;
		cisc->ins[3].input_operand[0].size 				= 32;
		cisc->ins[3].input_operand[0].instruction_index = it->instruction_index;
		cisc->ins[3].input_operand[0].variable 			= NULL;
		cisc->ins[3].input_operand[0].type 				= ASM_OPERAND_IMM;
		cisc->ins[3].input_operand[0].operand_type.imm 	= 0x0000000000000000ULL;
		asmOperand_decode_simd(it, &(cisc->ins[3].output_operand), 1, ASM_OPERAND_ROLE_SET_FRAG(ASM_OPERAND_ROLE_WRITE_1, 3), NULL, mem_addr);
	}
}

static void cisc_decode_special_pinsrw(struct instructionIterator* it, struct asmCiscIns* cisc, struct memAddress* mem_addr){
	struct asmOperand op2;

	asmOperand_decode(it, &op2, 1, ASM_OPERAND_ROLE_READ_3, NULL, mem_addr);

	cisc->valid 										= 1;
	cisc->nb_ins 										= (op2.operand_type.imm & 0x0000000000000001ULL) ?  3 : 2;

	cisc->ins[0].opcode 								= IR_AND;
	cisc->ins[0].nb_input_operand 						= 2;
	asmOperand_decode_simd(it, cisc->ins[0].input_operand, 1, ASM_OPERAND_ROLE_SET_FRAG(ASM_OPERAND_ROLE_WRITE_1,(op2.operand_type.imm >> 1) & 0x0000000000000003ULL), NULL, mem_addr);
	cisc->ins[0].input_operand[1].size 					= 32;
	cisc->ins[0].input_operand[1].instruction_index 	= it->instruction_index;
	cisc->ins[0].input_operand[1].variable 				= NULL;
	cisc->ins[0].input_operand[1].type 					= ASM_OPERAND_IMM;
	cisc->ins[0].input_operand[1].operand_type.imm 		= ~(0x000000000000ffffULL << ((op2.operand_type.imm & 0x0000000000000001ULL) * 16));
	memcpy(&(cisc->ins[0].output_operand), cisc->ins[0].input_operand, sizeof(struct asmOperand));

	if (op2.operand_type.imm & 0x0000000000000001ULL){
		cisc->ins[1].opcode 								= IR_SHR;
		cisc->ins[1].nb_input_operand 						= 2;
		asmOperand_decode(it, cisc->ins[1].input_operand, 1, ASM_OPERAND_ROLE_READ_2, NULL, mem_addr);
		cisc->ins[1].input_operand[1].size 					= 32;
		cisc->ins[1].input_operand[1].instruction_index 	= it->instruction_index;
		cisc->ins[1].input_operand[1].variable 				= NULL;
		cisc->ins[1].input_operand[1].type 					= ASM_OPERAND_IMM;
		cisc->ins[1].input_operand[1].operand_type.imm 		= 16;
		cisc->ins[1].output_operand.size 					= 32;
		cisc->ins[1].output_operand.instruction_index 		= it->instruction_index;
		cisc->ins[1].output_operand.variable 				= NULL;
		cisc->ins[1].output_operand.type 					= ASM_OPERAND_REG;
		cisc->ins[1].output_operand.operand_type.reg 		= IR_REG_TMP;

		cisc->ins[2].opcode 								= IR_OR;
		cisc->ins[2].nb_input_operand 						= 2;
		cisc->ins[2].input_operand[0].size 					= 32;
		cisc->ins[2].input_operand[0].instruction_index 	= it->instruction_index;
		cisc->ins[2].input_operand[0].variable 				= NULL;
		cisc->ins[2].input_operand[0].type 					= ASM_OPERAND_REG;
		cisc->ins[2].input_operand[0].operand_type.reg 		= IR_REG_TMP;
		memcpy(cisc->ins[2].input_operand + 1, cisc->ins[0].input_operand, sizeof(struct asmOperand));
		memcpy(&(cisc->ins[2].output_operand), cisc->ins[0].input_operand, sizeof(struct asmOperand));
	}
	else{
		cisc->ins[1].opcode 								= IR_OR;
		cisc->ins[1].nb_input_operand 						= 2;
		memcpy(cisc->ins[1].input_operand, cisc->ins[0].input_operand, sizeof(struct asmOperand));
		asmOperand_decode(it, cisc->ins[1].input_operand + 1, 1, ASM_OPERAND_ROLE_READ_2, NULL, mem_addr);
		memcpy(&(cisc->ins[1].output_operand), cisc->ins[0].input_operand, sizeof(struct asmOperand));

		if (cisc->ins[1].input_operand[1].type == ASM_OPERAND_REG){
			cisc->nb_ins 									= 3;
			memcpy(cisc->ins + 2, cisc->ins + 1, sizeof(struct asmRiscIns));

			cisc->ins[1].opcode 								= IR_AND;
			cisc->ins[1].nb_input_operand 						= 2;
			cisc->ins[1].input_operand[0].size 					= 32;
			cisc->ins[1].input_operand[0].instruction_index 	= it->instruction_index;
			cisc->ins[1].input_operand[0].variable 				= NULL;
			cisc->ins[1].input_operand[0].type 					= ASM_OPERAND_IMM;
			cisc->ins[1].input_operand[0].operand_type.imm 		= 0x000000000000ffffULL;
			cisc->ins[1].output_operand.size 					= 32;
			cisc->ins[1].output_operand.instruction_index 		= it->instruction_index;
			cisc->ins[1].output_operand.variable 				= NULL;
			cisc->ins[1].output_operand.type 					= ASM_OPERAND_REG;
			cisc->ins[1].output_operand.operand_type.reg 		= IR_REG_TMP;

			cisc->ins[2].input_operand[1].operand_type.reg 		= IR_REG_TMP;
		}
	}
}

static void cisc_decode_special_pshufd(struct instructionIterator* it, struct asmCiscIns* cisc, struct memAddress* mem_addr){
	struct asmOperand op2;

	asmOperand_decode(it, &op2, 1, ASM_OPERAND_ROLE_READ_2, NULL, mem_addr);

	cisc->valid 										= 1;
	cisc->nb_ins 										= 4;

	cisc->ins[0].opcode 								= IR_MOV;
	cisc->ins[0].nb_input_operand 						= 1;
	asmOperand_decode_simd(it, cisc->ins[0].input_operand, 1, ASM_OPERAND_ROLE_SET_FRAG(ASM_OPERAND_ROLE_READ_1,(op2.operand_type.imm >> 0) & 0x0000000000000003ULL), NULL, mem_addr);
	asmOperand_decode_simd(it, &(cisc->ins[0].output_operand), 1, ASM_OPERAND_ROLE_SET_FRAG(ASM_OPERAND_ROLE_WRITE_ALL, 0), NULL, mem_addr);

	cisc->ins[1].opcode 								= IR_MOV;
	cisc->ins[1].nb_input_operand 						= 1;
	asmOperand_decode_simd(it, cisc->ins[1].input_operand, 1, ASM_OPERAND_ROLE_SET_FRAG(ASM_OPERAND_ROLE_READ_1, (op2.operand_type.imm >> 2) & 0x0000000000000003ULL), NULL, mem_addr);
	asmOperand_decode_simd(it, &(cisc->ins[1].output_operand), 1, ASM_OPERAND_ROLE_SET_FRAG(ASM_OPERAND_ROLE_WRITE_ALL, 1), NULL, mem_addr);

	cisc->ins[2].opcode 								= IR_MOV;
	cisc->ins[2].nb_input_operand 						= 1;
	asmOperand_decode_simd(it, cisc->ins[2].input_operand, 1, ASM_OPERAND_ROLE_SET_FRAG(ASM_OPERAND_ROLE_READ_1, (op2.operand_type.imm >> 4) & 0x0000000000000003ULL), NULL, mem_addr);
	asmOperand_decode_simd(it, &(cisc->ins[2].output_operand), 1, ASM_OPERAND_ROLE_SET_FRAG(ASM_OPERAND_ROLE_WRITE_ALL, 2), NULL, mem_addr);

	cisc->ins[3].opcode 								= IR_MOV;
	cisc->ins[3].nb_input_operand 						= 1;
	asmOperand_decode_simd(it, cisc->ins[3].input_operand, 1, ASM_OPERAND_ROLE_SET_FRAG(ASM_OPERAND_ROLE_READ_1, (op2.operand_type.imm >> 6) & 0x0000000000000003ULL), NULL, mem_addr);
	asmOperand_decode_simd(it, &(cisc->ins[3].output_operand), 1, ASM_OPERAND_ROLE_SET_FRAG(ASM_OPERAND_ROLE_WRITE_ALL, 3), NULL, mem_addr);
}

static void cisc_decode_special_pslld(const struct assembly* assembly, struct instructionIterator* it, struct asmCiscIns* cisc, struct memAddress* mem_addr){
	if (assembly->dyn_blocks[it->dyn_block_index].block->data[it->instruction_offset] == 0x66){
		cisc->valid 										= 1;
		cisc->nb_ins 										= 4;

		cisc->ins[0].opcode 								= IR_SHL;
		cisc->ins[0].nb_input_operand 						= 2;
		asmOperand_decode_simd(it, cisc->ins[0].input_operand, 1, ASM_OPERAND_ROLE_SET_FRAG(ASM_OPERAND_ROLE_READ_1, 0), NULL, mem_addr);
		asmOperand_decode(it, cisc->ins[0].input_operand + 1, 1, ASM_OPERAND_ROLE_READ_2, NULL, mem_addr);
		asmOperand_decode_simd(it, &(cisc->ins[0].output_operand), 1, ASM_OPERAND_ROLE_SET_FRAG(ASM_OPERAND_ROLE_WRITE_ALL, 0), NULL, mem_addr);

		cisc->ins[1].opcode 								= IR_SHL;
		cisc->ins[1].nb_input_operand 						= 2;
		asmOperand_decode_simd(it, cisc->ins[1].input_operand, 1, ASM_OPERAND_ROLE_SET_FRAG(ASM_OPERAND_ROLE_READ_1, 1), NULL, mem_addr);
		memcpy(cisc->ins[1].input_operand + 1, cisc->ins[0].input_operand + 1, sizeof(struct asmOperand));
		asmOperand_decode_simd(it, &(cisc->ins[1].output_operand), 1, ASM_OPERAND_ROLE_SET_FRAG(ASM_OPERAND_ROLE_WRITE_ALL, 1), NULL, mem_addr);

		cisc->ins[2].opcode 								= IR_SHL;
		cisc->ins[2].nb_input_operand 						= 2;
		asmOperand_decode_simd(it, cisc->ins[2].input_operand, 1, ASM_OPERAND_ROLE_SET_FRAG(ASM_OPERAND_ROLE_READ_1, 2), NULL, mem_addr);
		memcpy(cisc->ins[2].input_operand + 1, cisc->ins[0].input_operand + 1, sizeof(struct asmOperand));
		asmOperand_decode_simd(it, &(cisc->ins[2].output_operand), 1, ASM_OPERAND_ROLE_SET_FRAG(ASM_OPERAND_ROLE_WRITE_ALL, 2), NULL, mem_addr);

		cisc->ins[3].opcode 								= IR_SHL;
		cisc->ins[3].nb_input_operand 						= 2;
		asmOperand_decode_simd(it, cisc->ins[3].input_operand, 1, ASM_OPERAND_ROLE_SET_FRAG(ASM_OPERAND_ROLE_READ_1, 3), NULL, mem_addr);
		memcpy(cisc->ins[3].input_operand + 1, cisc->ins[0].input_operand + 1, sizeof(struct asmOperand));
		asmOperand_decode_simd(it, &(cisc->ins[3].output_operand), 1, ASM_OPERAND_ROLE_SET_FRAG(ASM_OPERAND_ROLE_WRITE_ALL, 3), NULL, mem_addr);
	}
	else{
		cisc->valid 										= 1;
		cisc->nb_ins 										= 2;

		cisc->ins[0].opcode 								= IR_SHL;
		cisc->ins[0].nb_input_operand 						= 2;
		asmOperand_decode_simd(it, cisc->ins[0].input_operand, 1, ASM_OPERAND_ROLE_SET_FRAG(ASM_OPERAND_ROLE_READ_1, 0), NULL, mem_addr);
		asmOperand_decode(it, cisc->ins[0].input_operand + 1, 1, ASM_OPERAND_ROLE_READ_2, NULL, mem_addr);
		asmOperand_decode_simd(it, &(cisc->ins[0].output_operand), 1, ASM_OPERAND_ROLE_SET_FRAG(ASM_OPERAND_ROLE_WRITE_ALL, 0), NULL, mem_addr);

		cisc->ins[1].opcode 								= IR_SHL;
		cisc->ins[1].nb_input_operand 						= 2;
		asmOperand_decode_simd(it, cisc->ins[1].input_operand, 1, ASM_OPERAND_ROLE_SET_FRAG(ASM_OPERAND_ROLE_READ_1, 1), NULL, mem_addr);
		memcpy(cisc->ins[1].input_operand + 1, cisc->ins[0].input_operand + 1, sizeof(struct asmOperand));
		asmOperand_decode_simd(it, &(cisc->ins[1].output_operand), 1, ASM_OPERAND_ROLE_SET_FRAG(ASM_OPERAND_ROLE_WRITE_ALL, 1), NULL, mem_addr);
	}
}

static void cisc_decode_special_psrld(const struct assembly* assembly, struct instructionIterator* it, struct asmCiscIns* cisc, struct memAddress* mem_addr){
	if (assembly->dyn_blocks[it->dyn_block_index].block->data[it->instruction_offset] == 0x66){
		cisc->valid 										= 1;
		cisc->nb_ins 										= 4;

		cisc->ins[0].opcode 								= IR_SHR;
		cisc->ins[0].nb_input_operand 						= 2;
		asmOperand_decode_simd(it, cisc->ins[0].input_operand, 1, ASM_OPERAND_ROLE_SET_FRAG(ASM_OPERAND_ROLE_READ_1, 0), NULL, mem_addr);
		asmOperand_decode(it, cisc->ins[0].input_operand + 1, 1, ASM_OPERAND_ROLE_READ_2, NULL, mem_addr);
		asmOperand_decode_simd(it, &(cisc->ins[0].output_operand), 1, ASM_OPERAND_ROLE_SET_FRAG(ASM_OPERAND_ROLE_WRITE_ALL, 0), NULL, mem_addr);

		cisc->ins[1].opcode 								= IR_SHR;
		cisc->ins[1].nb_input_operand 						= 2;
		asmOperand_decode_simd(it, cisc->ins[1].input_operand, 1, ASM_OPERAND_ROLE_SET_FRAG(ASM_OPERAND_ROLE_READ_1, 1), NULL, mem_addr);
		memcpy(cisc->ins[1].input_operand + 1, cisc->ins[0].input_operand + 1, sizeof(struct asmOperand));
		asmOperand_decode_simd(it, &(cisc->ins[1].output_operand), 1, ASM_OPERAND_ROLE_SET_FRAG(ASM_OPERAND_ROLE_WRITE_ALL, 1), NULL, mem_addr);

		cisc->ins[2].opcode 								= IR_SHR;
		cisc->ins[2].nb_input_operand 						= 2;
		asmOperand_decode_simd(it, cisc->ins[2].input_operand, 1, ASM_OPERAND_ROLE_SET_FRAG(ASM_OPERAND_ROLE_READ_1, 2), NULL, mem_addr);
		memcpy(cisc->ins[2].input_operand + 1, cisc->ins[0].input_operand + 1, sizeof(struct asmOperand));
		asmOperand_decode_simd(it, &(cisc->ins[2].output_operand), 1, ASM_OPERAND_ROLE_SET_FRAG(ASM_OPERAND_ROLE_WRITE_ALL, 2), NULL, mem_addr);

		cisc->ins[3].opcode 								= IR_SHR;
		cisc->ins[3].nb_input_operand 						= 2;
		asmOperand_decode_simd(it, cisc->ins[3].input_operand, 1, ASM_OPERAND_ROLE_SET_FRAG(ASM_OPERAND_ROLE_READ_1, 3), NULL, mem_addr);
		memcpy(cisc->ins[3].input_operand + 1, cisc->ins[0].input_operand + 1, sizeof(struct asmOperand));
		asmOperand_decode_simd(it, &(cisc->ins[3].output_operand), 1, ASM_OPERAND_ROLE_SET_FRAG(ASM_OPERAND_ROLE_WRITE_ALL, 3), NULL, mem_addr);
	}
	else{
		cisc->valid 										= 1;
		cisc->nb_ins 										= 2;

		cisc->ins[0].opcode 								= IR_SHR;
		cisc->ins[0].nb_input_operand 						= 2;
		asmOperand_decode_simd(it, cisc->ins[0].input_operand, 1, ASM_OPERAND_ROLE_SET_FRAG(ASM_OPERAND_ROLE_READ_1, 0), NULL, mem_addr);
		asmOperand_decode(it, cisc->ins[0].input_operand + 1, 1, ASM_OPERAND_ROLE_READ_2, NULL, mem_addr);
		asmOperand_decode_simd(it, &(cisc->ins[0].output_operand), 1, ASM_OPERAND_ROLE_SET_FRAG(ASM_OPERAND_ROLE_WRITE_ALL, 0), NULL, mem_addr);

		cisc->ins[1].opcode 								= IR_SHR;
		cisc->ins[1].nb_input_operand 						= 2;
		asmOperand_decode_simd(it, cisc->ins[1].input_operand, 1, ASM_OPERAND_ROLE_SET_FRAG(ASM_OPERAND_ROLE_READ_1, 1), NULL, mem_addr);
		memcpy(cisc->ins[1].input_operand + 1, cisc->ins[0].input_operand + 1, sizeof(struct asmOperand));
		asmOperand_decode_simd(it, &(cisc->ins[1].output_operand), 1, ASM_OPERAND_ROLE_SET_FRAG(ASM_OPERAND_ROLE_WRITE_ALL, 1), NULL, mem_addr);
	}
}

static void cisc_decode_special_punpckldq(const struct assembly* assembly, struct instructionIterator* it, struct asmCiscIns* cisc, struct memAddress* mem_addr){
	if (assembly->dyn_blocks[it->dyn_block_index].block->data[it->instruction_offset] == 0x66){
		cisc->valid 										= 1;
		cisc->nb_ins 										= 3;

		cisc->ins[0].opcode 								= IR_MOV;
		cisc->ins[0].nb_input_operand 						= 1;
		asmOperand_decode_simd(it, cisc->ins[0].input_operand, 1, ASM_OPERAND_ROLE_SET_FRAG(ASM_OPERAND_ROLE_READ_2, 1), NULL, mem_addr);
		asmOperand_decode_simd(it, &(cisc->ins[0].output_operand), 1, ASM_OPERAND_ROLE_SET_FRAG(ASM_OPERAND_ROLE_WRITE_ALL, 3), NULL, mem_addr);

		cisc->ins[1].opcode 								= IR_MOV;
		cisc->ins[1].nb_input_operand 						= 1;
		asmOperand_decode_simd(it, cisc->ins[1].input_operand, 1, ASM_OPERAND_ROLE_SET_FRAG(ASM_OPERAND_ROLE_READ_1, 1), NULL, mem_addr);
		asmOperand_decode_simd(it, &(cisc->ins[1].output_operand), 1, ASM_OPERAND_ROLE_SET_FRAG(ASM_OPERAND_ROLE_WRITE_ALL, 2), NULL, mem_addr);

		cisc->ins[2].opcode 								= IR_MOV;
		cisc->ins[2].nb_input_operand 						= 1;
		asmOperand_decode_simd(it, cisc->ins[2].input_operand, 1, ASM_OPERAND_ROLE_SET_FRAG(ASM_OPERAND_ROLE_READ_2, 0), NULL, mem_addr);
		asmOperand_decode_simd(it, &(cisc->ins[2].output_operand), 1, ASM_OPERAND_ROLE_SET_FRAG(ASM_OPERAND_ROLE_WRITE_ALL, 1), NULL, mem_addr);
	}
	else{
		cisc->valid 										= 1;
		cisc->nb_ins 										= 1;

		cisc->ins[0].opcode 								= IR_MOV;
		cisc->ins[0].nb_input_operand 						= 1;
		asmOperand_decode_simd(it, cisc->ins[0].input_operand, 1, ASM_OPERAND_ROLE_SET_FRAG(ASM_OPERAND_ROLE_READ_2, 0), NULL, mem_addr);
		asmOperand_decode_simd(it, &(cisc->ins[0].output_operand), 1, ASM_OPERAND_ROLE_SET_FRAG(ASM_OPERAND_ROLE_WRITE_ALL, 1), NULL, mem_addr);
	}
}

static void cisc_decode_generic_vex(struct instructionIterator* it, struct asmCiscIns* cisc, struct memAddress* mem_addr){
	uint32_t length;

	switch ((length = xed_decoded_inst_vector_length_bits(&(it->xedd)))){
		case 128 	: {
			cisc_decode_generic_simd(it, cisc, mem_addr, 4);

			switch (cisc->ins[0].output_operand.type){
				case ASM_OPERAND_IMM : {
					log_err("this case is not supposed to happen");
				}
				case ASM_OPERAND_REG : {
					int32_t simd_base;

					if ((simd_base = ir_reg_get_simd_base(cisc->ins[0].output_operand.operand_type.reg)) != -1){
						asmRisc_set_reg_cst(cisc->ins + cisc->nb_ins + 0, ir_register_simd_frag[simd_base][4], 32, 0x0000000000000000ULL)
						asmRisc_set_reg_cst(cisc->ins + cisc->nb_ins + 1, ir_register_simd_frag[simd_base][5], 32, 0x0000000000000000ULL)
						asmRisc_set_reg_cst(cisc->ins + cisc->nb_ins + 2, ir_register_simd_frag[simd_base][6], 32, 0x0000000000000000ULL)
						asmRisc_set_reg_cst(cisc->ins + cisc->nb_ins + 3, ir_register_simd_frag[simd_base][7], 32, 0x0000000000000000ULL)

						cisc->nb_ins += 4;
					}
					break;
				}
				case ASM_OPERAND_MEM : {
					enum irRegister base 		= cisc->ins[cisc->nb_ins - 1].output_operand.operand_type.mem.base;
					enum irRegister index 		= cisc->ins[cisc->nb_ins - 1].output_operand.operand_type.mem.index;
					enum irRegister scale 		= cisc->ins[cisc->nb_ins - 1].output_operand.operand_type.mem.scale;
					uint32_t 		disp 		= cisc->ins[cisc->nb_ins - 1].output_operand.operand_type.mem.disp;
					ADDRESS 		con_addr 	= cisc->ins[cisc->nb_ins - 1].output_operand.operand_type.mem.con_addr;

					if (con_addr == MEMADDRESS_INVALID){
						asmRisc_set_mem_cst(cisc->ins + cisc->nb_ins + 0, base, index, scale, disp + 4 , MEMADDRESS_INVALID, 32, 0)
						asmRisc_set_mem_cst(cisc->ins + cisc->nb_ins + 1, base, index, scale, disp + 8 , MEMADDRESS_INVALID, 32, 0)
						asmRisc_set_mem_cst(cisc->ins + cisc->nb_ins + 2, base, index, scale, disp + 12, MEMADDRESS_INVALID, 32, 0)
						asmRisc_set_mem_cst(cisc->ins + cisc->nb_ins + 3, base, index, scale, disp + 16, MEMADDRESS_INVALID, 32, 0)
					}
					else{
						asmRisc_set_mem_cst(cisc->ins + cisc->nb_ins + 0, base, index, scale, disp + 4 , con_addr + 4 , 32, 0)
						asmRisc_set_mem_cst(cisc->ins + cisc->nb_ins + 1, base, index, scale, disp + 8 , con_addr + 8 , 32, 0)
						asmRisc_set_mem_cst(cisc->ins + cisc->nb_ins + 2, base, index, scale, disp + 12, con_addr + 12, 32, 0)
						asmRisc_set_mem_cst(cisc->ins + cisc->nb_ins + 3, base, index, scale, disp + 16, con_addr + 16, 32, 0)
					}

					cisc->nb_ins += 4;

					break;
				}
				
			}

			break;
		}
		case 256 	: {
			cisc_decode_generic_simd(it, cisc, mem_addr, 8);
			break;
		}
		default 	: {
			log_err_m("incorrect VEX length: %u", length);
		}
	}
}

static void cisc_decode_special_vzeroall(struct instructionIterator* it, struct asmCiscIns* cisc){
	cisc->valid 										= 1;
	cisc->nb_ins 										= 64;

	asmRisc_set_reg_cst(cisc->ins + 0 , IR_REG_XMM1_1, 32, 0x0000000000000000ULL)
	asmRisc_set_reg_cst(cisc->ins + 1 , IR_REG_XMM1_2, 32, 0x0000000000000000ULL)
	asmRisc_set_reg_cst(cisc->ins + 2 , IR_REG_XMM1_3, 32, 0x0000000000000000ULL)
	asmRisc_set_reg_cst(cisc->ins + 3 , IR_REG_XMM1_4, 32, 0x0000000000000000ULL)
	asmRisc_set_reg_cst(cisc->ins + 4 , IR_REG_XMM2_1, 32, 0x0000000000000000ULL)
	asmRisc_set_reg_cst(cisc->ins + 5 , IR_REG_XMM2_2, 32, 0x0000000000000000ULL)
	asmRisc_set_reg_cst(cisc->ins + 6 , IR_REG_XMM2_3, 32, 0x0000000000000000ULL)
	asmRisc_set_reg_cst(cisc->ins + 7 , IR_REG_XMM2_4, 32, 0x0000000000000000ULL)
	asmRisc_set_reg_cst(cisc->ins + 8 , IR_REG_XMM3_1, 32, 0x0000000000000000ULL)
	asmRisc_set_reg_cst(cisc->ins + 9 , IR_REG_XMM3_2, 32, 0x0000000000000000ULL)
	asmRisc_set_reg_cst(cisc->ins + 10, IR_REG_XMM3_3, 32, 0x0000000000000000ULL)
	asmRisc_set_reg_cst(cisc->ins + 11, IR_REG_XMM3_4, 32, 0x0000000000000000ULL)
	asmRisc_set_reg_cst(cisc->ins + 12, IR_REG_XMM4_1, 32, 0x0000000000000000ULL)
	asmRisc_set_reg_cst(cisc->ins + 13, IR_REG_XMM4_2, 32, 0x0000000000000000ULL)
	asmRisc_set_reg_cst(cisc->ins + 14, IR_REG_XMM4_3, 32, 0x0000000000000000ULL)
	asmRisc_set_reg_cst(cisc->ins + 15, IR_REG_XMM4_4, 32, 0x0000000000000000ULL)
	asmRisc_set_reg_cst(cisc->ins + 16, IR_REG_XMM5_1, 32, 0x0000000000000000ULL)
	asmRisc_set_reg_cst(cisc->ins + 17, IR_REG_XMM5_2, 32, 0x0000000000000000ULL)
	asmRisc_set_reg_cst(cisc->ins + 18, IR_REG_XMM5_3, 32, 0x0000000000000000ULL)
	asmRisc_set_reg_cst(cisc->ins + 19, IR_REG_XMM5_4, 32, 0x0000000000000000ULL)
	asmRisc_set_reg_cst(cisc->ins + 20, IR_REG_XMM6_1, 32, 0x0000000000000000ULL)
	asmRisc_set_reg_cst(cisc->ins + 21, IR_REG_XMM6_2, 32, 0x0000000000000000ULL)
	asmRisc_set_reg_cst(cisc->ins + 22, IR_REG_XMM6_3, 32, 0x0000000000000000ULL)
	asmRisc_set_reg_cst(cisc->ins + 23, IR_REG_XMM6_4, 32, 0x0000000000000000ULL)
	asmRisc_set_reg_cst(cisc->ins + 24, IR_REG_XMM7_1, 32, 0x0000000000000000ULL)
	asmRisc_set_reg_cst(cisc->ins + 25, IR_REG_XMM7_2, 32, 0x0000000000000000ULL)
	asmRisc_set_reg_cst(cisc->ins + 26, IR_REG_XMM7_3, 32, 0x0000000000000000ULL)
	asmRisc_set_reg_cst(cisc->ins + 27, IR_REG_XMM7_4, 32, 0x0000000000000000ULL)
	asmRisc_set_reg_cst(cisc->ins + 28, IR_REG_XMM8_1, 32, 0x0000000000000000ULL)
	asmRisc_set_reg_cst(cisc->ins + 29, IR_REG_XMM8_2, 32, 0x0000000000000000ULL)
	asmRisc_set_reg_cst(cisc->ins + 30, IR_REG_XMM8_3, 32, 0x0000000000000000ULL)
	asmRisc_set_reg_cst(cisc->ins + 31, IR_REG_XMM8_4, 32, 0x0000000000000000ULL)
	asmRisc_set_reg_cst(cisc->ins + 32, IR_REG_YMM1_5, 32, 0x0000000000000000ULL)
	asmRisc_set_reg_cst(cisc->ins + 33, IR_REG_YMM1_6, 32, 0x0000000000000000ULL)
	asmRisc_set_reg_cst(cisc->ins + 34, IR_REG_YMM1_7, 32, 0x0000000000000000ULL)
	asmRisc_set_reg_cst(cisc->ins + 35, IR_REG_YMM1_8, 32, 0x0000000000000000ULL)
	asmRisc_set_reg_cst(cisc->ins + 36, IR_REG_YMM2_5, 32, 0x0000000000000000ULL)
	asmRisc_set_reg_cst(cisc->ins + 37, IR_REG_YMM2_6, 32, 0x0000000000000000ULL)
	asmRisc_set_reg_cst(cisc->ins + 38, IR_REG_YMM2_7, 32, 0x0000000000000000ULL)
	asmRisc_set_reg_cst(cisc->ins + 39, IR_REG_YMM2_8, 32, 0x0000000000000000ULL)
	asmRisc_set_reg_cst(cisc->ins + 40, IR_REG_YMM3_5, 32, 0x0000000000000000ULL)
	asmRisc_set_reg_cst(cisc->ins + 41, IR_REG_YMM3_6, 32, 0x0000000000000000ULL)
	asmRisc_set_reg_cst(cisc->ins + 42, IR_REG_YMM3_7, 32, 0x0000000000000000ULL)
	asmRisc_set_reg_cst(cisc->ins + 43, IR_REG_YMM3_8, 32, 0x0000000000000000ULL)
	asmRisc_set_reg_cst(cisc->ins + 44, IR_REG_YMM4_5, 32, 0x0000000000000000ULL)
	asmRisc_set_reg_cst(cisc->ins + 45, IR_REG_YMM4_6, 32, 0x0000000000000000ULL)
	asmRisc_set_reg_cst(cisc->ins + 46, IR_REG_YMM4_7, 32, 0x0000000000000000ULL)
	asmRisc_set_reg_cst(cisc->ins + 47, IR_REG_YMM4_8, 32, 0x0000000000000000ULL)
	asmRisc_set_reg_cst(cisc->ins + 48, IR_REG_YMM5_5, 32, 0x0000000000000000ULL)
	asmRisc_set_reg_cst(cisc->ins + 49, IR_REG_YMM5_6, 32, 0x0000000000000000ULL)
	asmRisc_set_reg_cst(cisc->ins + 50, IR_REG_YMM5_7, 32, 0x0000000000000000ULL)
	asmRisc_set_reg_cst(cisc->ins + 51, IR_REG_YMM5_8, 32, 0x0000000000000000ULL)
	asmRisc_set_reg_cst(cisc->ins + 52, IR_REG_YMM6_5, 32, 0x0000000000000000ULL)
	asmRisc_set_reg_cst(cisc->ins + 53, IR_REG_YMM6_6, 32, 0x0000000000000000ULL)
	asmRisc_set_reg_cst(cisc->ins + 54, IR_REG_YMM6_7, 32, 0x0000000000000000ULL)
	asmRisc_set_reg_cst(cisc->ins + 55, IR_REG_YMM6_8, 32, 0x0000000000000000ULL)
	asmRisc_set_reg_cst(cisc->ins + 56, IR_REG_YMM7_5, 32, 0x0000000000000000ULL)
	asmRisc_set_reg_cst(cisc->ins + 57, IR_REG_YMM7_6, 32, 0x0000000000000000ULL)
	asmRisc_set_reg_cst(cisc->ins + 58, IR_REG_YMM7_7, 32, 0x0000000000000000ULL)
	asmRisc_set_reg_cst(cisc->ins + 59, IR_REG_YMM7_8, 32, 0x0000000000000000ULL)
	asmRisc_set_reg_cst(cisc->ins + 60, IR_REG_YMM8_5, 32, 0x0000000000000000ULL)
	asmRisc_set_reg_cst(cisc->ins + 61, IR_REG_YMM8_6, 32, 0x0000000000000000ULL)
	asmRisc_set_reg_cst(cisc->ins + 62, IR_REG_YMM8_7, 32, 0x0000000000000000ULL)
	asmRisc_set_reg_cst(cisc->ins + 63, IR_REG_YMM8_8, 32, 0x0000000000000000ULL)
}
