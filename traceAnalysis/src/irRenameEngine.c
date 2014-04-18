#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>

#include "irRenameEngine.h"

/* One can use a better allocator. Hint: it only grows */
#define aliasNodeTree_allocate(engine) 			((struct aliasNodeTree*)malloc(sizeof(struct aliasNodeTree)))
#define aliasNodeTree_free(engine, alias) 		free(alias)

int32_t compare_alias_to_alias(const void* alias1, const void* alias2);

void irRenameEngine_free_memory_node(void* alias){
	aliasNodeTree_free(((struct aliasNodeTree*)alias)->engine, alias);
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
		node = engine->register_table[operand->location.reg - 1].ir_node;
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
		engine->register_table[operand->location.reg - 1].ir_node = node;
		ir_node_get_operation(node)->data ++;
	}
	else{
		printf("ERROR: in %s, unexpected data type (REG or MEM)\n", __func__);
		return -1;
	}

	return 0;
}

int32_t irRenameEngine_set_ref(struct irRenameEngine* engine, struct operand* operand, struct node* node){
	struct aliasNodeTree 	alias_cmp;
	struct aliasNodeTree* 	alias_result;
	struct aliasNodeTree* 	alias_new;

	if (OPERAND_IS_MEM(*operand)){
		alias_cmp.ir_node = NULL;
		alias_cmp.address = operand->location.address;
		alias_cmp.engine = engine;

		alias_result = (struct aliasNodeTree*)tfind(&alias_cmp, &(engine->memory_bintree), compare_alias_to_alias);
		if (alias_result != NULL){
			if (alias_result->ir_node != NULL){
				ir_node_get_operation(alias_result->ir_node)->data --;
				if (ir_node_get_operation(alias_result->ir_node)->data == 0){
					ir_operation_set_inner(engine->ir, alias_result->ir_node);
				}
			}
			alias_result->ir_node = node;
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
		if (engine->register_table[operand->location.reg - 1].ir_node != NULL){
			ir_node_get_operation(engine->register_table[operand->location.reg - 1].ir_node)->data --;
			if (ir_node_get_operation(engine->register_table[operand->location.reg - 1].ir_node)->data == 0){
				ir_operation_set_inner(engine->ir, engine->register_table[operand->location.reg - 1].ir_node);
			}
		}
		engine->register_table[operand->location.reg - 1].ir_node = node;
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
