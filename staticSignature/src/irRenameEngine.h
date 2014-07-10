#ifndef IRRENAMEENGINE_H
#define IRRENAMEENGINE_H

#include <string.h>

#include "graph.h"
#include "ir.h"

#define IRRENAMEENGINE_TYPE_READ 	0x00
#define IRRENAMEENGINE_TYPE_WRITE 	0x01

#define ALIAS_IS_READ(type) 		(!(type & 0x01))
#define ALIAS_IS_WRITE(type) 		(type & 0x01)

struct alias{
	struct node* 		ir_node;
	uint8_t 			type;
};

struct irRenameEngine{
	struct alias 		register_alias[NB_IR_REGISTER];
	uint32_t 			mem_op_order;
	struct ir* 			ir;
};

#define irRenameEngine_init(engine, ir_) 													\
	memset(&((engine).register_alias), 0, sizeof(struct alias) * NB_IR_REGISTER); 			\
	(engine).mem_op_order = 0; 																\
	(engine).ir = (ir_);

struct node* irRenameEngine_get_register_ref(struct irRenameEngine* engine, enum irRegister reg);

#define irRenameEngine_get_mem_order(engine) ((engine)->mem_op_order ++)

void irRenameEngine_set_register_ref(struct irRenameEngine* engine, enum irRegister reg, struct node* node);

void irRenameEngine_tag_final_node(struct irRenameEngine* engine);

#endif