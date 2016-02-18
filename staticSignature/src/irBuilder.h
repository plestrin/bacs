#ifndef IRBUILDER_H
#define IRBUILDER_H

#include <string.h>

#include "graph.h"
#include "ir.h"

#define IRBUILDER_TYPE_READ 	0x00
#define IRBUILDER_TYPE_WRITE 	0x01
#define IRBUILDER_TYPE_EXTEND 	0x02
#define IRBUILDER_TYPE_USED 	0x04

#define alias_set_type(alias, type_) ((alias).type = (type_) | IRBUILDER_TYPE_USED)

#define alias_is_read(alias) 	(((alias).type & 0x03) == IRBUILDER_TYPE_READ)
#define alias_is_write(alias) 	(((alias).type & 0x03) == IRBUILDER_TYPE_WRITE)
#define alias_is_extend(alias) 	(((alias).type & 0x03) == IRBUILDER_TYPE_EXTEND)
#define alias_is_primer(alias) 	(((alias).type & IRBUILDER_TYPE_USED) == 0)

#define SIMD_DEFAULT_FRAG_SIZE 32

void irBuilder_init(struct irBuilder* builder);

struct node* irBuilder_get_std_register_ref(struct irBuilder* builder, struct ir* ir, enum irRegister reg, uint32_t instruction_index);
struct node* irBuilder_get_simd_register_ref(struct irBuilder* builder, struct ir* ir, enum irRegister reg, uint32_t instruction_index);

static inline struct node* irBuilder_get_register_ref(struct irBuilder* builder, struct ir* ir, enum irRegister reg, uint32_t instruction_index){
	if (irRegister_is_std(reg)){
		return irBuilder_get_std_register_ref(builder, ir, reg, instruction_index);
	}
	else{
		return irBuilder_get_simd_register_ref(builder, ir, reg, instruction_index);
	}
}

#define irBuilder_get_mem_order(builder) ((builder)->prev_mem_access)
#define irBuilder_set_mem_order(builder, mem_access_) (builder)->prev_mem_access = mem_access_
#define irBuilder_get_reg_order(builder) ((builder)->reg_op_order ++)

int32_t irBuilder_change_simd_frag(struct irBuilder* builder, struct ir* ir, enum irVirtualRegister vreg, uint8_t frag_size, uint32_t instruction_index);

void irBuilder_set_std_register_ref(struct irBuilder* builder, enum irRegister reg, struct node* node);
void irBuilder_set_simd_register_ref(struct irBuilder* builder, enum irRegister reg, struct node* node);

static inline void irBuilder_set_register_ref(struct irBuilder* builder, enum irRegister reg, struct node* node){
	#ifdef EXTRA_CHECK
	if (ir_node_get_operation(node)->type == IR_OPERATION_TYPE_OUT_MEM){
		log_err("wrong operation type");
	}
	#endif

	if (irRegister_is_std(reg)){
		irBuilder_set_std_register_ref(builder, reg, node);
	}
	else{
		irBuilder_set_simd_register_ref(builder, reg, node);
	}
}

#define irBuilder_get_vir_register_frag_size(builder, vreg) (builder)->simdAlias_buffer[(vreg) - IR_VIR_REGISTER_OFFSET].frag_size

#define irBuilder_clear_eax_std_call(builder) 												\
	(builder)->alias_buffer[IR_REG_EAX].ir_node = NULL; 									\
	(builder)->alias_buffer[IR_REG_AX].ir_node  = NULL; 									\
	(builder)->alias_buffer[IR_REG_AH].ir_node  = NULL; 									\
	(builder)->alias_buffer[IR_REG_AL].ir_node  = NULL;

void irBuilder_tag_final_node(struct irBuilder* builder);
void irBuilder_chg_final_node(struct irBuilder* builder, struct node* node_old, struct node* node_new);

#define irBuilder_del_final_node(builder, node) 											\
	log_warn("deleting final node, this fragment should not be used to build compound IR"); \
	irBuilder_chg_final_node(builder, node, NULL);

void irBuilder_propagate_alias(struct irBuilder* builder_dst, struct ir* ir_dst, const struct irBuilder* builder_src);

#define irBuilder_increment_call_stack(builder) 											\
	if ((builder)->stack_ptr + 1 == IR_CALL_STACK_MAX_SIZE){ 								\
		log_err("the top of the stack has been reached"); 									\
	} 																						\
	else{ 																					\
		(builder)->stack[++ (builder)->stack_ptr] = (builder)->func_id ++; 					\
	}

#define irBuilder_get_call_id(builder) ((builder)->stack[(builder)->stack_ptr])

#define irBuilder_decrement_call_stack(builder) 											\
	if ((builder)->stack_ptr == 0){ 														\
		log_err("the bottom of the stack has been reached"); 								\
	} 																						\
	else{ 																					\
		(builder)->stack_ptr --; 															\
	}

void irBuilder_update_call_stack(struct irBuilder* dst_builder, const struct irBuilder* src_builder);

#endif