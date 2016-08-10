#include <stdlib.h>
#include <stdio.h>

#include "irBuilder.h"
#include "base.h"

#define MAX_NB_INNER_REGISTER 3

struct irRegisterBuffer{
	uint8_t 			nb_register;
	enum irRegister 	registers[MAX_NB_INNER_REGISTER];
};

static void irBuilder_get_list(const struct irBuilder* builder, enum irRegister reg, struct irRegisterBuffer* list);
static enum irRegister irBuilder_pop_list(const struct irBuilder* builder, struct irRegisterBuffer* list);

#define NB_IR_NESTED_REGISTER 24
#define irBuilder_is_register_nested(reg) ((reg) < NB_IR_NESTED_REGISTER)

static const uint8_t registerFamily[NB_IR_NESTED_REGISTER] = {
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

static const uint8_t registerIndex[NB_IR_NESTED_REGISTER] = {
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

static const uint8_t larger[NB_IRREGISTER_FAMILY][MAX_REGISTER_PER_FAMILY][MAX_REGISTER_PER_FAMILY] = {
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

static const enum irOpcode partIns[NB_IRREGISTER_FAMILY][MAX_REGISTER_PER_FAMILY][MAX_REGISTER_PER_FAMILY] = {
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

void irBuilder_init(struct irBuilder* builder){
	uint32_t i;

	memset(builder->alias_buffer, 0, sizeof(struct alias) * NB_IR_STD_REGISTER);
	for (i = 0; i < NB_IR_VIR_REGISTER; i++){
		builder->simdAlias_buffer[i].frag_size = SIMD_DEFAULT_FRAG_SIZE;
		memset(builder->simdAlias_buffer[i].fragAlias_buffer, 0, sizeof(builder->simdAlias_buffer[i].fragAlias_buffer));
	}

	for (i = 0; i <= IR_CALL_STACK_PTR; i++){
		builder->stack[i] = i;
	}

	memset(builder->stack + i, 0, IR_CALL_STACK_MAX_SIZE - (IR_CALL_STACK_PTR + 1));

	builder->stack_ptr = IR_CALL_STACK_PTR;
	builder->func_id = IR_CALL_STACK_PTR + 1;

	builder->prev_mem_access = NULL;
	builder->reg_op_order = 1;
}

static void irBuilder_get_list(const struct irBuilder* builder, enum irRegister reg, struct irRegisterBuffer* list){
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
		if (builder->alias_buffer[list->registers[i]].ir_node != NULL && builder->alias_buffer[reg].order < builder->alias_buffer[list->registers[i]].order){
			if (nb_newer != i){
				list->registers[nb_newer] = list->registers[i];
			}
			nb_newer ++;
		}
	}
	list->nb_register = nb_newer;
}

static enum irRegister irBuilder_pop_list(const struct irBuilder* builder, struct irRegisterBuffer* list){
	uint8_t 			i;
	enum irRegister 	reg;
	uint32_t 			max_ts;
	uint8_t  			nb_older;
	
	reg = list->registers[0];
	max_ts = builder->alias_buffer[list->registers[0]].order;

	for (i = 1; i < list->nb_register; i++){
		if (max_ts < builder->alias_buffer[list->registers[i]].order){
			max_ts = builder->alias_buffer[list->registers[i]].order;
			reg = list->registers[i];
		}
		else if (max_ts == builder->alias_buffer[list->registers[i]].order && (alias_is_extend(builder->alias_buffer[reg]) || alias_is_write(builder->alias_buffer[list->registers[i]]))){
			reg = list->registers[i];
		}
	}

	for (i = 0, nb_older = 0; i < list->nb_register; i++){
		if (builder->alias_buffer[list->registers[i]].order != max_ts){
			if (nb_older != i){
				list->registers[nb_older] = list->registers[i];
			}
			nb_older ++;
		}
	}
	list->nb_register = nb_older;

	return reg;
}

struct node* irBuilder_get_std_register_ref(struct irBuilder* builder, struct ir* ir, enum irRegister reg, uint32_t instruction_index){
	struct irRegisterBuffer list;
	struct node* 			node = NULL;
	enum irRegister 		reg1;
	enum irRegister 		reg2;
	uint8_t 				family;
	enum irOpcode 			new_ins;

	if (irBuilder_is_register_nested(reg)){
		irBuilder_get_list(builder, reg, &list);
	}
	else{
		list.nb_register = 0;
	}

	if (list.nb_register){
		family = registerFamily[reg];
		reg1 = irBuilder_pop_list(builder, &list);

		#ifdef EXTRA_CHECK
		if (family != registerFamily[reg1]){
			log_err_m("register is in dependence list but not in the same family (%s - %s)", irRegister_2_string(reg1), irRegister_2_string(reg));
			return NULL;
		}
		#endif

		if (larger[family][registerIndex[reg1]][registerIndex[reg]]){
			new_ins = partIns[family][registerIndex[reg1]][registerIndex[reg]];

			if (new_ins != IR_INVALID){
				builder->alias_buffer[reg].ir_node = ir_add_inst(ir, instruction_index, irRegister_get_size(reg), new_ins, irBuilder_get_call_id(builder));
				builder->alias_buffer[reg].order = builder->alias_buffer[reg1].order;
				alias_set_type(builder->alias_buffer[reg], IRBUILDER_TYPE_EXTEND);
				if (builder->alias_buffer[reg].ir_node == NULL){
					log_err("unable to add operation to IR");
				}
				else{
					ir_add_dependence_check(ir, builder->alias_buffer[reg1].ir_node, builder->alias_buffer[reg].ir_node, IR_DEPENDENCE_TYPE_DIRECT);
					node = builder->alias_buffer[reg].ir_node;
				}
			}
			else{
				log_err("unable to choose correct instruction, INVALID");
			}
		}
		else if (list.nb_register){
			reg2 = irBuilder_pop_list(builder, &list);

			#ifdef EXTRA_CHECK
			if (family != registerFamily[reg2]){
				log_err_m("register is in dependence list but not in the same family (%s - %s)", irRegister_2_string(reg2), irRegister_2_string(reg));
				return NULL;
			}
			#endif

			if (list.nb_register){
				log_err("this case is not implemented yet, more than two elements in the dependence list");
			}
			else if (alias_is_read(builder->alias_buffer[reg1]) && alias_is_read(builder->alias_buffer[reg2])){
				builder->alias_buffer[reg].ir_node = ir_add_in_reg(ir, min(ir_node_get_operation(builder->alias_buffer[reg1].ir_node)->index, ir_node_get_operation(builder->alias_buffer[reg2].ir_node)->index), reg, (alias_is_primer(builder->alias_buffer[reg])) ? IR_IN_REG_IS_PRIMER : ~IR_IN_REG_IS_PRIMER);
				builder->alias_buffer[reg].order = builder->alias_buffer[reg1].order;
				alias_set_type(builder->alias_buffer[reg], IRBUILDER_TYPE_READ);

				if (builder->alias_buffer[reg].ir_node != NULL){
					alias_set_type(builder->alias_buffer[reg1], IRBUILDER_TYPE_EXTEND);

					ir_convert_operation_to_inst(builder->alias_buffer[reg1].ir_node, partIns[family][registerIndex[reg]][registerIndex[reg1]]);
					ir_add_dependence_check(ir, builder->alias_buffer[reg].ir_node, builder->alias_buffer[reg1].ir_node, IR_DEPENDENCE_TYPE_DIRECT)

					alias_set_type(builder->alias_buffer[reg2], IRBUILDER_TYPE_EXTEND);
					builder->alias_buffer[reg2].order = builder->alias_buffer[reg1].order;

					ir_convert_operation_to_inst(builder->alias_buffer[reg2].ir_node, partIns[family][registerIndex[reg]][registerIndex[reg2]]);
					ir_add_dependence_check(ir, builder->alias_buffer[reg].ir_node, builder->alias_buffer[reg2].ir_node, IR_DEPENDENCE_TYPE_DIRECT)

					node = builder->alias_buffer[reg].ir_node;
				}
				else{
					log_err("unable to add input to IR");
				}
			}
			else{
				log_err_m("this case is not implemented yet, %s and %s in the dependence list of %s", irRegister_2_string(reg1), irRegister_2_string(reg2), irRegister_2_string(reg));
			}
		}
		else if (alias_is_read(builder->alias_buffer[reg1])){
			new_ins = partIns[family][registerIndex[reg]][registerIndex[reg1]];

			if (new_ins != IR_INVALID){
				builder->alias_buffer[reg].ir_node = ir_add_in_reg(ir, ir_node_get_operation(builder->alias_buffer[reg1].ir_node)->index, reg, (alias_is_primer(builder->alias_buffer[reg])) ? IR_IN_REG_IS_PRIMER : ~IR_IN_REG_IS_PRIMER);
				builder->alias_buffer[reg].order = builder->alias_buffer[reg1].order;
				alias_set_type(builder->alias_buffer[reg], IRBUILDER_TYPE_READ);

				if (builder->alias_buffer[reg].ir_node != NULL){
					alias_set_type(builder->alias_buffer[reg1], IRBUILDER_TYPE_EXTEND);

					ir_convert_operation_to_inst(builder->alias_buffer[reg1].ir_node, new_ins);
					ir_add_dependence_check(ir, builder->alias_buffer[reg].ir_node, builder->alias_buffer[reg1].ir_node, IR_DEPENDENCE_TYPE_DIRECT);

					node = builder->alias_buffer[reg].ir_node;
				}
				else{
					log_err("unable to add input to IR");
				}
			}
			else{
				log_err("unable to choose correct instruction, INVALID");
			}
		}
		else if (builder->alias_buffer[reg].ir_node != NULL){
			uint32_t 		size_reg1;
			uint32_t 		size_reg;
			struct node* 	imm_mask;
			struct node* 	ins_and;
			struct node* 	ins_movzx;

			size_reg 	= irRegister_get_size(reg);
			size_reg1 	= irRegister_get_size(reg1);

			imm_mask 	= ir_add_immediate(ir, size_reg, (0xffffffffffffffff >> (64 - size_reg)) << size_reg1);
			ins_and 	= ir_add_inst(ir, instruction_index, size_reg, IR_AND, irBuilder_get_call_id(builder));
			node 		= ir_add_inst(ir, instruction_index, size_reg, IR_OR, irBuilder_get_call_id(builder));
			ins_movzx 	= ir_add_inst(ir, instruction_index, size_reg, IR_MOVZX, irBuilder_get_call_id(builder));

			if (imm_mask == NULL || ins_and == NULL || node == NULL || ins_movzx == NULL){
				log_err("unable to add node to IR");
			}
			else{
				ir_add_dependence_check(ir, imm_mask, ins_and, IR_DEPENDENCE_TYPE_DIRECT);
				ir_add_dependence_check(ir, ins_and, node, IR_DEPENDENCE_TYPE_DIRECT);
				ir_add_dependence_check(ir, ins_movzx, node, IR_DEPENDENCE_TYPE_DIRECT);
				ir_add_dependence_check(ir, builder->alias_buffer[reg].ir_node, ins_and, IR_DEPENDENCE_TYPE_DIRECT);
				ir_add_dependence_check(ir, builder->alias_buffer[reg1].ir_node, ins_movzx, IR_DEPENDENCE_TYPE_DIRECT);

				builder->alias_buffer[reg].ir_node = node;
				builder->alias_buffer[reg].order = builder->alias_buffer[reg1].order;
				alias_set_type(builder->alias_buffer[reg], IRBUILDER_TYPE_READ);
			}
		}
		else{
			log_err_m("this case is not implemented yet, smaller write dependence (%s - %s = NULL)", irRegister_2_string(reg1), irRegister_2_string(reg));
		}
	}
	else{
		node = builder->alias_buffer[reg].ir_node;
		if (node == NULL){
			node = ir_add_in_reg(ir, instruction_index, reg, (alias_is_primer(builder->alias_buffer[reg])) ? IR_IN_REG_IS_PRIMER : ~IR_IN_REG_IS_PRIMER);
			if (node != NULL){
				builder->alias_buffer[reg].ir_node = node;
				builder->alias_buffer[reg].order = irBuilder_get_reg_order(builder);
				alias_set_type(builder->alias_buffer[reg], IRBUILDER_TYPE_READ);
			}
			else{
				log_err("unable to add input to IR");
			}
		}
	}

	return node;
}

struct node* irBuilder_get_simd_register_ref(struct irBuilder* builder, struct ir* ir, enum irRegister reg, uint32_t instruction_index){
	enum irVirtualRegister 	vreg;
	uint32_t 				frag;
	struct node* 			node;

	vreg = irRegister_simd_get_virtual(reg);
	frag = irRegister_simd_get_frag(reg);

	if (builder->simdAlias_buffer[vreg - IR_VIR_REGISTER_OFFSET].frag_size != irRegister_simd_get_size(reg)){
		if (irBuilder_change_simd_frag(builder, ir, vreg, irRegister_simd_get_size(reg), instruction_index)){
			log_err_m("unable to change SIMD frag from %u to %u", builder->simdAlias_buffer[vreg - IR_VIR_REGISTER_OFFSET].frag_size, irRegister_simd_get_size(reg));
			return NULL;
		}
	}

	node = builder->simdAlias_buffer[vreg - IR_VIR_REGISTER_OFFSET].fragAlias_buffer[frag].ir_node;
	if (node == NULL){
		node = ir_add_in_reg(ir, instruction_index, reg, (alias_is_primer(builder->simdAlias_buffer[vreg - IR_VIR_REGISTER_OFFSET].fragAlias_buffer[frag])) ? IR_IN_REG_IS_PRIMER : ~IR_IN_REG_IS_PRIMER);
		if (node != NULL){
			builder->simdAlias_buffer[vreg - IR_VIR_REGISTER_OFFSET].fragAlias_buffer[frag].ir_node = node;
			alias_set_type(builder->simdAlias_buffer[vreg - IR_VIR_REGISTER_OFFSET].fragAlias_buffer[frag], IRBUILDER_TYPE_READ);
		}
		else{
			log_err("unable to add input to IR");
		}
	}

	return node;
}

static const uint32_t irVirtualRegister_size[NB_IR_VIR_REGISTER] = {
	64 , /* IR_VREG_MMX1 */
	64 , /* IR_VREG_MMX2 */
	64 , /* IR_VREG_MMX3 */
	64 , /* IR_VREG_MMX4 */
	64 , /* IR_VREG_MMX5 */
	64 , /* IR_VREG_MMX6 */
	64 , /* IR_VREG_MMX7 */
	64 , /* IR_VREG_MMX8 */
	128, /* IR_VREG_XMM1 */
	128, /* IR_VREG_XMM2 */
	128, /* IR_VREG_XMM3 */
	128, /* IR_VREG_XMM4 */
	128, /* IR_VREG_XMM5 */
	128, /* IR_VREG_XMM6 */
	128, /* IR_VREG_XMM7 */
	128, /* IR_VREG_XMM8 */
	128, /* IR_VREG_YMM1 */
	128, /* IR_VREG_YMM2 */
	128, /* IR_VREG_YMM3 */
	128, /* IR_VREG_YMM4 */
	128, /* IR_VREG_YMM5 */
	128, /* IR_VREG_YMM6 */
	128, /* IR_VREG_YMM7 */
	128  /* IR_VREG_YMM8 */
};

int32_t irBuilder_change_simd_frag(struct irBuilder* builder, struct ir* ir, enum irVirtualRegister vreg, uint8_t frag_size, uint32_t instruction_index){
	uint32_t 			i;
	uint32_t 			j;
	uint32_t 			old_frag_size;
	struct simdAlias* 	alias; 

	alias = builder->simdAlias_buffer + (vreg - IR_VIR_REGISTER_OFFSET);
	old_frag_size = alias->frag_size;

	if (old_frag_size > frag_size){
		struct node* new_node_buffer[IR_VIR_REGISTER_NB_FRAG_MAX] = {NULL};

		if (old_frag_size == 32 && frag_size == 16){
			for (i = 0; i < irVirtualRegister_size[vreg - IR_VIR_REGISTER_OFFSET] >> 5; i++){
				struct node* shr;
				struct node* dis;

				if (alias->fragAlias_buffer[i].ir_node == NULL){
					continue;
				}

				new_node_buffer[2*i + 0] = ir_add_inst(ir, instruction_index, 16, IR_PART1_16, irBuilder_get_call_id(builder));
				new_node_buffer[2*i + 1] = ir_add_inst(ir, instruction_index, 16, IR_PART1_16, irBuilder_get_call_id(builder));
				shr = ir_add_inst(ir, instruction_index, 32, IR_SHR, irBuilder_get_call_id(builder));
				dis = ir_add_immediate(ir, 32, 16);

				if (new_node_buffer[2*i + 0] != NULL){
					if (ir_add_dependence(ir, alias->fragAlias_buffer[i].ir_node, new_node_buffer[2*i + 0], IR_DEPENDENCE_TYPE_DIRECT) == NULL){
						log_err("unable to add dependence to IR");
					}
				}
				else{
					log_err("unable to add operation to IR");
				}

				if (shr != NULL){
					ir_add_dependence(ir, alias->fragAlias_buffer[i].ir_node, shr, IR_DEPENDENCE_TYPE_DIRECT);
					if (dis != NULL){
						if (ir_add_dependence(ir, dis, shr, IR_DEPENDENCE_TYPE_SHIFT_DISP) == NULL){
							log_err("unable to add dependence to IR");
						}
					}
					else{
						log_err("unable to add operation to IR");
					}

					if (new_node_buffer[2*i + 1] != NULL){
						if (ir_add_dependence(ir, shr, new_node_buffer[2*i + 1], IR_DEPENDENCE_TYPE_DIRECT) == NULL){
							log_err("unable to add dependence to IR");
						}
					}
					else{
						log_err("unable to add operation to IR");
					}
				}
				else{
					log_err("unable to add operation to IR");
				}
			}
		}
		else if (old_frag_size == 32 && frag_size == 8){
			for (i = 0; i < irVirtualRegister_size[vreg - IR_VIR_REGISTER_OFFSET] >> 5; i++){
				struct node* shr;
				struct node* dis;

				if (alias->fragAlias_buffer[i].ir_node == NULL){
					continue;
				}

				new_node_buffer[4*i + 0] = ir_add_inst(ir, instruction_index, 8, IR_PART1_8, irBuilder_get_call_id(builder));
				new_node_buffer[4*i + 1] = ir_add_inst(ir, instruction_index, 8, IR_PART2_8, irBuilder_get_call_id(builder));
				new_node_buffer[4*i + 2] = ir_add_inst(ir, instruction_index, 8, IR_PART1_8, irBuilder_get_call_id(builder));
				new_node_buffer[4*i + 3] = ir_add_inst(ir, instruction_index, 8, IR_PART2_8, irBuilder_get_call_id(builder));
				shr = ir_add_inst(ir, instruction_index, 32, IR_SHR, irBuilder_get_call_id(builder));
				dis = ir_add_immediate(ir, 32, 16);

				if (new_node_buffer[4*i + 0] != NULL){
					if (ir_add_dependence(ir, alias->fragAlias_buffer[i].ir_node, new_node_buffer[4*i + 0], IR_DEPENDENCE_TYPE_DIRECT) == NULL){
						log_err("unable to add dependence to IR");
					}
				}
				else{
					log_err("unable to add operation to IR");
				}
				if (new_node_buffer[4*i + 1] != NULL){
					if (ir_add_dependence(ir, alias->fragAlias_buffer[i].ir_node, new_node_buffer[4*i + 1], IR_DEPENDENCE_TYPE_DIRECT) == NULL){
						log_err("unable to add dependence to IR");
					}
				}
				else{
					log_err("unable to add operation to IR");
				}

				if (shr != NULL){
					ir_add_dependence(ir, alias->fragAlias_buffer[i].ir_node, shr, IR_DEPENDENCE_TYPE_DIRECT);
					if (dis != NULL){
						if (ir_add_dependence(ir, dis, shr, IR_DEPENDENCE_TYPE_SHIFT_DISP) == NULL){
							log_err("unable to add dependence to IR");
						}
					}
					else{
						log_err("unable to add operation to IR");
					}

					if (new_node_buffer[4*i + 2] != NULL){
						if (ir_add_dependence(ir, shr, new_node_buffer[4*i + 2], IR_DEPENDENCE_TYPE_DIRECT) == NULL){
							log_err("unable to add dependence to IR");
						}
					}
					else{
						log_err("unable to add operation to IR");
					}
					if (new_node_buffer[4*i + 3] != NULL){
						if (ir_add_dependence(ir, shr, new_node_buffer[4*i + 3], IR_DEPENDENCE_TYPE_DIRECT) == NULL){
							log_err("unable to add dependence to IR");
						}
					}
					else{
						log_err("unable to add operation to IR");
					}
				}
				else{
					log_err("unable to add operation to IR");
				}
			}
		}
		else if (old_frag_size == 16 && frag_size == 8){
			for (i = 0; i < irVirtualRegister_size[vreg - IR_VIR_REGISTER_OFFSET] >> 4; i++){
				if (alias->fragAlias_buffer[i].ir_node == NULL){
					continue;
				}

				new_node_buffer[2*i + 0] = ir_add_inst(ir, instruction_index, frag_size, IR_PART1_8, irBuilder_get_call_id(builder));
				new_node_buffer[2*i + 1] = ir_add_inst(ir, instruction_index, frag_size, IR_PART2_8, irBuilder_get_call_id(builder));

				if (new_node_buffer[2*i + 0] != NULL){
					if (ir_add_dependence(ir, alias->fragAlias_buffer[i].ir_node, new_node_buffer[2*i + 0], IR_DEPENDENCE_TYPE_DIRECT) == NULL){
						log_err("unable to add dependence to IR");
					}
				}
				else{
					log_err("unable to add operation to IR");
				}

				if (new_node_buffer[2*i + 1] != NULL){
					if (ir_add_dependence(ir, alias->fragAlias_buffer[i].ir_node, new_node_buffer[2*i + 1], IR_DEPENDENCE_TYPE_DIRECT) == NULL){
						log_err("unable to add dependence to IR");
					}
				}
				else{
					log_err("unable to add operation to IR");
				}
			}
		}
		#ifdef EXTRA_CHECK
		else{
			log_err_m("this case %u -> %u  is not handled yet", old_frag_size, frag_size);
			return -1;
		}
		#endif

		for (i = irVirtualRegister_size[vreg - IR_VIR_REGISTER_OFFSET] / frag_size; i; i--){
			alias->fragAlias_buffer[i - 1].ir_node = new_node_buffer[i - 1];
			alias->fragAlias_buffer[i - 1].type = alias->fragAlias_buffer[i / (old_frag_size / frag_size) - 1].type;
		}
	}
	else if (old_frag_size < frag_size){
		for (i = 0; i < irVirtualRegister_size[vreg - IR_VIR_REGISTER_OFFSET] / frag_size; i++){
			uint32_t nb_frag;
			uint32_t frag;

			for (j = 0, nb_frag = 0; j < (frag_size / old_frag_size); j++){
				frag = i * (frag_size / old_frag_size) + j;
				if (alias->fragAlias_buffer[frag].ir_node != NULL){
					nb_frag ++;
				}
			}
			if (nb_frag){
				for (j = 0; j < (frag_size / old_frag_size); j++){
					frag = i * (frag_size / old_frag_size) + j;
					if (alias->fragAlias_buffer[frag].ir_node == NULL){
						struct node* reg;

						reg = ir_add_in_reg(ir, instruction_index, irRegister_virtual_get_simd(vreg, old_frag_size, frag), (alias_is_primer(alias->fragAlias_buffer[frag])) ? IR_IN_REG_IS_PRIMER : ~IR_IN_REG_IS_PRIMER);
						if (reg != NULL){
							alias->fragAlias_buffer[frag].ir_node = reg;
							alias_set_type(alias->fragAlias_buffer[frag], IRBUILDER_TYPE_READ);
						}
						else{
							log_err("unable to add input to IR");
						}
					}
				}
			}
		}

		for (i = 0; i < irVirtualRegister_size[vreg - IR_VIR_REGISTER_OFFSET] / old_frag_size; i++){
			if (alias->fragAlias_buffer[i].ir_node != NULL){
				struct node* movzx;

				movzx = ir_add_inst(ir, instruction_index, frag_size, IR_MOVZX, irBuilder_get_call_id(builder));
				if (movzx != NULL){
					if (ir_add_dependence(ir, alias->fragAlias_buffer[i].ir_node, movzx, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
						log_err("unable to add dependence to IR");
					}
					alias->fragAlias_buffer[i].ir_node = movzx;
				}
				else{
					log_err("unable to add operation to IR");
				}

				if (i % (frag_size / old_frag_size)){
					struct node* shl;
					struct node* dis;

					shl = ir_add_inst(ir, instruction_index, frag_size, IR_SHL, irBuilder_get_call_id(builder));
					dis = ir_add_immediate(ir, frag_size, (i % (frag_size / old_frag_size)) * old_frag_size);
					if (shl != NULL){
						if (dis != NULL){
							if (ir_add_dependence(ir, dis, shl, IR_DEPENDENCE_TYPE_SHIFT_DISP) == NULL){
								log_err("unable to add dependence to IR");
							}
						}
						else{
							log_err("unable to add operation to IR");
						}
						if (ir_add_dependence(ir, movzx, shl, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
							log_err("unable to add dependence to IR");
						}
						alias->fragAlias_buffer[i].ir_node = shl;
					}
					else{
						log_err("unable to add operation to IR");
					}
				}
			}
		}

		for (i = 0; i < irVirtualRegister_size[vreg - IR_VIR_REGISTER_OFFSET] / frag_size; i++){
			struct node* 	or;
			uint32_t 		frag;

			or = ir_add_inst(ir, instruction_index, frag_size, IR_OR, irBuilder_get_call_id(builder));
			if (or != NULL){
				frag = i * (frag_size / old_frag_size);
				if (ir_add_dependence(ir, alias->fragAlias_buffer[frag].ir_node, or, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
					log_err("unable to add dependence to IR");
				}
				alias->fragAlias_buffer[i].ir_node = or;

				for (j = 1; j < (frag_size / old_frag_size); j++){
					frag = i * (frag_size / old_frag_size) + j;
					if (alias->fragAlias_buffer[frag].ir_node != NULL){
						if (ir_add_dependence(ir, alias->fragAlias_buffer[frag].ir_node, or, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
							log_err("unable to add dependence to IR");
						}
					}
				}
			}
			else{
				log_err("unable to add operation to IR");
			}
		}
	}

	alias->frag_size = frag_size;

	return 0;
}

void irBuilder_set_std_register_ref(struct irBuilder* builder, enum irRegister reg, struct node* node){
	builder->alias_buffer[reg].ir_node 	= node;
	builder->alias_buffer[reg].order  	= irBuilder_get_reg_order(builder);
	alias_set_type(builder->alias_buffer[reg], IRBUILDER_TYPE_WRITE);
	switch(reg){
		case IR_REG_EAX 	: {
			builder->alias_buffer[IR_REG_AX].ir_node = NULL;
			builder->alias_buffer[IR_REG_AH].ir_node = NULL;
			builder->alias_buffer[IR_REG_AL].ir_node = NULL;
			break;
		}
		case IR_REG_AX 		: {
			builder->alias_buffer[IR_REG_AH].ir_node = NULL;
			builder->alias_buffer[IR_REG_AL].ir_node = NULL;
			break;
		}
		case IR_REG_EBX 	: {
			builder->alias_buffer[IR_REG_BX].ir_node = NULL;
			builder->alias_buffer[IR_REG_BH].ir_node = NULL;
			builder->alias_buffer[IR_REG_BL].ir_node = NULL;
			break;
		}
		case IR_REG_BX 		: {
			builder->alias_buffer[IR_REG_BH].ir_node = NULL;
			builder->alias_buffer[IR_REG_BL].ir_node = NULL;
			break;
		}
		case IR_REG_ECX 	: {
			builder->alias_buffer[IR_REG_CX].ir_node = NULL;
			builder->alias_buffer[IR_REG_CH].ir_node = NULL;
			builder->alias_buffer[IR_REG_CL].ir_node = NULL;
			break;
		}
		case IR_REG_CX 		: {
			builder->alias_buffer[IR_REG_CH].ir_node = NULL;
			builder->alias_buffer[IR_REG_CL].ir_node = NULL;
			break;
		}
		case IR_REG_EDX 	: {
			builder->alias_buffer[IR_REG_DX].ir_node = NULL;
			builder->alias_buffer[IR_REG_DH].ir_node = NULL;
			builder->alias_buffer[IR_REG_DL].ir_node = NULL;
			break;
		}
		case IR_REG_DX 		: {
			builder->alias_buffer[IR_REG_DH].ir_node = NULL;
			builder->alias_buffer[IR_REG_DL].ir_node = NULL;
			break;
		}
		case IR_REG_ESP 		: {
			builder->alias_buffer[IR_REG_SP].ir_node = NULL;
			break;
		}
		case IR_REG_EBP 		: {
			builder->alias_buffer[IR_REG_BP].ir_node = NULL;
			break;
		}
		case IR_REG_ESI 		: {
			builder->alias_buffer[IR_REG_SI].ir_node = NULL;
			break;
		}
		case IR_REG_EDI 		: {
			builder->alias_buffer[IR_REG_DI].ir_node = NULL;
			break;
		}
		default 			: {
			break;
		}
	}
}

void irBuilder_set_simd_register_ref(struct irBuilder* builder, enum irRegister reg, struct node* node){
	enum irVirtualRegister 	vreg;
	uint32_t 				frag;

	vreg = irRegister_simd_get_virtual(reg);
	frag = irRegister_simd_get_frag(reg);

	builder->simdAlias_buffer[vreg - IR_VIR_REGISTER_OFFSET].fragAlias_buffer[frag].ir_node = node;
	alias_set_type(builder->simdAlias_buffer[vreg - IR_VIR_REGISTER_OFFSET].fragAlias_buffer[frag], IRBUILDER_TYPE_WRITE);

	builder->simdAlias_buffer[vreg - IR_VIR_REGISTER_OFFSET].frag_size = irRegister_get_size(reg);
}

void irBuilder_tag_final_node(struct irBuilder* builder){
	uint32_t 			i;
	uint32_t 			j;
	struct irOperation* operation;

	for (i = 0; i < NB_IR_STD_REGISTER; i++){
		if (i == IR_REG_TMP){
			continue;
		}

		if (builder->alias_buffer[i].ir_node != NULL  && alias_is_write(builder->alias_buffer[i])){
			operation = ir_node_get_operation(builder->alias_buffer[i].ir_node);
			operation->status_flag |= IR_OPERATION_STATUS_FLAG_FINAL;

			#ifdef EXTRA_CHECK
			if (operation->type == IR_OPERATION_TYPE_OUT_MEM){
				log_err("wrong operation type");
			}
			#endif
		}
	}

	for (i = 0; i < NB_IR_VIR_REGISTER; i++){
		for (j = 0; j < irVirtualRegister_size[i] / builder->simdAlias_buffer[i].frag_size; j++){
			if (builder->simdAlias_buffer[i].fragAlias_buffer[j].ir_node != NULL  && alias_is_write(builder->simdAlias_buffer[i].fragAlias_buffer[j])){
				operation = ir_node_get_operation(builder->simdAlias_buffer[i].fragAlias_buffer[j].ir_node);
				operation->status_flag |= IR_OPERATION_STATUS_FLAG_FINAL;

				#ifdef EXTRA_CHECK
				if (operation->type == IR_OPERATION_TYPE_OUT_MEM){
					log_err("wrong operation type");
				}
				#endif
			}
		}
	}
}

void irBuilder_chg_final_node(struct irBuilder* builder, struct node* node_old, struct node* node_new){
	uint32_t i;
	uint32_t j;
	#ifdef EXTRA_CHECK
	uint32_t found = 0;
	#endif

	#ifdef EXTRA_CHECK
	if (!(ir_node_get_operation(node_old)->status_flag & IR_OPERATION_STATUS_FLAG_FINAL)){
		log_err_m("node %p is not a final node", (void*)node_old);
	}
	#endif

	for (i = 0; i < NB_IR_STD_REGISTER; i++){
		if (builder->alias_buffer[i].ir_node == node_old && i != IR_REG_TMP){
			builder->alias_buffer[i].ir_node = node_new;
			#ifdef EXTRA_CHECK
			found = 1;
			#endif
		}
	}

	for (i = 0; i < NB_IR_VIR_REGISTER; i++){
		for (j = 0; j < irVirtualRegister_size[i] / builder->simdAlias_buffer[i].frag_size; j++){
			if (builder->simdAlias_buffer[i].fragAlias_buffer[j].ir_node == node_old){
				builder->simdAlias_buffer[i].fragAlias_buffer[j].ir_node = node_new;
				#ifdef EXTRA_CHECK
				found = 1;
				#endif
			}
		}
	}

	#ifdef EXTRA_CHECK
	if (!found){
		log_err_m("node %p is not in the alias buffer", (void*)node_old);
	}
	#endif

	ir_node_get_operation(node_old)->status_flag &= ~IR_OPERATION_STATUS_FLAG_FINAL;
	if (node_new != NULL){
		ir_node_get_operation(node_new)->status_flag |= IR_OPERATION_STATUS_FLAG_FINAL;
	}
}

void irBuilder_propagate_alias(struct irBuilder* builder_dst, struct ir* ir_dst, const struct irBuilder* builder_src){
	uint32_t i;
	uint32_t j;

	for (i = 0; i < NB_IR_STD_REGISTER; i++){
		if (i == IR_REG_TMP){
			continue;
		}

		if (builder_src->alias_buffer[i].ir_node != NULL && alias_is_write(builder_src->alias_buffer[i])){
			builder_dst->alias_buffer[i].ir_node 	= builder_src->alias_buffer[i].ir_node->ptr;
			builder_dst->alias_buffer[i].order 		= builder_src->alias_buffer[i].order + builder_dst->reg_op_order;
			builder_dst->alias_buffer[i].type 		= builder_src->alias_buffer[i].type;
		}
	}

	builder_dst->reg_op_order += builder_src->reg_op_order;

	for (i = 0; i < NB_IR_VIR_REGISTER; i++){
		if (builder_dst->simdAlias_buffer[i].frag_size != builder_src->simdAlias_buffer[i].frag_size){
			if (irBuilder_change_simd_frag(builder_dst, ir_dst, IR_VIR_REGISTER_OFFSET + i, builder_src->simdAlias_buffer[i].frag_size, IR_OPERATION_INDEX_UNKOWN)){
				log_err_m("unable to change SIMD frag from %u to %u", builder_dst->simdAlias_buffer[i].frag_size, builder_src->simdAlias_buffer[i].frag_size);
				continue;
			}
		}

		for (j = 0; j < irVirtualRegister_size[i] / builder_src->simdAlias_buffer[i].frag_size; j++){
			if (builder_src->simdAlias_buffer[i].fragAlias_buffer[j].ir_node != NULL  && alias_is_write(builder_src->simdAlias_buffer[i].fragAlias_buffer[j])){
				builder_dst->simdAlias_buffer[i].fragAlias_buffer[j].ir_node 	= builder_src->simdAlias_buffer[i].fragAlias_buffer[j].ir_node->ptr;
				builder_dst->simdAlias_buffer[i].fragAlias_buffer[j].type 		= builder_src->simdAlias_buffer[i].fragAlias_buffer[j].type;
			}
		}
	}
}

void irBuilder_update_call_stack(struct irBuilder* dst_builder, const struct irBuilder* src_builder){
	uint32_t i;
	uint32_t j;

	for (i = 0; i <= min(IR_CALL_STACK_PTR, src_builder->stack_ptr); i++){
		if (src_builder->stack[i] != i){
			break;
		}
	}

	if (i <= IR_CALL_STACK_PTR){
		if (dst_builder->stack_ptr < (IR_CALL_STACK_PTR - i + 1)){
			log_err("the bottom of the stack has been reached");
			return;
		}
		dst_builder->stack_ptr = dst_builder->stack_ptr - (IR_CALL_STACK_PTR - i + 1);
	}

	for (j = 0; j + i <= src_builder->stack_ptr; j++){
		if (dst_builder->stack_ptr + 1 < IR_CALL_STACK_MAX_SIZE){
			dst_builder->stack_ptr = dst_builder->stack_ptr + 1;
			dst_builder->stack[dst_builder->stack_ptr] = src_builder->stack[j + i] - IR_CALL_STACK_PTR + dst_builder->func_id - 1;
		}
		else{
			log_err("the top of the stack has been reached");
			break;
		}
	}

	dst_builder->func_id += src_builder->func_id - (IR_CALL_STACK_PTR + 1);
}
