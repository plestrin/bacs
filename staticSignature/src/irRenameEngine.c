#include <stdlib.h>
#include <stdio.h>

#include "irRenameEngine.h"

static inline void irImporter_part_reg_variable(struct ir* ir, struct alias* alias_src, struct alias* alias_dst, uint8_t alias_dst_size, enum irOpcode part_op);
/*static void irImporter_merge_reg_variable(struct ir* ir, struct alias* alias_src, struct alias* alias_dst, struct operand* operand, uint8_t alias_src_size, enum irOpcode part_op);*/

static inline void irImporter_part_reg_variable(struct ir* ir, struct alias* alias_src, struct alias* alias_dst, uint8_t alias_dst_size, enum irOpcode part_op){
	alias_dst->ir_node 	= ir_add_inst(ir, part_op, alias_dst_size);
	alias_dst->type 	= IRRENAMEENGINE_TYPE_READ;
	if (alias_dst->ir_node == NULL){
		printf("ERROR: in %s, unable to add operation to IR\n", __func__);
	}
	else{
		ir_add_dependence(ir, alias_src->ir_node, alias_dst->ir_node, IR_DEPENDENCE_TYPE_DIRECT);
	}
}

/*static void irImporter_merge_reg_variable(struct ir* ir, struct alias* alias_src, struct alias* alias_dst, struct operand* operand, uint8_t alias_src_size, enum irOpcode part_op){
	if (alias_src->alias_type.reg.ir_node == NULL){
		alias_src->type 					= ALIAS_REG_READ;
		alias_src->alias_type.reg.ir_node 	= irImporterAsm_add_input(ir, operand, alias_src_size);
		if (alias_src->alias_type.reg.ir_node == NULL){
			printf("ERROR: in %s, unable to add input to IR\n", __func__);
			return;
		}
		ir_node_get_operation(alias_src->alias_type.reg.ir_node)->data = 2;
	}
	ir_convert_input_to_inner(ir, alias_dst->alias_type.reg.ir_node, part_op);
	irImporterAsm_add_dependence(ir, alias_src->alias_type.reg.ir_node, alias_dst->alias_type.reg.ir_node, IR_DEPENDENCE_TYPE_DIRECT);
}*/

struct node* irRenameEngine_get_register_ref(struct irRenameEngine* engine, enum irRegister reg){
	struct node* node;

	node = engine->register_alias[reg].ir_node;
	if (node == NULL){
		switch(reg){
			case IR_REG_EAX 	: {
				if (engine->register_alias[IR_REG_AH].ir_node != NULL){
					printf("WARNING: in %s, opportunity to merge AH to create EAX, not implemented\n", __func__);
					/*if (ALIAS_IS_READ(engine->register_alias[IR_REG_AH].type)){
						irImporter_merge_reg_variable(engine->ir, engine->register_alias + IR_REG_EAX, engine->register_alias + IR_REG_AH, operand, 32, IR_PART2_8);
					}
					else{
						printf("WARNING: in %s, partial write AH\n", __func__);
					}*/
				}
				if (engine->register_alias[IR_REG_AL].ir_node != NULL){
					printf("WARNING: in %s, opportunity to merge AL to create EAX, not implemented\n", __func__);
					/*if (ALIAS_IS_READ(engine->register_alias[IR_REG_AL].type)){
						irImporter_merge_reg_variable(engine->ir, engine->register_alias + IR_REG_EAX, engine->register_alias + IR_REG_AL, operand, 32, IR_PART1_8);
					}
					else{
						printf("WARNING: in %s, partial write AL\n", __func__);
					}*/
				}
				/*node = engine->register_alias[IR_REG_EAX].ir_node;*/
				break;
			}
			case IR_REG_AX 	: {
				if (engine->register_alias[IR_REG_EAX].ir_node != NULL){
					printf("WARNING: in %s, opportunity to part EAX to create AX, not implemented\n", __func__);
				}
				break;
			}
			case IR_REG_AH 	: {
				if (engine->register_alias[IR_REG_EAX].ir_node != NULL){
					irImporter_part_reg_variable(engine->ir, engine->register_alias + IR_REG_EAX, engine->register_alias + IR_REG_AH, 8, IR_PART2_8);
				}
				node = engine->register_alias[IR_REG_AH].ir_node;
				break;
			}
			case IR_REG_AL 	: {
				if (engine->register_alias[IR_REG_EAX].ir_node != NULL){
					irImporter_part_reg_variable(engine->ir, engine->register_alias + IR_REG_EAX, engine->register_alias + IR_REG_AL, 8, IR_PART1_8);
				}
				node = engine->register_alias[IR_REG_AL].ir_node;
				break;
			}
			case IR_REG_EBX 	: {
				if (engine->register_alias[IR_REG_BH].ir_node != NULL){
					printf("WARNING: in %s, opportunity to merge BH to create EBX, not implemented\n", __func__);
					/*if (ALIAS_IS_READ(engine->register_alias[IR_REG_BH].type)){
						irImporter_merge_reg_variable(engine->ir, engine->register_alias + IR_REG_EBX, engine->register_alias + IR_REG_BH, operand, 32, IR_PART2_8);
					}
					else{
						printf("WARNING: in %s, partial write BH\n", __func__);
					}*/
				}
				if (engine->register_alias[IR_REG_BL].ir_node != NULL){
					printf("WARNING: in %s, opportunity to merge BL to create EBX, not implemented\n", __func__);
					/*if (ALIAS_IS_READ(engine->register_alias[IR_REG_BL].type)){
						irImporter_merge_reg_variable(engine->ir, engine->register_alias + IR_REG_EBX, engine->register_alias + IR_REG_BL, operand, 32, IR_PART1_8);
					}
					else{
						printf("WARNING: in %s, partial write BL\n", __func__);
					}*/
				}
				/*node = engine->register_alias[IR_REG_EBX].ir_node;*/
				break;
			}
			case IR_REG_BX 	: {
				if (engine->register_alias[IR_REG_EBX].ir_node != NULL){
					printf("WARNING: in %s, opportunity to part EBX to create BX, not implemented\n", __func__);
				}
				break;
			}
			case IR_REG_BH 	: {
				if (engine->register_alias[IR_REG_EBX].ir_node != NULL){
					irImporter_part_reg_variable(engine->ir, engine->register_alias + IR_REG_EBX, engine->register_alias + IR_REG_BH, 8, IR_PART2_8);
				}
				node = engine->register_alias[IR_REG_BH].ir_node;
				break;
			}
			case IR_REG_BL 	: {
				if (engine->register_alias[IR_REG_EBX].ir_node != NULL){
					irImporter_part_reg_variable(engine->ir, engine->register_alias + IR_REG_EBX, engine->register_alias + IR_REG_BL, 8, IR_PART1_8);
				}
				node = engine->register_alias[IR_REG_BL].ir_node;
				break;
			}
			case IR_REG_ECX 	: {
				if (engine->register_alias[IR_REG_CH].ir_node != NULL){
					printf("WARNING: in %s, opportunity to merge CH to create ECX, not implemented\n", __func__);
					/*if (ALIAS_IS_READ(engine->register_alias[IR_REG_CH].type)){
						irImporter_merge_reg_variable(engine->ir, engine->register_alias + IR_REG_ECX, engine->register_alias + IR_REG_CH, operand, 32, IR_PART2_8);
					}
					else{
						printf("WARNING: in %s, partial write CH\n", __func__);
					}*/
				}
				if (engine->register_alias[IR_REG_CL].ir_node != NULL){
					printf("WARNING: in %s, opportunity to merge CL to create ECX, not implemented\n", __func__);
					/*if (ALIAS_IS_READ(engine->register_alias[IR_REG_CL].type)){
						irImporter_merge_reg_variable(engine->ir, engine->register_alias + IR_REG_ECX, engine->register_alias + IR_REG_CL, operand, 32, IR_PART1_8);
					}
					else{
						printf("WARNING: in %s, partial write CL\n", __func__);
					}*/
				}
				/*node = engine->register_alias[IR_REG_ECX].ir_node;*/
				break;
			}
			case IR_REG_CX 	: {
				if (engine->register_alias[IR_REG_ECX].ir_node != NULL){
					printf("WARNING: in %s, opportunity to part ECX to create CX, not implemented\n", __func__);
				}
				break;
			}
			case IR_REG_CH 	: {
				if (engine->register_alias[IR_REG_ECX].ir_node != NULL){
					irImporter_part_reg_variable(engine->ir, engine->register_alias + IR_REG_ECX, engine->register_alias + IR_REG_CH, 8, IR_PART2_8);
				}
				node = engine->register_alias[IR_REG_CH].ir_node;
				break;
			}
			case IR_REG_CL 	: {
				if (engine->register_alias[IR_REG_ECX].ir_node != NULL){
					irImporter_part_reg_variable(engine->ir, engine->register_alias + IR_REG_ECX, engine->register_alias + IR_REG_CL, 8, IR_PART1_8);
				}
				node = engine->register_alias[IR_REG_CL].ir_node;
				break;
			}
			case IR_REG_EDX 	: {
				if (engine->register_alias[IR_REG_DH].ir_node != NULL){
					printf("WARNING: in %s, opportunity to merge DH to create EDX, not implemented\n", __func__);
					/*if (ALIAS_IS_READ(engine->register_alias[IR_REG_DH].type)){
						irImporter_merge_reg_variable(engine->ir, engine->register_alias + IR_REG_EDX, engine->register_alias + IR_REG_DH, operand, 32, IR_PART2_8);
					}
					else{
						printf("WARNING: in %s, partial write DH\n", __func__);
					}*/
				}
				if (engine->register_alias[IR_REG_DL].ir_node != NULL){
					printf("WARNING: in %s, opportunity to merge DL to create EDX, not implemented\n", __func__);
					/*if (ALIAS_IS_READ(engine->register_alias[IR_REG_DL].type)){
						irImporter_merge_reg_variable(engine->ir, engine->register_alias + IR_REG_EDX, engine->register_alias + IR_REG_DL, operand, 32, IR_PART1_8);
					}
					else{
						printf("WARNING: in %s, partial write DL\n", __func__);
					}*/
				}
				/*node = engine->register_alias[IR_REG_EDX].ir_node;*/
				break;
			}
			case IR_REG_DX 	: {
				if (engine->register_alias[IR_REG_EDX].ir_node != NULL){
					printf("WARNING: in %s, opportunity to part EDX to create DX, not implemented\n", __func__);
				}
				break;
			}
			case IR_REG_DH 	: {
				if (engine->register_alias[IR_REG_EDX].ir_node != NULL){
					irImporter_part_reg_variable(engine->ir, engine->register_alias + IR_REG_EDX, engine->register_alias + IR_REG_DH, 8, IR_PART2_8);
				}
				node = engine->register_alias[IR_REG_DH].ir_node;
				break;
			}
			case IR_REG_DL 	: {
				if (engine->register_alias[IR_REG_EDX].ir_node != NULL){
					irImporter_part_reg_variable(engine->ir, engine->register_alias + IR_REG_EDX, engine->register_alias + IR_REG_DL, 8, IR_PART1_8);
				}
				node = engine->register_alias[IR_REG_DL].ir_node;
				break;
			}
			case IR_REG_ESI 	: {
				break;
			}
			case IR_REG_EDI 	: {
				break;
			}
			case IR_REG_EBP 	: {
				break;
			}
			case IR_REG_ESP 	: {
				break;
			}
		}

		if (node == NULL){
			node = ir_add_in_reg(engine->ir, reg);
			if (node != NULL){
				engine->register_alias[reg].ir_node = node;
				engine->register_alias[reg].type = IRRENAMEENGINE_TYPE_READ;
			}
			else{
				printf("ERROR: in %s, unable to add input to IR\n", __func__);
			}
		}
	}

	return node;
}

void irRenameEngine_set_register_ref(struct irRenameEngine* engine, enum irRegister reg, struct node* node){
	engine->register_alias[reg].ir_node = node;
	engine->register_alias[reg].type = IRRENAMEENGINE_TYPE_WRITE;
	switch(reg){
		case IR_REG_EAX 	: {
			engine->register_alias[IR_REG_AX].ir_node = NULL;
			engine->register_alias[IR_REG_AH].ir_node = NULL;
			engine->register_alias[IR_REG_AL].ir_node = NULL;
			break;
		}
		case IR_REG_AX 	: {
			engine->register_alias[IR_REG_AH].ir_node = NULL;
			engine->register_alias[IR_REG_AL].ir_node = NULL;
			if (engine->register_alias[IR_REG_EAX].ir_node != NULL){
				printf("WARNING: in %s, writting register AX while EAX is valid\n", __func__);
			}
			break;
		}
		case IR_REG_AH 	: {
			if (engine->register_alias[IR_REG_AX].ir_node != NULL){
				printf("WARNING: in %s, writting register AH while AX is valid\n", __func__);
			}
			if (engine->register_alias[IR_REG_EAX].ir_node != NULL){
				printf("WARNING: in %s, writting register AH while EAX is valid\n", __func__);
			}
			break;
		}
		case IR_REG_AL 	: {
			if (engine->register_alias[IR_REG_AX].ir_node != NULL){
				printf("WARNING: in %s, writting register AL while AX is valid\n", __func__);
			}
			if (engine->register_alias[IR_REG_EAX].ir_node != NULL){
				printf("WARNING: in %s, writting register AL while EAX is valid\n", __func__);
			}
			break;
		}
		case IR_REG_EBX 	: {
			engine->register_alias[IR_REG_BX].ir_node = NULL;
			engine->register_alias[IR_REG_BH].ir_node = NULL;
			engine->register_alias[IR_REG_BL].ir_node = NULL;
			break;
		}
		case IR_REG_BX 	: {
			engine->register_alias[IR_REG_BH].ir_node = NULL;
			engine->register_alias[IR_REG_BL].ir_node = NULL;
			if (engine->register_alias[IR_REG_EBX].ir_node != NULL){
				printf("WARNING: in %s, writting register BX while EBX is valid\n", __func__);
			}
			break;
		}
		case IR_REG_BH 	: {
			if (engine->register_alias[IR_REG_BX].ir_node != NULL){
				printf("WARNING: in %s, writting register BH while BX is valid\n", __func__);
			}
			if (engine->register_alias[IR_REG_EBX].ir_node != NULL){
				printf("WARNING: in %s, writting register BH while EBX is valid\n", __func__);
			}
			break;
		}
		case IR_REG_BL 	: {
			if (engine->register_alias[IR_REG_BX].ir_node != NULL){
				printf("WARNING: in %s, writting register BL while BX is valid\n", __func__);
			}
			if (engine->register_alias[IR_REG_EBX].ir_node != NULL){
				printf("WARNING: in %s, writting register BL while EBX is valid\n", __func__);
			}
			break;
		}
		case IR_REG_ECX 	: {
			engine->register_alias[IR_REG_CX].ir_node = NULL;
			engine->register_alias[IR_REG_CH].ir_node = NULL;
			engine->register_alias[IR_REG_CL].ir_node = NULL;
			break;
		}
		case IR_REG_CX 	: {
			engine->register_alias[IR_REG_CH].ir_node = NULL;
			engine->register_alias[IR_REG_CL].ir_node = NULL;
			if (engine->register_alias[IR_REG_ECX].ir_node != NULL){
				printf("WARNING: in %s, writting register CX while ECX is valid\n", __func__);
			}
			break;
		}
		case IR_REG_CH 	: {
			if (engine->register_alias[IR_REG_CX].ir_node != NULL){
				printf("WARNING: in %s, writting register CH while CX is valid\n", __func__);
			}
			if (engine->register_alias[IR_REG_ECX].ir_node != NULL){
				printf("WARNING: in %s, writting register CH while ECX is valid\n", __func__);
			}
			break;
		}
		case IR_REG_CL 	: {
			if (engine->register_alias[IR_REG_CX].ir_node != NULL){
				printf("WARNING: in %s, writting register CL while CX is valid\n", __func__);
			}
			if (engine->register_alias[IR_REG_ECX].ir_node != NULL){
				printf("WARNING: in %s, writting register CL while ECX is valid\n", __func__);
			}
			break;
		}
		case IR_REG_EDX 	: {
			engine->register_alias[IR_REG_DX].ir_node = NULL;
			engine->register_alias[IR_REG_DH].ir_node = NULL;
			engine->register_alias[IR_REG_DL].ir_node = NULL;
			break;
		}
		case IR_REG_DX 	: {
			engine->register_alias[IR_REG_DH].ir_node = NULL;
			engine->register_alias[IR_REG_DL].ir_node = NULL;
			if (engine->register_alias[IR_REG_EDX].ir_node != NULL){
				printf("WARNING: in %s, writting register DX while EDX is valid\n", __func__);
			}
			break;
		}
		case IR_REG_DH 	: {
			if (engine->register_alias[IR_REG_DX].ir_node != NULL){
				printf("WARNING: in %s, writting register DH while DX is valid\n", __func__);
			}
			if (engine->register_alias[IR_REG_EDX].ir_node != NULL){
				printf("WARNING: in %s, writting register DH while EDX is valid\n", __func__);
				}
			break;
		}
		case IR_REG_DL 	: {
			if (engine->register_alias[IR_REG_DX].ir_node != NULL){
				printf("WARNING: in %s, writting register DL while DX is valid\n", __func__);
			}
			if (engine->register_alias[IR_REG_EDX].ir_node != NULL){
				printf("WARNING: in %s, writting register DL while EDX is valid\n", __func__);
			}
			break;
		}
		case IR_REG_ESI 	: {
			break;
		}
		case IR_REG_EDI 	: {
			break;
		}
		case IR_REG_EBP 	: {
			break;
		}
		case IR_REG_ESP 	: {
			break;
		}
	}
}
