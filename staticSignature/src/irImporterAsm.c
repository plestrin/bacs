#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "irImporterAsm.h"
#include "graph.h"
#include "irRenameEngine.h"

static enum irOpcode xedOpcode_2_irOpcode(xed_iclass_enum_t xed_opcode);
static enum irRegister xedRegister_2_irRegister(xed_reg_enum_t xed_reg);

#define IRIMPORTERASM_MAX_INPUT_OPERAND 	2
#define IRIMPORTERASM_MAX_RISC_INS 			4

static const enum irDependenceType dependence_label_table[NB_IR_OPCODE - 1][IRIMPORTERASM_MAX_INPUT_OPERAND] = {
	{IR_DEPENDENCE_TYPE_DIRECT, IR_DEPENDENCE_TYPE_DIRECT}, 		/* IR_ADD */
	{IR_DEPENDENCE_TYPE_DIRECT, IR_DEPENDENCE_TYPE_DIRECT}, 		/* IR_AND */
	{IR_DEPENDENCE_TYPE_DIRECT, IR_DEPENDENCE_TYPE_DIRECT}, 		/* IR_CMOV */
	{IR_DEPENDENCE_TYPE_DIRECT, IR_DEPENDENCE_TYPE_DIRECT}, 		/* IR_DIV */
	{IR_DEPENDENCE_TYPE_DIRECT, IR_DEPENDENCE_TYPE_DIRECT}, 		/* IR_IMUL */
	{IR_DEPENDENCE_TYPE_DIRECT, IR_DEPENDENCE_TYPE_DIRECT}, 		/* IR_LEA */
	{IR_DEPENDENCE_TYPE_DIRECT, IR_DEPENDENCE_TYPE_DIRECT}, 		/* IR_MOV */
	{IR_DEPENDENCE_TYPE_DIRECT, IR_DEPENDENCE_TYPE_DIRECT}, 		/* IR_MOVZX */
	{IR_DEPENDENCE_TYPE_DIRECT, IR_DEPENDENCE_TYPE_DIRECT}, 		/* IR_MUL */
	{IR_DEPENDENCE_TYPE_DIRECT, IR_DEPENDENCE_TYPE_DIRECT}, 		/* IR_NOT */
	{IR_DEPENDENCE_TYPE_DIRECT, IR_DEPENDENCE_TYPE_DIRECT}, 		/* IR_OR */
	{IR_DEPENDENCE_TYPE_DIRECT, IR_DEPENDENCE_TYPE_DIRECT}, 		/* IR_PART1_8 */
	{IR_DEPENDENCE_TYPE_DIRECT, IR_DEPENDENCE_TYPE_DIRECT}, 		/* IR_PART2_8 */
	{IR_DEPENDENCE_TYPE_DIRECT, IR_DEPENDENCE_TYPE_DIRECT}, 		/* IR_PART1_16 */
	{IR_DEPENDENCE_TYPE_DIRECT, IR_DEPENDENCE_TYPE_SHIFT_DISP}, 	/* IR_ROL */
	{IR_DEPENDENCE_TYPE_DIRECT, IR_DEPENDENCE_TYPE_SHIFT_DISP}, 	/* IR_ROR */
	{IR_DEPENDENCE_TYPE_DIRECT, IR_DEPENDENCE_TYPE_SHIFT_DISP}, 	/* IR_SHL */
	{IR_DEPENDENCE_TYPE_DIRECT, IR_DEPENDENCE_TYPE_SHIFT_DISP}, 	/* IR_SHR */
	{IR_DEPENDENCE_TYPE_DIRECT, IR_DEPENDENCE_TYPE_DIRECT}, 		/* IR_SUB */
	{IR_DEPENDENCE_TYPE_DIRECT, IR_DEPENDENCE_TYPE_DIRECT}, 		/* IR_XOR */
	{IR_DEPENDENCE_TYPE_DIRECT, IR_DEPENDENCE_TYPE_DIRECT}, 		/* IR_LOAD */
	{IR_DEPENDENCE_TYPE_DIRECT, IR_DEPENDENCE_TYPE_DIRECT}, 		/* IR_STORE */
	{IR_DEPENDENCE_TYPE_DIRECT, IR_DEPENDENCE_TYPE_DIRECT}, 		/* IR_JOKER */
};

static const uint8_t sign_extand_table[NB_IR_OPCODE - 1] = {
	1, /* IR_ADD */
	0, /* IR_AND */
	0, /* IR_CMOV */
	0, /* IR_DIV */
	0, /* IR_IMUL */
	0, /* IR_LEA */
	0, /* IR_MOV */
	0, /* IR_MOVZX */
	0, /* IR_MUL */
	0, /* IR_NOT */
	0, /* IR_OR */
	0, /* IR_PART1_8 */
	0, /* IR_PART2_8 */
	0, /* IR_PART1_16 */
	0, /* IR_ROL */
	0, /* IR_ROR */
	0, /* IR_SHL */
	0, /* IR_SHR */
	1, /* IR_SUB */
	0, /* IR_XOR */
	0, /* IR_LOAD */
	0, /* IR_STORE */
	0  /* IR_JOKER */
};

struct memOperand{
	xed_reg_enum_t 	base;
	xed_reg_enum_t 	index;
	uint8_t 		scale;
	uint32_t 		disp;
};

static struct node* memOperand_build_address(struct irRenameEngine* engine, struct memOperand* mem);

enum asmOperandType{
	ASM_OPERAND_IMM,
	ASM_OPERAND_REG,
	ASM_OPERAND_MEM
};

#define ASM_OPERAND_ROLE_READ_1 	0x00000001
#define ASM_OPERAND_ROLE_READ_2 	0x00000002
#define ASM_OPERAND_ROLE_WRITE_1 	0x00000100
#define ASM_OPERAND_ROLE_READ_ALL 	0x000000ff
#define ASM_OPERAND_ROLE_WRITE_ALL 	0x0000ff00
#define ASM_OPERAND_ROLE_ALL 		0x0000ffff

#define ASM_OPERAND_ROLE_IS_READ(role) 			((role) & 0x000000ff)
#define ASM_OPERAND_ROLE_IS_WRITE(role) 		((role) & 0x0000ff00)
#define ASM_OPERAND_ROLE_IS_READ_INDEX(role, index) 	((role) & (0x00000001 << ((index) & 0x0000007))) 
#define ASM_OPERAND_ROLE_IS_WRITE_INDEX(role, index) 	((role) & (0x00000100 << ((index) & 0x0000007))) 

struct asmOperand{
	uint16_t 				size;
	struct node*			variable;
	enum asmOperandType 	type;
	union{
		enum irRegister 	reg;
		uint64_t 			imm;
		struct memOperand	mem;
	} 						operand_type;
};

static void asmOperand_decode(xed_decoded_inst_t* xedd, struct asmOperand* operand_buffer, uint8_t max_nb_operand, uint32_t selector, uint8_t* nb_operand_);

static void asmOperand_fetch_input(struct irRenameEngine* engine, struct asmOperand* operand);
static void asmOperand_fetch_output(struct irRenameEngine* engine, struct asmOperand* operand, enum irOpcode opcode);

static void asmOperand_sign_extend(struct asmOperand* operand, uint16_t size);

struct asmRiscIns{
	enum irOpcode 			opcode;
	uint8_t 				nb_input_operand;
	struct asmOperand 		input_operand[IRIMPORTERASM_MAX_INPUT_OPERAND];
	struct asmOperand 		output_operand;
};

static void asmRisc_process(struct irRenameEngine* engine, struct asmRiscIns* risc);

static void asmRisc_process_special_cmov(struct irRenameEngine* engine, struct asmRiscIns* risc);
static void asmRisc_process_special_lea(struct irRenameEngine* engine, struct asmRiscIns* risc);
static void asmRisc_process_special_mov(struct irRenameEngine* engine, struct asmRiscIns* risc);

struct asmCiscIns{
	uint8_t 				valid;
	uint8_t 				nb_ins;
	struct asmRiscIns 		ins[IRIMPORTERASM_MAX_RISC_INS];
};

#define ASMCISCINS_IS_VALID(cisc) 		((cisc).valid)
#define ASMCISCINS_SET_VALID(cisc) 		((cisc).valid = 1)
#define ASMCISCINS_SET_INVALID(cisc) 	((cisc).valid = 0)

static void cisc_decode_special_call(struct asmCiscIns* cisc, ADDRESS pc);
static void cisc_decode_special_dec(xed_decoded_inst_t* xedd, struct asmCiscIns* cisc);
static void cisc_decode_special_inc(xed_decoded_inst_t* xedd, struct asmCiscIns* cisc);
static void cisc_decode_special_leave(struct asmCiscIns* cisc);
static void cisc_decode_special_pop(xed_decoded_inst_t* xedd, struct asmCiscIns* cisc);
static void cisc_decode_special_push(xed_decoded_inst_t* xedd, struct asmCiscIns* cisc);
static void cisc_decode_special_ret(struct asmCiscIns* cisc);

int32_t irImporterAsm_import(struct ir* ir, struct assembly* assembly){
	struct asmCiscIns 			cisc;
	struct irRenameEngine 		engine;
	struct instructionIterator 	it;
	uint32_t 					i;

	irRenameEngine_init(engine, ir)

	if (assembly_get_instruction(assembly, &it, 0)){
		printf("ERROR: in %s, unable to fetch first instruction from the assembly\n", __func__);
		return -1;
	}

	for (;;){
		ASMCISCINS_SET_INVALID(cisc);

		switch(xed_decoded_inst_get_iclass(&(it.xedd))){
			case XED_ICLASS_BSWAP 		: {break;}
			case XED_ICLASS_CALL_NEAR 	: {
				cisc_decode_special_call(&cisc, it.instruction_address);
				break;
			}
			case XED_ICLASS_CMP 		: {break;}
			case XED_ICLASS_DEC 		: {
				cisc_decode_special_dec(&(it.xedd), &cisc);
				break;
			}
			case XED_ICLASS_INC 		: {
				cisc_decode_special_inc(&(it.xedd), &cisc);
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
			case XED_ICLASS_JNZ 		: {break;}
			case XED_ICLASS_JZ 			: {break;}
			case XED_ICLASS_LEAVE 		: {
				cisc_decode_special_leave(&cisc);
				break;
			}
			case XED_ICLASS_NOP 		: {break;}
			case XED_ICLASS_POP 		: {
				cisc_decode_special_pop(&(it.xedd), &cisc);
				break;
			}
			case XED_ICLASS_PUSH 		: {
				cisc_decode_special_push(&(it.xedd), &cisc);
				break;
			}
			case XED_ICLASS_RET_FAR 	: {
				cisc_decode_special_ret(&cisc);
				break;
			}
			case XED_ICLASS_RET_NEAR 	: {
				cisc_decode_special_ret(&cisc);
				break;
			}
			case XED_ICLASS_TEST 		: {break;}
			default :{
				cisc.nb_ins = 1;
				cisc.ins[0].opcode = xedOpcode_2_irOpcode(xed_decoded_inst_get_iclass(&(it.xedd)));
				if (cisc.ins[0].opcode != IR_INVALID){
					ASMCISCINS_SET_VALID(cisc);
					asmOperand_decode(&(it.xedd), cisc.ins[0].input_operand, IRIMPORTERASM_MAX_INPUT_OPERAND, ASM_OPERAND_ROLE_READ_ALL, &(cisc.ins[0].nb_input_operand));
					asmOperand_decode(&(it.xedd), &(cisc.ins[0].output_operand), 1, ASM_OPERAND_ROLE_WRITE_ALL, NULL);
				}
			}
		}

		if (ASMCISCINS_IS_VALID(cisc)){
			for (i = 0; i < cisc.nb_ins; i++){
				switch(cisc.ins[i].opcode){
					case IR_CMOV 	: {
						asmRisc_process_special_cmov(&engine, cisc.ins + i);
						break;
					}
					case IR_LEA 	: {
						asmRisc_process_special_lea(&engine, cisc.ins + i);
						break;
					}
					case IR_MOV 	: {
						asmRisc_process_special_mov(&engine, cisc.ins + i);
						break;
					}
					default 				: {
						asmRisc_process(&engine, cisc.ins + i);
					}
				}
			}
		}

		if (instructionIterator_get_instruction_index(&it) ==  assembly_get_nb_instruction(assembly) - 1){
			break;
		}
		else{
			if (assembly_get_next_instruction(assembly, &it)){
				printf("ERROR: in %s, unable to fetch next instruction from the assembly\n", __func__);
				return -1;
			}
		}
	}

	irRenameEngine_tag_final_node(&engine);

	return 0;
}

static void asmOperand_decode(xed_decoded_inst_t* xedd, struct asmOperand* operand_buffer, uint8_t max_nb_operand, uint32_t selector, uint8_t* nb_operand_){
	uint32_t 				i;
	uint8_t 				nb_memops 	= 0;
	const xed_inst_t* 		xi 			= xed_decoded_inst_inst(xedd);
	const xed_operand_t* 	xed_op;
	xed_operand_enum_t 		op_name;
	uint32_t 				nb_read 	= 0;
	uint32_t 				nb_write 	= 0;
	uint32_t 				nb_operand 	= 0;

	for (i = 0; i < xed_inst_noperands(xi); i++){
		xed_op = xed_inst_operand(xi, i);
		op_name = xed_operand_name(xed_op);

		if ((xed_operand_read(xed_op) && ASM_OPERAND_ROLE_IS_READ_INDEX(selector, nb_read)) || (xed_operand_written(xed_op) && ASM_OPERAND_ROLE_IS_WRITE_INDEX(selector, nb_write))){
			switch(op_name){
				case XED_OPERAND_AGEN 	:
				case XED_OPERAND_MEM0 	:
				case XED_OPERAND_MEM1 	: {
					uint64_t disp;

					if (max_nb_operand == nb_operand){
						printf("ERROR: in %s, the max number of operand has been reached: %u\n", __func__, max_nb_operand);
						goto exit;
					}

					operand_buffer[nb_operand].size 					= xed_decoded_inst_get_memory_operand_length(xedd, nb_memops) * 8;
					operand_buffer[nb_operand].variable 				= NULL;
					operand_buffer[nb_operand].type 					= ASM_OPERAND_MEM;
					operand_buffer[nb_operand].operand_type.mem.base 	= xed_decoded_inst_get_base_reg(xedd, nb_memops);
					operand_buffer[nb_operand].operand_type.mem.index 	= xed_decoded_inst_get_index_reg(xedd, nb_memops);
					operand_buffer[nb_operand].operand_type.mem.scale 	= xed_decoded_inst_get_scale(xedd, nb_memops);

					disp = xed_decoded_inst_get_memory_displacement(xedd, nb_memops);
					if (disp){
						switch(xed_decoded_inst_get_memory_displacement_width(xedd, nb_memops)){
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
					operand_buffer[nb_operand].operand_type.mem.disp 	= disp;

					nb_operand ++;
					break;
				}
				case XED_OPERAND_IMM0 	: {
					if (max_nb_operand == nb_operand){
						printf("ERROR: in %s, the max number of operand has been reached: %u\n", __func__, max_nb_operand);
						goto exit;
					}

					operand_buffer[nb_operand].size 					= xed_decoded_inst_get_immediate_width_bits(xedd);
					operand_buffer[nb_operand].variable 				= NULL;
					operand_buffer[nb_operand].type 					= ASM_OPERAND_IMM;
					operand_buffer[nb_operand].operand_type.imm 		= xed_decoded_inst_get_unsigned_immediate(xedd);

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

					reg = xed_decoded_inst_get_reg(xedd, op_name);
					if (reg == XED_REG_EFLAGS){
						break;
					}

					if (max_nb_operand == nb_operand){
						printf("ERROR: in %s, the max number of operand has been reached: %u\n", __func__, max_nb_operand);
						goto exit;
					}

					operand_buffer[nb_operand].variable 				= NULL;
					operand_buffer[nb_operand].type 					= ASM_OPERAND_REG;
					operand_buffer[nb_operand].operand_type.reg 		= xedRegister_2_irRegister(reg);
					operand_buffer[nb_operand].size 					= irRegister_get_size(operand_buffer[nb_operand].operand_type.reg);

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
					printf("ERROR: in %s, operand type not supported: %s, opcode: %s\n", __func__, xed_operand_enum_t2str(op_name), xed_iclass_enum_t2str(xed_decoded_inst_get_iclass(xedd)));
					break;
				}
			}
		}

		if (xed_operand_read(xed_op)){
			nb_read ++;
		}
		if (xed_operand_written(xed_op)){
			nb_write ++;
		}
		if (op_name == XED_OPERAND_AGEN || op_name == XED_OPERAND_MEM0 || op_name == XED_OPERAND_MEM1){
			nb_memops ++;
		}
	}

	exit:
	if (nb_operand_ != NULL){
		*nb_operand_ = nb_operand;
	}
}

static void asmOperand_fetch_input(struct irRenameEngine* engine, struct asmOperand* operand){
	switch(operand->type){
		case ASM_OPERAND_IMM : {
			operand->variable = ir_add_immediate(engine->ir, operand->size, operand->operand_type.imm);
			if (operand->variable == NULL){
				printf("ERROR: in %s, unable to add immediate to IR\n", __func__);
			}
			break;
		}
		case ASM_OPERAND_REG : {
			operand->variable = irRenameEngine_get_register_ref(engine, operand->operand_type.reg);
			if (operand->variable == NULL){
				printf("ERROR: in %s, unable to register reference from the renaming engine\n", __func__);
			}
			break;
		}
		case ASM_OPERAND_MEM : {
			struct node* address;

			address = memOperand_build_address(engine, &(operand->operand_type.mem));
			if (address != NULL){
				operand->variable = ir_add_in_mem(engine->ir, address, operand->size, irRenameEngine_get_mem_order(engine));
				if (operand->variable == NULL){
					printf("ERROR: in %s, unable to add memory load to IR\n", __func__);
				}
				else{
					irRenameEngine_set_mem_order(engine, operand->variable);
				}
			}
			else{
				printf("ERROR: in %s, unable to build memory address\n", __func__);
			}
			break;
		}
	}
}

static void asmOperand_fetch_output(struct irRenameEngine* engine, struct asmOperand* operand, enum irOpcode opcode){
	switch(operand->type){
		case ASM_OPERAND_REG 	: {
			operand->variable = ir_add_inst(engine->ir, opcode, operand->size);
			if (operand->variable == NULL){
				printf("ERROR: in %s, unable to add operation to IR\n", __func__);
			}
			else{
				irRenameEngine_set_register_ref(engine, operand->operand_type.reg, operand->variable);
			}
			break;
		}
		case ASM_OPERAND_MEM 	: {
			struct node* address;
			struct node* mem_write;

			address = memOperand_build_address(engine, &(operand->operand_type.mem));
			if (address != NULL){
				operand->variable = ir_add_inst(engine->ir, opcode, operand->size);
				if (operand->variable != NULL){
					mem_write = ir_add_out_mem(engine->ir, address, operand->size, irRenameEngine_get_mem_order(engine));
					if (mem_write != NULL){
						irRenameEngine_set_mem_order(engine, mem_write);
						if (ir_add_dependence(engine->ir, operand->variable, mem_write, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
							printf("ERROR: in %s, unable to add output to add dependence to IR\n", __func__);
						}
					}
					else{
						printf("ERROR: in %s, unable to add memory write to IR\n", __func__);
					}
				}
				else{
					printf("ERROR: in %s, unable to add operation to IR\n", __func__);
				}
			}
			else{
				printf("ERROR: in %s, unable to build memory address\n", __func__);
			}
			break;
		}
		default 				: {
			printf("ERROR: in %s, this case is not supposed to happen (IMM read operand)\n", __func__);
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

static struct node* memOperand_build_address(struct irRenameEngine* engine, struct memOperand* mem){
	struct node* 	base 		= NULL;
	struct node* 	index 		= NULL;
	struct node* 	disp 		= NULL;
	struct node* 	scale 		= NULL;
	uint8_t 		nb_operand 	= 0;
	struct node* 	operands[3];
	struct node* 	address 	= NULL;

	if (mem->base != XED_REG_INVALID){
		base = irRenameEngine_get_register_ref(engine, xedRegister_2_irRegister(mem->base));
		if (base == NULL){
			printf("ERROR: in %s, unable to get register reference from the renaming engine\n", __func__);
		}
		else{
			operands[nb_operand] = base;
			nb_operand ++;
		}
	}

	if (mem->scale > 1){
		scale = ir_add_immediate(engine->ir, 8, mem->scale);
		if (scale == NULL){
			printf("ERROR: in %s, unable to add immediate to IR\n", __func__);
		}
	}

	if (mem->index != XED_REG_INVALID){
		index = irRenameEngine_get_register_ref(engine, xedRegister_2_irRegister(mem->index));
		if (index == NULL){
			printf("ERROR: in %s, unable to get register reference from the renaming engine\n", __func__);
		}
		else{
			if (scale == NULL){
				operands[nb_operand] = index;
				nb_operand ++;
			}
			else{
				struct node* shl;

				shl = ir_add_inst(engine->ir, IR_SHL, 32);
				if (shl != NULL){
					if (ir_add_dependence(engine->ir, index, shl, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
						printf("ERROR: in %s, unable to add dependence between IR nodes\n", __func__);
					}
					if (ir_add_dependence(engine->ir, scale, shl, IR_DEPENDENCE_TYPE_SHIFT_DISP) == NULL){
						printf("ERROR: in %s, unable to add dependence between IR nodes\n", __func__);
					}

					operands[nb_operand] = shl;
					nb_operand ++;
				}
				else{
					printf("ERROR: in %s, unable to add operation to IR\n", __func__);
				}
			}
		}
	}

	if (mem->disp){
		disp = ir_add_immediate(engine->ir, 32, mem->disp);
		if (disp == NULL){
			printf("ERROR: in %s, unable to add immediate to IR\n", __func__);
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
			address = ir_add_inst(engine->ir, IR_ADD, 32);
			if (address != NULL){
				if (ir_add_dependence(engine->ir, operands[0], address, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
					printf("ERROR: in %s, unable to add dependence between IR nodes\n", __func__);
				}
				if (ir_add_dependence(engine->ir, operands[1], address, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
					printf("ERROR: in %s, unable to add dependence between IR nodes\n", __func__);
				}
			}
			else{
				printf("ERROR: in %s, unable to add operation to IR\n", __func__);
			}
			break;
		}
		case 3 : {
			address = ir_add_inst(engine->ir, IR_ADD, 32);
			if (address != NULL){
				if (ir_add_dependence(engine->ir, operands[0], address, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
					printf("ERROR: in %s, unable to add dependence between IR nodes\n", __func__);
				}
				if (ir_add_dependence(engine->ir, operands[1], address, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
					printf("ERROR: in %s, unable to add dependence between IR nodes\n", __func__);
				}
				if (ir_add_dependence(engine->ir, operands[2], address, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
					printf("ERROR: in %s, unable to add dependence between IR nodes\n", __func__);
				}
			}
			else{
				printf("ERROR: in %s, unable to add operation to IR\n", __func__);
			}
			break;
		}
		default : {
			printf("ERROR: in %s, incorrect number of operand(s): %u\n", __func__, nb_operand);
		}
	}

	return address;
}

static void asmRisc_process(struct irRenameEngine* engine, struct asmRiscIns* risc){
	uint8_t i;

	if (sign_extand_table[risc->opcode]){
		for (i = 0; i < risc->nb_input_operand; i++){
			asmOperand_sign_extend(risc->input_operand + i, risc->output_operand.size);
		}
	}

	for (i = 0; i < risc->nb_input_operand; i++){
		asmOperand_fetch_input(engine, risc->input_operand + i);
	}
	asmOperand_fetch_output(engine, &(risc->output_operand), risc->opcode);

	if (risc->output_operand.variable != NULL){
		for (i = 0; i < risc->nb_input_operand; i++){
			if (risc->input_operand[i].variable != NULL){
				if (ir_add_dependence(engine->ir, risc->input_operand[i].variable, risc->output_operand.variable, dependence_label_table[risc->opcode][i]) == NULL){
					printf("ERROR: in %s, unable to add output to add dependence to IR\n", __func__);
				}
			}
		}
	}
}

static void asmRisc_process_special_cmov(struct irRenameEngine* engine, struct asmRiscIns* risc){
	printf("WARNING: in %s, translating CMOVxx instruction into glue operation\n", __func__);

	if (risc->nb_input_operand != 1){
		printf("ERROR: in %s, incorrect number of input operand\n", __func__);
		return;
	}

	memcpy(risc->input_operand + 1, &(risc->output_operand), sizeof(struct asmOperand));
	risc->nb_input_operand ++;

	asmOperand_fetch_input(engine, risc->input_operand + 0);
	asmOperand_fetch_input(engine, risc->input_operand + 1);

	asmOperand_fetch_output(engine, &(risc->output_operand), risc->opcode);

	if (risc->output_operand.variable != NULL){
		if (risc->input_operand[0].variable != NULL){
			if (ir_add_dependence(engine->ir, risc->input_operand[0].variable, risc->output_operand.variable, dependence_label_table[risc->opcode][0]) == NULL){
				printf("ERROR: in %s, unable to add output to add dependence to IR\n", __func__);
			}
		}
		else{
			printf("ERROR: in %s, unable to fetch first input operand\n", __func__);
		}

		if (risc->input_operand[1].variable != NULL){
			if (ir_add_dependence(engine->ir, risc->input_operand[1].variable, risc->output_operand.variable, dependence_label_table[risc->opcode][1]) == NULL){
				printf("ERROR: in %s, unable to add output to add dependence to IR\n", __func__);
			}
		}
		else{
			printf("ERROR: in %s, unable to fetch second input operand\n", __func__);
		}
	}
	else{
		printf("ERROR: in %s, unable to fetch output operand\n", __func__);
	}
}

static void asmRisc_process_special_lea(struct irRenameEngine* engine, struct asmRiscIns* risc){
	struct node* address;
	struct node* mem_write;

	if (risc->nb_input_operand != 1 || risc->input_operand[0].type != ASM_OPERAND_MEM){
		printf("ERROR: in %s, incorrect type or number of input operand\n", __func__);
		return;
	}

	risc->input_operand[0].variable = memOperand_build_address(engine, &(risc->input_operand[0].operand_type.mem));
	if (risc->input_operand[0].variable != NULL){
		switch(risc->output_operand.type){
			case ASM_OPERAND_REG 	: {
				irRenameEngine_set_register_ref(engine, risc->output_operand.operand_type.reg, risc->input_operand[0].variable);
				break;
			}
			case ASM_OPERAND_MEM 	: {
				address = memOperand_build_address(engine, &(risc->output_operand.operand_type.mem));
				if (address != NULL){
					mem_write = ir_add_out_mem(engine->ir, address, risc->output_operand.size, irRenameEngine_get_mem_order(engine));
					if (mem_write != NULL){
						irRenameEngine_set_mem_order(engine, mem_write);
						if (ir_add_dependence(engine->ir, risc->input_operand[0].variable, mem_write, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
							printf("ERROR: in %s, unable to add output to add dependence to IR\n", __func__);
						}
					}
					else{
						printf("ERROR: in %s, unable to add memory write to IR\n", __func__);
					}
				}
				else{
					printf("ERROR: in %s, unable to build memory address\n", __func__);
				}
				break;
			}
			default 				: {
				printf("ERROR: in %s, this case is not supposed to happen\n", __func__);
			}
		}
	}
	else{
		printf("ERROR: in %s, unable to build memory address\n", __func__);
	}
}

static void asmRisc_process_special_mov(struct irRenameEngine* engine, struct asmRiscIns* risc){
	struct node* address;
	struct node* mem_write;

	if (risc->nb_input_operand != 1){
		printf("ERROR: in %s, incorrect number of input operand\n", __func__);
		return;
	}

	asmOperand_fetch_input(engine, risc->input_operand);
	if (risc->input_operand[0].variable != NULL){
		switch(risc->output_operand.type){
			case ASM_OPERAND_REG 	: {
				irRenameEngine_set_register_ref(engine, risc->output_operand.operand_type.reg, risc->input_operand[0].variable);
				break;
			}
			case ASM_OPERAND_MEM 	: {
				address = memOperand_build_address(engine, &(risc->output_operand.operand_type.mem));
				if (address != NULL){
					mem_write = ir_add_out_mem(engine->ir, address, risc->output_operand.size, irRenameEngine_get_mem_order(engine));
					if (mem_write != NULL){
						irRenameEngine_set_mem_order(engine, mem_write);
						if (ir_add_dependence(engine->ir, risc->input_operand[0].variable, mem_write, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
							printf("ERROR: in %s, unable to add output to add dependence to IR\n", __func__);
						}
					}
					else{
						printf("ERROR: in %s, unable to add memory write to IR\n", __func__);
					}
				}
				else{
					printf("ERROR: in %s, unable to build memory address\n", __func__);
				}
				break;
			}
			default 				: {
				printf("ERROR: in %s, this case is not supposed to happen\n", __func__);
			}
		}
	}
	else{
		printf("ERROR: in %s, input variable is NULL\n", __func__);
	}
}

static enum irOpcode xedOpcode_2_irOpcode(xed_iclass_enum_t xed_opcode){
	switch (xed_opcode){
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
		case XED_ICLASS_DIV			: {return IR_DIV;}
		case XED_ICLASS_IMUL 		: {return IR_IMUL;}
		case XED_ICLASS_LEA 		: {return IR_LEA;}
		case XED_ICLASS_MOV 		: {return IR_MOV;}
		case XED_ICLASS_MOVZX 		: {return IR_MOVZX;}
		case XED_ICLASS_MUL 		: {return IR_MUL;}
		case XED_ICLASS_NOT 		: {return IR_NOT;}
		case XED_ICLASS_OR 			: {return IR_OR;}
		case XED_ICLASS_ROL 		: {return IR_ROL;}
		case XED_ICLASS_ROR 		: {return IR_ROR;}
		case XED_ICLASS_SHL 		: {return IR_SHL;}
		case XED_ICLASS_SHR 		: {return IR_SHR;}
		case XED_ICLASS_SUB 		: {return IR_SUB;}
		case XED_ICLASS_XOR 		: {return IR_XOR;}
		default : {
			printf("ERROR: in %s, this instruction (%s) cannot be translated into IR Opcode\n", __func__, xed_iclass_enum_t2str(xed_opcode));
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
		case XED_REG_EAX 	: {return IR_REG_EAX;}
		case XED_REG_ECX 	: {return IR_REG_ECX;}
		case XED_REG_EDX 	: {return IR_REG_EDX;}
		case XED_REG_EBX 	: {return IR_REG_EBX;}
		case XED_REG_ESP 	: {return IR_REG_ESP;}
		case XED_REG_EBP 	: {return IR_REG_EBP;}
		case XED_REG_ESI 	: {return IR_REG_ESI;}
		case XED_REG_EDI 	: {return IR_REG_EDI;}
		default : {
			printf("ERROR: in %s, this register (%s) cannot be translated into IR register\n", __func__, xed_reg_enum_t2str(xed_reg));
			return IR_REG_EAX;
		}
	}
}

static void cisc_decode_special_call(struct asmCiscIns* cisc, ADDRESS pc){
	cisc->valid 											= 1;
	cisc->nb_ins 											= 2;

	cisc->ins[0].opcode 									= IR_SUB;
	cisc->ins[0].nb_input_operand 							= 2;
	cisc->ins[0].input_operand[0].size 						= 32;
	cisc->ins[0].input_operand[0].variable 					= NULL;
	cisc->ins[0].input_operand[0].type 						= ASM_OPERAND_REG;
	cisc->ins[0].input_operand[0].operand_type.reg 			= IR_REG_ESP;
	cisc->ins[0].input_operand[1].size 						= 32;
	cisc->ins[0].input_operand[1].variable 					= NULL;
	cisc->ins[0].input_operand[1].type 						= ASM_OPERAND_IMM;
	cisc->ins[0].input_operand[1].operand_type.imm 			= 4;
	cisc->ins[0].output_operand.size 						= 32;
	cisc->ins[0].output_operand.variable 					= NULL;
	cisc->ins[0].output_operand.type 						= ASM_OPERAND_REG;
	cisc->ins[0].output_operand.operand_type.reg 			= IR_REG_ESP;

	cisc->ins[1].opcode 									= IR_MOV;
	cisc->ins[1].nb_input_operand 							= 1;
	cisc->ins[1].input_operand[0].size 						= 32;
	cisc->ins[1].input_operand[0].variable 					= NULL;
	cisc->ins[1].input_operand[0].type 						= ASM_OPERAND_IMM;
	cisc->ins[1].input_operand[0].operand_type.imm 			= pc;
	cisc->ins[1].output_operand.size 						= 32;
	cisc->ins[1].output_operand.variable 					= NULL;
	cisc->ins[1].output_operand.type 						= ASM_OPERAND_MEM;
	cisc->ins[1].output_operand.operand_type.mem.base 		= XED_REG_ESP;
	cisc->ins[1].output_operand.operand_type.mem.index 		= XED_REG_INVALID;
	cisc->ins[1].output_operand.operand_type.mem.scale 		= 1;
	cisc->ins[1].output_operand.operand_type.mem.disp 		= 0;

}

static void cisc_decode_special_dec(xed_decoded_inst_t* xedd, struct asmCiscIns* cisc){
	cisc->valid 											= 1;
	cisc->nb_ins 											= 1;

	cisc->ins[0].opcode 									= IR_SUB;
	cisc->ins[0].nb_input_operand 							= 2;
	asmOperand_decode(xedd, cisc->ins[0].input_operand, 1, ASM_OPERAND_ROLE_READ_1, NULL);
	cisc->ins[0].input_operand[1].size 						= cisc->ins[0].input_operand[0].size;
	cisc->ins[0].input_operand[1].variable 					= NULL;
	cisc->ins[0].input_operand[1].type 						= ASM_OPERAND_IMM;
	cisc->ins[0].input_operand[1].operand_type.imm 			= 1;
	asmOperand_decode(xedd, &(cisc->ins[0].output_operand), 1, ASM_OPERAND_ROLE_WRITE_1, NULL);
}

static void cisc_decode_special_inc(xed_decoded_inst_t* xedd, struct asmCiscIns* cisc){
	cisc->valid 											= 1;
	cisc->nb_ins 											= 1;

	cisc->ins[0].opcode 									= IR_ADD;
	cisc->ins[0].nb_input_operand 							= 2;
	asmOperand_decode(xedd, cisc->ins[0].input_operand, 1, ASM_OPERAND_ROLE_READ_1, NULL);
	cisc->ins[0].input_operand[1].size 						= cisc->ins[0].input_operand[0].size;
	cisc->ins[0].input_operand[1].variable 					= NULL;
	cisc->ins[0].input_operand[1].type 						= ASM_OPERAND_IMM;
	cisc->ins[0].input_operand[1].operand_type.imm 			= 1;
	asmOperand_decode(xedd, &(cisc->ins[0].output_operand), 1, ASM_OPERAND_ROLE_WRITE_1, NULL);
}

static void cisc_decode_special_leave(struct asmCiscIns* cisc){
	cisc->valid 											= 1;
	cisc->nb_ins 											= 3;

	cisc->ins[0].opcode 									= IR_MOV;
	cisc->ins[0].nb_input_operand 							= 1;
	cisc->ins[0].input_operand[0].size 						= 32;
	cisc->ins[0].input_operand[0].variable 					= NULL;
	cisc->ins[0].input_operand[0].type 						= ASM_OPERAND_REG;
	cisc->ins[0].input_operand[0].operand_type.reg 			= IR_REG_EBP;
	cisc->ins[0].output_operand.size 						= 32;
	cisc->ins[0].output_operand.variable 					= NULL;
	cisc->ins[0].output_operand.type 						= ASM_OPERAND_REG;
	cisc->ins[0].output_operand.operand_type.reg 			= IR_REG_ESP;

	cisc->ins[1].opcode 									= IR_MOV;
	cisc->ins[1].nb_input_operand 							= 1;
	cisc->ins[1].input_operand[0].size 						= 32;
	cisc->ins[1].input_operand[0].variable 					= NULL;
	cisc->ins[1].input_operand[0].type 						= ASM_OPERAND_MEM;
	cisc->ins[1].input_operand[0].operand_type.mem.base 	= XED_REG_ESP;
	cisc->ins[1].input_operand[0].operand_type.mem.index 	= XED_REG_INVALID;
	cisc->ins[1].input_operand[0].operand_type.mem.scale 	= 1;
	cisc->ins[1].input_operand[0].operand_type.mem.disp 	= 0;
	cisc->ins[1].output_operand.size 						= 32;
	cisc->ins[1].output_operand.variable 					= NULL;
	cisc->ins[1].output_operand.type 						= ASM_OPERAND_REG;
	cisc->ins[1].output_operand.operand_type.reg 			= IR_REG_EBP;

	cisc->ins[2].opcode 									= IR_ADD;
	cisc->ins[2].nb_input_operand 							= 2;
	cisc->ins[2].input_operand[0].size 						= 32;
	cisc->ins[2].input_operand[0].variable 					= NULL;
	cisc->ins[2].input_operand[0].type 						= ASM_OPERAND_REG;
	cisc->ins[2].input_operand[0].operand_type.reg 			= IR_REG_ESP;
	cisc->ins[2].input_operand[1].size 						= 32;
	cisc->ins[2].input_operand[1].variable 					= NULL;
	cisc->ins[2].input_operand[1].type 						= ASM_OPERAND_IMM;
	cisc->ins[2].input_operand[1].operand_type.imm 			= 4;
	cisc->ins[2].output_operand.size 						= 32;
	cisc->ins[2].output_operand.variable 					= NULL;
	cisc->ins[2].output_operand.type 						= ASM_OPERAND_REG;
	cisc->ins[2].output_operand.operand_type.reg 			= IR_REG_ESP;
}

static void cisc_decode_special_pop(xed_decoded_inst_t* xedd, struct asmCiscIns* cisc){
	cisc->valid 											= 1;
	cisc->nb_ins 											= 2;

	cisc->ins[0].opcode 									= IR_MOV;
	cisc->ins[0].nb_input_operand 							= 1;
	asmOperand_decode(xedd, &(cisc->ins[0].output_operand), 1, ASM_OPERAND_ROLE_WRITE_1, NULL);
	cisc->ins[0].input_operand[0].size 						= cisc->ins[0].output_operand.size;
	cisc->ins[0].input_operand[0].variable 					= NULL;
	cisc->ins[0].input_operand[0].type 						= ASM_OPERAND_MEM;
	cisc->ins[0].input_operand[0].operand_type.mem.base 	= XED_REG_ESP;
	cisc->ins[0].input_operand[0].operand_type.mem.index 	= XED_REG_INVALID;
	cisc->ins[0].input_operand[0].operand_type.mem.scale 	= 1;
	cisc->ins[0].input_operand[0].operand_type.mem.disp 	= 0;

	cisc->ins[1].opcode 									= IR_ADD;
	cisc->ins[1].nb_input_operand 							= 2;
	cisc->ins[1].input_operand[0].size 						= 32;
	cisc->ins[1].input_operand[0].variable 					= NULL;
	cisc->ins[1].input_operand[0].type 						= ASM_OPERAND_REG;
	cisc->ins[1].input_operand[0].operand_type.reg 			= IR_REG_ESP;
	cisc->ins[1].input_operand[1].size 						= 32;
	cisc->ins[1].input_operand[1].variable 					= NULL;
	cisc->ins[1].input_operand[1].type 						= ASM_OPERAND_IMM;
	cisc->ins[1].input_operand[1].operand_type.imm 			= cisc->ins[0].output_operand.size / 8;
	cisc->ins[1].output_operand.size 						= 32;
	cisc->ins[1].output_operand.variable 					= NULL;
	cisc->ins[1].output_operand.type 						= ASM_OPERAND_REG;
	cisc->ins[1].output_operand.operand_type.reg 			= IR_REG_ESP;
}

static void cisc_decode_special_push(xed_decoded_inst_t* xedd, struct asmCiscIns* cisc){
	cisc->valid 											= 1;
	cisc->nb_ins 											= 2;

	cisc->ins[1].opcode 									= IR_MOV;
	asmOperand_decode(xedd, cisc->ins[1].input_operand, 1, ASM_OPERAND_ROLE_READ_1, &(cisc->ins[1].nb_input_operand));
	asmOperand_sign_extend(cisc->ins[1].input_operand, 32);
	cisc->ins[1].output_operand.size 						= cisc->ins[1].input_operand[0].size;
	cisc->ins[1].output_operand.variable 					= NULL;
	cisc->ins[1].output_operand.type 						= ASM_OPERAND_MEM;
	cisc->ins[1].output_operand.operand_type.mem.base 		= XED_REG_ESP;
	cisc->ins[1].output_operand.operand_type.mem.index 		= XED_REG_INVALID;
	cisc->ins[1].output_operand.operand_type.mem.scale 		= 1;
	cisc->ins[1].output_operand.operand_type.mem.disp 		= 0;

	cisc->ins[0].opcode 									= IR_SUB;
	cisc->ins[0].nb_input_operand 							= 2;
	cisc->ins[0].input_operand[0].size 						= 32;
	cisc->ins[0].input_operand[0].variable 					= NULL;
	cisc->ins[0].input_operand[0].type 						= ASM_OPERAND_REG;
	cisc->ins[0].input_operand[0].operand_type.reg 			= IR_REG_ESP;
	cisc->ins[0].input_operand[1].size 						= 32;
	cisc->ins[0].input_operand[1].variable 					= NULL;
	cisc->ins[0].input_operand[1].type 						= ASM_OPERAND_IMM;
	cisc->ins[0].input_operand[1].operand_type.imm 			= cisc->ins[1].input_operand[0].size / 8;
	cisc->ins[0].output_operand.size 						= 32;
	cisc->ins[0].output_operand.variable 					= NULL;
	cisc->ins[0].output_operand.type 						= ASM_OPERAND_REG;
	cisc->ins[0].output_operand.operand_type.reg 			= IR_REG_ESP;
}

static void cisc_decode_special_ret(struct asmCiscIns* cisc){
	cisc->valid 											= 1;
	cisc->nb_ins 											= 1;

	cisc->ins[0].opcode 									= IR_ADD;
	cisc->ins[0].nb_input_operand 							= 2;
	cisc->ins[0].input_operand[0].size 						= 32;
	cisc->ins[0].input_operand[0].variable 					= NULL;
	cisc->ins[0].input_operand[0].type 						= ASM_OPERAND_REG;
	cisc->ins[0].input_operand[0].operand_type.reg 			= IR_REG_ESP;
	cisc->ins[0].input_operand[1].size 						= 32;
	cisc->ins[0].input_operand[1].variable 					= NULL;
	cisc->ins[0].input_operand[1].type 						= ASM_OPERAND_IMM;
	cisc->ins[0].input_operand[1].operand_type.imm 			= 4;
	cisc->ins[0].output_operand.size 						= 32;
	cisc->ins[0].output_operand.variable 					= NULL;
	cisc->ins[0].output_operand.type 						= ASM_OPERAND_REG;
	cisc->ins[0].output_operand.operand_type.reg 			= IR_REG_ESP;
}