#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "irImporterAsm.h"
#include "graph.h"
#include "irRenameEngine.h"

#define IRIMPORTERASM_MAX_INPUT_VARIABLE 4

struct asmInputVariable{
	uint32_t 			nb_input;
	struct node* 		base_variable;
	struct node* 		index_variable;
	struct node* 		disp_variable;
	struct node* 		variables[IRIMPORTERASM_MAX_INPUT_VARIABLE];
};

struct asmOutputVariable{
	struct node* 		base_variable;
	struct node* 		index_variable;
	struct node* 		disp_variable;
	struct node* 		variable;
};

static void asmInputVariable_fetch(struct irRenameEngine* engine, struct asmInputVariable* input_variables, xed_decoded_inst_t* xedd);
static void asmOutputVariable_fetch(struct irRenameEngine* engine, struct asmOutputVariable* output_variables, xed_decoded_inst_t* xedd);
static void asmVariable_build_mem_address(struct irRenameEngine* engine, struct node* base, struct node* index, struct node* disp, struct node** address);
static void asmVariable_build_dependence(struct ir* ir, struct asmInputVariable* input_variables, struct asmOutputVariable* output_variables);

static enum irOpcode xedOpcode_2_irOpcode(xed_iclass_enum_t xed_opcode);
static enum irRegister xedRegister_2_irRegister(xed_reg_enum_t xed_reg);

static void special_instruction_dec(struct irRenameEngine* engine, struct asmInputVariable* input_variables, struct asmOutputVariable* output_variables, xed_decoded_inst_t* xedd);
static void special_instruction_inc(struct irRenameEngine* engine, struct asmInputVariable* input_variables, struct asmOutputVariable* output_variables, xed_decoded_inst_t* xedd);
static void special_instruction_lea(struct irRenameEngine* engine, struct asmInputVariable* input_variables, struct asmOutputVariable* output_variables, xed_decoded_inst_t* xedd);
static void special_instruction_leave(struct irRenameEngine* engine, struct asmInputVariable* input_variables, struct asmOutputVariable* output_variables);
static void special_instruction_mov(struct irRenameEngine* engine, struct asmInputVariable* input_variables, struct asmOutputVariable* output_variables, xed_decoded_inst_t* xedd);
static void special_instruction_pop(struct irRenameEngine* engine, struct asmInputVariable* input_variables, struct asmOutputVariable* output_variables, xed_decoded_inst_t* xedd);
static void special_instruction_push(struct irRenameEngine* engine, struct asmInputVariable* input_variables, struct asmOutputVariable* output_variables, xed_decoded_inst_t* xedd);


int32_t irImporterAsm_import(struct ir* ir){
	struct asmInputVariable 	input_variables;
	struct asmOutputVariable 	output_variables;
	struct irRenameEngine 		engine;
	struct instructionIterator 	it;

	irRenameEngine_init(engine, ir)

	if (assembly_get_instruction(&(ir->trace->assembly), &it, 0)){
		printf("ERROR: in %s, unable to fetch first instruction from the assembly\n", __func__);
		return -1;
	}

	for (;;){
		switch(xed_decoded_inst_get_iclass(&(it.xedd))){
			case XED_ICLASS_BSWAP 		: {break;}
			case XED_ICLASS_CALL_NEAR 	: {break;}
			case XED_ICLASS_CMP 		: {break;}
			case XED_ICLASS_DEC 		: {
				special_instruction_dec(&engine, &input_variables, &output_variables, &(it.xedd));
				break;
			}
			case XED_ICLASS_INC 		: {
				special_instruction_inc(&engine, &input_variables, &output_variables, &(it.xedd));
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
			case XED_ICLASS_LEA 		: {
				special_instruction_lea(&engine, &input_variables, &output_variables, &(it.xedd));
				break;
			}
			case XED_ICLASS_LEAVE 		: {
				special_instruction_leave(&engine, &input_variables, &output_variables);
				break;
			}
			case XED_ICLASS_MOV 		: {
				special_instruction_mov(&engine, &input_variables, &output_variables, &(it.xedd));
				break;
			}
			case XED_ICLASS_NOP 		: {break;}
			case XED_ICLASS_POP 		: {
				special_instruction_pop(&engine, &input_variables, &output_variables, &(it.xedd));
				break;
			}
			case XED_ICLASS_PUSH 		: {
				special_instruction_push(&engine, &input_variables, &output_variables, &(it.xedd));
				break;
			}
			case XED_ICLASS_RET_FAR 	: {break;}
			case XED_ICLASS_RET_NEAR 	: {break;}
			case XED_ICLASS_TEST 		: {break;}
			default :{
				asmInputVariable_fetch(&engine, &input_variables, &(it.xedd));
				asmOutputVariable_fetch(&engine, &output_variables, &(it.xedd));
				asmVariable_build_dependence(ir, &input_variables, &output_variables);
			}
		}

		if (instructionIterator_get_instruction_index(&it) ==  assembly_get_nb_instruction(&(ir->trace->assembly)) - 1){
			break;
		}
		else{
			if (assembly_get_next_instruction(&(ir->trace->assembly), &it)){
				printf("ERROR: in %s, unable to fetch next instruction from the assembly\n", __func__);
				return -1;
			}
		}
	}

	irRenameEngine_tag_final_node(&engine);

	return 0;
}

static void asmInputVariable_fetch(struct irRenameEngine* engine, struct asmInputVariable* input_variables, xed_decoded_inst_t* xedd){
	uint32_t 				i;
	const xed_inst_t* 		xi = xed_decoded_inst_inst(xedd);
	const xed_operand_t* 	xed_op;
	xed_operand_enum_t 		op_name;
	uint8_t 				nb_memops;

	for (i = 0, nb_memops = 0, input_variables->nb_input = 0, input_variables->base_variable = NULL, input_variables->index_variable = NULL, input_variables->disp_variable = NULL; i < xed_inst_noperands(xi); i++){
		xed_op = xed_inst_operand(xi, i);
		if (xed_operand_read(xed_op)){
			op_name = xed_operand_name(xed_op);

			switch(op_name){
				case XED_OPERAND_AGEN 	:
				case XED_OPERAND_MEM0 	:
				case XED_OPERAND_MEM1 	: {
					xed_reg_enum_t 	base;
					xed_reg_enum_t 	index;
					uint64_t 		displacement;

					base = xed_decoded_inst_get_base_reg(xedd, nb_memops);
					if (base != XED_REG_INVALID){
						input_variables->base_variable = irRenameEngine_get_register_ref(engine, xedRegister_2_irRegister(base));
						if (input_variables->base_variable == NULL){
							printf("ERROR: in %s, unable to get register reference from the renaming engine\n", __func__);
						}
					}

					index = xed_decoded_inst_get_index_reg(xedd, nb_memops);
					if (index != XED_REG_INVALID){
						input_variables->index_variable = irRenameEngine_get_register_ref(engine, xedRegister_2_irRegister(index));
						if (input_variables->index_variable == NULL){
							printf("ERROR: in %s, unable to get register reference from the renaming engine\n", __func__);
						}
					}

					displacement = xed_decoded_inst_get_memory_displacement(xedd, nb_memops);
					if (displacement != 0){
						input_variables->disp_variable = ir_add_immediate(engine->ir, xed_decoded_inst_get_memory_displacement_width(xedd, nb_memops) * 8, 0, displacement);
						if (input_variables->disp_variable == NULL){
							printf("ERROR: in %s, unable to add immediate to IR\n", __func__);
						}
					}

					if (op_name != XED_OPERAND_AGEN){
						struct node* address;

						asmVariable_build_mem_address(engine, input_variables->base_variable, input_variables->index_variable, input_variables->disp_variable, &address);
						if (address != NULL){
							input_variables->nb_input[input_variables->variables] = ir_add_in_mem(engine->ir, address, xed_decoded_inst_get_memory_operand_length(xedd, nb_memops) * 8, irRenameEngine_get_mem_order(engine));
							if (input_variables->variables[input_variables->nb_input] != NULL){
								input_variables->nb_input ++;
								if (input_variables->nb_input == IRIMPORTERASM_MAX_INPUT_VARIABLE){
									printf("ERROR: in %s, the max number of input variable has been reached\n", __func__);
									break;
								}
							}
							else{
								printf("ERROR: in %s, unable to add memory load to IR\n", __func__);
							}
						}
						else{
							printf("ERROR: in %s, unable to build mem address\n", __func__);
						}
					}
					
					nb_memops ++;
					break;
				}
				case XED_OPERAND_IMM0 	: {
					uint16_t 	width;
					uint8_t 	signe;
					uint64_t 	value;

					width = xed_decoded_inst_get_immediate_width_bits(xedd);
					signe = xed_decoded_inst_get_immediate_is_signed(xedd);
					if (signe){
						value = (uint64_t)xed_decoded_inst_get_signed_immediate(xedd);
					}
					else{
						value = xed_decoded_inst_get_unsigned_immediate(xedd);
					}

					input_variables->variables[input_variables->nb_input] = ir_add_immediate(engine->ir, width, signe, value);
					if (input_variables->variables[input_variables->nb_input] == NULL){
						printf("ERROR: in %s, unable to add immediate to IR\n", __func__);
					}
					else{
						input_variables->nb_input ++;
						if (input_variables->nb_input == IRIMPORTERASM_MAX_INPUT_VARIABLE){
							printf("ERROR: in %s, the max number of input variable has been reached\n", __func__);
							break;
						}
					}

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
					input_variables->variables[input_variables->nb_input] = irRenameEngine_get_register_ref(engine, xedRegister_2_irRegister(xed_decoded_inst_get_reg(xedd, op_name)));
					if (input_variables->variables[input_variables->nb_input] == NULL){
						printf("ERROR: in %s, unable to register reference from the renaming engine\n", __func__);
					}
					else{
						input_variables->nb_input ++;
						if (input_variables->nb_input == IRIMPORTERASM_MAX_INPUT_VARIABLE){
							printf("ERROR: in %s, the max number of input variable has been reached\n", __func__);
							break;
						}
					}

					break;
				}
				case XED_OPERAND_BASE0 : {
					/* IGNORE: this is ESP for a stack instruction (example: PUSH) */
					break;
				}
				case XED_OPERAND_BASE1 : {
					/* IGNORE: this is ESP for stack instrcution (example: PUSH) */
					break;
				}
				default : {
					printf("ERROR: in %s, operand type not supported: %s, opcode: %s\n", __func__, xed_operand_enum_t2str(op_name), xed_iclass_enum_t2str(xed_decoded_inst_get_iclass(xedd)));
					break;
				}
			}
		}
		else{
			op_name = xed_operand_name(xed_op);
			if (op_name == XED_OPERAND_AGEN || op_name == XED_OPERAND_MEM0 || op_name == XED_OPERAND_MEM1){
				nb_memops ++;
			}
		}
	}
}

static void asmOutputVariable_fetch(struct irRenameEngine* engine, struct asmOutputVariable* output_variables, xed_decoded_inst_t* xedd){
	uint32_t 				i;
	const xed_inst_t* 		xi = xed_decoded_inst_inst(xedd);
	const xed_operand_t* 	xed_op;
	xed_operand_enum_t 		op_name;
	uint8_t 				nb_memops;

	for (i = 0, nb_memops = 0, output_variables->variable = NULL, output_variables->base_variable = NULL, output_variables->index_variable = NULL, output_variables->disp_variable = NULL; i < xed_inst_noperands(xi) && output_variables->variable == NULL; i++){
		xed_op = xed_inst_operand(xi, i);
		if (xed_operand_written(xed_op)){
			op_name = xed_operand_name(xed_op);

			switch(op_name){
				case XED_OPERAND_MEM0 	:
				case XED_OPERAND_MEM1 	: {
					xed_reg_enum_t 	base;
					xed_reg_enum_t 	index;
					uint64_t 		displacement;

					base = xed_decoded_inst_get_base_reg(xedd, nb_memops);
					if (base != XED_REG_INVALID){
						output_variables->base_variable = irRenameEngine_get_register_ref(engine, xedRegister_2_irRegister(base));
						if (output_variables->base_variable == NULL){
							printf("ERROR: in %s, unable to get register reference from the renaming engine\n", __func__);
						}
					}

					index = xed_decoded_inst_get_index_reg(xedd, nb_memops);
					if (index != XED_REG_INVALID){
						output_variables->index_variable = irRenameEngine_get_register_ref(engine, xedRegister_2_irRegister(index));
						if (output_variables->index_variable == NULL){
							printf("ERROR: in %s, unable to get register reference from the renaming engine\n", __func__);
						}
					}

					displacement = xed_decoded_inst_get_memory_displacement(xedd, nb_memops);
					if (displacement != 0){
						output_variables->disp_variable = ir_add_immediate(engine->ir, xed_decoded_inst_get_memory_displacement_width(xedd, nb_memops) * 8, 0, displacement);
						if (output_variables->disp_variable == NULL){
							printf("ERROR: in %s, unable to add immediate to IR\n", __func__);
						}
					}

					if (op_name != XED_OPERAND_AGEN){
						struct node* address;
						struct node* mem;

						asmVariable_build_mem_address(engine, output_variables->base_variable, output_variables->index_variable, output_variables->disp_variable, &address);
						if (address != NULL){
							output_variables->variable = ir_add_inst(engine->ir, xedOpcode_2_irOpcode(xed_decoded_inst_get_iclass(xedd)), xed_decoded_inst_get_memory_operand_length(xedd, nb_memops) * 8);
							if (output_variables->variable == NULL){
								printf("ERROR: in %s, unable to add operation to IR\n", __func__);
								continue;
							}
							mem = ir_add_out_mem(engine->ir, address, xed_decoded_inst_get_memory_operand_length(xedd, nb_memops) * 8, irRenameEngine_get_mem_order(engine));
							if (mem == NULL){
								printf("ERROR: in %s, unable to add memory write to IR\n", __func__);
							}
							else{
								if (ir_add_dependence(engine->ir, output_variables->variable, mem, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
									printf("ERROR: in %s, unable to add output to add dependence to IR\n", __func__);
								}
							}
						}
						else{
							printf("ERROR: in %s, unable to build mem address\n", __func__);
						}
					}
					
					nb_memops ++;
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
					enum irRegister reg;

					reg = xedRegister_2_irRegister(xed_decoded_inst_get_reg(xedd, op_name));
					output_variables->variable = ir_add_inst(engine->ir, xedOpcode_2_irOpcode(xed_decoded_inst_get_iclass(xedd)), irRegister_get_size(reg));
					if (output_variables->variable == NULL){
						printf("ERROR: in %s, unable to add operation to IR\n", __func__);
						continue;
					}
					irRenameEngine_set_register_ref(engine, reg, output_variables->variable);
					break;
				}
				default : {
					printf("ERROR: in %s, operand type not supported: %s, opcode: %s\n", __func__, xed_operand_enum_t2str(op_name), xed_iclass_enum_t2str(xed_decoded_inst_get_iclass(xedd)));
					break;
				}
			}
		}
		else{
			op_name = xed_operand_name(xed_op);
			if (op_name == XED_OPERAND_AGEN || op_name == XED_OPERAND_MEM0 || op_name == XED_OPERAND_MEM1){
				nb_memops ++;
			}
		}
	}
}

static void asmVariable_build_mem_address(struct irRenameEngine* engine, struct node* base, struct node* index, struct node* disp, struct node** address){
	uint8_t 		nb_operand = 0;
	struct node* 	operands[3];

	if (base != NULL){
		operands[nb_operand] = base;
		nb_operand ++;
	}

	if (index != NULL){
		operands[nb_operand] = index;
		nb_operand ++;
	}

	if (disp != NULL){
		operands[nb_operand] = disp;
		nb_operand ++;
	}

	switch(nb_operand){
		case 1 : {
			*address = operands[0];
			break;
		}
		case 2 : {
			*address = ir_add_inst(engine->ir, IR_ADD, 32);
			if (*address != NULL){
				if (ir_add_dependence(engine->ir, operands[0], *address, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
					printf("ERROR: in %s, unable to add dependence between ir nodes\n", __func__);
				}
				if (ir_add_dependence(engine->ir, operands[1], *address, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
					printf("ERROR: in %s, unable to add dependence between ir nodes\n", __func__);
				}
			}
			break;
		}
		case 3 : {
			*address = ir_add_inst(engine->ir, IR_ADD, 32);
			if (*address != NULL){
				if (ir_add_dependence(engine->ir, operands[0], *address, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
					printf("ERROR: in %s, unable to add dependence between ir nodes\n", __func__);
				}
				if (ir_add_dependence(engine->ir, operands[1], *address, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
					printf("ERROR: in %s, unable to add dependence between ir nodes\n", __func__);
				}
				if (ir_add_dependence(engine->ir, operands[2], *address, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
					printf("ERROR: in %s, unable to add dependence between ir nodes\n", __func__);
				}
			}
			break;
		}
		default : {
			printf("ERROR: in %s, incorrect number of operand(s): %u\n", __func__, nb_operand);
			*address = NULL;

		}
	}
}

static void asmVariable_build_dependence(struct ir* ir, struct asmInputVariable* input_variables, struct asmOutputVariable* output_variables){
	uint32_t i;

	if (output_variables->variable != NULL){
		for (i = 0; i < input_variables->nb_input; i++){
			if (ir_add_dependence(ir, input_variables->variables[i], output_variables->variable, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
				printf("ERROR: in %s, unable to add output to add dependence to IR\n", __func__);
			}
		}
	}
}

static enum irOpcode xedOpcode_2_irOpcode(xed_iclass_enum_t xed_opcode){
	switch (xed_opcode){
		case XED_ICLASS_ADD 	: {return IR_ADD;}
		case XED_ICLASS_AND 	: {return IR_AND;}
		case XED_ICLASS_DIV		: {return IR_DIV;}
		case XED_ICLASS_DEC 	: {return IR_SUB;}
		case XED_ICLASS_INC 	: {return IR_ADD;}
		case XED_ICLASS_LEA 	: {return IR_ADD;}
		case XED_ICLASS_MOVZX 	: {return IR_MOVZX;}
		case XED_ICLASS_MUL 	: {return IR_MUL;}
		case XED_ICLASS_NOT 	: {return IR_NOT;}
		case XED_ICLASS_OR 		: {return IR_OR;}
		case XED_ICLASS_ROL 	: {return IR_ROL;}
		case XED_ICLASS_ROR 	: {return IR_ROR;}
		case XED_ICLASS_SHL 	: {return IR_SHL;}
		case XED_ICLASS_SHR 	: {return IR_SHR;}
		case XED_ICLASS_SUB 	: {return IR_SUB;}
		case XED_ICLASS_XOR 	: {return IR_XOR;}
		default : {
			printf("ERROR: in %s, this instruction (%s) cannot be translated into ir Opcode\n", __func__, xed_iclass_enum_t2str(xed_opcode));
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
			printf("ERROR: in %s, this register (%s) cannot be translated into ir register\n", __func__, xed_reg_enum_t2str(xed_reg));
			return IR_REG_EAX;
		}
	}
}

static void special_instruction_dec(struct irRenameEngine* engine, struct asmInputVariable* input_variables, struct asmOutputVariable* output_variables, xed_decoded_inst_t* xedd){
	asmInputVariable_fetch(engine, input_variables, xedd);

	if (input_variables->nb_input != IRIMPORTERASM_MAX_INPUT_VARIABLE){
		input_variables->variables[input_variables->nb_input] = ir_add_immediate(engine->ir, 8, 0, 1); /* size may be tuned to fit DST operand */
		if (input_variables->variables[input_variables->nb_input] == NULL){
			printf("ERROR: in %s, unable to add immediate to IR\n", __func__);
		}
		else{
			input_variables->nb_input ++;
		}
	}
	else{
		printf("ERROR: in %s, unable to add IMM variable to input list\n", __func__);
	}

	asmOutputVariable_fetch(engine, output_variables, xedd);
	asmVariable_build_dependence(engine->ir, input_variables, output_variables);
}

static void special_instruction_inc(struct irRenameEngine* engine, struct asmInputVariable* input_variables, struct asmOutputVariable* output_variables, xed_decoded_inst_t* xedd){
	asmInputVariable_fetch(engine, input_variables, xedd);

	if (input_variables->nb_input != IRIMPORTERASM_MAX_INPUT_VARIABLE){
		input_variables->variables[input_variables->nb_input] = ir_add_immediate(engine->ir, 8, 0, 1);  /* size may be tuned to fit DST operand */
		if (input_variables->variables[input_variables->nb_input] == NULL){
			printf("ERROR: in %s, unable to add immediate to IR\n", __func__);
		}
		else{
			input_variables->nb_input ++;
		}
	}
	else{
		printf("ERROR: in %s, unable to add IMM variable to input list\n", __func__);
	}

	asmOutputVariable_fetch(engine, output_variables, xedd);
	asmVariable_build_dependence(engine->ir, input_variables, output_variables);
}

static void special_instruction_lea(struct irRenameEngine* engine, struct asmInputVariable* input_variables, struct asmOutputVariable* output_variables, xed_decoded_inst_t* xedd){
	uint32_t 				i;
	const xed_inst_t* 		xi = xed_decoded_inst_inst(xedd);
	const xed_operand_t* 	xed_op;
	xed_operand_enum_t 		op_name;

	asmInputVariable_fetch(engine, input_variables, xedd);

	for (i = 0, output_variables->variable = NULL; i < xed_inst_noperands(xi) && output_variables->variable == NULL; i++){
		xed_op = xed_inst_operand(xi, i);
		if (xed_operand_written(xed_op)){
			op_name = xed_operand_name(xed_op);

			switch(op_name){
				case XED_OPERAND_REG0 	: {
					enum irRegister reg;
					
					reg = xedRegister_2_irRegister(xed_decoded_inst_get_reg(xedd, op_name));
					output_variables->variable = ir_add_inst(engine->ir, xedOpcode_2_irOpcode(xed_decoded_inst_get_iclass(xedd)), irRegister_get_size(reg));
					if (output_variables->variable == NULL){
						printf("ERROR: in %s, unable to add operation to IR\n", __func__);
						continue;
					}
					irRenameEngine_set_register_ref(engine, reg, output_variables->variable);
					
					if (input_variables->base_variable != NULL){
						if (ir_add_dependence(engine->ir, input_variables->base_variable, output_variables->variable, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
							printf("ERROR: in %s, unable to add dependence to IR\n", __func__);
						}
					}
					if (input_variables->index_variable != NULL){
						if (ir_add_dependence(engine->ir, input_variables->index_variable, output_variables->variable, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
							printf("ERROR: in %s, unable to add dependence to IR\n", __func__);
						}
					}
					if (input_variables->disp_variable != NULL){
						if (ir_add_dependence(engine->ir, input_variables->disp_variable, output_variables->variable, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
							printf("ERROR: in %s, unable to add dependence to IR\n", __func__);
						}
					}

					break;
				}
				default : {
					printf("ERROR: in %s, operand type not supported: %s, opcode: %s\n", __func__, xed_operand_enum_t2str(op_name), xed_iclass_enum_t2str(xed_decoded_inst_get_iclass(xedd)));
					break;
				}
			}
		}
	}
}

static void special_instruction_leave(struct irRenameEngine* engine, struct asmInputVariable* input_variables, struct asmOutputVariable* output_variables){
	struct node* 	ebp_register;
	struct node* 	address;

	ebp_register = irRenameEngine_get_register_ref(engine, IR_REG_EBP);
	if (ebp_register != NULL){
		irRenameEngine_set_register_ref(engine, IR_REG_ESP, ebp_register);
	}
	else{
		printf("ERROR: in %s, unable to get register reference from the renaming engine\n", __func__);
	}

	input_variables->nb_input 			= 1;
	input_variables->base_variable 		= irRenameEngine_get_register_ref(engine, IR_REG_ESP);
	if (input_variables->base_variable == NULL){
		printf("ERROR: in %s, unable to get register reference from the renaming engine\n", __func__);
	}
	input_variables->index_variable 	= NULL;
	input_variables->disp_variable		= NULL;
	input_variables->variables[0] 		= NULL;

	asmVariable_build_mem_address(engine, input_variables->base_variable, input_variables->index_variable, input_variables->disp_variable, &address);
	if (address != NULL){
		input_variables->variables[0] = ir_add_in_mem(engine->ir, address, 32, irRenameEngine_get_mem_order(engine));
		if (input_variables->variables[0] != NULL){
			irRenameEngine_set_register_ref(engine, IR_REG_EBP, input_variables->variables[0]);
		}
		else{
			printf("ERROR: in %s, unable to add memory load to IR\n", __func__);
		}
	}
	else{
		printf("ERROR: in %s, unable to build mem address\n", __func__);
	}

	if (input_variables->variables[0] != NULL){
		input_variables->nb_input 			= 0;
		input_variables->base_variable 		= NULL;
		input_variables->index_variable 	= NULL;
		input_variables->disp_variable		= NULL;
		input_variables->variables[0] 		= irRenameEngine_get_register_ref(engine, IR_REG_ESP);
		if (input_variables->variables[0] == NULL){
			printf("ERROR: in %s, unable to get register reference from the renaming engine\n", __func__);
		}
		else{
			input_variables->nb_input ++;
		}

		if (input_variables->nb_input != IRIMPORTERASM_MAX_INPUT_VARIABLE){
			input_variables->variables[input_variables->nb_input] = ir_add_immediate(engine->ir, 32, 0, 4);
			if (input_variables->variables[input_variables->nb_input] == NULL){
				printf("ERROR: in %s, unable to add immediate to IR\n", __func__);
			}
			else{
				input_variables->nb_input ++;
			}
		}
		else{
			printf("ERROR: in %s, unable to add IMM variable to input list\n", __func__);
		}

		output_variables->variable = ir_add_inst(engine->ir, IR_ADD, 32);
		if (output_variables->variable == NULL){
			printf("ERROR: in %s, unable to add operation to IR\n", __func__);
		}
		else{
			irRenameEngine_set_register_ref(engine, IR_REG_ESP, output_variables->variable);
		}

		asmVariable_build_dependence(engine->ir, input_variables, output_variables);
	}
}

static void special_instruction_mov(struct irRenameEngine* engine, struct asmInputVariable* input_variables, struct asmOutputVariable* output_variables, xed_decoded_inst_t* xedd){
	uint32_t 				i;
	const xed_inst_t* 		xi = xed_decoded_inst_inst(xedd);
	const xed_operand_t* 	xed_op;
	xed_operand_enum_t 		op_name;
	uint8_t 				nb_memops;

	asmInputVariable_fetch(engine, input_variables, xedd);

	for (i = 0, nb_memops = 0, output_variables->variable = NULL, output_variables->base_variable = NULL, output_variables->index_variable = NULL, output_variables->disp_variable = NULL; i < xed_inst_noperands(xi); i++){
		xed_op = xed_inst_operand(xi, i);
		if (xed_operand_written(xed_op)){
			op_name = xed_operand_name(xed_op);

			switch(op_name){
				case XED_OPERAND_MEM0 	:
				case XED_OPERAND_MEM1 	: {
					xed_reg_enum_t 	base;
					xed_reg_enum_t 	index;
					uint64_t 		displacement;
					struct node* 	address;
					struct node* 	mem;

					base = xed_decoded_inst_get_base_reg(xedd, nb_memops);
					if (base != XED_REG_INVALID){
						output_variables->base_variable = irRenameEngine_get_register_ref(engine, xedRegister_2_irRegister(base));
						if (output_variables->base_variable == NULL){
							printf("ERROR: in %s, unable to get register reference from the renaming engine\n", __func__);
						}
					}

					index = xed_decoded_inst_get_index_reg(xedd, nb_memops);
					if (index != XED_REG_INVALID){
						output_variables->index_variable = irRenameEngine_get_register_ref(engine, xedRegister_2_irRegister(index));
						if (output_variables->index_variable == NULL){
							printf("ERROR: in %s, unable to get register reference from the renaming engine\n", __func__);
						}
					}

					displacement = xed_decoded_inst_get_memory_displacement(xedd, nb_memops);
					if (displacement != 0){
						output_variables->disp_variable = ir_add_immediate(engine->ir, xed_decoded_inst_get_memory_displacement_width(xedd, nb_memops) * 8, 0, displacement);
						if (output_variables->disp_variable == NULL){
							printf("ERROR: in %s, unable to add immediate to IR\n", __func__);
						}
					}

					asmVariable_build_mem_address(engine, output_variables->base_variable, output_variables->index_variable, output_variables->disp_variable, &address);
					if (address != NULL){
						mem = ir_add_out_mem(engine->ir, address, xed_decoded_inst_get_memory_operand_length(xedd, nb_memops) * 8, irRenameEngine_get_mem_order(engine));
						if (mem == NULL){
							printf("ERROR: in %s, unable to add memory write to IR\n", __func__);
						}
						else{
							if (ir_add_dependence(engine->ir, input_variables->variables[0], mem, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
								printf("ERROR: in %s, unable to add output to add dependence to IR\n", __func__);
							}
						}
					}
					else{
						printf("ERROR: in %s, unable to build mem address\n", __func__);
					}

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
					irRenameEngine_set_register_ref(engine, xedRegister_2_irRegister(xed_decoded_inst_get_reg(xedd, op_name)), input_variables->variables[0]);
					break;
				}
				default : {
					printf("ERROR: in %s, operand type not supported: %s, opcode: %s\n", __func__, xed_operand_enum_t2str(op_name), xed_iclass_enum_t2str(xed_decoded_inst_get_iclass(xedd)));
					break;
				}
			}
		}
		else{
			op_name = xed_operand_name(xed_op);
			if (op_name == XED_OPERAND_AGEN || op_name == XED_OPERAND_MEM0 || op_name == XED_OPERAND_MEM1){
				nb_memops ++;
			}
		}
	}
}

static void special_instruction_pop(struct irRenameEngine* engine, struct asmInputVariable* input_variables, struct asmOutputVariable* output_variables, xed_decoded_inst_t* xedd){
	uint32_t 				i;
	const xed_inst_t* 		xi = xed_decoded_inst_inst(xedd);
	const xed_operand_t* 	xed_op;
	xed_operand_enum_t 		op_name;
	uint8_t 				nb_memops;
	struct node* 			address;
	uint32_t 				order;
	uint8_t 				size = 0;

	input_variables->nb_input 			= 1;
	input_variables->base_variable 		= irRenameEngine_get_register_ref(engine, IR_REG_ESP);
	if (input_variables->base_variable == NULL){
		printf("ERROR: in %s, unable to get register reference from the renaming engine\n", __func__);
	}
	input_variables->index_variable 	= NULL;
	input_variables->disp_variable		= NULL;
	input_variables->variables[0] 		= NULL;

	asmVariable_build_mem_address(engine, input_variables->base_variable, input_variables->index_variable, input_variables->disp_variable, &address);
	if (address != NULL){
		order = irRenameEngine_get_mem_order(engine);

		for (i = 0, nb_memops = 0, output_variables->variable = NULL, output_variables->base_variable = NULL, output_variables->index_variable = NULL, output_variables->disp_variable = NULL; i < xed_inst_noperands(xi); i++){
			xed_op = xed_inst_operand(xi, i);
			if (xed_operand_written(xed_op)){
				op_name = xed_operand_name(xed_op);

				switch(op_name){
					case XED_OPERAND_MEM0 	:
					case XED_OPERAND_MEM1 	: {
						xed_reg_enum_t 	base;
						xed_reg_enum_t 	index;
						uint64_t 		displacement;
						struct node* 	addr;

						base = xed_decoded_inst_get_base_reg(xedd, nb_memops);
						if (base != XED_REG_INVALID){
							output_variables->base_variable = irRenameEngine_get_register_ref(engine, xedRegister_2_irRegister(base));
							if (output_variables->base_variable == NULL){
								printf("ERROR: in %s, unable to get register reference from the renaming engine\n", __func__);
							}
						}

						index = xed_decoded_inst_get_index_reg(xedd, nb_memops);
						if (index != XED_REG_INVALID){
							output_variables->index_variable = irRenameEngine_get_register_ref(engine, xedRegister_2_irRegister(index));
							if (output_variables->index_variable == NULL){
								printf("ERROR: in %s, unable to get register reference from the renaming engine\n", __func__);
							}
						}

						displacement = xed_decoded_inst_get_memory_displacement(xedd, nb_memops);
						if (displacement != 0){
							output_variables->disp_variable = ir_add_immediate(engine->ir, xed_decoded_inst_get_memory_displacement_width(xedd, nb_memops) * 8, 0, displacement);
							if (output_variables->disp_variable == NULL){
								printf("ERROR: in %s, unable to add immediate to IR\n", __func__);
							}
						}

						asmVariable_build_mem_address(engine, output_variables->base_variable, output_variables->index_variable, output_variables->disp_variable, &addr);
						if (addr != NULL){
							output_variables->variable = ir_add_out_mem(engine->ir, addr, xed_decoded_inst_get_memory_operand_length(xedd, nb_memops) * 8, irRenameEngine_get_mem_order(engine));
							if (output_variables->variable == NULL){
								printf("ERROR: in %s, unable to add memory write to IR\n", __func__);
							}
							else{
								size = xed_decoded_inst_get_memory_operand_length(xedd, nb_memops);
								input_variables->variables[0] = ir_add_in_mem(engine->ir, address, size * 8, order);
								if (input_variables->variables[0] != NULL){
									if (ir_add_dependence(engine->ir, input_variables->variables[0], output_variables->variable, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
										printf("ERROR: in %s, unable to add output to add dependence to IR\n", __func__);
									}
								}
								else{
									printf("ERROR: in %s, unable to add memory load to IR\n", __func__);
								}
							}
						}
						else{
							printf("ERROR: in %s, unable to build mem address\n", __func__);
						}

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
						enum irRegister reg;

						reg = xedRegister_2_irRegister(xed_decoded_inst_get_reg(xedd, op_name));
						size = irRegister_get_size(reg) / 8;
						input_variables->variables[0] = ir_add_in_mem(engine->ir, address, size * 8, order);
						if (input_variables->variables[0] != NULL){
							irRenameEngine_set_register_ref(engine, reg, input_variables->variables[0]);
						}
						else{
							printf("ERROR: in %s, unable to add memory load to IR\n", __func__);
						}
						break;
					}
					case XED_OPERAND_BASE0 : {
						break;
					}
					default : {
						printf("ERROR: in %s, operand type not supported: %s, opcode: %s\n", __func__, xed_operand_enum_t2str(op_name), xed_iclass_enum_t2str(xed_decoded_inst_get_iclass(xedd)));
						break;
					}
				}
			}
			else{
				op_name = xed_operand_name(xed_op);
				if (op_name == XED_OPERAND_AGEN || op_name == XED_OPERAND_MEM0 || op_name == XED_OPERAND_MEM1){
					nb_memops ++;
				}
			}
		}
	}
	else{
		printf("ERROR: in %s, unable to build mem address\n", __func__);
	}

	if (input_variables->variables[0] != NULL){
		input_variables->nb_input 			= 0;
		input_variables->base_variable 		= NULL;
		input_variables->index_variable 	= NULL;
		input_variables->disp_variable		= NULL;
		input_variables->variables[0] 		= irRenameEngine_get_register_ref(engine, IR_REG_ESP);
		if (input_variables->variables[0] == NULL){
			printf("ERROR: in %s, unable to get register reference from the renaming engine\n", __func__);
		}
		else{
			input_variables->nb_input ++;
		}

		if (input_variables->nb_input != IRIMPORTERASM_MAX_INPUT_VARIABLE){
			input_variables->variables[input_variables->nb_input] = ir_add_immediate(engine->ir, 32, 0, size);
			if (input_variables->variables[input_variables->nb_input] == NULL){
				printf("ERROR: in %s, unable to add immediate to IR\n", __func__);
			}
			else{
				input_variables->nb_input ++;
			}
		}
		else{
			printf("ERROR: in %s, unable to add IMM variable to input list\n", __func__);
		}

		output_variables->variable = ir_add_inst(engine->ir, IR_ADD, 32);
		if (output_variables->variable == NULL){
			printf("ERROR: in %s, unable to add operation to IR\n", __func__);
		}
		else{
			irRenameEngine_set_register_ref(engine, IR_REG_ESP, output_variables->variable);
		}

		asmVariable_build_dependence(engine->ir, input_variables, output_variables);
	}
}

static void special_instruction_push(struct irRenameEngine* engine, struct asmInputVariable* input_variables, struct asmOutputVariable* output_variables, xed_decoded_inst_t* xedd){
	struct node* 	address;
	uint8_t 		size = 0;

	asmInputVariable_fetch(engine, input_variables, xedd);

	output_variables->base_variable 		= irRenameEngine_get_register_ref(engine, IR_REG_ESP);
	if (output_variables->base_variable == NULL){
		printf("ERROR: in %s, unable to get register reference from the renaming engine\n", __func__);
	}
	output_variables->index_variable 	= NULL;
	output_variables->disp_variable		= NULL;
	output_variables->variable 			= NULL;

	if (input_variables->nb_input == 1){
		size = ir_node_get_operation(input_variables->variables[0])->size / 8;
		asmVariable_build_mem_address(engine, output_variables->base_variable, output_variables->index_variable, output_variables->disp_variable, &address);
		if (address != NULL){
			output_variables->variable = ir_add_out_mem(engine->ir, address, size * 8, irRenameEngine_get_mem_order(engine));
			if (output_variables->variable != NULL){
				if (ir_add_dependence(engine->ir, input_variables->variables[0], output_variables->variable, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
					printf("ERROR: in %s, unable to add output to add dependence to IR\n", __func__);
				}
			}
			else{
				printf("ERROR: in %s, unable to add memory store to IR\n", __func__);
			}
		}		
		else{
			printf("ERROR: in %s, unable to build mem address\n", __func__);
		}
	}
	else{
		printf("ERROR: in %s, fetch input variable returned a wrong number of input: %u\n", __func__, input_variables->nb_input);
	}

	if (output_variables->variable != NULL){
		input_variables->nb_input 			= 0;
		input_variables->base_variable 		= NULL;
		input_variables->index_variable 	= NULL;
		input_variables->disp_variable		= NULL;
		input_variables->variables[0] 		= irRenameEngine_get_register_ref(engine, IR_REG_ESP);
		if (input_variables->variables[0] == NULL){
			printf("ERROR: in %s, unable to get register reference from the renaming engine\n", __func__);
		}
		else{
			input_variables->nb_input ++;
		}

		if (input_variables->nb_input != IRIMPORTERASM_MAX_INPUT_VARIABLE){
			input_variables->variables[input_variables->nb_input] = ir_add_immediate(engine->ir, 32, 0, size);
			if (input_variables->variables[input_variables->nb_input] == NULL){
				printf("ERROR: in %s, unable to add immediate to IR\n", __func__);
			}
			else{
				input_variables->nb_input ++;
			}
		}
		else{
			printf("ERROR: in %s, unable to add IMM variable to input list\n", __func__);
		}

		output_variables->variable = ir_add_inst(engine->ir, IR_SUB, 32);
		if (output_variables->variable == NULL){
			printf("ERROR: in %s, unable to add operation to IR\n", __func__);
		}
		else{
			irRenameEngine_set_register_ref(engine, IR_REG_ESP, output_variables->variable);
		}

		asmVariable_build_dependence(engine->ir, input_variables, output_variables);
	}
}