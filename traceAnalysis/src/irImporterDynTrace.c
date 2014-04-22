#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "irImporterDynTrace.h"
#include "instruction.h"
#include "graph.h"
#include "irRenameEngine.h"

uint32_t irImporterDynTrace_get_input(struct operand* operands, uint32_t nb_operand, uint32_t index);
uint32_t irImporterDynTrace_get_output(struct operand* operands, uint32_t nb_operand, uint32_t index);
uint32_t irImporterDynTrace_get_base(struct operand* operands, uint32_t nb_operand);
uint32_t irImporterDynTrace_get_index(struct operand* operands, uint32_t nb_operand);

struct node* irImporterDynTrace_get_ir_src_variable(struct ir* ir, struct irRenameEngine* engine, struct operand* operand);
struct node* irImporterDynTrace_get_ir_dst_variable(struct ir* ir, struct irRenameEngine* engine, struct operand* operand, enum irOpcode opcode);

#define irImporterDynTrace_add_operation(ir, opcode) 			ir_add_output((ir), (opcode))
#define irImporterDynTrace_add_input(ir, operand) 				ir_add_input((ir), (operand))
#define irImporterDynTrace_add_dependence(ir, src, dst, type) 	ir_add_dependence((ir), (src), (dst), (type))

#define irImporterDynTrace_add_dependence_base_and_index(ir, engine, operands, nb_operand, mem_variable) 																	\
	{ 																																										\
		uint32_t 		index_operand_index; 																																\
		uint32_t 		base_operand_index; 																																\
		struct node* 	base_variable 	= NULL; 																															\
		struct node* 	index_variable 	= NULL; 																															\
																																											\
		base_operand_index 		= irImporterDynTrace_get_base((operands), (nb_operand)); 																					\
		index_operand_index 	= irImporterDynTrace_get_index((operands), (nb_operand)); 																					\
																																											\
		if (base_operand_index != (nb_operand)){ 																															\
			base_variable = irImporterDynTrace_get_ir_src_variable((ir), (engine), (operands) + base_operand_index); 														\
			if (base_variable != NULL){ 																																	\
				if (irImporterDynTrace_add_dependence((ir), base_variable, (mem_variable), IR_DEPENDENCE_TYPE_BASE) == NULL){ 												\
					printf("ERROR: in %s, unable to add dependence to IR\n", __func__); 																					\
				} 																																							\
			} 																																								\
			else{ 																																							\
				printf("ERROR: in %s, unable to access base IR variable\n", __func__); 																						\
			} 																																								\
		} 																																									\
																																											\
		if (index_operand_index != (nb_operand)){ 																															\
			index_variable = irImporterDynTrace_get_ir_src_variable((ir), (engine), (operands )+ index_operand_index); 														\
			if (index_variable != NULL){ 																																	\
				if (irImporterDynTrace_add_dependence((ir), index_variable, (mem_variable), IR_DEPENDENCE_TYPE_INDEX) == NULL){ 											\
					printf("ERROR: in %s, unable to add dependence to IR\n", __func__); 																					\
				} 																																							\
			} 																																								\
			else{ 																																							\
				printf("ERROR: in %s, unable to access index IR variable\n", __func__); 																					\
			} 																																								\
		} 																																									\
	}


int32_t irImporterDynTrace_import(struct ir* ir){
	uint32_t 				i;
	struct operand* 		operands;
	struct irRenameEngine 	engine;

	irRenameEngine_init(engine, ir)

	for (i = 0; i < ir->trace->nb_instruction; i++){
		operands = trace_get_ins_operands(ir->trace, i);

		switch(ir->trace->instructions[i].opcode){
			case XED_ICLASS_MOV : {
				uint32_t 		input0_operand_index;
				uint32_t 		output0_operand_index;
				struct node* 	input0_variable;

				input0_operand_index 	= irImporterDynTrace_get_input(operands, ir->trace->instructions[i].nb_operand, 0);
				output0_operand_index 	= irImporterDynTrace_get_output(operands, ir->trace->instructions[i].nb_operand, 0);

				if (input0_operand_index != ir->trace->instructions[i].nb_operand && output0_operand_index != ir->trace->instructions[i].nb_operand){
					input0_variable = irImporterDynTrace_get_ir_src_variable(ir, &engine, operands + input0_operand_index);
					if (input0_variable != NULL){
						irImporterDynTrace_add_dependence_base_and_index(ir, &engine, operands, ir->trace->instructions[i].nb_operand, input0_variable)

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
			case XED_ICLASS_MOVZX : {
				uint32_t 		input0_operand_index;
				uint32_t 		output0_operand_index;
				struct node* 	input0_variable;
				struct node* 	output0_variable;

				input0_operand_index = irImporterDynTrace_get_input(operands, ir->trace->instructions[i].nb_operand, 0);
				output0_operand_index = irImporterDynTrace_get_output(operands, ir->trace->instructions[i].nb_operand, 0);

				if (input0_operand_index != ir->trace->instructions[i].nb_operand && output0_operand_index != ir->trace->instructions[i].nb_operand){
					input0_variable = irImporterDynTrace_get_ir_src_variable(ir, &engine, operands + input0_operand_index);
					output0_variable = irImporterDynTrace_get_ir_dst_variable(ir, &engine, operands + output0_operand_index, IR_MOVZX);

					if (input0_variable != NULL && output0_variable != NULL){
						if (irImporterDynTrace_add_dependence(ir, input0_variable, output0_variable, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
							printf("ERROR: in %s, unable to add output to add dependence to IR\n", __func__);
						}
					}
					else{
						printf("ERROR: in %s, unable to access operand(s) IR variable\n", __func__);
					}
				}
				else{
					printf("WARNING: in %s, incorrect instruction format %s\n",  __func__, instruction_opcode_2_string(ir->trace->instructions[i].opcode));
				}
				break;
			}
			case XED_ICLASS_SHR : {
				uint32_t 		input0_operand_index;
				uint32_t 		output0_operand_index;
				struct node* 	input0_variable;
				struct node* 	output0_variable;

				input0_operand_index = irImporterDynTrace_get_input(operands, ir->trace->instructions[i].nb_operand, 0);
				output0_operand_index = irImporterDynTrace_get_output(operands, ir->trace->instructions[i].nb_operand, 0);

				if (input0_operand_index != ir->trace->instructions[i].nb_operand && output0_operand_index != ir->trace->instructions[i].nb_operand){
					input0_variable = irImporterDynTrace_get_ir_src_variable(ir, &engine, operands + input0_operand_index);
					output0_variable = irImporterDynTrace_get_ir_dst_variable(ir, &engine, operands + output0_operand_index, IR_SHR);

					if (input0_variable != NULL && output0_variable != NULL){
						if (irImporterDynTrace_add_dependence(ir, input0_variable, output0_variable, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
							printf("ERROR: in %s, unable to add output to add dependence to IR\n", __func__);
						}
					}
					else{
						printf("ERROR: in %s, unable to access operand(s) IR variable\n", __func__);
					}
				}
				else{
					printf("WARNING: in %s, incorrect instruction format %s\n",  __func__, instruction_opcode_2_string(ir->trace->instructions[i].opcode));
				}
				break;
			}
			case XED_ICLASS_XOR : {
				uint32_t 		input0_operand_index;
				uint32_t 		input1_operand_index;
				uint32_t 		output0_operand_index;
				struct node* 	input0_variable;
				struct node* 	input1_variable;
				struct node* 	output0_variable;

				input0_operand_index = irImporterDynTrace_get_input(operands, ir->trace->instructions[i].nb_operand, 0);
				input1_operand_index = irImporterDynTrace_get_input(operands, ir->trace->instructions[i].nb_operand, 1);
				output0_operand_index = irImporterDynTrace_get_output(operands, ir->trace->instructions[i].nb_operand, 0);

				if (input0_operand_index != ir->trace->instructions[i].nb_operand && output0_operand_index != ir->trace->instructions[i].nb_operand){
					input0_variable = irImporterDynTrace_get_ir_src_variable(ir, &engine, operands + input0_operand_index);
					if (input0_variable != NULL && OPERAND_IS_MEM(operands[input0_operand_index])){
						irImporterDynTrace_add_dependence_base_and_index(ir, &engine, operands, ir->trace->instructions[i].nb_operand, input0_variable)
					}

					input1_variable = irImporterDynTrace_get_ir_src_variable(ir, &engine, operands + input1_operand_index);
					if (input1_variable != NULL && OPERAND_IS_MEM(operands[input1_operand_index])){
						irImporterDynTrace_add_dependence_base_and_index(ir, &engine, operands, ir->trace->instructions[i].nb_operand, input1_variable)
					}

					output0_variable = irImporterDynTrace_get_ir_dst_variable(ir, &engine, operands + output0_operand_index, IR_XOR);
					if (output0_variable != NULL && OPERAND_IS_MEM(operands[output0_operand_index])){
						irImporterDynTrace_add_dependence_base_and_index(ir, &engine, operands, ir->trace->instructions[i].nb_operand, output0_variable)
					}

					if (input0_variable != NULL && input1_variable != NULL && output0_variable != NULL){
						if (irImporterDynTrace_add_dependence(ir, input0_variable, output0_variable, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
							printf("ERROR: in %s, unable to add output to add dependence to IR\n", __func__);
						}
						if (irImporterDynTrace_add_dependence(ir, input1_variable, output0_variable, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
							printf("ERROR: in %s, unable to add output to add dependence to IR\n", __func__);
						}
					}
					else{
						printf("ERROR: in %s, unable to access operand(s) IR variable\n", __func__);
					}
				}
				else{
					printf("WARNING: in %s, incorrect instruction format %s\n",  __func__, instruction_opcode_2_string(ir->trace->instructions[i].opcode));
				}
				break;
			}
			default : {
				printf("ERROR: in %s, incorrect opcode: %s\n", __func__, instruction_opcode_2_string(ir->trace->instructions[i].opcode));
				break;
			}
		}
	}

	irRenameEngine_clean(engine)

	return 0;
}

struct node* irImporterDynTrace_get_ir_src_variable(struct ir* ir, struct irRenameEngine* engine, struct operand* operand){
	struct node* variable;

	variable = irRenameEngine_get_ref(engine, operand);
	if (variable == NULL){
		variable = irImporterDynTrace_add_input(ir, operand);
		if (variable != NULL){
			ir_node_get_operation(variable)->data = 1;
			if (irRenameEngine_set_new_ref(engine, operand, variable)){
				printf("ERROR: in %s, unable to add new reference to the renaming engine\n", __func__);
			}
		}
		else{
			printf("ERROR: in %s, unable to add input to IR\n", __func__);
		}
	}

	return variable;
}

struct node* irImporterDynTrace_get_ir_dst_variable(struct ir* ir, struct irRenameEngine* engine, struct operand* operand, enum irOpcode opcode){
	struct node* variable;

	variable = irImporterDynTrace_add_operation(ir, opcode);
	if (variable == NULL){
		printf("ERROR: in %s, unable to add output to add operation to IR\n", __func__);
	}
	else{
		ir_node_get_operation(variable)->data = 0;
		if (irRenameEngine_set_ref(engine, operand, variable)){
			printf("ERROR: in %s, unable to set reference in the renameEngine\n", __func__);
		}
	}

	return variable;
}

uint32_t irImporterDynTrace_get_input(struct operand* operands, uint32_t nb_operand, uint32_t index){
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

uint32_t irImporterDynTrace_get_output(struct operand* operands, uint32_t nb_operand, uint32_t index){
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

uint32_t irImporterDynTrace_get_base(struct operand* operands, uint32_t nb_operand){
	uint32_t i;

	for (i = 0; i < nb_operand; i++){
		if (OPERAND_IS_BASE(operands[i])){
			break;
		}
	}

	return i;
}

uint32_t irImporterDynTrace_get_index(struct operand* operands, uint32_t nb_operand){
	uint32_t i;

	for (i = 0; i < nb_operand; i++){
		if (OPERAND_IS_INDEX(operands[i])){
			break;
		}
	}

	return i;
}