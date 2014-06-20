#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "irImporterAsm.h"
#include "instruction.h"
#include "graph.h"
#include "irRenameEngine.h"

struct node* irImporterAsm_get_ir_dst_variable(struct irRenameEngine* engine, struct operand* operand, enum irOpcode opcode);

#if 0
static inline const xed_operand_t* irImporterAsm_get_asm_input(const xed_inst_t* xi, const uint32_t index);
static inline const xed_operand_t* irImporterAsm_get_asm_output(const xed_inst_t* xi, const uint32_t index);

/* those methods are useless */
static uint32_t irImporterAsm_get_input(struct operand* operands, uint32_t nb_operand, uint32_t index);
static uint32_t irImporterAsm_get_output(struct operand* operands, uint32_t nb_operand, uint32_t index);
static uint32_t irImporterAsm_get_input_register(struct operand* operands, uint32_t nb_operand, uint32_t index);
static uint32_t irImporterAsm_get_output_register(struct operand* operands, uint32_t nb_operand, uint32_t index);
static uint32_t irImporterAsm_get_base(struct operand* operands, uint32_t nb_operand);
static uint32_t irImporterAsm_get_index(struct operand* operands, uint32_t nb_operand);

struct node* irImporterAsm_get_ir_src_variable(struct irRenameEngine* engine, struct operand* operand);
struct node* irImporterAsm_get_ir_src_variable_dependence_index_base(struct irRenameEngine* engine, struct operand* operands, uint32_t nb_operand, uint32_t index);
struct node* irImporterAsm_get_ir_dst_variable(struct irRenameEngine* engine, struct operand* operand, enum irOpcode opcode);


void irImporterAsm_add_dependence_base_and_index(struct irRenameEngine* engine, struct operand* operands, uint32_t nb_operand, struct node* mem_variable){
	uint32_t 		index_operand_index;
	uint32_t 		base_operand_index;
	struct node* 	base_variable 	= NULL;
	struct node* 	index_variable 	= NULL;

	base_operand_index 		= irImporterAsm_get_base(operands, nb_operand);
	index_operand_index 	= irImporterAsm_get_index(operands, nb_operand);

	if (base_operand_index != nb_operand){
		base_variable = irImporterAsm_get_ir_src_variable(engine, operands + base_operand_index);
		if (base_variable != NULL){
			if (irImporterAsm_add_dependence(engine->ir, base_variable, mem_variable, IR_DEPENDENCE_TYPE_BASE) == NULL){
				printf("ERROR: in %s, unable to add dependence to IR\n", __func__);
			}
		}
		else{
			printf("ERROR: in %s, unable to access base IR variable\n", __func__);
		}
	}

	if (index_operand_index != nb_operand){
		index_variable = irImporterAsm_get_ir_src_variable(engine, operands + index_operand_index);
		if (index_variable != NULL){
			if (irImporterAsm_add_dependence(engine->ir, index_variable, mem_variable, IR_DEPENDENCE_TYPE_INDEX) == NULL){
				printf("ERROR: in %s, unable to add dependence to IR\n", __func__);
			}
		}
		else{
			printf("ERROR: in %s, unable to access index IR variable\n", __func__);
		}
	}
}

/* ===================================================================== */
/* Instruction type function						                     */
/* ===================================================================== */

static void import_std_in_1ir_1or(struct ir* ir, struct irRenameEngine* engine, struct instruction* instruction, struct operand* operands, enum irOpcode opcode){
	uint32_t 		input0_operand_index;
	uint32_t 		output0_operand_index;
	struct node* 	input0_variable;
	struct node* 	output0_variable;

	input0_operand_index = irImporterAsm_get_input_register(operands, instruction->nb_operand, 0);
	output0_operand_index = irImporterAsm_get_output_register(operands, instruction->nb_operand, 0);

	if (input0_operand_index != instruction->nb_operand && output0_operand_index != instruction->nb_operand){
		input0_variable = irImporterAsm_get_ir_src_variable(engine, operands + input0_operand_index);
		output0_variable = irImporterAsm_get_ir_dst_variable(engine, operands + output0_operand_index, opcode);

		if (input0_variable != NULL && output0_variable != NULL){
			if (irImporterAsm_add_dependence(ir, input0_variable, output0_variable, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
				printf("ERROR: in %s, unable to add output to add dependence to IR\n", __func__);
			}
		}
		else{
			printf("ERROR: in %s, unable to access operand(s) IR variable\n", __func__);
		}
	}
	else{
		printf("WARNING: in %s, incorrect instruction format %s\n",  __func__, instruction_opcode_2_string(instruction->opcode));
	}
}

static void import_std_in_1imr_1or(struct ir* ir, struct irRenameEngine* engine, struct instruction* instruction, struct operand* operands, enum irOpcode opcode){
	uint32_t 		input0_operand_index;
	uint32_t 		output0_operand_index;
	struct node* 	input0_variable;
	struct node* 	output0_variable;

	input0_operand_index = irImporterAsm_get_input(operands, instruction->nb_operand, 0);
	output0_operand_index = irImporterAsm_get_output_register(operands, instruction->nb_operand, 0);

	if (input0_operand_index != instruction->nb_operand && output0_operand_index != instruction->nb_operand){
		input0_variable = irImporterAsm_get_ir_src_variable_dependence_index_base(engine, operands, instruction->nb_operand, input0_operand_index);
		output0_variable = irImporterAsm_get_ir_dst_variable(engine, operands + output0_operand_index, opcode);

		if (input0_variable != NULL && output0_variable != NULL){
			if (irImporterAsm_add_dependence(ir, input0_variable, output0_variable, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
				printf("ERROR: in %s, unable to add output to add dependence to IR\n", __func__);
			}
		}
		else{
			printf("ERROR: in %s, unable to access operand(s) IR variable\n", __func__);
		}
	}
	else{
		printf("WARNING: in %s, incorrect instruction format %s\n",  __func__, instruction_opcode_2_string(instruction->opcode));
	}
}

static void import_std_in_1imr_1omr(struct ir* ir, struct irRenameEngine* engine, struct instruction* instruction, struct operand* operands, enum irOpcode opcode){
	uint32_t 		input0_operand_index;
	uint32_t 		output0_operand_index;
	struct node* 	input0_variable;
	struct node* 	output0_variable;

	input0_operand_index = irImporterAsm_get_input(operands, instruction->nb_operand, 0);
	output0_operand_index = irImporterAsm_get_output(operands, instruction->nb_operand, 0);

	if (input0_operand_index != instruction->nb_operand && output0_operand_index != instruction->nb_operand){
		input0_variable = irImporterAsm_get_ir_src_variable_dependence_index_base(engine, operands, instruction->nb_operand, input0_operand_index);

		output0_variable = irImporterAsm_get_ir_dst_variable(engine, operands + output0_operand_index, opcode);
		if (input0_variable != NULL && output0_variable != NULL){
			if (irImporterAsm_add_dependence(ir, input0_variable, output0_variable, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
				printf("ERROR: in %s, unable to add output to add dependence to IR\n", __func__);
			}
		}
		else{
			printf("ERROR: in %s, unable to access operand(s) IR variable\n", __func__);
		}
	}
	else{
		printf("WARNING: in %s, incorrect instruction format %s\n",  __func__, instruction_opcode_2_string(instruction->opcode));
	}
}

static void import_std_ins_1imr_1ioptmr_1omr(struct ir* ir, struct irRenameEngine* engine, struct instruction* instruction, struct operand* operands, enum irOpcode opcode){
	uint32_t 		input0_operand_index;
	uint32_t 		input1_operand_index;
	uint32_t 		output0_operand_index;
	struct node* 	input0_variable;
	struct node* 	input1_variable;
	struct node* 	output0_variable;

	input0_operand_index = irImporterAsm_get_input(operands, instruction->nb_operand, 0);
	input1_operand_index = irImporterAsm_get_input(operands, instruction->nb_operand, 1);
	output0_operand_index = irImporterAsm_get_output(operands, instruction->nb_operand, 0);

	if (input0_operand_index != instruction->nb_operand && output0_operand_index != instruction->nb_operand){
		input0_variable = irImporterAsm_get_ir_src_variable_dependence_index_base(engine, operands, instruction->nb_operand, input0_operand_index);

		if (input1_operand_index != instruction->nb_operand){
			input1_variable = irImporterAsm_get_ir_src_variable_dependence_index_base(engine, operands, instruction->nb_operand, input1_operand_index);
		}
		else{
			input1_variable = NULL;
		}

		output0_variable = irImporterAsm_get_ir_dst_variable(engine, operands + output0_operand_index, opcode);

		if (input0_variable != NULL && output0_variable != NULL){
			if (irImporterAsm_add_dependence(ir, input0_variable, output0_variable, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
				printf("ERROR: in %s, unable to add output to add dependence to IR\n", __func__);
			}
			if (input1_variable != NULL){
				if (irImporterAsm_add_dependence(ir, input1_variable, output0_variable, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
					printf("ERROR: in %s, unable to add output to add dependence to IR\n", __func__);
				}
			}
		}
		else{
			printf("ERROR: in %s, unable to access operand(s) IR variable\n", __func__);
		}
	}
	else{
		printf("WARNING: in %s, incorrect instruction format %s\n",  __func__, instruction_opcode_2_string(instruction->opcode));
	}
}

/* ===================================================================== */
/* Import functions						                                 */
/* ===================================================================== */


/* 	We don't care about the format. No need to switch over the opcode. How do I design the stuff :
	For every input operands fetch it -> depending on the type take the appropriate measure 
	Push the output operande with the appropriate opcode
	Make dependance for every input operand to the output 
	WARNING: base and index relation (special care must be taken)


	Devise structure to hold input reference 
*/

#endif

static struct operand* irImporterAsm_get_input_register(struct operand* operands, uint32_t nb_operand, enum reg reg);
static struct operand* irImporterAsm_get_base_register(struct operand* operands, uint32_t nb_operand, enum reg reg);
static uint32_t irImporterAsm_get_output(struct operand* operands, uint32_t nb_operand, uint32_t index);
static uint32_t irImporterAsm_get_output_memory(struct operand* operands, uint32_t nb_operand, uint32_t index);

#define IRIMPORTERASM_MAX_INPUT_VARIABLE 4

struct asmInputVariable{
	uint32_t 			nb_input;
	struct node* 		base_variable;
	struct node* 		index_variable;
	struct node* 		variables[IRIMPORTERASM_MAX_INPUT_VARIABLE];
};

static void asmInputVariable_fetch(struct irRenameEngine* engine, struct asmInputVariable* input_variables, xed_decoded_inst_t* xedd, struct operand* operands, uint32_t nb_operand);
static void asmInputVariable_build_dependence(struct ir* ir, struct asmInputVariable* input_variables, struct node* output_variable);

static enum irOpcode xedOpcode_2_irOpcode(xed_iclass_enum_t xed_opcode);
static enum reg xedRegister_2_reg(xed_reg_enum_t xed_reg);


int32_t irImporterAsm_import(struct ir* ir){
	struct asmInputVariable 	input_variables;
	struct irRenameEngine 		engine;
	struct instructionIterator 	it;
	uint32_t 					output_operand_index;
	uint32_t 					i;
	struct operand* 			operands;
	struct node* 				output_variable;

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
				asmInputVariable_fetch(&engine, &input_variables, &(it.xedd), operands, ir->trace->instructions[i].nb_operand);
				/* a completer */
				break;
			}
			case XED_ICLASS_MOV : {
				asmInputVariable_fetch(&engine, &input_variables, &(it.xedd), operands, ir->trace->instructions[i].nb_operand);
				output_operand_index = irImporterAsm_get_output_memory(operands, ir->trace->instructions[i].nb_operand, 0);
				if (input_variables.nb_input == 1 && output_operand_index != ir->trace->instructions[i].nb_operand){
					if (irRenameEngine_set_ref(&engine, operands + output_operand_index, input_variables.variables[0])){
						printf("ERROR: in %s, unable to set reference in the renameEngine\n", __func__);
					}
				}
				else{
					printf("ERROR: in %s, incorrect instruction format: MOV\n",  __func__);
				}
				break;
			}
			case XED_ICLASS_POP : {
				/* a completer */
				break;
			}
			case XED_ICLASS_PUSH : {
				/* a completer */
				break;
			}
			default :{
				asmInputVariable_fetch(&engine, &input_variables, &(it.xedd), operands, ir->trace->instructions[i].nb_operand);
				/* temp don't use the trace */
				output_operand_index = irImporterAsm_get_output(operands, ir->trace->instructions[i].nb_operand, 0);
				if (output_operand_index != ir->trace->instructions[i].nb_operand){
					output_variable = irImporterAsm_get_ir_dst_variable(&engine, operands + output_operand_index, xedOpcode_2_irOpcode(xed_decoded_inst_get_iclass(&(it.xedd))));
					asmInputVariable_build_dependence(ir, &input_variables, output_variable);
				}
				else{
					printf("ERROR: in %s, unable to find output argument: GENERIC (%s)\n",  __func__, xed_iclass_enum_t2str(xed_decoded_inst_get_iclass(&(it.xedd))));
				}
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

	printf("%u operand(s)\n", xed_inst_noperands(xi));
	for (i = 0, input_variables->nb_input = 0, input_variables->base_variable = NULL, input_variables->index_variable = NULL; i < xed_inst_noperands(xi); i++){
		if (xed_operand_read(xed_inst_operand(xi, i))){
			xed_op = xed_inst_operand(xi, i);
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

					printf("\t- memory operand\n"); /* pour le debug */

					base = xed_decoded_inst_get_base_reg(xedd, i);
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

					index = xed_decoded_inst_get_index_reg(xedd, i);
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

					/* a completer MEMOPS */
					
					break;
				}
				case XED_OPERAND_IMM0 	: {
					uint16_t 	width;
					uint8_t 	signe;
					uint64_t 	value;

					printf("\t- immediate 0 operand\n"); /* pour le debug */

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

					printf("\t- register operand\n"); /* pour le debug */
					
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
				case XED_OPERAND_BASE0 	:
				case XED_OPERAND_BASE1 	: {
					printf("\t- base operand\n"); /* pour le debug */
					/* a completer REGISTER */
					break;
				}
				default : {
					printf("ERROR: in %s, operand type not supported: %s\n", __func__, xed_operand_enum_t2str(op_name));
				}
			}
		}
	}
}

static void asmInputVariable_build_dependence(struct ir* ir, struct asmInputVariable* input_variables, struct node* output_variable){
	uint32_t i;

	if (output_variable != NULL){
		for (i = 0; i < input_variables->nb_input; i++){
			if (irImporterAsm_add_dependence(ir, input_variables->variables[i], output_variable, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
				printf("ERROR: in %s, unable to add output to add dependence to IR\n", __func__);
			}
		}
	}
}

/*
struct node* irImporterAsm_get_ir_src_variable(struct irRenameEngine* engine, struct operand* operand){
	struct node* variable;

	variable = irRenameEngine_get_ref(engine, operand);
	if (variable == NULL){
		variable = irImporterAsm_add_input(engine->ir, operand);
		if (variable != NULL){
			ir_node_get_operation(variable)->data = 1;
			if (irRenameEngine_set_new_ref(engine, operand, variable)){
				printf("ERROR: in %s, unable to add new reference in the renaming engine\n", __func__);
			}
		}
		else{
			printf("ERROR: in %s, unable to add input to IR\n", __func__);
		}
	}

	return variable;
}

struct node* irImporterAsm_get_ir_src_variable_dependence_index_base(struct irRenameEngine* engine, struct operand* operands, uint32_t nb_operand, uint32_t index){
	struct node* variable;

	variable = irRenameEngine_get_ref(engine, operands + index);
	if (variable == NULL){
		variable = irImporterAsm_add_input(engine->ir, operands + index);
		if (variable != NULL){
			ir_node_get_operation(variable)->data = 1;
			if (irRenameEngine_set_new_ref(engine, operands + index, variable)){
				printf("ERROR: in %s, unable to add new reference in the renaming engine\n", __func__);
			}

			if (OPERAND_IS_MEM(operands[index])){
				irImporterAsm_add_dependence_base_and_index(engine, operands, nb_operand, variable);
			}
		}
		else{
			printf("ERROR: in %s, unable to add input to IR\n", __func__);
		}
	}

	return variable;
}
*/

struct node* irImporterAsm_get_ir_dst_variable(struct irRenameEngine* engine, struct operand* operand, enum irOpcode opcode){
	struct node* variable;

	variable = irImporterAsm_add_operation(engine->ir, opcode, operand);
	if (variable == NULL){
		printf("ERROR: in %s, unable to add operation to IR\n", __func__);
	}
	else{
		ir_node_get_operation(variable)->data = 0;
		if (irRenameEngine_set_ref(engine, operand, variable)){
			printf("ERROR: in %s, unable to set reference in the renameEngine\n", __func__);
		}
	}

	return variable;
}

/*
static inline const xed_operand_t* irImporterAsm_get_asm_input(const xed_inst_t* xi, const uint32_t index){
	uint32_t 		i;
	uint32_t 		j;

	for (i = 0, j = 0; i < xed_inst_noperands(xi); i++){
		if (xed_operand_read(xed_inst_operand(xi, i))){
			if (j ++ == index){
				return xed_inst_operand(xi, i);
			}
		}
	}

	return NULL;
}

static inline const xed_operand_t* irImporterAsm_get_asm_output(const xed_inst_t* xi, const uint32_t index){
	uint32_t 		i;
	uint32_t 		j;

	for (i = 0, j = 0; i < xed_inst_noperands(xi); i++){
		if (xed_operand_written(xed_inst_operand(xi, i))){
			if (j ++ == index){
				return xed_inst_operand(xi, i);
			}
		}
	}

	return NULL;
}

static uint32_t irImporterAsm_get_input(struct operand* operands, uint32_t nb_operand, uint32_t index){
	uint32_t i;
	uint32_t j;

	for (i = 0, j = 0; i < nb_operand; i++){
		if (OPERAND_IS_READ(operands[i]) && !OPERAND_IS_INDEX(operands[i]) && !OPERAND_IS_BASE(operands[i])){
			if (j++ == index){
				break;
			}
		}
	}

	return i;
}



*/

static struct operand* irImporterAsm_get_input_register(struct operand* operands, uint32_t nb_operand, enum reg reg){
	uint32_t i;

	for (i = 0; i < nb_operand; i++){
		if (OPERAND_IS_REG(operands[i]) && operands[i].location.reg == reg){
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

static uint32_t irImporterAsm_get_output(struct operand* operands, uint32_t nb_operand, uint32_t index){
	uint32_t i;
	uint32_t j;

	for (i = 0, j = 0; i < nb_operand; i++){
		if (OPERAND_IS_WRITE(operands[i])){
			if (j++ == index){
				break;
			}
		}
	}

	return i;
}

static uint32_t irImporterAsm_get_output_memory(struct operand* operands, uint32_t nb_operand, uint32_t index){
	uint32_t i;
	uint32_t j;

	for (i = 0, j = 0; i < nb_operand; i++){
		if (OPERAND_IS_WRITE(operands[i]) && OPERAND_IS_MEM(operands[i])){
			if (j++ == index){
				break;
			}
		}
	}

	return i;
}

/*

static uint32_t irImporterAsm_get_output_register(struct operand* operands, uint32_t nb_operand, uint32_t index){
	uint32_t i;
	uint32_t j;

	for (i = 0, j = 0; i < nb_operand; i++){
		if (OPERAND_IS_WRITE(operands[i]) && OPERAND_IS_REG(operands[i])){
			if (j++ == index){
				break;
			}
		}
	}

	return i;
}

static uint32_t irImporterAsm_get_base(struct operand* operands, uint32_t nb_operand){
	uint32_t i;

	for (i = 0; i < nb_operand; i++){
		if (OPERAND_IS_BASE(operands[i])){
			break;
		}
	}

	return i;
}

static uint32_t irImporterAsm_get_index(struct operand* operands, uint32_t nb_operand){
	uint32_t i;

	for (i = 0; i < nb_operand; i++){
		if (OPERAND_IS_INDEX(operands[i])){
			break;
		}
	}

	return i;
}

*/


static enum irOpcode xedOpcode_2_irOpcode(xed_iclass_enum_t xed_opcode){
	switch (xed_opcode){
		case XED_ICLASS_ADD : {return IR_ADD;}
		case XED_ICLASS_AND : {return IR_AND;}
		case XED_ICLASS_LEA : {return IR_ADD;}
		default : {
			printf("ERROR: in %s, this instruction (%s) cannot be translated into ir Opcode\n", __func__, xed_operand_enum_t2str(xed_opcode));
			return IR_ADD;
		}
	}
}

static enum reg xedRegister_2_reg(xed_reg_enum_t xed_reg){
	switch(xed_reg){
		case XED_REG_EAX : {return REGISTER_EAX;}
		case XED_REG_ECX : {return REGISTER_ECX;}
		case XED_REG_EDX : {return REGISTER_EDX;}
		case XED_REG_EBX : {return REGISTER_EBX;}
		/*case XED_REG_ESP : {return REGISTER_ESP;}*/
		case XED_REG_EBP : {return REGISTER_EBP;}
		case XED_REG_ESI : {return REGISTER_ESI;}
		case XED_REG_EDI : {return REGISTER_EDI;}
		default : {
			printf("ERROR: in %s, this register (%s) cannot be translated into custom register\n", __func__, xed_operand_enum_t2str(xed_reg));
			return REGISTER_INVALID;
		}
	}
}