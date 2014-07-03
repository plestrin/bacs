#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>

#include "irRenameEngine.h"
#include "irImporterDynTrace.h"

/* One can use a better allocator. Hint: it only grows - no free before the end */
#define alias_allocate(engine) 			((struct alias*)malloc(sizeof(struct alias)))
#define alias_free(engine, alias) 		free(alias)

int32_t compare_alias_to_alias(const void* alias1, const void* alias2);

void irRenameEngine_free_memory_node(void* data){
	struct alias* alias = (struct alias*)data;
	if (alias->alias_type.mem.next != NULL){
		irRenameEngine_free_memory_node(alias->alias_type.mem.next);
	}
	alias_free(alias->alias_type.mem.engine, alias);
}

static void irRenameEngine_reset_reg_variable(struct ir* ir, struct alias* alias, struct node* ir_node);
static void irRenameEngine_reset_mem_variable(struct irRenameEngine* engine, struct alias* alias, struct operand* operand, struct node* ir_node);

static void irImporterDynTrace_part_reg_variable(struct ir* ir, struct alias* alias_src, struct alias* alias_dst, struct operand* operand, uint8_t alias_dst_size, enum irOpcode part_op);
static void irImporterDynTrace_part_mem_variable(struct irRenameEngine* engine, struct alias* alias_src, struct node** ir_node, struct operand* operand, enum irOpcode part_op);
static void irImporterDynTrace_merge_reg_variable(struct ir* ir, struct alias* alias_src, struct alias* alias_dst, struct operand* operand, uint8_t alias_src_size, enum irOpcode part_op);
static void irImporterDynTrace_merge_mem_variable(struct irRenameEngine* engine, struct alias* alias_dst, struct operand* operand, enum irOpcode part_op);


static void irRenameEngine_reset_reg_variable(struct ir* ir, struct alias* alias, struct node* ir_node){
	if (alias->alias_type.reg.ir_node != NULL){
		ir_node_get_operation(alias->alias_type.reg.ir_node)->data --;
		if (ir_node_get_operation(alias->alias_type.reg.ir_node)->data == 0){
			ir_convert_output_to_inner(ir, alias->alias_type.reg.ir_node);
		}
	}
	alias->type 					= ALIAS_REG_WRITE;
	alias->alias_type.reg.ir_node 	= ir_node;
}

static void irRenameEngine_reset_mem_variable(struct irRenameEngine* engine, struct alias* alias, struct operand* operand, struct node* ir_node){
	if (alias->alias_type.mem.next != NULL){
		irRenameEngine_reset_mem_variable(engine, alias->alias_type.mem.next, NULL, NULL);
	}

	ir_node_get_operation(alias->alias_type.mem.ir_node)->data --;
	if (ir_node_get_operation(alias->alias_type.mem.ir_node)->data == 0){
		ir_convert_output_to_inner(engine->ir, alias->alias_type.mem.ir_node);
	}

	if (ir_node != NULL){
		alias->type 					= ALIAS_MEM_WRITE;
		alias->alias_type.mem.ir_node 	= ir_node;
		alias->alias_type.mem.address 	= operand->location.address;
		alias->alias_type.mem.size 		= operand->size;
		alias->alias_type.mem.next 		= NULL;
	}
	else{
		alias_free(engine, alias);
	}
}

static void irImporterDynTrace_part_reg_variable(struct ir* ir, struct alias* alias_src, struct alias* alias_dst, struct operand* operand, uint8_t alias_dst_size, enum irOpcode part_op){
	alias_dst->type 					= ALIAS_REG_READ;
	alias_dst->alias_type.reg.ir_node 	= irImporterDynTrace_add_operation(ir, part_op, operand, alias_dst_size);
	if (alias_dst->alias_type.reg.ir_node == NULL){
		printf("ERROR: in %s, unable to add operation to IR\n", __func__);
	}
	else{
		ir_node_get_operation(alias_dst->alias_type.reg.ir_node)->data = 1;
		irImporterDynTrace_add_dependence(ir, alias_src->alias_type.reg.ir_node, alias_dst->alias_type.reg.ir_node, IR_DEPENDENCE_TYPE_DIRECT);
	}
}

static void irImporterDynTrace_part_mem_variable(struct irRenameEngine* engine, struct alias* alias_src, struct node** ir_node, struct operand* operand, enum irOpcode part_op){
	struct alias*	alias_new;
	struct alias* 	alias_cursor;

	alias_new = alias_allocate(engine);
	if (alias_new == NULL){
		printf("ERROR: in %s, unable to aloocate alias\n", __func__);
		return;
	}

	*ir_node = irImporterDynTrace_add_operation(engine->ir, part_op, operand, operand->size * 8);
	if (*ir_node == NULL){
		printf("ERROR: in %s, unable to add input to IR\n", __func__);
		alias_free(engine, alias_new);
		return;
	}
	ir_node_get_operation(*ir_node)->data = 1;
	irImporterDynTrace_add_dependence(engine->ir, alias_src->alias_type.mem.ir_node, *ir_node, IR_DEPENDENCE_TYPE_DIRECT);

	alias_new->type 					= ALIAS_MEM_READ;
	alias_new->alias_type.mem.ir_node 	= *ir_node;
	alias_new->alias_type.mem.address 	= operand->location.address;
	alias_new->alias_type.mem.size 		= operand->size;
	alias_new->alias_type.mem.engine 	= engine;

	alias_cursor = alias_src;
	while(alias_cursor->alias_type.mem.next != NULL && alias_cursor->alias_type.mem.next->alias_type.mem.size > operand->size){
		alias_cursor = alias_cursor->alias_type.mem.next;
	}

	alias_new->alias_type.mem.next = alias_cursor->alias_type.mem.next;
	alias_new->alias_type.mem.prev = alias_cursor;

	if (alias_cursor->alias_type.mem.next != NULL){
		alias_cursor->alias_type.mem.next->alias_type.mem.prev = alias_new;
	}
	alias_cursor->alias_type.mem.next = alias_new;
}

static void irImporterDynTrace_merge_reg_variable(struct ir* ir, struct alias* alias_src, struct alias* alias_dst, struct operand* operand, uint8_t alias_src_size, enum irOpcode part_op){
	if (alias_src->alias_type.reg.ir_node == NULL){
		alias_src->type 					= ALIAS_REG_READ;
		alias_src->alias_type.reg.ir_node 	= irImporterDynTrace_add_input(ir, operand, alias_src_size);
		if (alias_src->alias_type.reg.ir_node == NULL){
			printf("ERROR: in %s, unable to add input to IR\n", __func__);
			return;
		}
		ir_node_get_operation(alias_src->alias_type.reg.ir_node)->data = 2;
	}
	ir_convert_input_to_inner(ir, alias_dst->alias_type.reg.ir_node, part_op);
	irImporterDynTrace_add_dependence(ir, alias_src->alias_type.reg.ir_node, alias_dst->alias_type.reg.ir_node, IR_DEPENDENCE_TYPE_DIRECT);
}

static void irImporterDynTrace_merge_mem_variable(struct irRenameEngine* engine, struct alias* alias_dst, struct operand* operand, enum irOpcode part_op){
	struct alias*	alias_new;
	struct node* 	ir_node;

	alias_new = alias_allocate(engine);
	if (alias_new == NULL){
		printf("ERROR: in %s, unable to aloocate alias\n", __func__);
		return;
	}

	ir_node = irImporterDynTrace_add_input(engine->ir, operand, operand->size * 8);
	if (ir_node == NULL){
		printf("ERROR: in %s, unable to add input to IR\n", __func__);
		alias_free(engine, alias_new);
		return;
	}
	ir_node_get_operation(ir_node)->data = 2;

	if (alias_dst->alias_type.mem.prev == NULL){
		alias_new->type 					= alias_dst->type;
		alias_new->alias_type.mem.ir_node 	= alias_dst->alias_type.mem.ir_node;
		alias_new->alias_type.mem.address 	= alias_dst->alias_type.mem.address;
		alias_new->alias_type.mem.size 		= alias_dst->alias_type.mem.size;
		alias_new->alias_type.mem.engine 	= alias_dst->alias_type.mem.engine;
		alias_new->alias_type.mem.next 		= alias_dst->alias_type.mem.next;
		alias_new->alias_type.mem.prev 		= alias_dst;

		alias_dst->type 					= ALIAS_MEM_READ;
		alias_dst->alias_type.mem.ir_node 	= ir_node;
		alias_dst->alias_type.mem.address 	= operand->location.address;
		alias_dst->alias_type.mem.size 		= operand->size;
		alias_dst->alias_type.mem.engine 	= engine;
		alias_dst->alias_type.mem.next 		= alias_new;
		alias_dst->alias_type.mem.prev 		= NULL;

		ir_convert_input_to_inner(engine->ir, alias_new->alias_type.mem.ir_node, part_op);
		irImporterDynTrace_add_dependence(engine->ir, alias_dst->alias_type.mem.ir_node, alias_new->alias_type.mem.ir_node, IR_DEPENDENCE_TYPE_DIRECT);
	}
	else{
		printf("WARNING: in %s, this case is not implemented yet\n", __func__);
	}
}

struct node* irRenameEngine_get_ref(struct irRenameEngine* engine, struct operand* operand){
	struct alias** 	alias_result;
	struct alias 	alias_cmp;
	struct alias* 	alias_cursor;
	struct node* 	node = NULL;

	if (OPERAND_IS_MEM(*operand)){
		alias_cmp.type 						= ALIAS_MEM_READ;
		alias_cmp.alias_type.mem.ir_node 	= NULL;
		alias_cmp.alias_type.mem.address 	= operand->location.address;
		alias_cmp.alias_type.mem.engine 	= engine;

		alias_result = (struct alias**)tfind(&alias_cmp, &(engine->memory_bintree), compare_alias_to_alias);
		if (alias_result != NULL){
			alias_cursor = *alias_result;
			while (alias_cursor != NULL){
				if (alias_cursor->alias_type.mem.size == operand->size){
					node = (*alias_result)->alias_type.mem.ir_node;
					break;
				}
				alias_cursor = alias_cursor->alias_type.mem.next;
			}
			
			if (node == NULL){
				if ((*alias_result)->alias_type.mem.size > operand->size){
					/* pour le debug */
					printf("WARNING: in %s, this is a border case (part mem variable) be careful lad\n", __func__);
					irImporterDynTrace_part_mem_variable(engine, *alias_result, &node, operand, IR_PART1_8);
				}
				else{
					/* pour le debug */
					printf("WARNING: in %s, this is a border case (merge mem variable) be careful lad\n", __func__);
					irImporterDynTrace_merge_mem_variable(engine, *alias_result, operand, IR_PART1_8);
					node = (*alias_result)->alias_type.mem.ir_node;
				}
			}
		}
	}
	else if (OPERAND_IS_REG(*operand)){
		node = irRenameEngine_get_register_ref(engine, operand->location.reg, operand);
	}
	else{
		printf("ERROR: in %s, unexpected data type (REG or MEM)\n", __func__);
	}

	return node;
}

struct node* irRenameEngine_get_register_ref(struct irRenameEngine* engine, enum reg reg, struct operand* operand){
	struct node* node = NULL;

	if (reg != REGISTER_INVALID){
		node = engine->register_table[reg - 1].alias_type.reg.ir_node;
		if (node == NULL){
			switch(reg){
				case REGISTER_EAX 	: {
					if (engine->register_table[REGISTER_AH - 1].alias_type.reg.ir_node != NULL){
						if (ALIAS_IS_READ(engine->register_table[REGISTER_AH - 1].type)){
							irImporterDynTrace_merge_reg_variable(engine->ir, engine->register_table + (REGISTER_EAX - 1), engine->register_table + (REGISTER_AH - 1), operand, 32, IR_PART2_8);
						}
						else{
							printf("WARNING: in %s, partial write AH\n", __func__);
						}
					}
					if (engine->register_table[REGISTER_AL - 1].alias_type.reg.ir_node != NULL){
						if (ALIAS_IS_READ(engine->register_table[REGISTER_AL - 1].type)){
							irImporterDynTrace_merge_reg_variable(engine->ir, engine->register_table + (REGISTER_EAX - 1), engine->register_table + (REGISTER_AL - 1), operand, 32, IR_PART1_8);
						}
						else{
							printf("WARNING: in %s, partial write AL\n", __func__);
						}
					}
					node = engine->register_table[REGISTER_EAX - 1].alias_type.reg.ir_node;
					break;
				}
				case REGISTER_AX 	: {
					break;
				}
				case REGISTER_AH 	: {
					if (engine->register_table[REGISTER_EAX - 1].alias_type.reg.ir_node != NULL){
						irImporterDynTrace_part_reg_variable(engine->ir, engine->register_table + (REGISTER_EAX - 1), engine->register_table + (REGISTER_AH - 1), operand, 8, IR_PART2_8);
					}
					node = engine->register_table[REGISTER_AH - 1].alias_type.reg.ir_node;
					break;
				}
				case REGISTER_AL 	: {
					if (engine->register_table[REGISTER_EAX - 1].alias_type.reg.ir_node != NULL){
						irImporterDynTrace_part_reg_variable(engine->ir, engine->register_table + (REGISTER_EAX - 1), engine->register_table + (REGISTER_AL - 1), operand, 8, IR_PART1_8);
					}
					node = engine->register_table[REGISTER_AL - 1].alias_type.reg.ir_node;
					break;
				}
				case REGISTER_EBX 	: {
					if (engine->register_table[REGISTER_BH - 1].alias_type.reg.ir_node != NULL){
						if (ALIAS_IS_READ(engine->register_table[REGISTER_BH - 1].type)){
							irImporterDynTrace_merge_reg_variable(engine->ir, engine->register_table + (REGISTER_EBX - 1), engine->register_table + (REGISTER_BH - 1), operand, 32, IR_PART2_8);
						}
						else{
							printf("WARNING: in %s, partial write BH\n", __func__);
						}
					}
					if (engine->register_table[REGISTER_BL - 1].alias_type.reg.ir_node != NULL){
						if (ALIAS_IS_READ(engine->register_table[REGISTER_BL - 1].type)){
							irImporterDynTrace_merge_reg_variable(engine->ir, engine->register_table + (REGISTER_EBX - 1), engine->register_table + (REGISTER_BL - 1), operand, 32, IR_PART1_8);
						}
						else{
							printf("WARNING: in %s, partial write BL\n", __func__);
						}
					}
					node = engine->register_table[REGISTER_EBX - 1].alias_type.reg.ir_node;
					break;
				}
				case REGISTER_BX 	: {
					break;
				}
				case REGISTER_BH 	: {
					if (engine->register_table[REGISTER_EBX - 1].alias_type.reg.ir_node != NULL){
						irImporterDynTrace_part_reg_variable(engine->ir, engine->register_table + (REGISTER_EBX - 1), engine->register_table + (REGISTER_BH - 1), operand, 8, IR_PART2_8);
					}
					node = engine->register_table[REGISTER_BH - 1].alias_type.reg.ir_node;
					break;
				}
				case REGISTER_BL 	: {
					if (engine->register_table[REGISTER_EBX - 1].alias_type.reg.ir_node != NULL){
						irImporterDynTrace_part_reg_variable(engine->ir, engine->register_table + (REGISTER_EBX - 1), engine->register_table + (REGISTER_BL - 1), operand, 8, IR_PART1_8);
					}
					node = engine->register_table[REGISTER_BL - 1].alias_type.reg.ir_node;
					break;
				}
				case REGISTER_ECX 	: {
					if (engine->register_table[REGISTER_CH - 1].alias_type.reg.ir_node != NULL){
						if (ALIAS_IS_READ(engine->register_table[REGISTER_CH - 1].type)){
							irImporterDynTrace_merge_reg_variable(engine->ir, engine->register_table + (REGISTER_ECX - 1), engine->register_table + (REGISTER_CH - 1), operand, 32, IR_PART2_8);
						}
						else{
							printf("WARNING: in %s, partial write CH\n", __func__);
						}
					}
					if (engine->register_table[REGISTER_CL - 1].alias_type.reg.ir_node != NULL){
						if (ALIAS_IS_READ(engine->register_table[REGISTER_CL - 1].type)){
							irImporterDynTrace_merge_reg_variable(engine->ir, engine->register_table + (REGISTER_ECX - 1), engine->register_table + (REGISTER_CL - 1), operand, 32, IR_PART1_8);
						}
						else{
							printf("WARNING: in %s, partial write CL\n", __func__);
						}
					}
					node = engine->register_table[REGISTER_ECX - 1].alias_type.reg.ir_node;
					break;
				}
				case REGISTER_CX 	: {
					break;
				}
				case REGISTER_CH 	: {
					if (engine->register_table[REGISTER_ECX - 1].alias_type.reg.ir_node != NULL){
						irImporterDynTrace_part_reg_variable(engine->ir, engine->register_table + (REGISTER_ECX - 1), engine->register_table + (REGISTER_CH - 1), operand, 8, IR_PART2_8);
					}
					node = engine->register_table[REGISTER_CH - 1].alias_type.reg.ir_node;
					break;
				}
				case REGISTER_CL 	: {
					if (engine->register_table[REGISTER_ECX - 1].alias_type.reg.ir_node != NULL){
						irImporterDynTrace_part_reg_variable(engine->ir, engine->register_table + (REGISTER_ECX - 1), engine->register_table + (REGISTER_CL - 1), operand, 8, IR_PART1_8);
					}
					node = engine->register_table[REGISTER_CL - 1].alias_type.reg.ir_node;
					break;
				}
				case REGISTER_EDX 	: {
					if (engine->register_table[REGISTER_DH - 1].alias_type.reg.ir_node != NULL){
						if (ALIAS_IS_READ(engine->register_table[REGISTER_DH - 1].type)){
							irImporterDynTrace_merge_reg_variable(engine->ir, engine->register_table + (REGISTER_EDX - 1), engine->register_table + (REGISTER_DH - 1), operand, 32, IR_PART2_8);
						}
						else{
							printf("WARNING: in %s, partial write DH\n", __func__);
						}
					}
					if (engine->register_table[REGISTER_DL - 1].alias_type.reg.ir_node != NULL){
						if (ALIAS_IS_READ(engine->register_table[REGISTER_DL - 1].type)){
							irImporterDynTrace_merge_reg_variable(engine->ir, engine->register_table + (REGISTER_EDX - 1), engine->register_table + (REGISTER_DL - 1), operand, 32, IR_PART1_8);
						}
						else{
							printf("WARNING: in %s, partial write DL\n", __func__);
						}
					}
					node = engine->register_table[REGISTER_EDX - 1].alias_type.reg.ir_node;
					break;
				}
				case REGISTER_DX 	: {
					break;
				}
				case REGISTER_DH 	: {
					if (engine->register_table[REGISTER_EDX - 1].alias_type.reg.ir_node != NULL){
						irImporterDynTrace_part_reg_variable(engine->ir, engine->register_table + (REGISTER_EDX - 1), engine->register_table + (REGISTER_DH - 1), operand, 8, IR_PART2_8);
					}
					node = engine->register_table[REGISTER_DH - 1].alias_type.reg.ir_node;
					break;
				}
				case REGISTER_DL 	: {
					if (engine->register_table[REGISTER_EDX - 1].alias_type.reg.ir_node != NULL){
						irImporterDynTrace_part_reg_variable(engine->ir, engine->register_table + (REGISTER_EDX - 1), engine->register_table + (REGISTER_DL - 1), operand, 8, IR_PART1_8);
					}
					node = engine->register_table[REGISTER_DL - 1].alias_type.reg.ir_node;
					break;
				}
				case REGISTER_ESI 	: {
					break;
				}
				case REGISTER_EDI 	: {
					break;
				}
				case REGISTER_EBP 	: {
					break;
				}
				case REGISTER_ESP 	: {
					break;
				}
				default 			: {
					printf("ERROR: in %s, this case is not supposed to happen\n", __func__);
					break;
				}
			}
		}
	}

	return node;
}

int32_t irRenameEngine_set_new_ref(struct irRenameEngine* engine, struct operand* operand, struct node* node){
	struct alias* 	alias;

	if (OPERAND_IS_MEM(*operand)){
		alias = alias_allocate(engine);
		if (alias == NULL){
			printf("ERROR: in %s, unable to allocate alias\n", __func__);
			return -1;
		}
		alias->type 					= ALIAS_MEM_READ;
		alias->alias_type.mem.ir_node 	= node;
		alias->alias_type.mem.address 	= operand->location.address;
		alias->alias_type.mem.size 		= operand->size;
		alias->alias_type.mem.engine 	= engine;
		alias->alias_type.mem.next 		= NULL;
		alias->alias_type.mem.prev 		= NULL;

		if (*(void**)tsearch(alias, &(engine->memory_bintree), compare_alias_to_alias) != alias){
			printf("ERROR: in %s, unable to add element to the binary tree\n", __func__);
			alias_free(engine, alias);
			return -1;
		}
		else{
			ir_node_get_operation(node)->data ++;
		}
	}
	else if (OPERAND_IS_REG(*operand)){
		engine->register_table[operand->location.reg - 1].type 						= ALIAS_REG_READ;
		engine->register_table[operand->location.reg - 1].alias_type.reg.ir_node 	= node;
		ir_node_get_operation(node)->data ++;
	}
	else{
		printf("ERROR: in %s, unexpected data type (REG or MEM)\n", __func__);
		return -1;
	}

	return 0;
}

int32_t irRenameEngine_set_register_new_ref(struct irRenameEngine* engine, enum reg reg, struct node* node){
	if (reg != REGISTER_INVALID){
		engine->register_table[reg - 1].type 					= ALIAS_REG_READ;
		engine->register_table[reg - 1].alias_type.reg.ir_node 	= node;
		ir_node_get_operation(node)->data ++;
		return 0;
	}
	else{
		return -1;
	}
}

int32_t irRenameEngine_set_ref(struct irRenameEngine* engine, struct operand* operand, struct node* node){
	struct alias 	alias_cmp;
	struct alias** 	alias_result;
	struct alias* 	alias_new;
	struct alias* 	alias_cursor;

	if (OPERAND_IS_MEM(*operand)){
		alias_cmp.type 						= ALIAS_MEM_WRITE;
		alias_cmp.alias_type.mem.ir_node 	= NULL;
		alias_cmp.alias_type.mem.address 	= operand->location.address;
		alias_cmp.alias_type.mem.engine 	= engine;

		alias_result = (struct alias**)tsearch(&alias_cmp, &(engine->memory_bintree), compare_alias_to_alias);
		if (alias_result != NULL){
			if (*alias_result != &alias_cmp){
				alias_cursor = *alias_result;
				while (alias_cursor != NULL){
					if (alias_cursor->alias_type.mem.size > operand->size){
						printf("WARNING: in %s, writting mem while larger access are valid\n", __func__);
					}
					else{
						irRenameEngine_reset_mem_variable(engine, alias_cursor, operand, node);
						ir_node_get_operation(node)->data ++;
						break;
					}
					alias_cursor = alias_cursor->alias_type.mem.next;
				}
			}
			else{
				alias_new = alias_allocate(engine);
				if (alias_new == NULL){
					printf("ERROR: in %s, unable to allocate alias\n", __func__);
					return -1;
				}

				alias_new->type 					= ALIAS_MEM_WRITE;
				alias_new->alias_type.mem.ir_node 	= node;
				alias_new->alias_type.mem.address 	= operand->location.address;
				alias_new->alias_type.mem.size 		= operand->size;
				alias_new->alias_type.mem.engine 	= engine;
				alias_new->alias_type.mem.next 		= NULL;
				alias_new->alias_type.mem.prev 		= NULL; 

				ir_node_get_operation(node)->data ++;
				*alias_result = alias_new;
			}
		}
		else{
			printf("ERROR: in %s, unable to add element to the binary tree\n", __func__);
			return -1;
		}
	}
	else if (OPERAND_IS_REG(*operand)){
		return irRenameEngine_set_register_ref(engine, operand->location.reg, node);
	}
	else{
		printf("ERROR: in %s, unexpected data type (REG or MEM)\n", __func__);
		return -1;
	}

	return 0;
}

int32_t irRenameEngine_set_register_ref(struct irRenameEngine* engine, enum reg reg, struct node* node){
	irRenameEngine_reset_reg_variable(engine->ir, engine->register_table + (reg - 1), node);
	switch(reg){
		case REGISTER_EAX 	: {
			irRenameEngine_reset_reg_variable(engine->ir, engine->register_table + (REGISTER_AX - 1), NULL);
			irRenameEngine_reset_reg_variable(engine->ir, engine->register_table + (REGISTER_AH - 1), NULL);
			irRenameEngine_reset_reg_variable(engine->ir, engine->register_table + (REGISTER_AL - 1), NULL);
			break;
		}
		case REGISTER_AX 	: {
			irRenameEngine_reset_reg_variable(engine->ir, engine->register_table + (REGISTER_AH - 1), NULL);
			irRenameEngine_reset_reg_variable(engine->ir, engine->register_table + (REGISTER_AL - 1), NULL);
			if (engine->register_table[REGISTER_EAX - 1].alias_type.reg.ir_node != NULL){
				printf("WARNING: in %s, writting register AX while EAX is valid\n", __func__);
			}
			break;
		}
		case REGISTER_AH 	: {
			if (engine->register_table[REGISTER_AX - 1].alias_type.reg.ir_node != NULL){
				printf("WARNING: in %s, writting register AH while AX is valid\n", __func__);
			}
			if (engine->register_table[REGISTER_EAX - 1].alias_type.reg.ir_node != NULL){
				printf("WARNING: in %s, writting register AH while EAX is valid\n", __func__);
			}
			break;
		}
		case REGISTER_AL 	: {
			if (engine->register_table[REGISTER_AX - 1].alias_type.reg.ir_node != NULL){
				printf("WARNING: in %s, writting register AL while AX is valid\n", __func__);
			}
			if (engine->register_table[REGISTER_EAX - 1].alias_type.reg.ir_node != NULL){
				printf("WARNING: in %s, writting register AL while EAX is valid\n", __func__);
			}
			break;
		}
		case REGISTER_EBX 	: {
			irRenameEngine_reset_reg_variable(engine->ir, engine->register_table + (REGISTER_BX - 1), NULL);
			irRenameEngine_reset_reg_variable(engine->ir, engine->register_table + (REGISTER_BH - 1), NULL);
			irRenameEngine_reset_reg_variable(engine->ir, engine->register_table + (REGISTER_BL - 1), NULL);
			break;
		}
		case REGISTER_BX 	: {
			irRenameEngine_reset_reg_variable(engine->ir, engine->register_table + (REGISTER_BH - 1), NULL);
			irRenameEngine_reset_reg_variable(engine->ir, engine->register_table + (REGISTER_BL - 1), NULL);
			if (engine->register_table[REGISTER_EBX - 1].alias_type.reg.ir_node != NULL){
				printf("WARNING: in %s, writting register BX while EBX is valid\n", __func__);
			}
			break;
		}
		case REGISTER_BH 	: {
			if (engine->register_table[REGISTER_BX - 1].alias_type.reg.ir_node != NULL){
				printf("WARNING: in %s, writting register BH while BX is valid\n", __func__);
			}
			if (engine->register_table[REGISTER_EBX - 1].alias_type.reg.ir_node != NULL){
				printf("WARNING: in %s, writting register BH while EBX is valid\n", __func__);
			}
			break;
		}
		case REGISTER_BL 	: {
			if (engine->register_table[REGISTER_BX - 1].alias_type.reg.ir_node != NULL){
				printf("WARNING: in %s, writting register BL while BX is valid\n", __func__);
			}
			if (engine->register_table[REGISTER_EBX - 1].alias_type.reg.ir_node != NULL){
				printf("WARNING: in %s, writting register BL while EBX is valid\n", __func__);
			}
			break;
		}
		case REGISTER_ECX 	: {
			irRenameEngine_reset_reg_variable(engine->ir, engine->register_table + (REGISTER_CX - 1), NULL);
			irRenameEngine_reset_reg_variable(engine->ir, engine->register_table + (REGISTER_CH - 1), NULL);
			irRenameEngine_reset_reg_variable(engine->ir, engine->register_table + (REGISTER_CL - 1), NULL);
			break;
		}
		case REGISTER_CX 	: {
			irRenameEngine_reset_reg_variable(engine->ir, engine->register_table + (REGISTER_CH - 1), NULL);
			irRenameEngine_reset_reg_variable(engine->ir, engine->register_table + (REGISTER_CL - 1), NULL);
			if (engine->register_table[REGISTER_ECX - 1].alias_type.reg.ir_node != NULL){
				printf("WARNING: in %s, writting register CX while ECX is valid\n", __func__);
			}
			break;
		}
		case REGISTER_CH 	: {
			if (engine->register_table[REGISTER_CX - 1].alias_type.reg.ir_node != NULL){
				printf("WARNING: in %s, writting register CH while CX is valid\n", __func__);
			}
			if (engine->register_table[REGISTER_ECX - 1].alias_type.reg.ir_node != NULL){
				printf("WARNING: in %s, writting register CH while ECX is valid\n", __func__);
			}
			break;
		}
		case REGISTER_CL 	: {
			if (engine->register_table[REGISTER_CX - 1].alias_type.reg.ir_node != NULL){
				printf("WARNING: in %s, writting register CL while CX is valid\n", __func__);
			}
			if (engine->register_table[REGISTER_ECX - 1].alias_type.reg.ir_node != NULL){
				printf("WARNING: in %s, writting register CL while ECX is valid\n", __func__);
			}
			break;
		}
		case REGISTER_EDX 	: {
			irRenameEngine_reset_reg_variable(engine->ir, engine->register_table + (REGISTER_DX - 1), NULL);
			irRenameEngine_reset_reg_variable(engine->ir, engine->register_table + (REGISTER_DH - 1), NULL);
			irRenameEngine_reset_reg_variable(engine->ir, engine->register_table + (REGISTER_DL - 1), NULL);
			break;
		}
		case REGISTER_DX 	: {
			irRenameEngine_reset_reg_variable(engine->ir, engine->register_table + (REGISTER_DH - 1), NULL);
			irRenameEngine_reset_reg_variable(engine->ir, engine->register_table + (REGISTER_DL - 1), NULL);
			if (engine->register_table[REGISTER_EDX - 1].alias_type.reg.ir_node != NULL){
				printf("WARNING: in %s, writting register DX while EDX is valid\n", __func__);
			}
			break;
		}
		case REGISTER_DH 	: {
			if (engine->register_table[REGISTER_DX - 1].alias_type.reg.ir_node != NULL){
				printf("WARNING: in %s, writting register DH while DX is valid\n", __func__);
			}
			if (engine->register_table[REGISTER_EDX - 1].alias_type.reg.ir_node != NULL){
				printf("WARNING: in %s, writting register DH while EDX is valid\n", __func__);
				}
			break;
		}
		case REGISTER_DL 	: {
			if (engine->register_table[REGISTER_DX - 1].alias_type.reg.ir_node != NULL){
				printf("WARNING: in %s, writting register DL while DX is valid\n", __func__);
			}
			if (engine->register_table[REGISTER_EDX - 1].alias_type.reg.ir_node != NULL){
				printf("WARNING: in %s, writting register DL while EDX is valid\n", __func__);
			}
			break;
		}
		case REGISTER_ESI 	: {
			break;
		}
		case REGISTER_EDI 	: {
			break;
		}
		case REGISTER_EBP 	: {
			break;
		}
		default : {
			printf("WARNING: in %s, this case is not supposed to happen\n", __func__);
		}
	}
	ir_node_get_operation(node)->data ++;

	return 0;
}

/* ===================================================================== */
/* Compare functions						                             */
/* ===================================================================== */

int32_t compare_alias_to_alias(const void* alias1, const void* alias2){
	if (((struct alias*)alias1)->alias_type.mem.address > ((struct alias*)alias2)->alias_type.mem.address){
		return 1;
	}
	else if (((struct alias*)alias1)->alias_type.mem.address < ((struct alias*)alias2)->alias_type.mem.address){
		return -1;
	}
	else{
		return 0;
	}
}
