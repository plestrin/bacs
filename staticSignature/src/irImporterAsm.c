#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "irImporterAsm.h"
#include "irBuilder.h"
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

struct memOperand{
	xed_reg_enum_t 	base;
	xed_reg_enum_t 	index;
	uint8_t 		scale;
	uint32_t 		disp;
	ADDRESS 		con_addr;
};

static struct node* memOperand_build_address(struct ir* ir, const struct memOperand* mem, uint32_t instruction_index);

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

#define ASM_OPERAND_ROLE_IS_READ(role) 					((role) & ASM_OPERAND_ROLE_READ_ALL)
#define ASM_OPERAND_ROLE_IS_WRITE(role) 				((role) & ASM_OPERAND_ROLE_WRITE_ALL)
#define ASM_OPERAND_ROLE_IS_READ_INDEX(role, index) 	((role) & (0x00000001 << ((index) & 0x0000007))) 
#define ASM_OPERAND_ROLE_IS_WRITE_INDEX(role, index) 	((role) & (0x00000100 << ((index) & 0x0000007)))

struct asmOperand{
	uint32_t 				size;
	uint32_t 				instruction_index;
	struct node*			variable;
	enum asmOperandType 	type;
	union{
		enum irRegister 	reg;
		uint64_t 			imm;
		struct memOperand	mem;
	} 						operand_type;
};

static void asmOperand_decode(struct instructionIterator* it, struct asmOperand* operand_buffer, uint8_t max_nb_operand, uint32_t selector, uint8_t* nb_operand_, const struct memAddress* mem_addr);

static void asmOperand_fetch_input(struct ir* ir, struct asmOperand* operand);
static void asmOperand_fetch_output(struct ir* ir, struct asmOperand* operand, enum irOpcode opcode);

static void asmOperand_sign_extend(struct asmOperand* operand, uint16_t size);

struct asmRiscIns{
	enum irOpcode 			opcode;
	uint8_t 				nb_input_operand;
	struct asmOperand 		input_operand[IRIMPORTERASM_MAX_INPUT_OPERAND];
	struct asmOperand 		output_operand;
};

static int32_t asmRisc_process_part1(struct ir* ir, struct asmRiscIns* risc);
static int32_t asmRisc_process_part2(struct ir* ir, struct asmRiscIns* risc);

static inline int32_t asmRisc_process(struct ir* ir, struct asmRiscIns* risc){
	if (!asmRisc_process_part1(ir, risc)){
		return asmRisc_process_part2(ir, risc);
	}
	return -1;
}

static int32_t asmRisc_process_special_cmov(struct ir* ir, struct asmRiscIns* risc);
static int32_t asmRisc_process_special_lea(struct ir* ir, struct asmRiscIns* risc);
static int32_t asmRisc_process_special_mov_part1(struct ir* ir, struct asmRiscIns* risc);
static int32_t asmRisc_process_special_mov_part2(struct ir* ir, struct asmRiscIns* risc);

static inline int32_t asmRisc_process_special_mov(struct ir* ir, struct asmRiscIns* risc){
	if (!asmRisc_process_special_mov_part1(ir, risc)){
		return asmRisc_process_special_mov_part2(ir, risc);
	}
	return -1;
}

enum ciscType{
	CISC_TYPE_INVALID,
	CISC_TYPE_SEQ,
	CISC_TYPE_PARA
};

struct asmCiscIns{
	enum ciscType 			type;
	uint8_t 				nb_ins;
	struct asmRiscIns 		ins[IRIMPORTERASM_MAX_RISC_INS];
};

#define ASMCISCINS_IS_VALID(cisc) 		((cisc).type != CISC_TYPE_INVALID)
#define ASMCISCINS_SET_INVALID(cisc) 	((cisc).type = CISC_TYPE_INVALID)

static void cisc_decode_special_call(struct instructionIterator* it, struct asmCiscIns* cisc, const struct memAddress* mem_addr);
static void cisc_decode_special_dec(struct instructionIterator* it, struct asmCiscIns* cisc, const struct memAddress* mem_addr);
static void cisc_decode_special_div(struct instructionIterator* it, struct asmCiscIns* cisc, const struct memAddress* mem_addr);
static void cisc_decode_special_inc(struct instructionIterator* it, struct asmCiscIns* cisc, const struct memAddress* mem_addr);
static void cisc_decode_special_leave(struct instructionIterator* it, struct asmCiscIns* cisc, const struct memAddress* mem_addr);
static void cisc_decode_special_movsd(struct instructionIterator* it, struct asmCiscIns* cisc, const struct memAddress* mem_addr);
static void cisc_decode_special_pop(struct instructionIterator* it, struct asmCiscIns* cisc, const struct memAddress* mem_addr);
static void cisc_decode_special_push(struct instructionIterator* it, struct asmCiscIns* cisc, const struct memAddress* mem_addr);
static void cisc_decode_special_ret(struct instructionIterator* it, struct asmCiscIns* cisc); /* attention */
static void cisc_decode_special_setxx(struct instructionIterator* it, struct asmCiscIns* cisc, const struct memAddress* mem_addr);
static void cisc_decode_special_stosd(struct instructionIterator* it, struct asmCiscIns* cisc, const struct memAddress* mem_addr);
static void cisc_decode_special_xchg(struct instructionIterator* it, struct asmCiscIns* cisc, const struct memAddress* mem_addr);

enum simdType{
	SIMD_TYPE_VARIABLE,
	SIMD_TYPE_BYTE,
	SIMD_TYPE_WORD,
	SIMD_TYPE_DWORD
};

static void simd_decode_generic(struct ir* ir, struct instructionIterator* it, struct asmCiscIns* cisc, const struct memAddress* mem_addr, enum simdType type);
static void simd_decode_generic_vex(struct ir* ir, struct instructionIterator* it, struct asmCiscIns* cisc, const struct memAddress* mem_addr, enum simdType type);

static void simd_decode_special_movsd_xmm(struct ir* ir, struct instructionIterator* it, struct asmCiscIns* cisc, const struct memAddress* mem_addr);
static void simd_decode_special_pinsrw(struct ir* ir, struct instructionIterator* it, struct asmCiscIns* cisc, const struct memAddress* mem_addr);
static void simd_decode_special_pshufd(struct ir* ir, struct instructionIterator* it, struct asmCiscIns* cisc, const struct memAddress* mem_addr);
static void simd_decode_special_pslld_psrld(struct ir* ir, struct instructionIterator* it, struct asmCiscIns* cisc, const struct memAddress* mem_addr);
static void simd_decode_special_punpckldq(struct ir* ir, struct instructionIterator* it, struct asmCiscIns* cisc, const struct memAddress* mem_addr);
static void simd_decode_special_vzeroall(struct instructionIterator* it, struct asmCiscIns* cisc);

#define irImporterAsm_get_mem_trace(trace, ins_it) (((trace) != NULL && instructionIterator_is_mem_addr_valid(ins_it)) ? ((trace)->mem_addr_buffer + instructionIterator_get_mem_addr_index(ins_it)) : NULL)

static void irImporter_handle_instruction(struct ir* ir, struct instructionIterator* it, const struct memAddress* mem_addr){
	struct asmCiscIns 	cisc;
	uint32_t 			i;
	int32_t 			error_code;

	ASMCISCINS_SET_INVALID(cisc);

	/* for stdcall and cdecl eax is caller-saved so it's safe to erase */
	if (it->instruction_index > 0 && it->prev_black_listed){
		#ifdef VERBOSE
		log_info_m("assuming stdcall/cdecl @ %u", it->instruction_index);
		#endif
		irBuilder_clear_eax_std_call(&(ir->builder));
	}

	switch(xed_decoded_inst_get_iclass(&(it->xedd))){
		case XED_ICLASS_BSWAP 		: {break;}
		case XED_ICLASS_BT 			: {break;}
		case XED_ICLASS_CALL_NEAR 	: {
			cisc_decode_special_call(it, &cisc, mem_addr);
			irBuilder_increment_call_stack(&(ir->builder))
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
		case XED_ICLASS_MOVAPS 		: {simd_decode_generic(ir, it, &cisc, mem_addr, SIMD_TYPE_VARIABLE); break;}
		case XED_ICLASS_MOVD 		: {simd_decode_generic(ir, it, &cisc, mem_addr, SIMD_TYPE_VARIABLE); break;}
		case XED_ICLASS_MOVDQA 		: {simd_decode_generic(ir, it, &cisc, mem_addr, SIMD_TYPE_VARIABLE); break;}
		case XED_ICLASS_MOVDQU 		: {simd_decode_generic(ir, it, &cisc, mem_addr, SIMD_TYPE_VARIABLE); break;}
		case XED_ICLASS_MOVQ 		: {simd_decode_generic(ir, it, &cisc, mem_addr, SIMD_TYPE_VARIABLE); break;}
		case XED_ICLASS_MOVSD 		: {
			cisc_decode_special_movsd(it, &cisc, mem_addr);
			break;
		}
		case XED_ICLASS_MOVSD_XMM 		: {
			simd_decode_special_movsd_xmm(ir, it, &cisc, mem_addr);
			break;
		}
		case XED_ICLASS_MOVUPS 		: {simd_decode_generic(ir, it, &cisc, mem_addr, SIMD_TYPE_VARIABLE); break;}
		case XED_ICLASS_NOP 		: {break;}
		case XED_ICLASS_PADDB 		: {simd_decode_generic(ir, it, &cisc, mem_addr, SIMD_TYPE_BYTE); break;}
		case XED_ICLASS_PADDD 		: {simd_decode_generic(ir, it, &cisc, mem_addr, SIMD_TYPE_DWORD); break;}
		case XED_ICLASS_PADDW 		: {simd_decode_generic(ir, it, &cisc, mem_addr, SIMD_TYPE_WORD); break;}
		case XED_ICLASS_PAND 		: {simd_decode_generic(ir, it, &cisc, mem_addr, SIMD_TYPE_VARIABLE); break;}
		case XED_ICLASS_PINSRW 		: {
			simd_decode_special_pinsrw(ir, it, &cisc, mem_addr);
			break;
		}
		case XED_ICLASS_POR 		: {simd_decode_generic(ir, it, &cisc, mem_addr, SIMD_TYPE_VARIABLE); break;}
		case XED_ICLASS_POP 		: {
			cisc_decode_special_pop(it, &cisc, mem_addr);
			break;
		}
		case XED_ICLASS_PSHUFD 		: {
			simd_decode_special_pshufd(ir, it, &cisc, mem_addr);
			break;
		}
		case XED_ICLASS_PSLLD 		: {
			simd_decode_special_pslld_psrld(ir, it, &cisc, mem_addr);
			break;
		}
		case XED_ICLASS_PSRLD 		: {
			simd_decode_special_pslld_psrld(ir, it, &cisc, mem_addr);
			break;
		}
		case XED_ICLASS_PUNPCKLDQ 	: {
			simd_decode_special_punpckldq(ir, it, &cisc, mem_addr);
			break;
		}
		case XED_ICLASS_PUSH 		: {
			cisc_decode_special_push(it, &cisc, mem_addr);
			break;
		}
		case XED_ICLASS_PXOR 		: {simd_decode_generic(ir, it, &cisc, mem_addr, SIMD_TYPE_VARIABLE); break;}
		case XED_ICLASS_RET_FAR 	: {
			cisc_decode_special_ret(it, &cisc);
			irBuilder_decrement_call_stack(&(ir->builder))
			break;
		}
		case XED_ICLASS_RET_NEAR 	: {
			cisc_decode_special_ret(it, &cisc);
			irBuilder_decrement_call_stack(&(ir->builder))
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
		case XED_ICLASS_VMOVAPS 	: {simd_decode_generic_vex(ir, it, &cisc, mem_addr, SIMD_TYPE_VARIABLE); break;}
		case XED_ICLASS_VMOVD 		: {simd_decode_generic_vex(ir, it, &cisc, mem_addr, SIMD_TYPE_VARIABLE); break;}
		case XED_ICLASS_VMOVDQA 	: {simd_decode_generic_vex(ir, it, &cisc, mem_addr, SIMD_TYPE_VARIABLE); break;}
		case XED_ICLASS_VMOVDQU 	: {simd_decode_generic_vex(ir, it, &cisc, mem_addr, SIMD_TYPE_VARIABLE); break;}
		case XED_ICLASS_VMOVQ 		: {simd_decode_generic_vex(ir, it, &cisc, mem_addr, SIMD_TYPE_VARIABLE); break;}
		case XED_ICLASS_VMOVUPS 	: {simd_decode_generic_vex(ir, it, &cisc, mem_addr, SIMD_TYPE_VARIABLE); break;}
		case XED_ICLASS_VPADDB 		: {simd_decode_generic_vex(ir, it, &cisc, mem_addr, SIMD_TYPE_BYTE); break;}
		case XED_ICLASS_VPADDD 		: {simd_decode_generic_vex(ir, it, &cisc, mem_addr, SIMD_TYPE_DWORD); break;}
		case XED_ICLASS_VPADDW 		: {simd_decode_generic_vex(ir, it, &cisc, mem_addr, SIMD_TYPE_WORD); break;}
		case XED_ICLASS_VPAND 		: {simd_decode_generic_vex(ir, it, &cisc, mem_addr, SIMD_TYPE_VARIABLE); break;}
		case XED_ICLASS_VPOR 		: {simd_decode_generic_vex(ir, it, &cisc, mem_addr, SIMD_TYPE_VARIABLE); break;}
		case XED_ICLASS_VPXOR 		: {simd_decode_generic_vex(ir, it, &cisc, mem_addr, SIMD_TYPE_VARIABLE); break;}
		case XED_ICLASS_VXORPS 		: {simd_decode_generic_vex(ir, it, &cisc, mem_addr, SIMD_TYPE_VARIABLE); break;}
		case XED_ICLASS_VZEROALL 	: {
			simd_decode_special_vzeroall(it, &cisc);
			break;
		}
		case XED_ICLASS_XORPS 		: {simd_decode_generic(ir, it, &cisc, mem_addr, SIMD_TYPE_VARIABLE); break;}
		default :{
			cisc.nb_ins = 1;
			cisc.ins[0].opcode = xedOpcode_2_irOpcode(xed_decoded_inst_get_iclass(&(it->xedd)));
			if (cisc.ins[0].opcode != IR_INVALID){
				cisc.type = CISC_TYPE_SEQ;
				asmOperand_decode(it, cisc.ins[0].input_operand, IRIMPORTERASM_MAX_INPUT_OPERAND, ASM_OPERAND_ROLE_READ_ALL, &(cisc.ins[0].nb_input_operand), mem_addr);
				asmOperand_decode(it, &(cisc.ins[0].output_operand), 1, ASM_OPERAND_ROLE_WRITE_ALL, NULL, mem_addr);
			}
			else{
				log_err_m("unable to convert instruction @ %u", it->instruction_index);
			}
		}
	}

	switch(cisc.type){
		case CISC_TYPE_INVALID 	: {break;}
		case CISC_TYPE_SEQ 		: {
			for (i = 0; i < cisc.nb_ins; i++){
				switch(cisc.ins[i].opcode){
					case IR_CMOV 	: {error_code = asmRisc_process_special_cmov(ir, cisc.ins + i); break;}
					case IR_LEA 	: {error_code = asmRisc_process_special_lea(ir, cisc.ins + i); break;}
					case IR_MOV 	: {error_code = asmRisc_process_special_mov(ir, cisc.ins + i); break;}
					default 		: {error_code = asmRisc_process(ir, cisc.ins + i); break;}
				}
				if (error_code){
					log_err_m("error code returned after processing risc %u @ %u", i, it->instruction_index);
				}
			}
			break;
		}
		case CISC_TYPE_PARA 	: {
			for (i = 0; i < cisc.nb_ins; i++){
				switch(cisc.ins[i].opcode){
					case IR_CMOV 	: {log_err("CMOV instruction is not handled yet"); break;}
					case IR_LEA 	: {log_err("LEA instruction is not handled yet"); break;}
					case IR_MOV 	: {error_code = asmRisc_process_special_mov_part1(ir, cisc.ins + i); break;}
					default 		: {error_code = asmRisc_process_part1(ir, cisc.ins + i); break;}
				}
				if (error_code){
					log_err_m("error code returned after processing PART1 risc %u @ %u", i, it->instruction_index);
					break;
				}
			}
			for ( ; i; i--){
				switch(cisc.ins[i - 1].opcode){
					case IR_CMOV 	: {break;}
					case IR_LEA 	: {break;}
					case IR_MOV 	: {error_code = asmRisc_process_special_mov_part2(ir, cisc.ins + i - 1); break;}
					default 		: {error_code = asmRisc_process_part2(ir, cisc.ins + i - 1); break;}
				}
				if (error_code){
					log_err_m("error code returned after processing PART2 risc %u @ %u", i, it->instruction_index);
				}
			}
			break;
		}
	}
}

static void irImporter_handle_irComponent(struct ir* ir, const struct irComponent* ir_component){
	if (ir_concat(ir, ir_component->ir)){
		log_err("unable to concat IR");
	}
}

int32_t irImporterAsm_import(struct ir* ir, const struct assembly* assembly, const struct memTrace* mem_trace){
	struct instructionIterator it;

	irBuilder_init(&(ir->builder));

	if (assembly_get_first_instruction(assembly, &it)){
		log_err("unable to fetch first instruction from the assembly");
		return -1;
	}

	for (; ; ){
		irImporter_handle_instruction(ir, &it, irImporterAsm_get_mem_trace(mem_trace, &it));
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

	irBuilder_tag_final_node(&(ir->builder));

	return 0;
}

int32_t irImporterAsm_import_compound(struct ir* ir, const struct assembly* assembly, const struct memTrace* mem_trace, struct irComponent** ir_component_buffer, uint32_t nb_ir_component){
	struct instructionIterator 	it;
	uint32_t 					i;

	irBuilder_init(&(ir->builder));

	if (assembly_get_first_instruction(assembly, &it)){
		log_err("unable to fetch first instruction from the assembly");
		return -1;
	}

	for (i = 0; ; ){
		if (i < nb_ir_component && instructionIterator_get_instruction_index(&it) == ir_component_buffer[i]->instruction_start){
			irImporter_handle_irComponent(ir, ir_component_buffer[i]);
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
			irImporter_handle_instruction(ir, &it, irImporterAsm_get_mem_trace(mem_trace, &it));
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

	irBuilder_tag_final_node(&(ir->builder));

	#ifdef IR_FULL_CHECK
	ir_check_memory(ir);
	#endif

	return 0;
}

static void asmOperand_decode(struct instructionIterator* it, struct asmOperand* operand_buffer, uint8_t max_nb_operand, uint32_t selector, uint8_t* nb_operand_, const struct memAddress* mem_addr){
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

					operand_buffer[nb_operand].size 			= xed_decoded_inst_get_immediate_width_bits(&(it->xedd));
					operand_buffer[nb_operand].variable 		= NULL;
					operand_buffer[nb_operand].type 			= ASM_OPERAND_IMM;
					operand_buffer[nb_operand].operand_type.imm = xed_decoded_inst_get_unsigned_immediate(&(it->xedd));

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
					
					operand_buffer[nb_operand].size 					= xed_get_register_width_bits(reg);
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
	#ifdef EXTRA_CHECK
	if (nb_operand == 0){
		log_warn("decode 0 operand");
	}
	#endif
	if (nb_operand_ != NULL){
		*nb_operand_ = nb_operand;
	}
}

static void asmOperand_fetch_input(struct ir* ir, struct asmOperand* operand){
	switch(operand->type){
		case ASM_OPERAND_IMM : {
			operand->variable = ir_add_immediate(ir, operand->size, operand->operand_type.imm);
			if (operand->variable == NULL){
				log_err("unable to add immediate to IR");
			}
			break;
		}
		case ASM_OPERAND_REG : {
			operand->variable = irBuilder_get_register_ref(&(ir->builder), ir, operand->operand_type.reg, operand->instruction_index);
			if (operand->variable == NULL){
				log_err("unable to register reference from the builder");
			}
			break;
		}
		case ASM_OPERAND_MEM : {
			struct node* address;

			address = memOperand_build_address(ir, &(operand->operand_type.mem), operand->instruction_index);
			if (address != NULL){
				operand->variable = ir_add_in_mem_(ir, operand->instruction_index, operand->size, address, irBuilder_get_mem_order(&(ir->builder)), operand->operand_type.mem.con_addr);
				if (operand->variable == NULL){
					log_err("unable to add memory load to IR");
				}
				else{
					irBuilder_set_mem_order(&(ir->builder), operand->variable);
				}
			}
			else{
				log_err("unable to build memory address");
			}
			break;
		}
	}
}

static void asmOperand_fetch_output(struct ir* ir, struct asmOperand* operand, enum irOpcode opcode){
	switch(operand->type){
		case ASM_OPERAND_REG 	: {
			operand->variable = ir_add_inst(ir, operand->instruction_index, operand->size, opcode, irBuilder_get_call_id(&(ir->builder)));
			if (operand->variable == NULL){
				log_err("unable to add operation to IR");
			}
			else{
				irBuilder_set_register_ref(&(ir->builder), operand->operand_type.reg, operand->variable);
			}
			break;
		}
		case ASM_OPERAND_MEM 	: {
			struct node* address;
			struct node* mem_write;

			address = memOperand_build_address(ir, &(operand->operand_type.mem), operand->instruction_index);
			if (address != NULL){
				operand->variable = ir_add_inst(ir, operand->instruction_index, operand->size, opcode, irBuilder_get_call_id(&(ir->builder)));
				if (operand->variable != NULL){
					mem_write = ir_add_out_mem_(ir, operand->instruction_index, operand->size, address, irBuilder_get_mem_order(&(ir->builder)), operand->operand_type.mem.con_addr);
					if (mem_write != NULL){
						irBuilder_set_mem_order(&(ir->builder), mem_write);
						if (ir_add_dependence(ir, operand->variable, mem_write, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
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

static struct node* memOperand_build_address(struct ir* ir, const struct memOperand* mem, uint32_t instruction_index){
	struct node* 	base 		= NULL;
	struct node* 	index 		= NULL;
	struct node* 	disp 		= NULL;
	struct node* 	scale 		= NULL;
	uint8_t 		nb_operand 	= 0;
	struct node* 	operands[3];
	struct node* 	address 	= NULL;

	if (mem->base != XED_REG_INVALID){
		base = irBuilder_get_std_register_ref(&(ir->builder), ir, xedRegister_2_irRegister(mem->base), instruction_index);
		if (base == NULL){
			log_err("unable to get register reference from the builder");
		}
		else{
			operands[nb_operand] = base;
			nb_operand ++;
		}
	}

	if (mem->scale > 1){
		scale = ir_add_immediate(ir, 8, __builtin_ffs(mem->scale) - 1);
		if (scale == NULL){
			log_err("unable to add immediate to IR");
		}
	}

	if (mem->index != XED_REG_INVALID){
		index = irBuilder_get_std_register_ref(&(ir->builder), ir, xedRegister_2_irRegister(mem->index), instruction_index);
		if (index == NULL){
			log_err("unable to get register reference from the builder");
		}
		else{
			if (scale == NULL){
				operands[nb_operand] = index;
				nb_operand ++;
			}
			else{
				struct node* shl;

				shl = ir_add_inst(ir, IR_OPERATION_INDEX_ADDRESS, 32, IR_SHL, irBuilder_get_call_id(&(ir->builder)));
				if (shl != NULL){
					if (ir_add_dependence(ir, index, shl, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
						log_err("unable to add dependence between IR nodes");
					}
					if (ir_add_dependence(ir, scale, shl, IR_DEPENDENCE_TYPE_SHIFT_DISP) == NULL){
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
		disp = ir_add_immediate(ir, 32, mem->disp);
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
			address = ir_add_inst(ir, IR_OPERATION_INDEX_ADDRESS, 32, IR_ADD, irBuilder_get_call_id(&(ir->builder)));
			if (address != NULL){
				if (ir_add_dependence(ir, operands[0], address, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
					log_err("unable to add dependence between IR nodes");
				}
				if (ir_add_dependence(ir, operands[1], address, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
					log_err("unable to add dependence between IR nodes");
				}
			}
			else{
				log_err("unable to add operation to IR");
			}
			break;
		}
		case 3 : {
			address = ir_add_inst(ir, IR_OPERATION_INDEX_ADDRESS, 32, IR_ADD, irBuilder_get_call_id(&(ir->builder)));
			if (address != NULL){
				if (ir_add_dependence(ir, operands[0], address, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
					log_err("unable to add dependence between IR nodes");
				}
				if (ir_add_dependence(ir, operands[1], address, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
					log_err("unable to add dependence between IR nodes");
				}
				if (ir_add_dependence(ir, operands[2], address, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
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

static int32_t asmRisc_process_part1(struct ir* ir, struct asmRiscIns* risc){
	uint8_t i;

	if (sign_extand_table[risc->opcode]){
		for (i = 0; i < risc->nb_input_operand; i++){
			asmOperand_sign_extend(risc->input_operand + i, risc->output_operand.size);
		}
	}

	for (i = 0; i < risc->nb_input_operand; i++){
		asmOperand_fetch_input(ir, risc->input_operand + i);
	}

	return 0;
}

static int32_t asmRisc_process_part2(struct ir* ir, struct asmRiscIns* risc){
	uint8_t i;
	int32_t error_code = 0;

	asmOperand_fetch_output(ir, &(risc->output_operand), risc->opcode);

	if (risc->output_operand.variable != NULL){
		for (i = 0; i < risc->nb_input_operand; i++){
			if (risc->input_operand[i].variable != NULL){
				if (ir_add_dependence(ir, risc->input_operand[i].variable, risc->output_operand.variable, dependence_label_table[risc->opcode][i]) == NULL){
					log_err("unable to add output to add dependence to IR");
					error_code = -1;
				}
			}
		}
	}

	return error_code;
}

static int32_t asmRisc_process_special_cmov(struct ir* ir, struct asmRiscIns* risc){
	int32_t error_code = 0;

	log_warn("using CMOV glue instruction since runtime behavior is unknown");

	if (risc->nb_input_operand == 1){
		memcpy(risc->input_operand + 1, &(risc->output_operand), sizeof(struct asmOperand));
		risc->nb_input_operand ++;
	}
	#ifdef EXTRA_CHECK
	if (risc->nb_input_operand != 2){
		log_err("incorrect number of input operand");
		return -1;
	}
	#endif

	asmOperand_fetch_input(ir, risc->input_operand + 0);
	asmOperand_fetch_input(ir, risc->input_operand + 1);

	asmOperand_fetch_output(ir, &(risc->output_operand), risc->opcode);

	if (risc->output_operand.variable == NULL){
		log_err("unable to fetch output operand");
		return -1;
	}

	if (risc->input_operand[0].variable != NULL){
		if (ir_add_dependence(ir, risc->input_operand[0].variable, risc->output_operand.variable, dependence_label_table[risc->opcode][0]) == NULL){
			log_err("unable to add output to add dependence to IR");
			error_code = -1;
		}
	}
	else{
		log_err("unable to fetch first input operand");
		error_code = -1;
	}

	if (risc->input_operand[1].variable != NULL){
		if (ir_add_dependence(ir, risc->input_operand[1].variable, risc->output_operand.variable, dependence_label_table[risc->opcode][1]) == NULL){
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

static int32_t asmRisc_process_special_lea(struct ir* ir, struct asmRiscIns* risc){
	struct node* 	address;
	struct node* 	mem_write;
	int32_t 		error_code = 0;

	#ifdef EXTRA_CHECK
	if (risc->nb_input_operand != 1 || risc->input_operand[0].type != ASM_OPERAND_MEM){
		log_err("incorrect type or number of input operand");
		return -1;
	}
	#endif

	risc->input_operand[0].variable = memOperand_build_address(ir, &(risc->input_operand[0].operand_type.mem), risc->input_operand[0].instruction_index);
	if (risc->input_operand[0].variable == NULL){
		log_err("unable to build memory address");
		return -1;
	}

	switch(risc->output_operand.type){
		case ASM_OPERAND_REG 	: {
			irBuilder_set_std_register_ref(&(ir->builder), risc->output_operand.operand_type.reg, risc->input_operand[0].variable);
			break;
		}
		case ASM_OPERAND_MEM 	: {
			address = memOperand_build_address(ir, &(risc->output_operand.operand_type.mem), risc->output_operand.instruction_index);
			if (address != NULL){
				mem_write = ir_add_out_mem_(ir, risc->output_operand.instruction_index, risc->output_operand.size, address, irBuilder_get_mem_order(&(ir->builder)), risc->output_operand.operand_type.mem.con_addr);
				if (mem_write != NULL){
					irBuilder_set_mem_order(&(ir->builder), mem_write);
					if (ir_add_dependence(ir, risc->input_operand[0].variable, mem_write, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
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

static int32_t asmRisc_process_special_mov_part1(struct ir* ir, struct asmRiscIns* risc){
	#ifdef EXTRA_CHECK
	if (risc->nb_input_operand != 1){
		log_err("incorrect number of input operand");
		return -1;
	}
	#endif

	asmOperand_fetch_input(ir, risc->input_operand);
	if (risc->input_operand[0].variable == NULL){
		log_err("input variable is NULL");
		return -1;
	}

	return 0;
}

static int32_t asmRisc_process_special_mov_part2(struct ir* ir, struct asmRiscIns* risc){
	struct node* 	address;
	struct node* 	mem_write;
	int32_t 		error_code = 0;

	switch(risc->output_operand.type){
		case ASM_OPERAND_REG 	: {
			irBuilder_set_register_ref(&(ir->builder), risc->output_operand.operand_type.reg, risc->input_operand[0].variable);
			break;
		}
		case ASM_OPERAND_MEM 	: {
			address = memOperand_build_address(ir, &(risc->output_operand.operand_type.mem), risc->output_operand.instruction_index);
			if (address != NULL){
				mem_write = ir_add_out_mem_(ir, risc->output_operand.instruction_index, risc->output_operand.size, address, irBuilder_get_mem_order(&(ir->builder)), risc->output_operand.operand_type.mem.con_addr);
				if (mem_write != NULL){
					irBuilder_set_mem_order(&(ir->builder), mem_write);
					if (ir_add_dependence(ir, risc->input_operand[0].variable, mem_write, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
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
		case XED_ICLASS_MOVD 		: {return IR_MOV;}
		case XED_ICLASS_MOVDQA 		: {return IR_MOV;}
		case XED_ICLASS_MOVDQU 		: {return IR_MOV;}
		case XED_ICLASS_MOVQ 		: {return IR_MOV;}
		case XED_ICLASS_MOVUPS 		: {return IR_MOV;}
		case XED_ICLASS_MOVZX 		: {return IR_MOVZX;}
		case XED_ICLASS_MUL 		: {return IR_MUL;}
		case XED_ICLASS_NEG 		: {return IR_NEG;}
		case XED_ICLASS_NOT 		: {return IR_NOT;}
		case XED_ICLASS_OR 			: {return IR_OR;}
		case XED_ICLASS_PADDB 		: {return IR_ADD;}
		case XED_ICLASS_PADDD 		: {return IR_ADD;}
		case XED_ICLASS_PADDW 		: {return IR_ADD;}
		case XED_ICLASS_PAND 		: {return IR_AND;}
		case XED_ICLASS_POR 		: {return IR_OR;}
		case XED_ICLASS_PSLLD 		: {return IR_SHL;}
		case XED_ICLASS_PSRLD 		: {return IR_SHR;}
		case XED_ICLASS_PXOR 		: {return IR_XOR;}
		case XED_ICLASS_ROL 		: {return IR_ROL;}
		case XED_ICLASS_ROR 		: {return IR_ROR;}
		case XED_ICLASS_SHL 		: {return IR_SHL;}
		case XED_ICLASS_SHLD 		: {return IR_SHLD;}
		case XED_ICLASS_SHR 		: {return IR_SHR;}
		case XED_ICLASS_SHRD 		: {return IR_SHRD;}
		case XED_ICLASS_SUB 		: {return IR_SUB;}
		case XED_ICLASS_VMOVAPS 	: {return IR_MOV;}
		case XED_ICLASS_VMOVD 		: {return IR_MOV;}
		case XED_ICLASS_VMOVDQA 	: {return IR_MOV;}
		case XED_ICLASS_VMOVDQU 	: {return IR_MOV;}
		case XED_ICLASS_VMOVQ 		: {return IR_MOV;}
		case XED_ICLASS_VMOVUPS 	: {return IR_MOV;}
		case XED_ICLASS_VPADDD 		: {return IR_ADD;}
		case XED_ICLASS_VPAND 		: {return IR_AND;}
		case XED_ICLASS_VPOR 		: {return IR_OR;}
		case XED_ICLASS_VPXOR 		: {return IR_XOR;}
		case XED_ICLASS_VXORPS  	: {return IR_XOR;}
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
		case XED_REG_MMX0 	: {return IR_VREG_MMX1;}
		case XED_REG_MMX1 	: {return IR_VREG_MMX2;}
		case XED_REG_MMX2 	: {return IR_VREG_MMX3;}
		case XED_REG_MMX3 	: {return IR_VREG_MMX4;}
		case XED_REG_MMX4 	: {return IR_VREG_MMX5;}
		case XED_REG_MMX5 	: {return IR_VREG_MMX6;}
		case XED_REG_MMX6 	: {return IR_VREG_MMX7;}
		case XED_REG_MMX7 	: {return IR_VREG_MMX8;}
		case XED_REG_XMM0 	: {return IR_VREG_XMM1;}
		case XED_REG_XMM1 	: {return IR_VREG_XMM2;}
		case XED_REG_XMM2 	: {return IR_VREG_XMM3;}
		case XED_REG_XMM3 	: {return IR_VREG_XMM4;}
		case XED_REG_XMM4 	: {return IR_VREG_XMM5;}
		case XED_REG_XMM5 	: {return IR_VREG_XMM6;}
		case XED_REG_XMM6 	: {return IR_VREG_XMM7;}
		case XED_REG_XMM7 	: {return IR_VREG_XMM8;}
		case XED_REG_YMM0 	: {return IR_VREG_YMM1;}
		case XED_REG_YMM1 	: {return IR_VREG_YMM2;}
		case XED_REG_YMM2 	: {return IR_VREG_YMM3;}
		case XED_REG_YMM3 	: {return IR_VREG_YMM4;}
		case XED_REG_YMM4 	: {return IR_VREG_YMM5;}
		case XED_REG_YMM5 	: {return IR_VREG_YMM6;}
		case XED_REG_YMM6 	: {return IR_VREG_YMM7;}
		case XED_REG_YMM7 	: {return IR_VREG_YMM8;}
		default : {
			log_err_m("this register (%s) cannot be translated into IR register", xed_reg_enum_t2str(xed_reg));
			return IR_REG_EAX;
		}
	}
}

#define asmOperand_set_imm(operand, size_, imm_) 											\
	(operand).size 				= size_; 													\
	(operand).variable 			= NULL; 													\
	(operand).type 				= ASM_OPERAND_IMM; 											\
	(operand).operand_type.imm 	= imm_;

#define asmRisc_set_reg_cst(risc, reg_, size_, imm_) 										\
	{ 																						\
		(risc)->opcode 								= IR_MOV; 								\
		(risc)->nb_input_operand 					= 1; 									\
		asmOperand_set_imm((risc)->input_operand[0], size_, imm_)							\
		(risc)->output_operand.size 				= (size_); 								\
		(risc)->output_operand.instruction_index 	= it->instruction_index; 				\
		(risc)->output_operand.variable 			= NULL; 								\
		(risc)->output_operand.type 				= ASM_OPERAND_REG; 						\
		(risc)->output_operand.operand_type.reg 	= (reg_); 								\
	}

#define asmOperand_copy(operand_dst, operand_src) memcpy(operand_dst, operand_src, sizeof(struct asmOperand))

static void cisc_decode_special_call(struct instructionIterator* it, struct asmCiscIns* cisc, const struct memAddress* mem_addr){
	cisc->type 												= CISC_TYPE_SEQ;
	cisc->nb_ins 											= 2;

	cisc->ins[0].opcode 									= IR_SUB;
	cisc->ins[0].nb_input_operand 							= 2;
	cisc->ins[0].input_operand[0].size 						= 32;
	cisc->ins[0].input_operand[0].instruction_index 		= it->instruction_index;
	cisc->ins[0].input_operand[0].variable 					= NULL;
	cisc->ins[0].input_operand[0].type 						= ASM_OPERAND_REG;
	cisc->ins[0].input_operand[0].operand_type.reg 			= IR_REG_ESP;
	asmOperand_set_imm(cisc->ins[0].input_operand[1], 32, 4)
	cisc->ins[0].output_operand.size 						= 32;
	cisc->ins[0].output_operand.instruction_index 			= it->instruction_index;
	cisc->ins[0].output_operand.variable 					= NULL;
	cisc->ins[0].output_operand.type 						= ASM_OPERAND_REG;
	cisc->ins[0].output_operand.operand_type.reg 			= IR_REG_ESP;

	cisc->ins[1].opcode 									= IR_MOV;
	cisc->ins[1].nb_input_operand 							= 1;
	asmOperand_set_imm(cisc->ins[1].input_operand[0], 32, it->instruction_address)
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

static void cisc_decode_special_dec(struct instructionIterator* it, struct asmCiscIns* cisc, const struct memAddress* mem_addr){
	cisc->type 												= CISC_TYPE_SEQ;
	cisc->nb_ins 											= 1;

	cisc->ins[0].opcode 									= IR_SUB;
	cisc->ins[0].nb_input_operand 							= 2;
	asmOperand_decode(it, cisc->ins[0].input_operand, 1, ASM_OPERAND_ROLE_READ_1, NULL, mem_addr);
	asmOperand_set_imm(cisc->ins[0].input_operand[1], cisc->ins[0].input_operand[0].size, 1)
	asmOperand_decode(it, &(cisc->ins[0].output_operand), 1, ASM_OPERAND_ROLE_WRITE_1, NULL, mem_addr);
}

static void cisc_decode_special_div(struct instructionIterator* it, struct asmCiscIns* cisc, const struct memAddress* mem_addr){
	cisc->type 												= CISC_TYPE_SEQ;
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

static void cisc_decode_special_inc(struct instructionIterator* it, struct asmCiscIns* cisc, const struct memAddress* mem_addr){
	cisc->type 												= CISC_TYPE_SEQ;
	cisc->nb_ins 											= 1;

	cisc->ins[0].opcode 									= IR_ADD;
	cisc->ins[0].nb_input_operand 							= 2;
	asmOperand_decode(it, cisc->ins[0].input_operand, 1, ASM_OPERAND_ROLE_READ_1, NULL, mem_addr);
	asmOperand_set_imm(cisc->ins[0].input_operand[1], cisc->ins[0].input_operand[0].size, 1)
	asmOperand_decode(it, &(cisc->ins[0].output_operand), 1, ASM_OPERAND_ROLE_WRITE_1, NULL, mem_addr);
}

static void cisc_decode_special_leave(struct instructionIterator* it, struct asmCiscIns* cisc, const struct memAddress* mem_addr){
	cisc->type 											= CISC_TYPE_SEQ;
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
	asmOperand_set_imm(cisc->ins[2].input_operand[1], 32, 4)
	cisc->ins[2].output_operand.size 						= 32;
	cisc->ins[2].output_operand.instruction_index 			= it->instruction_index;
	cisc->ins[2].output_operand.variable 					= NULL;
	cisc->ins[2].output_operand.type 						= ASM_OPERAND_REG;
	cisc->ins[2].output_operand.operand_type.reg 			= IR_REG_ESP;
}

static void cisc_decode_special_movsd(struct instructionIterator* it, struct asmCiscIns* cisc, const struct memAddress* mem_addr){
	log_warn("unknown value for the direction flag");

	cisc->type 												= CISC_TYPE_SEQ;
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
	asmOperand_set_imm(cisc->ins[1].input_operand[0], 32, 4)
	asmOperand_set_imm(cisc->ins[1].input_operand[1], 32, 0xfffffffffffffffcULL)
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

static void cisc_decode_special_pop(struct instructionIterator* it, struct asmCiscIns* cisc, const struct memAddress* mem_addr){
	cisc->type 												= CISC_TYPE_SEQ;
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
	asmOperand_set_imm(cisc->ins[1].input_operand[1], 32, cisc->ins[0].output_operand.size / 8)
	cisc->ins[1].output_operand.size 						= 32;
	cisc->ins[1].output_operand.instruction_index 			= it->instruction_index;
	cisc->ins[1].output_operand.variable 					= NULL;
	cisc->ins[1].output_operand.type 						= ASM_OPERAND_REG;
	cisc->ins[1].output_operand.operand_type.reg 			= IR_REG_ESP;
}

static void cisc_decode_special_push(struct instructionIterator* it, struct asmCiscIns* cisc, const struct memAddress* mem_addr){
	cisc->type 												= CISC_TYPE_SEQ;
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
	asmOperand_set_imm(cisc->ins[1].input_operand[1], 32, cisc->ins[0].input_operand[0].size / 8)
	cisc->ins[1].output_operand.size 						= 32;
	cisc->ins[1].output_operand.instruction_index 			= it->instruction_index;
	cisc->ins[1].output_operand.variable 					= NULL;
	cisc->ins[1].output_operand.type 						= ASM_OPERAND_REG;
	cisc->ins[1].output_operand.operand_type.reg 			= IR_REG_ESP;
}

static void cisc_decode_special_setxx(struct instructionIterator* it, struct asmCiscIns* cisc, const struct memAddress* mem_addr){
	cisc->type 												= CISC_TYPE_SEQ;
	cisc->nb_ins 											= 1;

	cisc->ins[0].opcode 									= IR_CMOV;
	cisc->ins[0].nb_input_operand 							= 2;
	asmOperand_set_imm(cisc->ins[0].input_operand[0], 8, 0)
	asmOperand_set_imm(cisc->ins[0].input_operand[1], 8, 1)
	asmOperand_decode(it, &(cisc->ins[0].output_operand), 1, ASM_OPERAND_ROLE_WRITE_1, NULL, mem_addr);
}

static void cisc_decode_special_stosd(struct instructionIterator* it, struct asmCiscIns* cisc, const struct memAddress* mem_addr){
	log_warn("unknown value for the direction flag");

	cisc->type 												= CISC_TYPE_SEQ;
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
	asmOperand_set_imm(cisc->ins[1].input_operand[0], 32, 4)
	asmOperand_set_imm(cisc->ins[1].input_operand[1], 32, 0xfffffffffffffffcULL)
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

static void cisc_decode_special_ret(struct instructionIterator* it, struct asmCiscIns* cisc){
	cisc->type 												= CISC_TYPE_SEQ;
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
		cisc->ins[0].input_operand[1].operand_type.imm 		= 4 + ((uint64_t)xed_decoded_inst_get_byte(&(it->xedd), 2) << 8) + xed_decoded_inst_get_byte(&(it->xedd), 1);
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

static void cisc_decode_special_xchg(struct instructionIterator* it, struct asmCiscIns* cisc, const struct memAddress* mem_addr){
	cisc->type 												= CISC_TYPE_SEQ;
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
	asmOperand_copy(&(cisc->ins[1].output_operand), cisc->ins[0].input_operand);

	cisc->ins[2].opcode 									= IR_MOV;
	cisc->ins[2].nb_input_operand 							= 1;
	cisc->ins[2].input_operand[0].size 						= cisc->ins[0].input_operand[0].size;
	cisc->ins[2].input_operand[0].instruction_index 		= it->instruction_index;
	cisc->ins[2].input_operand[0].variable 					= NULL;
	cisc->ins[2].input_operand[0].type 						= ASM_OPERAND_REG;
	cisc->ins[2].input_operand[0].operand_type.reg 			= IR_REG_TMP;
	asmOperand_copy(&(cisc->ins[2].output_operand), cisc->ins[1].input_operand);
}

static uint32_t asmOperand_get_min_frag_size(const struct irBuilder* builder, const struct asmOperand* operand_buffer, uint32_t nb_operand){
	uint32_t i;
	uint32_t min_frag_size = SIMD_DEFAULT_FRAG_SIZE;

	for (i = 0; i < nb_operand; i++){
		if (operand_buffer[i].type == ASM_OPERAND_REG && !irRegister_is_std(operand_buffer[i].operand_type.reg)){
			uint32_t current_frag_size;

			current_frag_size = irBuilder_get_vir_register_frag_size(builder, operand_buffer[i].operand_type.reg);
			min_frag_size = min(min_frag_size, current_frag_size);
		}
	}

	return min_frag_size;
}

static void asmOperand_frag(struct asmOperand* op_dst, const struct asmOperand* op_src, uint32_t frag_size, uint32_t frag_index){
	#ifdef EXTRA_CHECK
	if (frag_size != 8 && frag_size != 16 && frag_size != 32){
		log_err_m("incorrect frag size: %u", frag_size);
	}
	if (frag_index >= 2*IR_VIR_REGISTER_NB_FRAG_MAX){
		log_err_m("incorrect frag index: %u", frag_index);
	}
	#endif

	op_dst->size 		= frag_size;
	op_dst->variable 	= NULL;
	op_dst->type 		= op_src->type;

	switch(op_src->type){
		case ASM_OPERAND_IMM : {
			op_dst->operand_type.imm 			= op_src->operand_type.imm >> (frag_index * frag_size);
			break;
		}
		case ASM_OPERAND_REG : {
			if (irRegister_is_virtual(op_src->operand_type.reg)){
				op_dst->instruction_index 		= op_src->instruction_index;
				op_dst->operand_type.reg 		= irRegister_virtual_get_simd(op_src->operand_type.reg, frag_size, frag_index);
			}
			else if (!irRegister_is_std(op_src->operand_type.reg)){
				op_dst->instruction_index 		= op_src->instruction_index;
				op_dst->operand_type.reg 		= irRegister_simd_frag(op_src->operand_type.reg, frag_size, frag_index);
			}
			else if (op_src->size == frag_size){
				asmOperand_copy(op_dst, op_src);
			}
			else{
				log_err_m("trying to frag register %s (fragmentation is designed for SIMD register)", irRegister_2_string(op_src->operand_type.reg));
			}
			break;
		}
		case ASM_OPERAND_MEM : {
			op_dst->instruction_index 			= op_src->instruction_index;
			op_dst->operand_type.mem.base 		= op_src->operand_type.mem.base;
			op_dst->operand_type.mem.index 		= op_src->operand_type.mem.index;
			op_dst->operand_type.mem.scale 		= op_src->operand_type.mem.scale;
			op_dst->operand_type.mem.disp 		= op_src->operand_type.mem.disp + (frag_size / 8) * frag_index;
			op_dst->operand_type.mem.con_addr 	= (op_src->operand_type.mem.con_addr == MEMADDRESS_INVALID) ? MEMADDRESS_INVALID : (op_src->operand_type.mem.con_addr + (frag_size / 8) * frag_index);
			break;
		}
	}
}

static uint32_t asmSimd_frag_all(struct asmRiscIns* risc_dst, uint32_t nb_risc, const struct asmRiscIns* simd, uint32_t frag_size){
	uint32_t i;
	uint32_t j;

	#ifdef EXTRA_CHECK
	if (simd->output_operand.size % frag_size){
		log_err_m("the size of the output operand %u is not a multiple of the frag size %u", simd->output_operand.size, frag_size);
	}
	#endif

	for (i = 0; i < nb_risc; i++){
		risc_dst[i].opcode 				= simd->opcode;
		risc_dst[i].nb_input_operand 	= simd->nb_input_operand;
		for (j = 0; j < simd->nb_input_operand; j++){
			if (simd->input_operand[j].size >= (i + 1) * frag_size){
				asmOperand_frag(risc_dst[i].input_operand + j, simd->input_operand + j, frag_size, i);
			}
			else{
				asmOperand_set_imm(risc_dst[i].input_operand[j], frag_size, 0)
			}
		}
		asmOperand_frag(&(risc_dst[i].output_operand), &(simd->output_operand), frag_size, i);
		if ((i + 1) * frag_size >= simd->output_operand.size){
			break;
		}
	}

	if (i == nb_risc){
		log_err("the maximum of risc instruction(s) has been reached, SIMD frag is likely to be incomplete");
	}
	else{
		i ++;
	}

	return i;
}

static uint32_t asmSimd_frag_first(struct asmRiscIns* risc_dst, uint32_t nb_risc, const struct asmRiscIns* simd, uint32_t frag_size){
	uint32_t i;

	#ifdef EXTRA_CHECK
	if (simd->nb_input_operand < 2){
		log_err("this function expects a SIMD instruction with at least two inputs");
	}
	if (simd->output_operand.size != simd->input_operand[0].size){
		log_err("this function expects a SIMD instruction with I1 and O1 of the same size");
	}
	#endif

	for (i = 0; i < nb_risc; i++){
		risc_dst[i].opcode 				= simd->opcode;
		risc_dst[i].nb_input_operand 	= simd->nb_input_operand;
		asmOperand_frag(risc_dst[i].input_operand, simd->input_operand, frag_size, i);
		memcpy(risc_dst[i].input_operand + 1, simd->input_operand + 1, sizeof(struct asmOperand) * simd->nb_input_operand - 1);
		asmOperand_frag(&(risc_dst[i].output_operand), &(simd->output_operand), frag_size, i);
		if ((i + 1) * frag_size >= simd->output_operand.size){
			break;
		}
	}

	if (i == nb_risc){
		log_err("the maximum of risc instruction(s) has been reached, SIMD frag is likely to be incomplete");
	}
	else{
		i ++;
	}

	return i;
}

static void simd_decode_generic(struct ir* ir, struct instructionIterator* it, struct asmCiscIns* cisc, const struct memAddress* mem_addr, enum simdType type){
	struct asmRiscIns 	local_simd;
	uint32_t 			frag_size = SIMD_DEFAULT_FRAG_SIZE;

	local_simd.opcode = xedOpcode_2_irOpcode(xed_decoded_inst_get_iclass(&(it->xedd)));
	if (local_simd.opcode != IR_INVALID){
		asmOperand_decode(it, local_simd.input_operand, IRIMPORTERASM_MAX_INPUT_OPERAND, ASM_OPERAND_ROLE_READ_ALL, &(local_simd.nb_input_operand), mem_addr);
		asmOperand_decode(it, &(local_simd.output_operand), 1, ASM_OPERAND_ROLE_WRITE_ALL, NULL, mem_addr);

		switch(type){
			case SIMD_TYPE_VARIABLE : {
				frag_size = asmOperand_get_min_frag_size(&(ir->builder), local_simd.input_operand, local_simd.nb_input_operand);
				break;
			}
			case SIMD_TYPE_BYTE 	: {
				frag_size = 8;
				break;
			}
			case SIMD_TYPE_WORD 	: {
				frag_size = 16;
				break;
			}
			case SIMD_TYPE_DWORD 	: {
				frag_size = 32;
				break;
			}
		}

		cisc->type 		= CISC_TYPE_PARA;
		cisc->nb_ins 	= asmSimd_frag_all(cisc->ins, IRIMPORTERASM_MAX_RISC_INS, &local_simd, frag_size);
	}
	else{
		log_err_m("unable to convert instruction @ %u", it->instruction_index);
	}
}

static void simd_decode_generic_vex(struct ir* ir, struct instructionIterator* it, struct asmCiscIns* cisc, const struct memAddress* mem_addr, enum simdType type){
	uint32_t reg_dst_index;
	uint32_t frag_dst_size;
	uint32_t i;

	simd_decode_generic(ir, it, cisc, mem_addr, type);

	if (ASMCISCINS_IS_VALID(*cisc)){
		if (xed_decoded_inst_vector_length_bits(&(it->xedd)) == 128){
			if (cisc->ins[0].output_operand.type == ASM_OPERAND_REG){
				reg_dst_index = irRegister_simd_get_index(cisc->ins[0].output_operand.operand_type.reg);
				frag_dst_size = irRegister_simd_get_size(cisc->ins[0].output_operand.operand_type.reg);

				for (i = cisc->nb_ins * frag_dst_size; i < 256; i += frag_dst_size){
					if (cisc->nb_ins == IRIMPORTERASM_MAX_RISC_INS){
						log_err("the maximum of RISC instruction(s) has been reached, SIMD frag is likely to be incomplete");
						break;
					}
					if (i < 128){
						asmRisc_set_reg_cst(cisc->ins + cisc->nb_ins, IR_REGISTER_SIMD_XMM | (reg_dst_index << 12) | (frag_dst_size << 5) | (i / frag_dst_size), frag_dst_size, 0)
					}
					else{
						asmRisc_set_reg_cst(cisc->ins + cisc->nb_ins, IR_REGISTER_SIMD_YMM | (reg_dst_index << 12) | (frag_dst_size << 5) | ((i - 128) / frag_dst_size), frag_dst_size, 0)
					}
					cisc->nb_ins ++;
				}
			}
		}
	}
}

static void simd_decode_special_movsd_xmm(struct ir* ir, struct instructionIterator* it, struct asmCiscIns* cisc, const struct memAddress* mem_addr){
	struct asmRiscIns 	local_simd;
	uint32_t 			frag_size;

	local_simd.opcode = IR_MOV;
	asmOperand_decode(it, local_simd.input_operand, IRIMPORTERASM_MAX_INPUT_OPERAND, ASM_OPERAND_ROLE_READ_ALL, &(local_simd.nb_input_operand), mem_addr);
	frag_size = asmOperand_get_min_frag_size(&(ir->builder), local_simd.input_operand, local_simd.nb_input_operand);

	asmOperand_decode(it, &(local_simd.output_operand), 1, ASM_OPERAND_ROLE_WRITE_ALL, NULL, mem_addr);

	#ifdef EXTRA_CHECK
	if (local_simd.nb_input_operand != 0){
		log_err_m("incorrect MOVSD_XMM format (%u input arg(s))", local_simd.nb_input_operand);
		return;
	}
	#endif

	if (local_simd.input_operand[0].type == ASM_OPERAND_REG && local_simd.output_operand.type == ASM_OPERAND_REG){
		local_simd.input_operand[0].size = 64;
		local_simd.output_operand.size = 64;
	}

	cisc->type 		= CISC_TYPE_PARA;
	cisc->nb_ins 	= asmSimd_frag_all(cisc->ins, IRIMPORTERASM_MAX_RISC_INS, &local_simd, frag_size);
}

static void simd_decode_special_pinsrw(struct ir* ir, struct instructionIterator* it, struct asmCiscIns* cisc, const struct memAddress* mem_addr){
	struct asmOperand 	operand_buffer[3];
	uint8_t 			nb_operand;

	asmOperand_decode(it, operand_buffer, 3, ASM_OPERAND_ROLE_READ_ALL, &nb_operand, mem_addr);

	#ifdef EXTRA_CHECK
	if (nb_operand != 3){
		log_err_m("incorrect PINSRW format (%u input arg(s))", nb_operand);
		return;
	}
	if (operand_buffer[0].type != ASM_OPERAND_REG){
		log_err("incorrect PINSRW format (first operand is not REG)");
		return;
	}
	if (operand_buffer[2].type != ASM_OPERAND_IMM){
		log_err("incorrect PINSRW format (last operand is not IMM)");
		return;
	}
	#endif

	if (operand_buffer[0].size == 64){
		operand_buffer[2].operand_type.imm &= 0x0000000000000003ULL;
	}
	else{
		operand_buffer[2].operand_type.imm &= 0x0000000000000007ULL;
	}

	switch(irBuilder_get_vir_register_frag_size(&(ir->builder), operand_buffer[0].operand_type.reg)){
		case 8  : {
			cisc->nb_ins 										= 2;
			cisc->type 											= CISC_TYPE_SEQ;
			cisc->ins[0].opcode 								= IR_PART1_8;
			cisc->ins[0].nb_input_operand 						= 1;
			asmOperand_copy(cisc->ins[0].input_operand, operand_buffer + 1);
			cisc->ins[0].output_operand.size 					= 8;
			cisc->ins[0].output_operand.instruction_index 		= it->instruction_index;
			cisc->ins[0].output_operand.variable 				= NULL;
			cisc->ins[0].output_operand.type 					= ASM_OPERAND_REG;
			cisc->ins[0].output_operand.operand_type.reg 		= irRegister_virtual_get_simd(operand_buffer[0].operand_type.reg, 8, 2 * operand_buffer[2].operand_type.imm + 0);
			
			cisc->ins[1].opcode 								= IR_PART2_8;
			cisc->ins[1].nb_input_operand 						= 1;
			asmOperand_copy(cisc->ins[1].input_operand, operand_buffer + 1);
			cisc->ins[1].output_operand.size 					= 8;
			cisc->ins[1].output_operand.instruction_index 		= it->instruction_index;
			cisc->ins[1].output_operand.variable 				= NULL;
			cisc->ins[1].output_operand.type 					= ASM_OPERAND_REG;
			cisc->ins[1].output_operand.operand_type.reg 		= irRegister_virtual_get_simd(operand_buffer[0].operand_type.reg, 8, 2 * operand_buffer[2].operand_type.imm + 1);
			break;
		}
		case 16 : {
			cisc->nb_ins 	= 1;
			cisc->type 		= CISC_TYPE_SEQ;
			if (operand_buffer[1].size != 16){
				cisc->ins[0].opcode 							= IR_PART1_16;
				cisc->ins[0].nb_input_operand 					= 1;
				asmOperand_copy(cisc->ins[0].input_operand, operand_buffer + 1);
				cisc->ins[0].output_operand.size 				= 16;
				cisc->ins[0].output_operand.instruction_index 	= it->instruction_index;
				cisc->ins[0].output_operand.variable 			= NULL;
				cisc->ins[0].output_operand.type 				= ASM_OPERAND_REG;
				cisc->ins[0].output_operand.operand_type.reg 	= irRegister_virtual_get_simd(operand_buffer[0].operand_type.reg, 16, operand_buffer[2].operand_type.imm);
			}
			else{
				cisc->ins[0].opcode 							= IR_MOV;
				cisc->ins[0].nb_input_operand 					= 1;
				asmOperand_copy(cisc->ins[0].input_operand, operand_buffer + 1);
				cisc->ins[0].output_operand.size 				= 16;
				cisc->ins[0].output_operand.instruction_index 	= it->instruction_index;
				cisc->ins[0].output_operand.variable 			= NULL;
				cisc->ins[0].output_operand.type 				= ASM_OPERAND_REG;
				cisc->ins[0].output_operand.operand_type.reg 	= irRegister_virtual_get_simd(operand_buffer[0].operand_type.reg, 16, operand_buffer[2].operand_type.imm);
			}
			break;
		}
		case 32 : {
			uint64_t mask;

			cisc->nb_ins 	= 1;
			cisc->type 		= CISC_TYPE_SEQ;

			mask =  0x000000000000ffffULL << ((operand_buffer[2].operand_type.imm & 0x0000000000000001ULL) * 16);

			cisc->ins[0].opcode 								= IR_AND;
			cisc->ins[0].nb_input_operand 						= 2;
			cisc->ins[0].input_operand[0].size 					= 32;
			cisc->ins[0].input_operand[0].instruction_index 	= it->instruction_index;
			cisc->ins[0].input_operand[0].variable 				= NULL;
			cisc->ins[0].input_operand[0].type 					= ASM_OPERAND_REG;
			cisc->ins[0].input_operand[0].operand_type.reg 		= irRegister_virtual_get_simd(operand_buffer[0].operand_type.reg, 32, operand_buffer[2].operand_type.imm >> 1);
			asmOperand_set_imm(cisc->ins[0].input_operand[1], 32, ~mask)
			asmOperand_copy(&(cisc->ins[0].output_operand), cisc->ins[0].input_operand);

			if (operand_buffer[1].size != 32){
				cisc->ins[1].opcode 							= IR_MOVZX;
				cisc->ins[1].nb_input_operand 					= 1;
				asmOperand_copy(cisc->ins[1].input_operand, operand_buffer + 1);
				cisc->ins[1].output_operand.size 				= 32;
				cisc->ins[1].output_operand.instruction_index 	= it->instruction_index;
				cisc->ins[1].output_operand.variable 			= NULL;
				cisc->ins[1].output_operand.type 				= ASM_OPERAND_REG;
				cisc->ins[1].output_operand.operand_type.reg 	= IR_REG_TMP;

				cisc->nb_ins ++;

				if (operand_buffer[2].operand_type.imm & 0x0000000000000001ULL){
					cisc->ins[2].opcode 						= IR_SHL;
					cisc->ins[2].nb_input_operand 				= 2;
					asmOperand_copy(cisc->ins[2].input_operand, &(cisc->ins[1].output_operand));
					asmOperand_set_imm(cisc->ins[2].input_operand[1], 32, 8)
					asmOperand_copy(&(cisc->ins[2].output_operand), cisc->ins[2].input_operand);

					cisc->nb_ins ++;
				}
			}
			else if (operand_buffer[2].operand_type.imm & 0x0000000000000001ULL){
				cisc->ins[1].opcode 							= IR_SHL;
				cisc->ins[1].nb_input_operand 					= 2;
				asmOperand_copy(cisc->ins[1].input_operand, operand_buffer + 1);
				asmOperand_set_imm(cisc->ins[1].input_operand[1], 32, 16)
				cisc->ins[1].output_operand.size 				= 32;
				cisc->ins[1].output_operand.instruction_index 	= it->instruction_index;
				cisc->ins[1].output_operand.variable 			= NULL;
				cisc->ins[1].output_operand.type 				= ASM_OPERAND_REG;
				cisc->ins[1].output_operand.operand_type.reg 	= IR_REG_TMP;

				cisc->nb_ins ++;
			}
			else{
				cisc->ins[1].opcode 							= IR_AND;
				cisc->ins[1].nb_input_operand 					= 2;
				asmOperand_copy(cisc->ins[1].input_operand, operand_buffer + 1);
				asmOperand_set_imm(cisc->ins[1].input_operand[1], 32, mask)
				cisc->ins[1].output_operand.size 				= 32;
				cisc->ins[1].output_operand.instruction_index 	= it->instruction_index;
				cisc->ins[1].output_operand.variable 			= NULL;
				cisc->ins[1].output_operand.type 				= ASM_OPERAND_REG;
				cisc->ins[1].output_operand.operand_type.reg 	= IR_REG_TMP;

				cisc->nb_ins ++;
			}

			cisc->ins[cisc->nb_ins].opcode 						= IR_OR;
			cisc->ins[cisc->nb_ins].nb_input_operand 			= 2;
			asmOperand_copy(cisc->ins[cisc->nb_ins].input_operand, &(cisc->ins[0].output_operand));
			asmOperand_copy(cisc->ins[cisc->nb_ins].input_operand + 1, &(cisc->ins[cisc->nb_ins - 1].output_operand));
			asmOperand_copy(&(cisc->ins[cisc->nb_ins].output_operand), cisc->ins[cisc->nb_ins].input_operand);

			cisc->nb_ins ++;
			
			break;
		}
		default : {
			log_err("incorrect SIMD frag size");
			return;
		}
	}
}

static void simd_decode_special_pshufd(struct ir* ir, struct instructionIterator* it, struct asmCiscIns* cisc, const struct memAddress* mem_addr){
	struct asmOperand 	in_operand_buffer[2];
	struct asmOperand 	ou_operand_buffer[1];
	uint8_t 			nb_operand;
	struct asmRiscIns 	local_simd;
	uint32_t 			frag_size = SIMD_DEFAULT_FRAG_SIZE;

	asmOperand_decode(it, in_operand_buffer, 2, ASM_OPERAND_ROLE_READ_ALL, &nb_operand, mem_addr);

	#ifdef EXTRA_CHECK
	if (nb_operand != 2){
		log_err_m("incorrect PSHUFD format (%u input arg(s))", nb_operand);
		return;
	}
	if (in_operand_buffer[1].type != ASM_OPERAND_IMM){
		log_err("incorrect PSHUFD format (last operand is not IMM)");
		return;
	}
	#endif

	asmOperand_decode(it, ou_operand_buffer, 1, ASM_OPERAND_ROLE_WRITE_ALL, &nb_operand, mem_addr);

	#ifdef EXTRA_CHECK
	if (nb_operand != 1){
		log_err_m("incorrect PSHUFD format (%u output arg(s))", nb_operand);
		return;
	}
	#endif

	cisc->type 		= CISC_TYPE_PARA;
	cisc->nb_ins 	= 0;

	if (in_operand_buffer[0].type == ASM_OPERAND_REG){
		frag_size = irBuilder_get_vir_register_frag_size(&(ir->builder), in_operand_buffer[0].operand_type.reg);
	}

	local_simd.opcode 				= IR_MOV;
	local_simd.nb_input_operand 	= 1;
	asmOperand_frag(local_simd.input_operand, in_operand_buffer, 32, (in_operand_buffer[1].operand_type.imm >> 0) & 0x0000000000000003ULL);
	asmOperand_frag(&(local_simd.output_operand), ou_operand_buffer, 32, 0);

	cisc->nb_ins += asmSimd_frag_all(cisc->ins + cisc->nb_ins, IRIMPORTERASM_MAX_RISC_INS - cisc->nb_ins, &local_simd, frag_size);

	asmOperand_frag(local_simd.input_operand, in_operand_buffer, 32, (in_operand_buffer[1].operand_type.imm >> 2) & 0x0000000000000003ULL);
	asmOperand_frag(&(local_simd.output_operand), ou_operand_buffer, 32, 1);

	cisc->nb_ins += asmSimd_frag_all(cisc->ins + cisc->nb_ins, IRIMPORTERASM_MAX_RISC_INS - cisc->nb_ins, &local_simd, frag_size);

	asmOperand_frag(local_simd.input_operand, in_operand_buffer, 32, (in_operand_buffer[1].operand_type.imm >> 4) & 0x0000000000000003ULL);
	asmOperand_frag(&(local_simd.output_operand), ou_operand_buffer, 32, 2);

	cisc->nb_ins += asmSimd_frag_all(cisc->ins + cisc->nb_ins, IRIMPORTERASM_MAX_RISC_INS - cisc->nb_ins, &local_simd, frag_size);

	asmOperand_frag(local_simd.input_operand, in_operand_buffer, 32, (in_operand_buffer[1].operand_type.imm >> 6) & 0x0000000000000003ULL);
	asmOperand_frag(&(local_simd.output_operand), ou_operand_buffer, 32, 3);

	cisc->nb_ins += asmSimd_frag_all(cisc->ins + cisc->nb_ins, IRIMPORTERASM_MAX_RISC_INS - cisc->nb_ins, &local_simd, frag_size);
}

static void simd_decode_special_pslld_psrld(struct ir* ir, struct instructionIterator* it, struct asmCiscIns* cisc, const struct memAddress* mem_addr){
	struct asmRiscIns local_simd;

	local_simd.opcode = xedOpcode_2_irOpcode(xed_decoded_inst_get_iclass(&(it->xedd)));
	asmOperand_decode(it, local_simd.input_operand, IRIMPORTERASM_MAX_INPUT_OPERAND, ASM_OPERAND_ROLE_READ_ALL, &(local_simd.nb_input_operand), mem_addr);

	#ifdef EXTRA_CHECK
	if (local_simd.input_operand[0].type != ASM_OPERAND_REG){
		log_err("incorrect PS*LD format (first argument is not a register)");
		return;
	}
	if (local_simd.nb_input_operand != 2){
		log_err_m("incorrect PS*LD format (%u input arg(s))", local_simd.nb_input_operand);
	}
	#endif

	if (irBuilder_get_vir_register_frag_size(&(ir->builder), local_simd.input_operand[0].operand_type.reg) != 32 && local_simd.input_operand[1].type == ASM_OPERAND_IMM){
		log_warn_m("possible simplification: shift %llu -> to be implemented", local_simd.input_operand[1].operand_type.imm);
	}

	if (local_simd.input_operand[1].type == ASM_OPERAND_MEM || local_simd.input_operand[1].type == ASM_OPERAND_REG){
		log_err("this case is not implemented yet: PS*LD second operand is not an IMM -> skip instruction");
		return;
	}

	asmOperand_decode(it, &(local_simd.output_operand), 1, ASM_OPERAND_ROLE_WRITE_ALL, NULL, mem_addr);

	cisc->type 		= CISC_TYPE_PARA;
	cisc->nb_ins 	= asmSimd_frag_first(cisc->ins, IRIMPORTERASM_MAX_RISC_INS, &local_simd, 32);
}

static void simd_decode_special_punpckldq(struct ir* ir, struct instructionIterator* it, struct asmCiscIns* cisc, const struct memAddress* mem_addr){
	struct asmOperand 	operand_buffer[2];
	uint8_t 			nb_operand;
	uint32_t 			i1;
	uint32_t 			i2;
	struct asmRiscIns 	local_simd;
	uint32_t 			frag_size1 = SIMD_DEFAULT_FRAG_SIZE;
	uint32_t 			frag_size2 = SIMD_DEFAULT_FRAG_SIZE;

	asmOperand_decode(it, operand_buffer, 2, ASM_OPERAND_ROLE_READ_ALL, &nb_operand, mem_addr);

	#ifdef EXTRA_CHECK
	if (nb_operand != 2){
		log_err_m("incorrect PUNPCKLDQ format (%u input arg(s))", nb_operand);
		return;
	}
	#endif

	cisc->type 		= CISC_TYPE_PARA;
	cisc->nb_ins 	= 0;

	if (operand_buffer[0].type == ASM_OPERAND_REG){
		frag_size1 = irBuilder_get_vir_register_frag_size(&(ir->builder), operand_buffer[0].operand_type.reg);
	}
	if (operand_buffer[1].type == ASM_OPERAND_REG){
		frag_size2 = irBuilder_get_vir_register_frag_size(&(ir->builder), operand_buffer[1].operand_type.reg);
	}

	local_simd.opcode 				= IR_MOV;
	local_simd.nb_input_operand 	= 1;

	for (i1 = 1, i2 = 0; i1 + i2 < (operand_buffer[0].size >> 5); ){
		if (i2 < i1){
			asmOperand_frag(local_simd.input_operand, operand_buffer + 1, 32, i2);
			asmOperand_frag(&(local_simd.output_operand), operand_buffer, 32, i1 + i2);

			cisc->nb_ins += asmSimd_frag_all(cisc->ins + cisc->nb_ins, IRIMPORTERASM_MAX_RISC_INS - cisc->nb_ins, &local_simd, frag_size2);
			i2 ++;
		}
		else{
			asmOperand_frag(local_simd.input_operand, operand_buffer, 32, i1);
			asmOperand_frag(&(local_simd.output_operand), operand_buffer, 32, i1 + i2);

			cisc->nb_ins += asmSimd_frag_all(cisc->ins + cisc->nb_ins, IRIMPORTERASM_MAX_RISC_INS - cisc->nb_ins, &local_simd, frag_size1);
			i1 ++;
		}
	}
}

static void simd_decode_special_vzeroall(struct instructionIterator* it, struct asmCiscIns* cisc){
	struct asmRiscIns local_simd;

	cisc->type 		= CISC_TYPE_PARA;
	cisc->nb_ins 	= 0;

	asmRisc_set_reg_cst(&local_simd, IR_VREG_YMM1, 256, 0)
	cisc->nb_ins += asmSimd_frag_all(cisc->ins + cisc->nb_ins, IRIMPORTERASM_MAX_RISC_INS - cisc->nb_ins, &local_simd, 32);
	asmRisc_set_reg_cst(&local_simd, IR_VREG_YMM2, 256, 0)
	cisc->nb_ins += asmSimd_frag_all(cisc->ins + cisc->nb_ins, IRIMPORTERASM_MAX_RISC_INS - cisc->nb_ins, &local_simd, 32);
	asmRisc_set_reg_cst(&local_simd, IR_VREG_YMM3, 256, 0)
	cisc->nb_ins += asmSimd_frag_all(cisc->ins + cisc->nb_ins, IRIMPORTERASM_MAX_RISC_INS - cisc->nb_ins, &local_simd, 32);
	asmRisc_set_reg_cst(&local_simd, IR_VREG_YMM4, 256, 0)
	cisc->nb_ins += asmSimd_frag_all(cisc->ins + cisc->nb_ins, IRIMPORTERASM_MAX_RISC_INS - cisc->nb_ins, &local_simd, 32);
	asmRisc_set_reg_cst(&local_simd, IR_VREG_YMM5, 256, 0)
	cisc->nb_ins += asmSimd_frag_all(cisc->ins + cisc->nb_ins, IRIMPORTERASM_MAX_RISC_INS - cisc->nb_ins, &local_simd, 32);
	asmRisc_set_reg_cst(&local_simd, IR_VREG_YMM6, 256, 0)
	cisc->nb_ins += asmSimd_frag_all(cisc->ins + cisc->nb_ins, IRIMPORTERASM_MAX_RISC_INS - cisc->nb_ins, &local_simd, 32);
	asmRisc_set_reg_cst(&local_simd, IR_VREG_YMM7, 256, 0)
	cisc->nb_ins += asmSimd_frag_all(cisc->ins + cisc->nb_ins, IRIMPORTERASM_MAX_RISC_INS - cisc->nb_ins, &local_simd, 32);
	asmRisc_set_reg_cst(&local_simd, IR_VREG_YMM8, 256, 0)
	cisc->nb_ins += asmSimd_frag_all(cisc->ins + cisc->nb_ins, IRIMPORTERASM_MAX_RISC_INS - cisc->nb_ins, &local_simd, 32);
}
