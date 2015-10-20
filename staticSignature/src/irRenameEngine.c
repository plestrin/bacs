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

void irRenameEngine_init(struct irRenameEngine* engine, struct ir* ir){
	uint32_t i;

	engine->prev_mem_access = NULL;
	engine->reg_op_order 	= 1;
	engine->func_id 		= IR_CALL_STACK_PTR + 1;
	engine->ir 				= ir;

	memset(&(ir->alias_buffer), 0, sizeof(struct alias) * NB_IR_REGISTER);

	for (i = 0; i <= IR_CALL_STACK_PTR; i++){
		engine->ir->stack[i] = i;
	}

	memset(ir->stack + i, 0, IR_CALL_STACK_MAX_SIZE - (IR_CALL_STACK_PTR + 1));

	ir->stack_ptr = IR_CALL_STACK_PTR;
}

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
		if (engine->ir->alias_buffer[list->registers[i]].ir_node != NULL && engine->ir->alias_buffer[reg].order < engine->ir->alias_buffer[list->registers[i]].order){
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
	max_ts = engine->ir->alias_buffer[list->registers[0]].order;

	for (i = 1; i < list->nb_register; i++){
		if (max_ts < engine->ir->alias_buffer[list->registers[i]].order){
			max_ts = engine->ir->alias_buffer[list->registers[i]].order;
			reg = list->registers[i];
		}
		else if (max_ts == engine->ir->alias_buffer[list->registers[i]].order && (alias_is_extend(engine->ir->alias_buffer[reg]) || alias_is_write(engine->ir->alias_buffer[list->registers[i]]))){
			reg = list->registers[i];
		}
	}

	for (i = 0, nb_older = 0; i < list->nb_register; i++){
		if (engine->ir->alias_buffer[list->registers[i]].order != max_ts){
			if (nb_older != i){
				list->registers[nb_older] = list->registers[i];
			}
			nb_older ++;
		}
	}
	list->nb_register = nb_older;

	return reg;
}

struct node* irRenameEngine_get_register_ref(struct irRenameEngine* engine, enum irRegister reg, uint32_t instruction_index, uint32_t dst){
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
					engine->ir->alias_buffer[reg].ir_node = ir_add_inst(engine->ir, instruction_index, irRegister_get_size(reg), new_ins, dst);
					engine->ir->alias_buffer[reg].order = engine->ir->alias_buffer[reg1].order;
					alias_set_type(engine->ir->alias_buffer[reg], IRRENAMEENGINE_TYPE_EXTEND);
					if (engine->ir->alias_buffer[reg].ir_node == NULL){
						log_err("unable to add operation to IR");
					}
					else{
						ir_add_dependence(engine->ir, engine->ir->alias_buffer[reg1].ir_node, engine->ir->alias_buffer[reg].ir_node, IR_DEPENDENCE_TYPE_DIRECT);
						node = engine->ir->alias_buffer[reg].ir_node;
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
						log_err("this case is not implemented yet, more than two elements in the dependence list");
					}
					else if (alias_is_read(engine->ir->alias_buffer[reg1])){
						engine->ir->alias_buffer[reg].ir_node = ir_add_in_reg(engine->ir, min(ir_node_get_operation(engine->ir->alias_buffer[reg1].ir_node)->index, ir_node_get_operation(engine->ir->alias_buffer[reg2].ir_node)->index), reg, (alias_is_primer(engine->ir->alias_buffer[reg])) ? IR_IN_REG_IS_PRIMER : ~IR_IN_REG_IS_PRIMER);
						engine->ir->alias_buffer[reg].order = engine->ir->alias_buffer[reg1].order;
						alias_set_type(engine->ir->alias_buffer[reg], IRRENAMEENGINE_TYPE_READ);

						if (engine->ir->alias_buffer[reg].ir_node != NULL){
							alias_set_type(engine->ir->alias_buffer[reg1], IRRENAMEENGINE_TYPE_EXTEND);

							ir_convert_node_to_inst(engine->ir->alias_buffer[reg1].ir_node, ir_node_get_operation(engine->ir->alias_buffer[reg1].ir_node)->index, irRegister_get_size(reg1), partIns[family][registerIndex[reg]][registerIndex[reg1]]);
							ir_add_dependence(engine->ir, engine->ir->alias_buffer[reg].ir_node, engine->ir->alias_buffer[reg1].ir_node, IR_DEPENDENCE_TYPE_DIRECT);

							alias_set_type(engine->ir->alias_buffer[reg2], IRRENAMEENGINE_TYPE_EXTEND);
							engine->ir->alias_buffer[reg2].order = engine->ir->alias_buffer[reg1].order;

							ir_convert_node_to_inst(engine->ir->alias_buffer[reg2].ir_node, ir_node_get_operation(engine->ir->alias_buffer[reg2].ir_node)->index, irRegister_get_size(reg2), partIns[family][registerIndex[reg]][registerIndex[reg2]]);
							ir_add_dependence(engine->ir, engine->ir->alias_buffer[reg].ir_node, engine->ir->alias_buffer[reg2].ir_node, IR_DEPENDENCE_TYPE_DIRECT);

							node = engine->ir->alias_buffer[reg].ir_node;
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
					if (alias_is_read(engine->ir->alias_buffer[reg1])){
						new_ins = partIns[family][registerIndex[reg]][registerIndex[reg1]];

						if (new_ins != IR_INVALID){
							engine->ir->alias_buffer[reg].ir_node = ir_add_in_reg(engine->ir, ir_node_get_operation(engine->ir->alias_buffer[reg1].ir_node)->index, reg, (alias_is_primer(engine->ir->alias_buffer[reg])) ? IR_IN_REG_IS_PRIMER : ~IR_IN_REG_IS_PRIMER);
							engine->ir->alias_buffer[reg].order = engine->ir->alias_buffer[reg1].order;
							alias_set_type(engine->ir->alias_buffer[reg], IRRENAMEENGINE_TYPE_READ);

							if (engine->ir->alias_buffer[reg].ir_node != NULL){
								alias_set_type(engine->ir->alias_buffer[reg1], IRRENAMEENGINE_TYPE_EXTEND);

								ir_convert_node_to_inst(engine->ir->alias_buffer[reg1].ir_node, ir_node_get_operation(engine->ir->alias_buffer[reg1].ir_node)->index, irRegister_get_size(reg1), new_ins);
								ir_add_dependence(engine->ir, engine->ir->alias_buffer[reg].ir_node, engine->ir->alias_buffer[reg1].ir_node, IR_DEPENDENCE_TYPE_DIRECT);

								node = engine->ir->alias_buffer[reg].ir_node;
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
						if (engine->ir->alias_buffer[reg].ir_node){
							uint32_t 		size_reg1;
							uint32_t 		size_reg;
							struct node* 	imm_mask;
							struct node* 	ins_and;
							struct node* 	ins_movzx;

							size_reg 	= irRegister_get_size(reg);
							size_reg1 	= irRegister_get_size(reg1);

							imm_mask 	= ir_add_immediate(engine->ir, size_reg, (0xffffffffffffffff >> (64 - size_reg)) << size_reg1);
							ins_and 	= ir_add_inst(engine->ir, instruction_index, size_reg, IR_AND, dst);
							node 		= ir_add_inst(engine->ir, instruction_index, size_reg, IR_OR, dst);
							ins_movzx 	= ir_add_inst(engine->ir, instruction_index, size_reg, IR_MOVZX, dst);

							if (imm_mask == NULL || ins_and == NULL || node == NULL || ins_movzx == NULL){
								log_err("unable to add node to IR");
							}
							else{
								ir_add_dependence(engine->ir, imm_mask, ins_and, IR_DEPENDENCE_TYPE_DIRECT);
								ir_add_dependence(engine->ir, ins_and, node, IR_DEPENDENCE_TYPE_DIRECT);
								ir_add_dependence(engine->ir, ins_movzx, node, IR_DEPENDENCE_TYPE_DIRECT);
								ir_add_dependence(engine->ir, engine->ir->alias_buffer[reg].ir_node, ins_and, IR_DEPENDENCE_TYPE_DIRECT);
								ir_add_dependence(engine->ir, engine->ir->alias_buffer[reg1].ir_node, ins_movzx, IR_DEPENDENCE_TYPE_DIRECT);

								engine->ir->alias_buffer[reg].ir_node = node;
								engine->ir->alias_buffer[reg].order = engine->ir->alias_buffer[reg1].order;
								alias_set_type(engine->ir->alias_buffer[reg], IRRENAMEENGINE_TYPE_READ);
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
		node = engine->ir->alias_buffer[reg].ir_node;
		if (node == NULL){
			node = ir_add_in_reg(engine->ir, instruction_index, reg, (alias_is_primer(engine->ir->alias_buffer[reg])) ? IR_IN_REG_IS_PRIMER : ~IR_IN_REG_IS_PRIMER);
			if (node != NULL){
				engine->ir->alias_buffer[reg].ir_node = node;
				engine->ir->alias_buffer[reg].order = irRenameEngine_get_reg_order(engine);
				alias_set_type(engine->ir->alias_buffer[reg], IRRENAMEENGINE_TYPE_READ);
			}
			else{
				log_err("unable to add input to IR");
			}
		}
	}

	return node;
}

void irRenameEngine_set_register_ref(struct irRenameEngine* engine, enum irRegister reg, struct node* node){
	engine->ir->alias_buffer[reg].ir_node 	= node;
	engine->ir->alias_buffer[reg].order  	= irRenameEngine_get_reg_order(engine);
	alias_set_type(engine->ir->alias_buffer[reg], IRRENAMEENGINE_TYPE_WRITE);
	switch(reg){
		case IR_REG_EAX 	: {
			engine->ir->alias_buffer[IR_REG_AX].ir_node = NULL;
			engine->ir->alias_buffer[IR_REG_AH].ir_node = NULL;
			engine->ir->alias_buffer[IR_REG_AL].ir_node = NULL;
			break;
		}
		case IR_REG_AX 		: {
			engine->ir->alias_buffer[IR_REG_AH].ir_node = NULL;
			engine->ir->alias_buffer[IR_REG_AL].ir_node = NULL;
			break;
		}
		case IR_REG_EBX 	: {
			engine->ir->alias_buffer[IR_REG_BX].ir_node = NULL;
			engine->ir->alias_buffer[IR_REG_BH].ir_node = NULL;
			engine->ir->alias_buffer[IR_REG_BL].ir_node = NULL;
			break;
		}
		case IR_REG_BX 		: {
			engine->ir->alias_buffer[IR_REG_BH].ir_node = NULL;
			engine->ir->alias_buffer[IR_REG_BL].ir_node = NULL;
			break;
		}
		case IR_REG_ECX 	: {
			engine->ir->alias_buffer[IR_REG_CX].ir_node = NULL;
			engine->ir->alias_buffer[IR_REG_CH].ir_node = NULL;
			engine->ir->alias_buffer[IR_REG_CL].ir_node = NULL;
			break;
		}
		case IR_REG_CX 		: {
			engine->ir->alias_buffer[IR_REG_CH].ir_node = NULL;
			engine->ir->alias_buffer[IR_REG_CL].ir_node = NULL;
			break;
		}
		case IR_REG_EDX 	: {
			engine->ir->alias_buffer[IR_REG_DX].ir_node = NULL;
			engine->ir->alias_buffer[IR_REG_DH].ir_node = NULL;
			engine->ir->alias_buffer[IR_REG_DL].ir_node = NULL;
			break;
		}
		case IR_REG_DX 		: {
			engine->ir->alias_buffer[IR_REG_DH].ir_node = NULL;
			engine->ir->alias_buffer[IR_REG_DL].ir_node = NULL;
			break;
		}
		case IR_REG_ESP 		: {
			engine->ir->alias_buffer[IR_REG_SP].ir_node = NULL;
			break;
		}
		case IR_REG_EBP 		: {
			engine->ir->alias_buffer[IR_REG_BP].ir_node = NULL;
			break;
		}
		case IR_REG_ESI 		: {
			engine->ir->alias_buffer[IR_REG_SI].ir_node = NULL;
			break;
		}
		case IR_REG_EDI 		: {
			engine->ir->alias_buffer[IR_REG_DI].ir_node = NULL;
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

		if (engine->ir->alias_buffer[i].ir_node != NULL  && alias_is_write(engine->ir->alias_buffer[i])){
			operation = ir_node_get_operation(engine->ir->alias_buffer[i].ir_node);
			operation->status_flag |= IR_OPERATION_STATUS_FLAG_FINAL;
		}
	}
}

void irRenameEngine_change_node(struct alias* alias_buffer, struct node* node_old, struct node* node_new){
	uint32_t i;

	for (i = 0; i < NB_IR_REGISTER; i++){
		if (alias_buffer[i].ir_node == node_old){
			alias_buffer[i].ir_node = node_new;
		}
	}

	ir_node_get_operation(node_old)->status_flag &= ~IR_OPERATION_STATUS_FLAG_FINAL;
	if (node_new != NULL){
		ir_node_get_operation(node_new)->status_flag |= IR_OPERATION_STATUS_FLAG_FINAL;
	}
}

void irRenameEngine_propagate_alias(struct irRenameEngine* engine_dst, struct alias* alias_buffer_src){
	uint32_t i;
	uint32_t order = 0;

	for (i = 0; i < NB_IR_REGISTER; i++){
		if (i == IR_REG_TMP){
			continue;
		}

		if (alias_buffer_src[i].ir_node != NULL){
			order = max(order, alias_buffer_src[i].order);

			if (alias_is_write(alias_buffer_src[i])){
				engine_dst->ir->alias_buffer[i].ir_node 	= alias_buffer_src[i].ir_node->ptr;
				engine_dst->ir->alias_buffer[i].order 		= alias_buffer_src[i].order + engine_dst->reg_op_order;
				engine_dst->ir->alias_buffer[i].type 		= alias_buffer_src[i].type;
			}
		}
	}

	engine_dst->reg_op_order += order;
}

void irRenameEngine_update_call_stack(struct irRenameEngine* engine, uint32_t* stack, uint32_t stack_ptr){
	uint32_t i;
	uint32_t j;

	for (i = 0; i <= min(IR_CALL_STACK_PTR, stack_ptr); i++){
		if (stack[i] != i){
			break;
		}
	}

	if (i < IR_CALL_STACK_PTR){
		if (engine->ir->stack_ptr < (IR_CALL_STACK_PTR - i + 1)){
			log_err("the bottom of the stack has been reached");
			return;
		}
		engine->ir->stack_ptr = engine->ir->stack_ptr - (IR_CALL_STACK_PTR - i + 1);
	}

	for (j = 0; j + i <= stack_ptr; j++){
		if (engine->ir->stack_ptr + 1 < IR_CALL_STACK_MAX_SIZE){
			engine->ir->stack_ptr = engine->ir->stack_ptr + 1;
			engine->ir->stack[engine->ir->stack_ptr] = stack[j + i] - IR_CALL_STACK_PTR + engine->func_id - 1;
		}
		else{
			log_err("the top of the stack has been reached");
			break;
		}
	}
}
