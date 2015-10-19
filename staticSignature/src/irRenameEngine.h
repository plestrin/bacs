#ifndef IRRENAMEENGINE_H
#define IRRENAMEENGINE_H

#include <string.h>

#include "graph.h"
#include "ir.h"

#define IRRENAMEENGINE_TYPE_READ 	0x00
#define IRRENAMEENGINE_TYPE_WRITE 	0x01
#define IRRENAMEENGINE_TYPE_EXTEND 	0x02
#define IRRENAMEENGINE_TYPE_USED 	0x04

#define alias_set_type(alias, type_) ((alias).type = (type_) | IRRENAMEENGINE_TYPE_USED)

#define alias_is_read(alias) 	(((alias).type & 0x03) == IRRENAMEENGINE_TYPE_READ)
#define alias_is_write(alias) 	(((alias).type & 0x03) == IRRENAMEENGINE_TYPE_WRITE)
#define alias_is_extend(alias) 	(((alias).type & 0x03) == IRRENAMEENGINE_TYPE_EXTEND)
#define alias_is_primer(alias) 	(((alias).type & IRRENAMEENGINE_TYPE_USED) == 0)

struct irRenameEngine{
	struct node* 		prev_mem_access;
	uint32_t 			reg_op_order;
	struct ir* 			ir;
};

#define irRenameEngine_init(engine, ir_) 													\
	memset(&((ir_)->alias_buffer), 0, sizeof(struct alias) * NB_IR_REGISTER); 				\
	(engine).prev_mem_access = NULL; 														\
	(engine).reg_op_order = 1; 																\
	(engine).ir = (ir_);

struct node* irRenameEngine_get_register_ref(struct irRenameEngine* engine, enum irRegister reg, uint32_t instruction_index, uint32_t dst);

#define irRenameEngine_get_mem_order(engine) ((engine)->prev_mem_access)
#define irRenameEngine_set_mem_order(engine, mem_access_) (engine)->prev_mem_access = mem_access_
#define irRenameEngine_get_reg_order(engine) ((engine)->reg_op_order ++)

void irRenameEngine_set_register_ref(struct irRenameEngine* engine, enum irRegister reg, struct node* node);

#define irRenameEngine_clear_eax_std_call(engine) 											\
	(engine).ir->alias_buffer[IR_REG_EAX].ir_node = NULL; 									\
	(engine).ir->alias_buffer[IR_REG_AX].ir_node  = NULL; 									\
	(engine).ir->alias_buffer[IR_REG_AH].ir_node  = NULL; 									\
	(engine).ir->alias_buffer[IR_REG_AL].ir_node  = NULL;

void irRenameEngine_tag_final_node(struct irRenameEngine* engine);
void irRenameEngine_delete_node(struct alias* alias_buffer, struct node* node);
void irRenameEngine_propagate_alias(struct irRenameEngine* engine_dst, struct alias* alias_buffer_src);

#endif