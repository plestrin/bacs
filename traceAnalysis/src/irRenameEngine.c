#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>

#include "irRenameEngine.h"
#include "irImporterDynTrace.h"

/* One can use a better allocator. Hint: it only grows - no free before the end */
#define alias_allocate(engine) 			((struct alias*)malloc(sizeof(struct alias)))
#define alias_free(engine, alias) 		free(alias)

int32_t compare_alias_to_alias(const void* alias1, const void* alias2);

void irRenameEngine_free_memory_node(void* alias){
	alias_free(((struct alias*)alias)->alias_type.mem.engine, alias);
}

#define irRenameEngine_reset_register(alias, new_node) 																													\
	if ((alias).alias_type.reg.ir_node != NULL){ 																														\
		ir_node_get_operation((alias).alias_type.reg.ir_node)->data --; 																								\
		if (ir_node_get_operation((alias).alias_type.reg.ir_node)->data == 0){ 																							\
			ir_convert_output_to_inner(engine->ir, (alias).alias_type.reg.ir_node); 																					\
		} 																																								\
	} 																																									\
	(alias).type 					= ALIAS_REG_WRITE; 																													\
	(alias).alias_type.reg.ir_node 	= (new_node);

#define irImporterDynTrace_add_part_variable(ir, alias_src, alias_dst, operand) 																						\
	(alias_dst).alias_type.reg.ir_node = irImporterDynTrace_add_operation((ir), IR_PART, (operand));																	\
	if ((alias_dst).alias_type.reg.ir_node == NULL){ 																													\
		printf("ERROR: in %s, unable to add operation to IR\n", __func__); 																								\
	} 																																									\
	else{ 																																								\
		ir_node_get_operation((alias_dst).alias_type.reg.ir_node)->data = 1; 																							\
		irImporterDynTrace_add_dependence((ir), (alias_src).alias_type.reg.ir_node, (alias_dst).alias_type.reg.ir_node, IR_DEPENDENCE_TYPE_DIRECT); 					\
	}

#define irImporterDynTrace_merge_variable(ir, alias_src, alias_dst, operand) 																							\
	if ((alias_src).alias_type.reg.ir_node == NULL){ 																													\
		(alias_src).alias_type.reg.ir_node = irImporterDynTrace_add_input((ir), (operand)); 																			\
		ir_node_get_operation((alias_src).alias_type.reg.ir_node)->data = 2; 																							\
	} 																																									\
	if ((alias_src).alias_type.reg.ir_node == NULL){ 																													\
		printf("ERROR: in %s, unable to input to IR\n", __func__); 																										\
	} 																																									\
	else{ 																																								\
		ir_convert_input_to_inner((ir), (alias_dst).alias_type.reg.ir_node, IR_PART); 																					\
		irImporterDynTrace_add_dependence((ir), (alias_src).alias_type.reg.ir_node, (alias_dst).alias_type.reg.ir_node, IR_DEPENDENCE_TYPE_DIRECT); 					\
	}

struct node* irRenameEngine_get_ref(struct irRenameEngine* engine, struct operand* operand){
	struct alias** 	alias_result;
	struct alias 	alias_cmp;
	struct node* 	node = NULL;

	if (OPERAND_IS_MEM(*operand)){
		alias_cmp.type 						= ALIAS_MEM_READ;
		alias_cmp.alias_type.mem.ir_node 	= NULL;
		alias_cmp.alias_type.mem.address 	= operand->location.address;
		alias_cmp.alias_type.mem.engine 	= engine;

		alias_result = (struct alias**)tfind(&alias_cmp, &(engine->memory_bintree), compare_alias_to_alias);
		if (alias_result != NULL){
			node = (*alias_result)->alias_type.mem.ir_node;
		}
	}
	else if (OPERAND_IS_REG(*operand)){
		node = engine->register_table[operand->location.reg - 1].alias_type.reg.ir_node;
		if (node == NULL){
			switch(operand->location.reg){
				case REGISTER_EAX 	: {
					if (engine->register_table[REGISTER_AH - 1].alias_type.reg.ir_node != NULL){
						if (ALIAS_IS_READ(engine->register_table[REGISTER_AH - 1].type)){
							irImporterDynTrace_merge_variable(engine->ir, engine->register_table[REGISTER_EAX - 1], engine->register_table[REGISTER_AH - 1], operand)
						}
						else{
							printf("WARNING: in %s, partial write\n", __func__);
						}
					}
					if (engine->register_table[REGISTER_AL - 1].alias_type.reg.ir_node != NULL){
						if (ALIAS_IS_READ(engine->register_table[REGISTER_AL - 1].type)){
							irImporterDynTrace_merge_variable(engine->ir, engine->register_table[REGISTER_EAX - 1], engine->register_table[REGISTER_AL - 1], operand)
						}
						else{
							printf("WARNING: in %s, partial write\n", __func__);
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
						irImporterDynTrace_add_part_variable(engine->ir, engine->register_table[REGISTER_EAX - 1], engine->register_table[REGISTER_AH - 1], operand)
					}
					node = engine->register_table[REGISTER_AH - 1].alias_type.reg.ir_node;
					break;
				}
				case REGISTER_AL 	: {
					if (engine->register_table[REGISTER_EAX - 1].alias_type.reg.ir_node != NULL){
						irImporterDynTrace_add_part_variable(engine->ir, engine->register_table[REGISTER_EAX - 1], engine->register_table[REGISTER_AL - 1], operand)
					}
					node = engine->register_table[REGISTER_AL - 1].alias_type.reg.ir_node;
					break;
				}
				case REGISTER_EBX 	: {
					if (engine->register_table[REGISTER_BH - 1].alias_type.reg.ir_node != NULL){
						if (ALIAS_IS_READ(engine->register_table[REGISTER_BH - 1].type)){
							irImporterDynTrace_merge_variable(engine->ir, engine->register_table[REGISTER_EBX - 1], engine->register_table[REGISTER_BH - 1], operand)
						}
						else{
							printf("WARNING: in %s, partial write\n", __func__);
						}
					}
					if (engine->register_table[REGISTER_BL - 1].alias_type.reg.ir_node != NULL){
						if (ALIAS_IS_READ(engine->register_table[REGISTER_BL - 1].type)){
							irImporterDynTrace_merge_variable(engine->ir, engine->register_table[REGISTER_EBX - 1], engine->register_table[REGISTER_BL - 1], operand)
						}
						else{
							printf("WARNING: in %s, partial write\n", __func__);
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
						irImporterDynTrace_add_part_variable(engine->ir, engine->register_table[REGISTER_EBX - 1], engine->register_table[REGISTER_BH - 1], operand)
					}
					node = engine->register_table[REGISTER_BH - 1].alias_type.reg.ir_node;
					break;
				}
				case REGISTER_BL 	: {
					if (engine->register_table[REGISTER_EBX - 1].alias_type.reg.ir_node != NULL){
						irImporterDynTrace_add_part_variable(engine->ir, engine->register_table[REGISTER_EBX - 1], engine->register_table[REGISTER_BL - 1], operand)
					}
					node = engine->register_table[REGISTER_BL - 1].alias_type.reg.ir_node;
					break;
				}
				case REGISTER_ECX 	: {
					if (engine->register_table[REGISTER_CH - 1].alias_type.reg.ir_node != NULL){
						if (ALIAS_IS_READ(engine->register_table[REGISTER_CH - 1].type)){
							irImporterDynTrace_merge_variable(engine->ir, engine->register_table[REGISTER_ECX - 1], engine->register_table[REGISTER_CH - 1], operand)
						}
						else{
							printf("WARNING: in %s, partial write\n", __func__);
						}
					}
					if (engine->register_table[REGISTER_CL - 1].alias_type.reg.ir_node != NULL){
						if (ALIAS_IS_READ(engine->register_table[REGISTER_CL - 1].type)){
							irImporterDynTrace_merge_variable(engine->ir, engine->register_table[REGISTER_ECX - 1], engine->register_table[REGISTER_CL - 1], operand)
						}
						else{
							printf("WARNING: in %s, partial write\n", __func__);
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
						irImporterDynTrace_add_part_variable(engine->ir, engine->register_table[REGISTER_ECX - 1], engine->register_table[REGISTER_CH - 1], operand)
					}
					node = engine->register_table[REGISTER_CH - 1].alias_type.reg.ir_node;
					break;
				}
				case REGISTER_CL 	: {
					if (engine->register_table[REGISTER_ECX - 1].alias_type.reg.ir_node != NULL){
						irImporterDynTrace_add_part_variable(engine->ir, engine->register_table[REGISTER_ECX - 1], engine->register_table[REGISTER_CL - 1], operand)
					}
					node = engine->register_table[REGISTER_CL - 1].alias_type.reg.ir_node;
					break;
				}
				case REGISTER_EDX 	: {
					if (engine->register_table[REGISTER_DH - 1].alias_type.reg.ir_node != NULL){
						if (ALIAS_IS_READ(engine->register_table[REGISTER_DH - 1].type)){
							irImporterDynTrace_merge_variable(engine->ir, engine->register_table[REGISTER_EDX - 1], engine->register_table[REGISTER_DH - 1], operand)
						}
						else{
							printf("WARNING: in %s, partialwrite\n", __func__);
						}
					}
					if (engine->register_table[REGISTER_DL - 1].alias_type.reg.ir_node != NULL){
						if (ALIAS_IS_READ(engine->register_table[REGISTER_DL - 1].type)){
							irImporterDynTrace_merge_variable(engine->ir, engine->register_table[REGISTER_EDX - 1], engine->register_table[REGISTER_DL - 1], operand)
						}
						else{
							printf("WARNING: in %s, partialwrite\n", __func__);
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
						irImporterDynTrace_add_part_variable(engine->ir, engine->register_table[REGISTER_EDX - 1], engine->register_table[REGISTER_DH - 1], operand)
					}
					node = engine->register_table[REGISTER_DH - 1].alias_type.reg.ir_node;
					break;
				}
				case REGISTER_DL 	: {
					if (engine->register_table[REGISTER_EDX - 1].alias_type.reg.ir_node != NULL){
						irImporterDynTrace_add_part_variable(engine->ir, engine->register_table[REGISTER_EDX - 1], engine->register_table[REGISTER_DL - 1], operand)
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
				default : {
					printf("WARNING: in %s, this case is not supposed to happen\n", __func__);
				}
			}
		}
	}
	else{
		printf("ERROR: in %s, unexpected data type (REG or MEM)\n", __func__);
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
		alias->type 					= ALIAS_MEM_WRITE;
		alias->alias_type.mem.ir_node 	= node;
		alias->alias_type.mem.address 	= operand->location.address;
		alias->alias_type.mem.engine 	= engine;

		if (*(void**)tsearch(alias, &(engine->memory_bintree), compare_alias_to_alias) != alias){
			printf("ERROR: in %s, unable to add element to the binary tree\n", __func__);
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

int32_t irRenameEngine_set_ref(struct irRenameEngine* engine, struct operand* operand, struct node* node){
	struct alias 	alias_cmp;
	struct alias** 	alias_result;
	struct alias* 	alias_new;

	if (OPERAND_IS_MEM(*operand)){
		alias_cmp.type 						= ALIAS_MEM_WRITE;
		alias_cmp.alias_type.mem.ir_node 	= NULL;
		alias_cmp.alias_type.mem.address 	= operand->location.address;
		alias_cmp.alias_type.mem.engine 	= engine;

		alias_result = (struct alias**)tfind(&alias_cmp, &(engine->memory_bintree), compare_alias_to_alias);
		if (alias_result != NULL){
			if ((*alias_result)->alias_type.mem.ir_node != NULL){
				ir_node_get_operation((*alias_result)->alias_type.mem.ir_node)->data --;
				if (ir_node_get_operation((*alias_result)->alias_type.mem.ir_node)->data == 0){
					ir_convert_output_to_inner(engine->ir, (*alias_result)->alias_type.mem.ir_node);
				}
			}
			(*alias_result)->alias_type.mem.ir_node = node;
			ir_node_get_operation(node)->data ++;
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
			alias_new->alias_type.mem.engine 	= engine;

			if (*(void**)tsearch(alias_new, &(engine->memory_bintree), compare_alias_to_alias) != alias_new){
				printf("ERROR: in %s, unable to add element to the binary tree\n", __func__);
				return -1;
			}
			else{
				ir_node_get_operation(node)->data ++;
			}
		}
	}
	else if (OPERAND_IS_REG(*operand)){
		irRenameEngine_reset_register(engine->register_table[operand->location.reg - 1], node)
		switch(operand->location.reg){
			case REGISTER_EAX 	: {
				irRenameEngine_reset_register(engine->register_table[REGISTER_AX - 1], NULL)
				irRenameEngine_reset_register(engine->register_table[REGISTER_AH - 1], NULL)
				irRenameEngine_reset_register(engine->register_table[REGISTER_AL - 1], NULL)
				break;
			}
			case REGISTER_AX 	: {
				irRenameEngine_reset_register(engine->register_table[REGISTER_AH - 1], NULL)
				irRenameEngine_reset_register(engine->register_table[REGISTER_AL - 1], NULL)

				break;
			}
			case REGISTER_AH 	: {
				break;
			}
			case REGISTER_AL 	: {
				break;
			}
			case REGISTER_EBX 	: {
				irRenameEngine_reset_register(engine->register_table[REGISTER_BX - 1], NULL)
				irRenameEngine_reset_register(engine->register_table[REGISTER_BH - 1], NULL)
				irRenameEngine_reset_register(engine->register_table[REGISTER_BL - 1], NULL)
				break;
			}
			case REGISTER_BX 	: {
				irRenameEngine_reset_register(engine->register_table[REGISTER_BH - 1], NULL)
				irRenameEngine_reset_register(engine->register_table[REGISTER_BL - 1], NULL)
				break;
			}
			case REGISTER_BH 	: {
				break;
			}
			case REGISTER_BL 	: {
				break;
			}
			case REGISTER_ECX 	: {
				irRenameEngine_reset_register(engine->register_table[REGISTER_CX - 1], NULL)
				irRenameEngine_reset_register(engine->register_table[REGISTER_CH - 1], NULL)
				irRenameEngine_reset_register(engine->register_table[REGISTER_CL - 1], NULL)
				break;
			}
			case REGISTER_CX 	: {
				irRenameEngine_reset_register(engine->register_table[REGISTER_CH - 1], NULL)
				irRenameEngine_reset_register(engine->register_table[REGISTER_CL - 1], NULL)
				break;
			}
			case REGISTER_CH 	: {
				break;
			}
			case REGISTER_CL 	: {
				break;
			}
			case REGISTER_EDX 	: {
				irRenameEngine_reset_register(engine->register_table[REGISTER_DX - 1], NULL)
				irRenameEngine_reset_register(engine->register_table[REGISTER_DH - 1], NULL)
				irRenameEngine_reset_register(engine->register_table[REGISTER_DL - 1], NULL)
				break;
			}
			case REGISTER_DX 	: {
				irRenameEngine_reset_register(engine->register_table[REGISTER_DH - 1], NULL)
				irRenameEngine_reset_register(engine->register_table[REGISTER_DL - 1], NULL)
				break;
			}
			case REGISTER_DH 	: {
				break;
			}
			case REGISTER_DL 	: {
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
	}
	else{
		printf("ERROR: in %s, unexpected data type (REG or MEM)\n", __func__);
		return -1;
	}

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
