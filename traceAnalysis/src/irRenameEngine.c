#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>

#include "irRenameEngine.h"
#include "irImporterDynTrace.h"

/* One can use a better allocator. Hint: it only grows - no free before the end */
#define aliasNodeTree_allocate(engine) 			((struct aliasNodeTree*)malloc(sizeof(struct aliasNodeTree)))
#define aliasNodeTree_free(engine, alias) 		free(alias)

int32_t compare_alias_to_alias(const void* alias1, const void* alias2);

void irRenameEngine_free_memory_node(void* alias){
	aliasNodeTree_free(((struct aliasNodeTree*)alias)->engine, alias);
}

#define irRenameEngine_reset_register(register_entry, new_node) 																										\
	if ((register_entry) != NULL){ 																																		\
		ir_node_get_operation(register_entry)->data --; 																												\
		if (ir_node_get_operation(register_entry)->data == 0){ 																											\
			ir_operation_set_inner(engine->ir, register_entry); 																										\
		} 																																								\
	} 																																									\
	(register_entry) = (new_node);

#define irImporterDynTrace_add_part_variable(ir, src_node, dst_node) 																									\
	(dst_node) = irImporterDynTrace_add_operation((ir), IR_PART);																										\
	if ((dst_node) == NULL){ 																																			\
		printf("ERROR: in %s, unable to add operation to IR\n", __func__); 																								\
	} 																																									\
	else{ 																																								\
		ir_node_get_operation(dst_node)->data = 1; 																														\
		irImporterDynTrace_add_dependence((ir), (src_node), (dst_node), IR_DEPENDENCE_TYPE_DIRECT); 																	\
	}

struct node* irRenameEngine_get_ref(struct irRenameEngine* engine, struct operand* operand){
	struct aliasNodeTree** 	alias_result;
	struct aliasNodeTree 	alias_cmp;
	struct node* 			node = NULL;

	if (OPERAND_IS_MEM(*operand)){
		alias_cmp.ir_node = NULL;
		alias_cmp.address = operand->location.address;
		alias_cmp.engine = engine;

		alias_result = (struct aliasNodeTree**)tfind(&alias_cmp, &(engine->memory_bintree), compare_alias_to_alias);
		if (alias_result != NULL){
			node = (*alias_result)->ir_node;
		}
	}
	else if (OPERAND_IS_REG(*operand)){
		node = engine->register_table[operand->location.reg - 1];
		if (node == NULL){
			switch(operand->location.reg){
				case REGISTER_EAX 	: {
					break;
				}
				case REGISTER_AX 	: {
					break;
				}
				case REGISTER_AH 	: {
					if (engine->register_table[REGISTER_EAX - 1] != NULL){
						printf("Might produce part operation AH\n");
					}
					break;
				}
				case REGISTER_AL 	: {
					if (engine->register_table[REGISTER_EAX - 1] != NULL){
						printf("Might produce part operation AL\n");
					}
					break;
				}
				case REGISTER_EBX 	: {
					break;
				}
				case REGISTER_BX 	: {
					break;
				}
				case REGISTER_BH 	: {
					if (engine->register_table[REGISTER_EBX - 1] != NULL){
						printf("Might produce part operation BH\n");
					}
					break;
				}
				case REGISTER_BL 	: {
					if (engine->register_table[REGISTER_EBX - 1] != NULL){
						printf("Might produce part operation BL\n");
					}
					break;
				}
				case REGISTER_ECX 	: {
					break;
				}
				case REGISTER_CX 	: {
					break;
				}
				case REGISTER_CH 	: {
					if (engine->register_table[REGISTER_ECX - 1] != NULL){
						printf("Might produce part operation CH\n");
					}
					break;
				}
				case REGISTER_CL 	: {
					if (engine->register_table[REGISTER_ECX - 1] != NULL){
						printf("Might produce part operation CL\n");
					}
					break;
				}
				case REGISTER_EDX 	: {
					break;
				}
				case REGISTER_DX 	: {
					break;
				}
				case REGISTER_DH 	: {
					if (engine->register_table[REGISTER_EDX - 1] != NULL){
						irImporterDynTrace_add_part_variable(engine->ir, engine->register_table[REGISTER_EDX - 1], engine->register_table[REGISTER_DH - 1])
					}
					node = engine->register_table[REGISTER_DH - 1];
					break;
				}
				case REGISTER_DL 	: {
					if (engine->register_table[REGISTER_EDX - 1] != NULL){
						irImporterDynTrace_add_part_variable(engine->ir, engine->register_table[REGISTER_EDX - 1], engine->register_table[REGISTER_DL - 1])
					}
					node = engine->register_table[REGISTER_DL - 1];
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
	struct aliasNodeTree* 	alias;

	if (OPERAND_IS_MEM(*operand)){
		alias = aliasNodeTree_allocate(engine);
		alias->ir_node 	= node;
		alias->address 	= operand->location.address;
		alias->engine 	= engine;

		if (alias == NULL){
			printf("ERROR: in %s, unable to allocate aliasNodeTree\n", __func__);
			return -1;
		}

		if (*(void**)tsearch(alias, &(engine->memory_bintree), compare_alias_to_alias) != alias){
			printf("ERROR: in %s, unable to add element to the binary tree\n", __func__);
			return -1;
		}
		else{
			ir_node_get_operation(node)->data ++;
		}
	}
	else if (OPERAND_IS_REG(*operand)){
		engine->register_table[operand->location.reg - 1] = node;
		ir_node_get_operation(node)->data ++;
		#if 0
		switch(operand->location.reg){
			case REGISTER_EAX 	: 
			case REGISTER_AX 	: 
			case REGISTER_AH 	: 
			case REGISTER_AL 	: 
			case REGISTER_EBX 	: 
			case REGISTER_BX 	: 
			case REGISTER_BH 	: 
			case REGISTER_BL 	: 
			case REGISTER_ECX 	: 
			case REGISTER_CX 	: 
			case REGISTER_CH 	: 
			case REGISTER_CL 	: 
			case REGISTER_EDX 	: 
			case REGISTER_DX 	: 
			case REGISTER_DH 	: 
			case REGISTER_DL 	: 
			case REGISTER_ESI 	: 
			case REGISTER_EDI 	: 
			case REGISTER_EBP 	: {
				
				break;
			}
			default : {
				printf("WARNING: in %s, this case is not supposed to happen\n", __func__);
			}
		}
		#endif
	}
	else{
		printf("ERROR: in %s, unexpected data type (REG or MEM)\n", __func__);
		return -1;
	}

	return 0;
}

int32_t irRenameEngine_set_ref(struct irRenameEngine* engine, struct operand* operand, struct node* node){
	struct aliasNodeTree 	alias_cmp;
	struct aliasNodeTree** 	alias_result;
	struct aliasNodeTree* 	alias_new;

	if (OPERAND_IS_MEM(*operand)){
		alias_cmp.ir_node = NULL;
		alias_cmp.address = operand->location.address;
		alias_cmp.engine = engine;

		alias_result = (struct aliasNodeTree**)tfind(&alias_cmp, &(engine->memory_bintree), compare_alias_to_alias);
		if (alias_result != NULL){
			if ((*alias_result)->ir_node != NULL){
				ir_node_get_operation((*alias_result)->ir_node)->data --;
				if (ir_node_get_operation((*alias_result)->ir_node)->data == 0){
					ir_operation_set_inner(engine->ir, (*alias_result)->ir_node);
				}
			}
			(*alias_result)->ir_node = node;
			ir_node_get_operation(node)->data ++;
		}
		else{
			alias_new = aliasNodeTree_allocate(engine);
			alias_new->ir_node 	= node;
			alias_new->address 	= operand->location.address;
			alias_new->engine 	= engine;
			if (alias_new == NULL){
				printf("ERROR: in %s, unable to allocate aliasNodeTree\n", __func__);
				return -1;
			}

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
	if (((struct aliasNodeTree*)alias1)->address > ((struct aliasNodeTree*)alias2)->address){
		return 1;
	}
	else if (((struct aliasNodeTree*)alias1)->address < ((struct aliasNodeTree*)alias2)->address){
		return -1;
	}
	else{
		return 0;
	}
}
