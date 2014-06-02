#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "irImporterAsm.h"
#include "instruction.h"
#include "graph.h"
#include "irRenameEngine.h"

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


int32_t irImporterAsm_import(struct ir* ir){
	/*struct operand* 			operands;*/
	struct irRenameEngine 		engine;
	struct instructionIterator 	it;
	int32_t 					result = 0;
	const xed_inst_t* 				xi;

	irRenameEngine_init(engine, ir)

	if (assembly_get_instruction(&(ir->trace->assembly), &it, 0)){
		printf("ERROR: in %s, unable to fetch first instruction from the assembly\n", __func__);
		result = -1;
		goto exit;
	}

	while(1){
		switch(xed_decoded_inst_get_iclass(&(it.xedd))){
			case XED_ICLASS_MOV : {
				xi = xed_decoded_inst_inst(&(it.xedd));
				if (xed_inst_noperands(xi) != 2){
					printf("ERROR: in %s, incorrect instruction format: expected 2 operands but get %u\n", __func__, xed_inst_noperands(xi));
				}
				else{
					const xed_operand_t* i1 =  irImporterAsm_get_asm_input(xi, 0);
					const xed_operand_t* o1 =  irImporterAsm_get_asm_output(xi, 0);

					if (i1 != NULL && o1 != NULL){

					}
					else{
						printf("ERROR: in %s, incorrect instruction format: expected input(1) and ouput(2)\n", __func__);
					}
				}
				break;
			}
			default :{
				printf("ERROR: in %s, this instruction: %s is not implemented yet\n", __func__, instruction_opcode_2_string(xed_decoded_inst_get_iclass(&(it.xedd)))); 
			}
		}

		if (instructionIterator_get_instruction_index(&it) ==  assembly_get_nb_instruction(&(ir->trace->assembly)) - 1){
			break;
		}
		else{
			if (assembly_get_next_instruction(&(ir->trace->assembly), &it)){
				printf("ERROR: in %s, unable to fetch next instruction from the assembly\n", __func__);
				break;
			}
		}
	}


	#if 0
	for (i = 0; i < ir->trace->nb_instruction; i++){
		operands = trace_get_ins_operands(ir->trace, i);

		switch(ir->trace->instructions[i].opcode){
			case XED_ICLASS_ADD : {
				/*
					ADD 	r/m8 	imm8
					ADD 	r/m16 	imm8
					ADD 	r/m32 	imm8
					ADD 	r/m16 	imm16
					ADD 	r/m32 	imm32
					ADD 	r/m8 	r8
					ADD 	r/m16 	r16
					ADD 	r/m32 	r32
					ADD 	r8 		r/m8
					ADD 	r16 	r/m16
					ADD 	r32 	r/m32
				*/

				import_std_ins_1imr_1ioptmr_1omr(ir, &engine, ir->trace->instructions + i, operands, IR_ADD);
				break;
			}
			case XED_ICLASS_AND : {
				/*
					AND 	r/m8 	imm8
					AND 	r/m16 	imm8
					AND 	r/m32 	imm8
					AND 	r/m16 	imm16
					AND 	r/m32 	imm32
					AND 	r/m8 	r8
					AND 	r/m16 	r16
					AND 	r/m32 	r32
					AND 	r8 		r/m8
					AND 	r16 	r/m16
					AND 	r32 	r/m32
				*/

				import_std_ins_1imr_1ioptmr_1omr(ir, &engine, ir->trace->instructions + i, operands, IR_AND);
				break;
			}
			case XED_ICLASS_BSWAP : {
				/*
					BSWAP 	r32
				*/

				import_std_in_1ir_1or(ir, &engine, ir->trace->instructions + i, operands, IR_BSWAP);
				break;
			}
			case XED_ICLASS_DEC : {
				/*
					DEC 	r/m8
					DEC 	r/m16
					DEC 	r/m32
				*/

				import_std_in_1imr_1omr(ir, &engine, ir->trace->instructions + i, operands, IR_DEC);
				break;
			}
			case XED_ICLASS_LEA : {
				/*
					LEA 	r16 	m
					LEA 	r32 	m
				*/

				uint32_t 		base_operand_index;
				uint32_t 		index_operand_index;
				uint32_t 		output0_operand_index;
				struct node* 	base_variable;
				struct node* 	index_variable;
				struct node* 	output0_variable;

				base_operand_index 		= irImporterAsm_get_base(operands, ir->trace->instructions[i].nb_operand);
				index_operand_index 	= irImporterAsm_get_index(operands, ir->trace->instructions[i].nb_operand);
				output0_operand_index 	= irImporterAsm_get_output_register(operands, ir->trace->instructions[i].nb_operand, 0);

				if (output0_operand_index != ir->trace->instructions[i].nb_operand){
					if (base_operand_index != ir->trace->instructions[i].nb_operand){
						base_variable = irImporterAsm_get_ir_src_variable(&engine, operands + base_operand_index);
						if (base_variable == NULL){
							printf("ERROR: in %s, unable to access operand(s) IR variable\n", __func__);
						}
					}
					else{
						base_variable = NULL;
					}

					if (index_operand_index != ir->trace->instructions[i].nb_operand){
						index_variable = irImporterAsm_get_ir_src_variable(&engine, operands + index_operand_index);
						if (index_variable == NULL){
							printf("ERROR: in %s, unable to access operand(s) IR variable\n", __func__);
						}
					}
					else{
						index_variable = NULL;
					}

					output0_variable = irImporterAsm_get_ir_dst_variable(&engine, operands + output0_operand_index, IR_ADD);
					if (output0_variable != NULL){
						if (base_variable != NULL){
							if (irImporterAsm_add_dependence(ir, base_variable, output0_variable, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
								printf("ERROR: in %s, unable to add output to add dependence to IR\n", __func__);
							}
						}
						if (index_variable != NULL){
							if (irImporterAsm_add_dependence(ir, index_variable, output0_variable, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
								printf("ERROR: in %s, unable to add output to add dependence to IR\n", __func__);
							}
						}
					}
					else{
						printf("ERROR: in %s, unable to access operand(s) IR variable\n", __func__);
					} 
				}
				else{
					printf("WARNING: in %s, incorrect instruction format (operand index): %s\n",  __func__, instruction_opcode_2_string(ir->trace->instructions[i].opcode));
				}
				break;
			}
			case XED_ICLASS_MOV : {
				uint32_t 		input0_operand_index;
				uint32_t 		output0_operand_index;
				struct node* 	input0_variable;

				input0_operand_index 	= irImporterAsm_get_input(operands, ir->trace->instructions[i].nb_operand, 0);
				output0_operand_index 	= irImporterAsm_get_output(operands, ir->trace->instructions[i].nb_operand, 0);

				if (input0_operand_index != ir->trace->instructions[i].nb_operand && output0_operand_index != ir->trace->instructions[i].nb_operand){
					input0_variable = irImporterAsm_get_ir_src_variable_dependence_index_base(&engine, operands, ir->trace->instructions[i].nb_operand, input0_operand_index);
					if (input0_variable != NULL){
						if (irRenameEngine_set_ref(&engine, operands + output0_operand_index, input0_variable)){
							printf("ERROR: in %s, unable to set reference in the renameEngine\n", __func__);
						}
					}
					else{
						printf("ERROR: in %s, unable to access operand(s) IR variable\n", __func__);
					}
				}
				else{
					printf("WARNING: in %s, incorrect instruction format (operand index): %s\n",  __func__, instruction_opcode_2_string(ir->trace->instructions[i].opcode));
				}
				break;
			}
			case XED_ICLASS_NOT : {
				/*
					NOT 	r/m8
					NOT 	r/m16
					NOT 	r/m32
				*/

				import_std_in_1imr_1omr(ir, &engine, ir->trace->instructions + i, operands, IR_NOT);
				break;
			}
			case XED_ICLASS_MOVZX : {
				/*
					MOVZX 	r16 	r/m8
					MOVZX 	r32 	r/m8
					MOVZX 	r32 	r/m16
				*/

				import_std_in_1imr_1or(ir, &engine, ir->trace->instructions + i, operands, IR_MOVZX);
				break;
			}
			case XED_ICLASS_OR : {
				import_std_ins_1imr_1ioptmr_1omr(ir, &engine, ir->trace->instructions + i, operands, IR_OR);
				break;
			}
			case XED_ICLASS_ROR : {
				import_std_in_1imr_1omr(ir, &engine, ir->trace->instructions + i, operands, IR_ROR);
				break;
			}
			case XED_ICLASS_SAR : {
				import_std_in_1imr_1omr(ir, &engine, ir->trace->instructions + i, operands, IR_SAR);
				break;
			}
			case XED_ICLASS_SHL : {
				import_std_in_1imr_1omr(ir, &engine, ir->trace->instructions + i, operands, IR_SHL);
				break;
			}
			case XED_ICLASS_SHR : {
				import_std_in_1imr_1omr(ir, &engine, ir->trace->instructions + i, operands, IR_SHR);
				break;
			}
			case XED_ICLASS_SUB : {
				import_std_ins_1imr_1ioptmr_1omr(ir, &engine, ir->trace->instructions + i, operands, IR_SUB);
				break;
			}
			case XED_ICLASS_XOR : {
				import_std_ins_1imr_1ioptmr_1omr(ir, &engine, ir->trace->instructions + i, operands, IR_XOR);
				break;
			}
			default : {
				printf("ERROR: in %s, incorrect opcode: %s\n", __func__, instruction_opcode_2_string(ir->trace->instructions[i].opcode));
				break;
			}
		}
	}

	#endif

	exit:
	irRenameEngine_clean(engine)

	return result;
}

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


static uint32_t irImporterAsm_get_input_register(struct operand* operands, uint32_t nb_operand, uint32_t index){
	uint32_t i;
	uint32_t j;

	for (i = 0, j = 0; i < nb_operand; i++){
		if (OPERAND_IS_READ(operands[i]) && !OPERAND_IS_INDEX(operands[i]) && !OPERAND_IS_BASE(operands[i]) && OPERAND_IS_REG(operands[i])){
			if (j++ == index){
				break;
			}
		}
	}

	return i;
}

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