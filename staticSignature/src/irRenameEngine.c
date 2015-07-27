#include <stdlib.h>
#include <stdio.h>

#include "irRenameEngine.h"
#include "base.h"

#define MAX_NB_INNER_REGISTER 3

struct irRegisterBuffer{
	uint8_t 			nb_register;
	enum irRegister 	registers[MAX_NB_INNER_REGISTER];
};

static void irRenameEngine_get_list(struct irRenameEngine* engine, enum irRegister reg, struct irRegisterBuffer* list);
static enum irRegister irRenameEngine_pop_list(struct irRenameEngine* engine, struct irRegisterBuffer* list);

#define irRenameEngine_is_register_nested(reg) ((reg) <= 23)
#define NB_IR_NESTED_REGISTER 24

static uint8_t registerFamily[NB_IR_NESTED_REGISTER] = {
	0, /* IR_REG_EAX */
	0, /* IR_REG_AX */
	0, /* IR_REG_AH */
	0, /* IR_REG_AL */
	1, /* IR_REG_EBX */
	1, /* IR_REG_BX */
	1, /* IR_REG_BH */
	1, /* IR_REG_BL */
	2, /* IR_REG_ECX */
	2, /* IR_REG_CX */
	2, /* IR_REG_CH */
	2, /* IR_REG_CL */
	3, /* IR_REG_EDX */
	3, /* IR_REG_DX */
	3, /* IR_REG_DH */
	3, /* IR_REG_DL */
	4, /* IR_REG_ESP */
	4, /* IR_REG_SP */
	5, /* IR_REG_EBP */
	5, /* IR_REG_BP */
	6, /* IR_REG_ESI */
	6, /* IR_REG_SI */
	7, /* IR_REG_EDI */
	7  /* IR_REG_DI */
};

static uint8_t registerIndex[NB_IR_NESTED_REGISTER] = {
	0, /* IR_REG_EAX */
	1, /* IR_REG_AX */
	2, /* IR_REG_AH */
	3, /* IR_REG_AL */
	0, /* IR_REG_EBX */
	1, /* IR_REG_BX */
	2, /* IR_REG_BH */
	3, /* IR_REG_BL */
	0, /* IR_REG_ECX */
	1, /* IR_REG_CX */
	2, /* IR_REG_CH */
	3, /* IR_REG_CL */
	0, /* IR_REG_EDX */
	1, /* IR_REG_DX */
	2, /* IR_REG_DH */
	3, /* IR_REG_DL */
	0, /* IR_REG_ESP */
	1, /* IR_REG_SP */
	0, /* IR_REG_EBP */
	1, /* IR_REG_BP */
	0, /* IR_REG_ESI */
	1, /* IR_REG_SI */
	0, /* IR_REG_EDI */
	1  /* IR_REG_DI */
};

#define NB_IRREGISTER_FAMILY 	8
#define MAX_REGISTER_PER_FAMILY 4

static uint8_t larger[NB_IRREGISTER_FAMILY][MAX_REGISTER_PER_FAMILY][MAX_REGISTER_PER_FAMILY] = {
	{
		{0, 1, 1, 1}, /* _A_ family */
		{0, 0, 1, 1},
		{0, 0, 0, 0},
		{0, 0, 0, 0}
	},
	{
		{0, 1, 1, 1}, /* _B_ family */
		{0, 0, 1, 1},
		{0, 0, 0, 0},
		{0, 0, 0, 0}
	},
	{
		{0, 1, 1, 1}, /* _C_ family */
		{0, 0, 1, 1},
		{0, 0, 0, 0},
		{0, 0, 0, 0}
	},
	{
		{0, 1, 1, 1}, /* _D_ family */
		{0, 0, 1, 1},
		{0, 0, 0, 0},
		{0, 0, 0, 0}
	},
	{
		{0, 1, 0, 0}, /* _SP family */
		{0, 0, 0, 0},
		{0, 0, 0, 0},
		{0, 0, 0, 0}
	},
	{
		{0, 1, 0, 0}, /* _BP family */
		{0, 0, 0, 0},
		{0, 0, 0, 0},
		{0, 0, 0, 0}
	},
	{
		{0, 1, 0, 0}, /* _SI family */
		{0, 0, 0, 0},
		{0, 0, 0, 0},
		{0, 0, 0, 0}
	},
	{
		{0, 1, 0, 0}, /* _DI family */
		{0, 0, 0, 0},
		{0, 0, 0, 0},
		{0, 0, 0, 0}
	}
};

static enum irOpcode partIns[NB_IRREGISTER_FAMILY][MAX_REGISTER_PER_FAMILY][MAX_REGISTER_PER_FAMILY] = {
	{
		{IR_INVALID, IR_PART1_16, IR_PART2_8, IR_PART1_8}, /* _A_ family */
		{IR_INVALID, IR_INVALID , IR_PART2_8, IR_PART1_8},
		{IR_INVALID, IR_INVALID , IR_INVALID, IR_INVALID},
		{IR_INVALID, IR_INVALID , IR_INVALID, IR_INVALID}
	},
	{
		{IR_INVALID, IR_PART1_16, IR_PART2_8, IR_PART1_8}, /* _B_ family */
		{IR_INVALID, IR_INVALID , IR_PART2_8, IR_PART1_8},
		{IR_INVALID, IR_INVALID , IR_INVALID, IR_INVALID},
		{IR_INVALID, IR_INVALID , IR_INVALID, IR_INVALID}
	},
	{
		{IR_INVALID, IR_PART1_16, IR_PART2_8, IR_PART1_8}, /* _C_ family */
		{IR_INVALID, IR_INVALID , IR_PART2_8, IR_PART1_8},
		{IR_INVALID, IR_INVALID , IR_INVALID, IR_INVALID},
		{IR_INVALID, IR_INVALID , IR_INVALID, IR_INVALID}
	},
	{
		{IR_INVALID, IR_PART1_16, IR_PART2_8, IR_PART1_8}, /* _D_ family */
		{IR_INVALID, IR_INVALID , IR_PART2_8, IR_PART1_8},
		{IR_INVALID, IR_INVALID , IR_INVALID, IR_INVALID},
		{IR_INVALID, IR_INVALID , IR_INVALID, IR_INVALID}
	},
	{
		{IR_INVALID, IR_PART1_16, IR_INVALID, IR_INVALID}, /* _SP family */
		{IR_INVALID, IR_INVALID , IR_INVALID, IR_INVALID},
		{IR_INVALID, IR_INVALID , IR_INVALID, IR_INVALID},
		{IR_INVALID, IR_INVALID , IR_INVALID, IR_INVALID}
	},
	{
		{IR_INVALID, IR_PART1_16, IR_INVALID, IR_INVALID}, /* _BP family */
		{IR_INVALID, IR_INVALID , IR_INVALID, IR_INVALID},
		{IR_INVALID, IR_INVALID , IR_INVALID, IR_INVALID},
		{IR_INVALID, IR_INVALID , IR_INVALID, IR_INVALID}
	},
	{
		{IR_INVALID, IR_PART1_16, IR_INVALID, IR_INVALID}, /* _SI family */
		{IR_INVALID, IR_INVALID , IR_INVALID, IR_INVALID},
		{IR_INVALID, IR_INVALID , IR_INVALID, IR_INVALID},
		{IR_INVALID, IR_INVALID , IR_INVALID, IR_INVALID}
	},
	{
		{IR_INVALID, IR_PART1_16, IR_INVALID, IR_INVALID}, /* _DI family */
		{IR_INVALID, IR_INVALID , IR_INVALID, IR_INVALID},
		{IR_INVALID, IR_INVALID , IR_INVALID, IR_INVALID},
		{IR_INVALID, IR_INVALID , IR_INVALID, IR_INVALID}
	}
};

static void irRenameEngine_get_list(struct irRenameEngine* engine, enum irRegister reg, struct irRegisterBuffer* list){
	uint8_t i;
	uint8_t nb_inner = 0;
	uint8_t nb_newer;

	switch(reg){
		case IR_REG_EAX 	: {
			list->registers[0] = IR_REG_AX;
			list->registers[1] = IR_REG_AH;
			list->registers[2] = IR_REG_AL;

			nb_inner = 3;
			break;
		}
		case IR_REG_AX 		: {
			list->registers[0] = IR_REG_EAX;
			list->registers[1] = IR_REG_AH;
			list->registers[2] = IR_REG_AL;

			nb_inner = 3;
			break;
		}
		case IR_REG_AH 		: {
			list->registers[0] = IR_REG_EAX;
			list->registers[1] = IR_REG_AX;

			nb_inner = 2;
			break;
		}
		case IR_REG_AL 		: {
			list->registers[0] = IR_REG_EAX;
			list->registers[1] = IR_REG_AX;

			nb_inner = 2;
			break;
		}
		case IR_REG_EBX 	: {
			list->registers[0] = IR_REG_BX;
			list->registers[1] = IR_REG_BH;
			list->registers[2] = IR_REG_BL;

			nb_inner = 3;
			break;
		}
		case IR_REG_BX 		: {
			list->registers[0] = IR_REG_EBX;
			list->registers[1] = IR_REG_BH;
			list->registers[2] = IR_REG_BL;

			nb_inner = 3;
			break;
		}
		case IR_REG_BH 		: {
			list->registers[0] = IR_REG_EBX;
			list->registers[1] = IR_REG_BX;

			nb_inner = 2;
			break;
		}
		case IR_REG_BL 		: {
			list->registers[0] = IR_REG_EBX;
			list->registers[1] = IR_REG_BX;

			nb_inner = 2;
			break;
		}
		case IR_REG_ECX 	: {
			list->registers[0] = IR_REG_CX;
			list->registers[1] = IR_REG_CH;
			list->registers[2] = IR_REG_CL;

			nb_inner = 3;
			break;
		}
		case IR_REG_CX 		: {
			list->registers[0] = IR_REG_ECX;
			list->registers[1] = IR_REG_CH;
			list->registers[2] = IR_REG_CL;

			nb_inner = 3;
			break;
		}
		case IR_REG_CH 		: {
			list->registers[0] = IR_REG_ECX;
			list->registers[1] = IR_REG_CX;

			nb_inner = 2;
			break;
		}
		case IR_REG_CL 		: {
			list->registers[0] = IR_REG_ECX;
			list->registers[1] = IR_REG_CX;

			nb_inner = 2;
			break;
		}
		case IR_REG_EDX 	: {
			list->registers[0] = IR_REG_DX;
			list->registers[1] = IR_REG_DH;
			list->registers[2] = IR_REG_DL;

			nb_inner = 3;
			break;
		}
		case IR_REG_DX 		: {
			list->registers[0] = IR_REG_EDX;
			list->registers[1] = IR_REG_DH;
			list->registers[2] = IR_REG_DL;

			nb_inner = 3;
			break;
		}
		case IR_REG_DH 		: {
			list->registers[0] = IR_REG_EDX;
			list->registers[1] = IR_REG_DX;

			nb_inner = 2;
			break;
		}
		case IR_REG_DL 		: {
			list->registers[0] = IR_REG_EDX;
			list->registers[1] = IR_REG_DX;

			nb_inner = 2;
			break;
		}
		case IR_REG_ESP 	: {
			list->registers[0] = IR_REG_SP;

			nb_inner = 1;
			break;
		}
		case IR_REG_SP 	: {
			list->registers[0] = IR_REG_ESP;

			nb_inner = 1;
			break;
		}
		case IR_REG_EBP 	: {
			list->registers[0] = IR_REG_BP;

			nb_inner = 1;
			break;
		}
		case IR_REG_BP 	: {
			list->registers[0] = IR_REG_EBP;

			nb_inner = 1;
			break;
		}
		case IR_REG_ESI 	: {
			list->registers[0] = IR_REG_SI;

			nb_inner = 1;
			break;
		}
		case IR_REG_SI 	: {
			list->registers[0] = IR_REG_ESI;

			nb_inner = 1;
			break;
		}
		case IR_REG_EDI 	: {
			list->registers[0] = IR_REG_DI;

			nb_inner = 1;
			break;
		}
		case IR_REG_DI 	: {
			list->registers[0] = IR_REG_EDI;

			nb_inner = 1;
			break;
		}
		default 		: {
			nb_inner = 0;
			break;
		}
	}

	for (i = 0, nb_newer = 0; i < nb_inner; i++){
		if (engine->register_alias[list->registers[i]].ir_node != NULL && engine->register_alias[reg].order < engine->register_alias[list->registers[i]].order){
			if (nb_newer != i){
				list->registers[nb_newer] = list->registers[i];
			}
			nb_newer ++;
		}
	}
	list->nb_register = nb_newer;
}

static enum irRegister irRenameEngine_pop_list(struct irRenameEngine* engine, struct irRegisterBuffer* list){
	uint8_t 			i;
	enum irRegister 	reg;
	uint32_t 			max_ts;
	uint8_t  			nb_older;
	
	reg = list->registers[0];
	max_ts = engine->register_alias[list->registers[0]].order;

	for (i = 1; i < list->nb_register; i++){
		if (max_ts < engine->register_alias[list->registers[i]].order){
			max_ts = engine->register_alias[list->registers[i]].order;
			reg = list->registers[i];
		}
		else if (max_ts == engine->register_alias[list->registers[i]].order && (ALIAS_IS_EXTEND(engine->register_alias[reg].type) || ALIAS_IS_WRITE(engine->register_alias[list->registers[i]].type))){
			reg = list->registers[i];
		}
	}

	for (i = 0, nb_older = 0; i < list->nb_register; i++){
		if (engine->register_alias[list->registers[i]].order != max_ts){
			if (nb_older != i){
				list->registers[nb_older] = list->registers[i];
			}
			nb_older ++;
		}
	}
	list->nb_register = nb_older;

	return reg;
}

struct node* irRenameEngine_get_register_ref(struct irRenameEngine* engine, enum irRegister reg, uint32_t instruction_index, uint64_t bb_id){
	struct irRegisterBuffer list;
	struct node* 			node = NULL;
	enum irRegister 		reg1;
	enum irRegister 		reg2;
	uint8_t 				family;
	enum irOpcode 			new_ins;

	if (irRenameEngine_is_register_nested(reg)){
		irRenameEngine_get_list(engine, reg, &list);
	}
	else {
		list.nb_register = 0;
	}
	if (list.nb_register){
		reg1 = irRenameEngine_pop_list(engine, &list);
		family = registerFamily[reg];
		if (family == registerFamily[reg1]){
			if (larger[family][registerIndex[reg1]][registerIndex[reg]]){
				new_ins = partIns[family][registerIndex[reg1]][registerIndex[reg]];

				if (new_ins != IR_INVALID){
					engine->register_alias[reg].ir_node = ir_add_inst(engine->ir, instruction_index, irRegister_get_size(reg), new_ins, bb_id);
					engine->register_alias[reg].order 	= engine->register_alias[reg1].order;
					engine->register_alias[reg].type 	= IRRENAMEENGINE_TYPE_EXTEND;
					if (engine->register_alias[reg].ir_node == NULL){
						log_err("unable to add operation to IR");
					}
					else{
						ir_add_dependence(engine->ir, engine->register_alias[reg1].ir_node, engine->register_alias[reg].ir_node, IR_DEPENDENCE_TYPE_DIRECT);
						node = engine->register_alias[reg].ir_node;
					}
				}
				else{
					log_err("unable to choose correct instruction, INVALID");
				}
			}
			else{
				if (list.nb_register){
					reg2 = irRenameEngine_pop_list(engine, &list);
					if (list.nb_register){
						log_err("this case is not implemented yet, more than two element in the dependence list");
					}
					else if (ALIAS_IS_READ(engine->register_alias[reg1].type)){
						engine->register_alias[reg].ir_node = ir_add_in_reg(engine->ir, instruction_index, reg);
						engine->register_alias[reg].order 	= engine->register_alias[reg1].order;
						engine->register_alias[reg].type 	= IRRENAMEENGINE_TYPE_READ;

						if (engine->register_alias[reg].ir_node != NULL){
							engine->register_alias[reg1].type = IRRENAMEENGINE_TYPE_EXTEND;

							ir_convert_node_to_inst(engine->register_alias[reg1].ir_node, ir_node_get_operation(engine->register_alias[reg1].ir_node)->index, irRegister_get_size(reg1), partIns[family][registerIndex[reg]][registerIndex[reg1]]);
							ir_add_dependence(engine->ir, engine->register_alias[reg].ir_node, engine->register_alias[reg1].ir_node, IR_DEPENDENCE_TYPE_DIRECT);

							engine->register_alias[reg2].type = IRRENAMEENGINE_TYPE_EXTEND;
							engine->register_alias[reg2].order = engine->register_alias[reg1].order;

							ir_convert_node_to_inst(engine->register_alias[reg2].ir_node, ir_node_get_operation(engine->register_alias[reg2].ir_node)->index, irRegister_get_size(reg2), partIns[family][registerIndex[reg]][registerIndex[reg2]]);
							ir_add_dependence(engine->ir, engine->register_alias[reg].ir_node, engine->register_alias[reg2].ir_node, IR_DEPENDENCE_TYPE_DIRECT);

							node = engine->register_alias[reg].ir_node;
						}
						else{
							log_err("unable to add input to IR");
						}
					}
					else{
						log_err_m("this case is not implemented yet, %s and %s in the dependence list of %s", irRegister_2_string(reg1), irRegister_2_string(reg2), irRegister_2_string(reg));
					}
				}
				else{
					if (ALIAS_IS_READ(engine->register_alias[reg1].type)){
						new_ins = partIns[family][registerIndex[reg]][registerIndex[reg1]];

						if (new_ins != IR_INVALID){
							engine->register_alias[reg].ir_node = ir_add_in_reg(engine->ir, instruction_index, reg);
							engine->register_alias[reg].order 	= engine->register_alias[reg1].order;
							engine->register_alias[reg].type 	= IRRENAMEENGINE_TYPE_READ;

							if (engine->register_alias[reg].ir_node != NULL){
								engine->register_alias[reg1].type = IRRENAMEENGINE_TYPE_EXTEND;

								ir_convert_node_to_inst(engine->register_alias[reg1].ir_node, ir_node_get_operation(engine->register_alias[reg1].ir_node)->index, irRegister_get_size(reg1), new_ins);
								ir_add_dependence(engine->ir, engine->register_alias[reg].ir_node, engine->register_alias[reg1].ir_node, IR_DEPENDENCE_TYPE_DIRECT);

								node = engine->register_alias[reg].ir_node;
							}
							else{
								log_err("unable to add input to IR");
							}
						}
						else{
							log_err("unable to choose correct instruction, INVALID");
						}
					}
					else{
						if (engine->register_alias[reg].ir_node){
							uint32_t 		size_reg1;
							uint32_t 		size_reg;
							struct node* 	imm_mask;
							struct node* 	ins_and;
							struct node* 	ins_movzx;

							size_reg 	= irRegister_get_size(reg);
							size_reg1 	= irRegister_get_size(reg1);

							imm_mask 	= ir_add_immediate(engine->ir, size_reg, (0xffffffffffffffff >> (64 - size_reg)) << size_reg1);
							ins_and 	= ir_add_inst(engine->ir, instruction_index, size_reg, IR_AND, bb_id);
							node 		= ir_add_inst(engine->ir, instruction_index, size_reg, IR_OR, bb_id);
							ins_movzx 	= ir_add_inst(engine->ir, instruction_index, size_reg, IR_MOVZX, bb_id);

							if (imm_mask == NULL || ins_and == NULL || node == NULL || ins_movzx == NULL){
								log_err("unable to add node to IR");
							}
							else{
								ir_add_dependence(engine->ir, imm_mask, ins_and, IR_DEPENDENCE_TYPE_DIRECT);
								ir_add_dependence(engine->ir, ins_and, node, IR_DEPENDENCE_TYPE_DIRECT);
								ir_add_dependence(engine->ir, ins_movzx, node, IR_DEPENDENCE_TYPE_DIRECT);
								ir_add_dependence(engine->ir, engine->register_alias[reg].ir_node, ins_and, IR_DEPENDENCE_TYPE_DIRECT);
								ir_add_dependence(engine->ir, engine->register_alias[reg1].ir_node, ins_movzx, IR_DEPENDENCE_TYPE_DIRECT);

								engine->register_alias[reg].ir_node = node;
								engine->register_alias[reg].order 	= engine->register_alias[reg1].order;
								engine->register_alias[reg].type 	= IRRENAMEENGINE_TYPE_READ;
							}
						}
						else{
							log_err_m("this case is not implemented yet, smaller write dependence (%s - %s = NULL)", irRegister_2_string(reg1), irRegister_2_string(reg));
						}
					}
				}
			}
		}
		else{
			log_err_m("register is in dependence list but not in the same family (%s - %s)", irRegister_2_string(reg1), irRegister_2_string(reg));
		}
	}
	else{
		node = engine->register_alias[reg].ir_node;
		if (node == NULL){
			node = ir_add_in_reg(engine->ir, instruction_index, reg);
			if (node != NULL){
				engine->register_alias[reg].ir_node = node;
				engine->register_alias[reg].order 	= irRenameEngine_get_reg_order(engine);
				engine->register_alias[reg].type 	= IRRENAMEENGINE_TYPE_READ;
			}
			else{
				log_err("unable to add input to IR");
			}
		}
	}

	return node;
}

void irRenameEngine_set_register_ref(struct irRenameEngine* engine, enum irRegister reg, struct node* node){
	engine->register_alias[reg].ir_node 	= node;
	engine->register_alias[reg].order  		= irRenameEngine_get_reg_order(engine);
	engine->register_alias[reg].type 		= IRRENAMEENGINE_TYPE_WRITE;
	switch(reg){
		case IR_REG_EAX 	: {
			engine->register_alias[IR_REG_AX].ir_node = NULL;
			engine->register_alias[IR_REG_AH].ir_node = NULL;
			engine->register_alias[IR_REG_AL].ir_node = NULL;
			break;
		}
		case IR_REG_AX 		: {
			engine->register_alias[IR_REG_AH].ir_node = NULL;
			engine->register_alias[IR_REG_AL].ir_node = NULL;
			break;
		}
		case IR_REG_EBX 	: {
			engine->register_alias[IR_REG_BX].ir_node = NULL;
			engine->register_alias[IR_REG_BH].ir_node = NULL;
			engine->register_alias[IR_REG_BL].ir_node = NULL;
			break;
		}
		case IR_REG_BX 		: {
			engine->register_alias[IR_REG_BH].ir_node = NULL;
			engine->register_alias[IR_REG_BL].ir_node = NULL;
			break;
		}
		case IR_REG_ECX 	: {
			engine->register_alias[IR_REG_CX].ir_node = NULL;
			engine->register_alias[IR_REG_CH].ir_node = NULL;
			engine->register_alias[IR_REG_CL].ir_node = NULL;
			break;
		}
		case IR_REG_CX 		: {
			engine->register_alias[IR_REG_CH].ir_node = NULL;
			engine->register_alias[IR_REG_CL].ir_node = NULL;
			break;
		}
		case IR_REG_EDX 	: {
			engine->register_alias[IR_REG_DX].ir_node = NULL;
			engine->register_alias[IR_REG_DH].ir_node = NULL;
			engine->register_alias[IR_REG_DL].ir_node = NULL;
			break;
		}
		case IR_REG_DX 		: {
			engine->register_alias[IR_REG_DH].ir_node = NULL;
			engine->register_alias[IR_REG_DL].ir_node = NULL;
			break;
		}
		case IR_REG_ESP 		: {
			engine->register_alias[IR_REG_SP].ir_node = NULL;
			break;
		}
		case IR_REG_EBP 		: {
			engine->register_alias[IR_REG_BP].ir_node = NULL;
			break;
		}
		case IR_REG_ESI 		: {
			engine->register_alias[IR_REG_SI].ir_node = NULL;
			break;
		}
		case IR_REG_EDI 		: {
			engine->register_alias[IR_REG_DI].ir_node = NULL;
			break;
		}
		default 			: {
			break;
		}
	}
}

void irRenameEngine_tag_final_node(struct irRenameEngine* engine){
	uint32_t 			i;
	struct irOperation* operation;

	for (i = 0; i < NB_IR_REGISTER; i++){
		if (i == IR_REG_TMP){
			continue;
		}

		if (engine->register_alias[i].ir_node != NULL  && ALIAS_IS_WRITE(engine->register_alias[i].type)){
			operation = ir_node_get_operation(engine->register_alias[i].ir_node);
			operation->status_flag |= IR_OPERATION_STATUS_FLAG_FINAL;
		}
	}
}
