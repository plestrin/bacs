#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "irImporterAsm.h"
#include "instruction.h"
#include "graph.h"
#include "irRenameEngine.h"

static struct operand* irImporterAsm_get_input_memory(struct operand* operands, uint32_t nb_operand, uint32_t index);
static struct operand* irImporterAsm_get_input_register(struct operand* operands, uint32_t nb_operand, enum reg reg);
static struct operand* irImporterAsm_get_base_register(struct operand* operands, uint32_t nb_operand, enum reg reg);
static struct operand* irImporterAsm_get_output_register(struct operand* operands, uint32_t nb_operand, enum reg reg);

#define IRIMPORTERASM_MAX_INPUT_VARIABLE 4

struct asmInputVariable{
	uint32_t 			nb_input;
	struct node* 		base_variable;
	struct node* 		index_variable;
	struct node* 		disp_variable;
	struct node* 		variables[IRIMPORTERASM_MAX_INPUT_VARIABLE];
};

struct asmOutputVariable{
	struct node* 		variable;
};

static void asmInputVariable_fetch(struct irRenameEngine* engine, struct asmInputVariable* input_variables, xed_decoded_inst_t* xedd, struct operand* operands, uint32_t nb_operand);
static void asmOutputVariable_fetch(struct irRenameEngine* engine, struct asmOutputVariable* output_variables, xed_decoded_inst_t* xedd, struct operand* operands, uint32_t nb_operand);
static void asmInputVariable_build_dependence(struct ir* ir, struct asmInputVariable* input_variables, struct asmOutputVariable* output_variables);

static enum irOpcode xedOpcode_2_irOpcode(xed_iclass_enum_t xed_opcode);
static enum reg xedRegister_2_reg(xed_reg_enum_t xed_reg);

static void special_instruction_mov(struct irRenameEngine* engine, struct asmInputVariable* input_variables, struct asmOutputVariable* output_variables, xed_decoded_inst_t* xedd, struct operand* operands, uint32_t nb_operand);
static void special_instruction_lea(struct irRenameEngine* engine, struct asmInputVariable* input_variables, struct asmOutputVariable* output_variables, xed_decoded_inst_t* xedd, struct operand* operands, uint32_t nb_operand);


int32_t irImporterAsm_import(struct ir* ir){
	struct asmInputVariable 	input_variables;
	struct asmOutputVariable 	output_variables;
	struct irRenameEngine 		engine;
	struct instructionIterator 	it;
	uint32_t 					i;
	struct operand* 			operands;

	irRenameEngine_init(engine, ir)

	if (assembly_get_instruction(&(ir->trace->assembly), &it, 0)){
		printf("ERROR: in %s, unable to fetch first instruction from the assembly\n", __func__);
		irRenameEngine_clean(engine)
		return -1;
	}

	for (i = 0;; i++){
		operands = trace_get_ins_operands(ir->trace, i);

		switch(xed_decoded_inst_get_iclass(&(it.xedd))){
			case XED_ICLASS_CMP : {break;}
			case XED_ICLASS_JNZ : {break;}
			case XED_ICLASS_LEA : {
				special_instruction_lea(&engine, &input_variables, &output_variables, &(it.xedd), operands, ir->trace->instructions[i].nb_operand);
				break;
			}
			case XED_ICLASS_MOV : {
				special_instruction_mov(&engine, &input_variables, &output_variables, &(it.xedd), operands, ir->trace->instructions[i].nb_operand);
				break;
			}
			case XED_ICLASS_POP : {
				/* a completer */
				printf("WARNING: in %s, this case is not implemented yet (POP instruction) @ %u\n", __func__, i);
				break;
			}
			case XED_ICLASS_PUSH : {
				/* a completer */
				printf("WARNING: in %s, this case is not implemented yet (PUSH instruction) @ %u\n", __func__, i);
				break;
			}
			default :{
				asmInputVariable_fetch(&engine, &input_variables, &(it.xedd), operands, ir->trace->instructions[i].nb_operand);
				asmOutputVariable_fetch(&engine, &output_variables, &(it.xedd), operands, ir->trace->instructions[i].nb_operand);
				asmInputVariable_build_dependence(ir, &input_variables, &output_variables);
			}
		}

		if (instructionIterator_get_instruction_index(&it) ==  assembly_get_nb_instruction(&(ir->trace->assembly)) - 1){
			break;
		}
		else{
			if (assembly_get_next_instruction(&(ir->trace->assembly), &it)){
				printf("ERROR: in %s, unable to fetch next instruction from the assembly\n", __func__);
				irRenameEngine_clean(engine)
				return -1;
			}
		}
	}

	irRenameEngine_clean(engine)

	return 0;
}

static void asmInputVariable_fetch(struct irRenameEngine* engine, struct asmInputVariable* input_variables, xed_decoded_inst_t* xedd, struct operand* operands, uint32_t nb_operand){
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
					enum reg 		reg;
					struct operand* base_operand;
					struct operand* index_operand;
					struct operand* mem_operand;
					uint64_t 		displacement;

					base = xed_decoded_inst_get_base_reg(xedd, nb_memops);
					if (base != XED_REG_INVALID){
						reg = xedRegister_2_reg(base);
						base_operand = irImporterAsm_get_base_register(operands, nb_operand, reg);

						input_variables->base_variable = irRenameEngine_get_register_ref(engine, reg, base_operand);
						if (input_variables->base_variable == NULL){
							input_variables->base_variable = irImporterAsm_add_input(engine->ir, base_operand);
							if (input_variables->base_variable != NULL){
								ir_node_get_operation(input_variables->base_variable)->data = 1;
								if (irRenameEngine_set_register_new_ref(engine, reg, input_variables->base_variable)){
									printf("ERROR: in %s, unable to add new reference in the renaming engine\n", __func__);
								}
							}
							else{
								printf("ERROR: in %s, unable to add input to IR\n", __func__);
							}
						}
					}

					index = xed_decoded_inst_get_index_reg(xedd, nb_memops);
					if (index != XED_REG_INVALID){
						reg = xedRegister_2_reg(index);
						index_operand = irImporterAsm_get_base_register(operands, nb_operand, reg);

						input_variables->index_variable = irRenameEngine_get_register_ref(engine, reg, index_operand);
						if (input_variables->index_variable == NULL){
							input_variables->index_variable = irImporterAsm_add_input(engine->ir, index_operand);
							if (input_variables->index_variable != NULL){
								ir_node_get_operation(input_variables->base_variable)->data = 1;
								if (irRenameEngine_set_register_new_ref(engine, reg, input_variables->index_variable)){
									printf("ERROR: in %s, unable to add new reference in the renaming engine\n", __func__);
								}
							}
							else{
								printf("ERROR: in %s, unable to add input to IR\n", __func__);
							}
						}
					}

					displacement = xed_decoded_inst_get_memory_displacement(xedd, nb_memops);
					if (displacement != 0){
						input_variables->disp_variable = irImporterAsm_add_imm(engine->ir, xed_decoded_inst_get_memory_displacement_width(xedd, nb_memops) * 8, 0, displacement);
						if (input_variables->disp_variable == NULL){
							printf("ERROR: in %s, unable to add immediate to IR\n", __func__);
						}
					}

					if (op_name != XED_OPERAND_AGEN){
						mem_operand = irImporterAsm_get_input_memory(operands, nb_operand, nb_memops);
						if (mem_operand == NULL){
							printf("ERROR: in %s, input memory operands with no specified address, unable to rename\n", __func__);
						}
						else{
							input_variables->variables[input_variables->nb_input] = irRenameEngine_get_ref(engine, mem_operand);
							if (input_variables->variables[input_variables->nb_input] == NULL){
								input_variables->variables[input_variables->nb_input] = irImporterAsm_add_input(engine->ir, mem_operand);
								if (input_variables->variables[input_variables->nb_input] != NULL){
									ir_node_get_operation(input_variables->variables[input_variables->nb_input])->data = 1;
									if (irRenameEngine_set_new_ref(engine, mem_operand, input_variables->variables[input_variables->nb_input])){
										printf("ERROR: in %s, unable to add new reference in the renaming engine\n", __func__);
									}


									if (input_variables->base_variable != NULL){
										if (irImporterAsm_add_dependence(engine->ir, input_variables->base_variable, input_variables->variables[input_variables->nb_input], IR_DEPENDENCE_TYPE_BASE) == NULL){
											printf("ERROR: in %s, unable to add base dependence to IR\n", __func__);
										}
									}
									if (input_variables->index_variable != NULL){
										if (irImporterAsm_add_dependence(engine->ir, input_variables->index_variable, input_variables->variables[input_variables->nb_input], IR_DEPENDENCE_TYPE_INDEX) == NULL){
											printf("ERROR: in %s, unable to add index dependence to IR\n", __func__);
										}
									}
									if (input_variables->disp_variable != NULL){
										if (irImporterAsm_add_dependence(engine->ir, input_variables->disp_variable, input_variables->variables[input_variables->nb_input], IR_DEPENDENCE_TYPE_DISP) == NULL){
											printf("ERROR: in %s, unable to add disp dependence to IR\n", __func__);
										}
									}

									input_variables->nb_input ++;
									if (input_variables->nb_input == IRIMPORTERASM_MAX_INPUT_VARIABLE){
										printf("ERROR: in %s, the max number of input variable has been reached\n", __func__);
										break;
									}
								}
								else{
									printf("ERROR: in %s, unable to add input to IR\n", __func__);
								}
							}
							else{
								input_variables->nb_input ++;
								if (input_variables->nb_input == IRIMPORTERASM_MAX_INPUT_VARIABLE){
									printf("ERROR: in %s, the max number of input variable has been reached\n", __func__);
									break;
								}
							}
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

					input_variables->variables[input_variables->nb_input] = irImporterAsm_add_imm(engine->ir, width, signe, value);
					if (input_variables->variables[input_variables->nb_input] == NULL){
						printf("ERROR: in %s, unable to add immediate to IR\n", __func__);
					}
					else{
						ir_node_get_operation(input_variables->variables[input_variables->nb_input])->data = 1;
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
					enum reg 		reg;
					struct operand* register_operand;
					
					reg = xedRegister_2_reg(xed_decoded_inst_get_reg(xedd, op_name));
					register_operand = irImporterAsm_get_input_register(operands, nb_operand, reg);

					input_variables->variables[input_variables->nb_input] = irRenameEngine_get_register_ref(engine, reg, register_operand);
					if (input_variables->variables[input_variables->nb_input] == NULL){
						input_variables->variables[input_variables->nb_input] = irImporterAsm_add_input(engine->ir, register_operand);
						if (input_variables->variables[input_variables->nb_input] != NULL){
							ir_node_get_operation(input_variables->variables[input_variables->nb_input])->data = 1;
							if (irRenameEngine_set_register_new_ref(engine, reg, input_variables->variables[input_variables->nb_input])){
								printf("ERROR: in %s, unable to add new reference in the renaming engine\n", __func__);
							}
							input_variables->nb_input ++;
							if (input_variables->nb_input == IRIMPORTERASM_MAX_INPUT_VARIABLE){
								printf("ERROR: in %s, the max number of input variable has been reached\n", __func__);
								break;
							}
						}
						else{
							printf("ERROR: in %s, unable to add input to IR\n", __func__);
						}
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
				default : {
					printf("ERROR: in %s, operand type not supported: %s\n", __func__, xed_operand_enum_t2str(op_name));
					break;
				}
			}
		}
	}
}

static void asmOutputVariable_fetch(struct irRenameEngine* engine, struct asmOutputVariable* output_variables, xed_decoded_inst_t* xedd, struct operand* operands, uint32_t nb_operand){
	uint32_t 				i;
	const xed_inst_t* 		xi = xed_decoded_inst_inst(xedd);
	const xed_operand_t* 	xed_op;
	xed_operand_enum_t 		op_name;

	for (i = 0, output_variables->variable = NULL; i < xed_inst_noperands(xi) && output_variables->variable == NULL; i++){
		xed_op = xed_inst_operand(xi, i);
		if (xed_operand_written(xed_op)){
			op_name = xed_operand_name(xed_op);

			switch(op_name){
				case XED_OPERAND_AGEN 	:
				case XED_OPERAND_MEM0 	:
				case XED_OPERAND_MEM1 	: {
					printf("\t- write memory operand\n"); /* pour le debug */
					/* a completer */
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
					enum reg 		reg;
					struct operand* register_operand;
					
					reg = xedRegister_2_reg(xed_decoded_inst_get_reg(xedd, op_name));
					register_operand = irImporterAsm_get_output_register(operands, nb_operand, reg);

					output_variables->variable = irImporterAsm_add_operation(engine->ir, xedOpcode_2_irOpcode(xed_decoded_inst_get_iclass(xedd)), register_operand);
					if (output_variables->variable == NULL){
						printf("ERROR: in %s, unable to add operation to IR\n", __func__);
						continue;
					}

					ir_node_get_operation(output_variables->variable)->data = 0;

					if (irRenameEngine_set_register_ref(engine, reg, output_variables->variable)){
						printf("ERROR: in %s, unable to set register reference in the renameEngine\n", __func__);
					}

					break;
				}
				default : {
					printf("ERROR: in %s, operand type not supported: %s\n", __func__, xed_operand_enum_t2str(op_name));
					break;
				}
			}
		}
	}
}

static void asmInputVariable_build_dependence(struct ir* ir, struct asmInputVariable* input_variables, struct asmOutputVariable* output_variables){
	uint32_t i;

	if (output_variables->variable != NULL){
		for (i = 0; i < input_variables->nb_input; i++){
			if (irImporterAsm_add_dependence(ir, input_variables->variables[i], output_variables->variable, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
				printf("ERROR: in %s, unable to add output to add dependence to IR\n", __func__);
			}
		}
	}
}

static struct operand* irImporterAsm_get_input_memory(struct operand* operands, uint32_t nb_operand, uint32_t index){
	uint32_t i;
	uint32_t j;

	for (i = 0, j = 0; i < nb_operand; i++){
		if (OPERAND_IS_READ(operands[i]) && OPERAND_IS_MEM(operands[i])){
			if (j++ == index){
				return operands + i;
			}
		}
	}

	return NULL;
}

static struct operand* irImporterAsm_get_input_register(struct operand* operands, uint32_t nb_operand, enum reg reg){
	uint32_t i;

	for (i = 0; i < nb_operand; i++){
		if (OPERAND_IS_REG(operands[i]) && OPERAND_IS_READ(operands[i]) && operands[i].location.reg == reg){
			return operands + i;
		}
	}

	return NULL;
}

static struct operand* irImporterAsm_get_base_register(struct operand* operands, uint32_t nb_operand, enum reg reg){
	uint32_t i;

	for (i = 0; i < nb_operand; i++){
		if (OPERAND_IS_BASE(operands[i]) && operands[i].location.reg == reg){
			return operands + i;
		}
	}

	return NULL;
}

static struct operand* irImporterAsm_get_output_register(struct operand* operands, uint32_t nb_operand, enum reg reg){
	uint32_t i;

	for (i = 0; i < nb_operand; i++){
		if (OPERAND_IS_REG(operands[i]) && OPERAND_IS_WRITE(operands[i]) && operands[i].location.reg == reg){
			return operands + i;
		}
	}

	return NULL;
}

static enum irOpcode xedOpcode_2_irOpcode(xed_iclass_enum_t xed_opcode){
	switch (xed_opcode){
		case XED_ICLASS_ADD : {return IR_ADD;}
		case XED_ICLASS_AND : {return IR_AND;}
		case XED_ICLASS_LEA : {return IR_ADD;}
		case XED_ICLASS_NOT : {return IR_NOT;}
		case XED_ICLASS_OR 	: {return IR_OR;}
		case XED_ICLASS_ROL : {return IR_ROL;}
		case XED_ICLASS_ROR : {return IR_ROR;}
		case XED_ICLASS_SHL : {return IR_SHL;}
		case XED_ICLASS_SHR : {return IR_SHR;}
		case XED_ICLASS_SUB : {return IR_SUB;}
		case XED_ICLASS_XOR : {return IR_XOR;}
		default : {
			printf("ERROR: in %s, this instruction (%s) cannot be translated into ir Opcode\n", __func__, xed_iclass_enum_t2str(xed_opcode));
			return IR_ADD;
		}
	}
}

static enum reg xedRegister_2_reg(xed_reg_enum_t xed_reg){
	switch(xed_reg){
		case XED_REG_AX 	: {return REGISTER_AX;} 	
		case XED_REG_CX 	: {return REGISTER_CX;} 	
		case XED_REG_DX 	: {return REGISTER_DX;} 	
		case XED_REG_BX 	: {return REGISTER_BX;} 
		case XED_REG_EAX 	: {return REGISTER_EAX;}
		case XED_REG_ECX 	: {return REGISTER_ECX;}
		case XED_REG_EDX 	: {return REGISTER_EDX;}
		case XED_REG_EBX 	: {return REGISTER_EBX;}
		case XED_REG_ESP 	: {return REGISTER_ESP;}
		case XED_REG_EBP 	: {return REGISTER_EBP;}
		case XED_REG_ESI 	: {return REGISTER_ESI;}
		case XED_REG_EDI 	: {return REGISTER_EDI;}
		default : {
			printf("ERROR: in %s, this register (%s) cannot be translated into custom register\n", __func__, xed_reg_enum_t2str(xed_reg));
			return REGISTER_INVALID;
		}
	}
}

static void special_instruction_mov(struct irRenameEngine* engine, struct asmInputVariable* input_variables, struct asmOutputVariable* output_variables, xed_decoded_inst_t* xedd, struct operand* operands, uint32_t nb_operand){
	uint32_t 				i;
	const xed_inst_t* 		xi = xed_decoded_inst_inst(xedd);
	const xed_operand_t* 	xed_op;
	xed_operand_enum_t 		op_name;

	asmInputVariable_fetch(engine, input_variables, xedd, operands, nb_operand);

	for (i = 0, output_variables->variable = NULL; i < xed_inst_noperands(xi); i++){
		xed_op = xed_inst_operand(xi, i);
		if (xed_operand_written(xed_op)){
			op_name = xed_operand_name(xed_op);

			switch(op_name){
				case XED_OPERAND_AGEN 	:
				case XED_OPERAND_MEM0 	:
				case XED_OPERAND_MEM1 	: {
					printf("\t- write memory operand\n"); /* pour le debug */
					/* a completer */
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
					enum reg 		reg;
					
					reg = xedRegister_2_reg(xed_decoded_inst_get_reg(xedd, op_name));
					if (irRenameEngine_set_register_ref(engine, reg, input_variables->variables[0])){
						printf("ERROR: in %s, unable to set register reference in the renameEngine\n", __func__);
					}

					break;
				}
				default : {
					printf("ERROR: in %s, operand type not supported: %s\n", __func__, xed_operand_enum_t2str(op_name));
					break;
				}
			}
		}
	}
}

static void special_instruction_lea(struct irRenameEngine* engine, struct asmInputVariable* input_variables, struct asmOutputVariable* output_variables, xed_decoded_inst_t* xedd, struct operand* operands, uint32_t nb_operand){
	uint32_t 				i;
	const xed_inst_t* 		xi = xed_decoded_inst_inst(xedd);
	const xed_operand_t* 	xed_op;
	xed_operand_enum_t 		op_name;

	asmInputVariable_fetch(engine, input_variables, xedd, operands, nb_operand);

	for (i = 0, output_variables->variable = NULL; i < xed_inst_noperands(xi) && output_variables->variable == NULL; i++){
		xed_op = xed_inst_operand(xi, i);
		if (xed_operand_written(xed_op)){
			op_name = xed_operand_name(xed_op);

			switch(op_name){
				case XED_OPERAND_REG0 	: {
					enum reg 		reg;
					struct operand* register_operand;
					
					reg = xedRegister_2_reg(xed_decoded_inst_get_reg(xedd, op_name));
					register_operand = irImporterAsm_get_output_register(operands, nb_operand, reg);

					output_variables->variable = irImporterAsm_add_operation(engine->ir, xedOpcode_2_irOpcode(xed_decoded_inst_get_iclass(xedd)), register_operand);
					if (output_variables->variable == NULL){
						printf("ERROR: in %s, unable to add operation to IR\n", __func__);
						continue;
					}

					ir_node_get_operation(output_variables->variable)->data = 0;

					if (irRenameEngine_set_register_ref(engine, reg, output_variables->variable)){
						printf("ERROR: in %s, unable to set register reference in the renameEngine\n", __func__);
					}

					
					if (input_variables->base_variable != NULL){
						if (irImporterAsm_add_dependence(engine->ir, input_variables->base_variable, output_variables->variable, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
							printf("ERROR: in %s, unable to add dependence to IR\n", __func__);
						}
					}
					if (input_variables->index_variable != NULL){
						if (irImporterAsm_add_dependence(engine->ir, input_variables->index_variable, output_variables->variable, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
							printf("ERROR: in %s, unable to add dependence to IR\n", __func__);
						}
					}
					if (input_variables->disp_variable != NULL){
						if (irImporterAsm_add_dependence(engine->ir, input_variables->disp_variable, output_variables->variable, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
							printf("ERROR: in %s, unable to add dependence to IR\n", __func__);
						}
					}

					break;
				}
				default : {
					printf("ERROR: in %s, operand type not supported: %s\n", __func__, xed_operand_enum_t2str(op_name));
					break;
				}
			}
		}
	}
}