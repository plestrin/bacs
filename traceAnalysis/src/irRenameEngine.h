#ifndef IRRENAMEENGINE_H
#define IRRENAMEENGINE_H

#define _GNU_SOURCE

#include <search.h>

#include "instruction.h"
#include "graph.h"
#include "ir.h"

struct aliasNodeTree{
	struct node* 			ir_node;
	ADDRESS 				address;
	struct irRenameEngine* 	engine;
};

struct irRenameEngine{
	void* 					memory_bintree;
	struct aliasNodeTree 	register_table[NB_REGISTER];
	struct ir* 				ir;
};

#define irRenameEngine_init(engine, ir_) 													\
	(engine).memory_bintree = NULL; 														\
	memset(&((engine).register_table), 0, sizeof(struct aliasNodeTree)*NB_REGISTER); 		\
	(engine).ir = (ir_);

void irRenameEngine_free_memory_node(void* alias);

struct node* irRenameEngine_get_ref(struct irRenameEngine* engine, struct operand* operand);
int32_t irRenameEngine_set_ref(struct irRenameEngine* engine, struct operand* operand, struct node* node);
int32_t irRenameEngine_set_new_ref(struct irRenameEngine* engine, struct operand* operand, struct node* node);

#define irRenameEngine_clean(engine) 														\
	tdestroy((engine).memory_bintree, irRenameEngine_free_memory_node);

#endif