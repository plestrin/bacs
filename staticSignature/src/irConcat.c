#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "irConcat.h"
#include "irImporterAsm.h"
#include "base.h"

struct irCopyArg{
	uint32_t 				seed;
	uint32_t 				order;
	struct node* 			prev_mem_access;
	struct node* 			next_mem_access;
	int32_t 				off_dst;
	uint32_t* 				stack;
	uint32_t 				stack_ptr;
	uint32_t 				max_dst;
};

int32_t irOperation_copy(void* data_dst, const void* data_src, void* arg){
	struct irOperation* op_dst = (struct irOperation*)data_dst;
	struct irOperation* op_src = (struct irOperation*)data_src;
	struct irCopyArg* 	copy_arg = (struct irCopyArg*)arg;

	memcpy(op_dst, op_src, sizeof(struct irOperation));

	switch(op_dst->type){
		case IR_OPERATION_TYPE_IN_REG 	: {
			break;
		}
		case IR_OPERATION_TYPE_IN_MEM 	: 
		case IR_OPERATION_TYPE_OUT_MEM 	: {
			if (op_dst->operation_type.mem.prev == NULL){
				if (copy_arg->prev_mem_access != NULL){
					op_dst->operation_type.mem.prev = copy_arg->prev_mem_access;
					ir_node_get_operation(copy_arg->prev_mem_access)->operation_type.mem.next =  irOperation_get_node(data_dst);
				}
			}
			else if (op_dst->operation_type.mem.prev->ptr != NULL){
				ir_node_get_operation(op_dst->operation_type.mem.prev->ptr)->operation_type.mem.next = irOperation_get_node(data_dst);
				op_dst->operation_type.mem.prev = op_dst->operation_type.mem.prev->ptr;
			}
			if (op_dst->operation_type.mem.next == NULL){
				copy_arg->next_mem_access = irOperation_get_node(data_dst);
			}
			else if (op_dst->operation_type.mem.next->ptr != NULL){
				ir_node_get_operation(op_dst->operation_type.mem.next->ptr)->operation_type.mem.prev = irOperation_get_node(data_dst);
				op_dst->operation_type.mem.next = op_dst->operation_type.mem.next->ptr;
			}
			op_dst->operation_type.mem.order += copy_arg->order;
			break;
		}
		case IR_OPERATION_TYPE_IMM 		: {
			break;
		}
		case IR_OPERATION_TYPE_INST 	: {
			copy_arg->max_dst = max(copy_arg->max_dst, op_dst->operation_type.inst.dst);
			if (op_dst->operation_type.inst.dst <= IR_CALL_STACK_PTR){
				if (IR_CALL_STACK_PTR - op_dst->operation_type.inst.dst > copy_arg->stack_ptr){
					log_err("the bottom of the stack has been reached");
					op_dst->operation_type.inst.dst = IR_OPERATION_DST_UNKOWN;
				}
				else{
					op_dst->operation_type.inst.dst = copy_arg->stack[copy_arg->stack_ptr - (IR_CALL_STACK_PTR - op_dst->operation_type.inst.dst)];
				}
			}
			else{
				op_dst->operation_type.inst.dst 	= op_dst->operation_type.inst.dst + copy_arg->off_dst;
			}
			op_dst->operation_type.inst.seed 		= copy_arg->seed;
			break;
		}
		case IR_OPERATION_TYPE_SYMBOL 	: {
			op_dst->operation_type.symbol.result 	= NULL;
			op_dst->operation_type.symbol.index 	= 0;
		}
	}

	op_dst->status_flag &= ~IR_OPERATION_STATUS_FLAG_FINAL;

	return 0;
}

#pragma GCC diagnostic ignored "-Wunused-parameter"
int32_t irDependence_copy(void* data_dst, const void* data_src, void* arg){
	memcpy(data_dst, data_src, sizeof(struct irDependence));
	return 0;
}

int32_t ir_concat(struct ir* ir_dst, struct ir* ir_src, struct irRenameEngine* engine){
	struct irCopyArg 	copy_arg;
	struct node* 		node_cursor;
	struct irOperation* operation_cursor;
	struct node* 		ref;

	copy_arg.seed 				= ir_dst->range_seed;
	copy_arg.prev_mem_access 	= irRenameEngine_get_mem_order(engine);
	copy_arg.next_mem_access 	= irRenameEngine_get_mem_order(engine);
	if (copy_arg.prev_mem_access != NULL){
		copy_arg.order 			= ir_node_get_operation(copy_arg.prev_mem_access)->operation_type.mem.order;
	}
	else{
		copy_arg.order 			= 1;
	}
	copy_arg.off_dst 			= irRenameEngine_get_call_id(engine) - IR_CALL_STACK_PTR;
	copy_arg.stack 				= engine->ir->stack;
	copy_arg.stack_ptr 			= engine->ir->stack_ptr;
	copy_arg.max_dst 			= 0;

	if (graph_concat(&(ir_dst->graph), &(ir_src->graph), irOperation_copy, irDependence_copy, &copy_arg)){
		log_err("unable to concat graph");
		return - 1;
	}

	for (node_cursor = graph_get_head_node(&(ir_src->graph)); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
		operation_cursor = ir_node_get_operation(node_cursor->ptr);

		if ((operation_cursor->type == IR_OPERATION_TYPE_IN_REG) && (operation_cursor->operation_type.in_reg.primer == IR_IN_REG_IS_PRIMER)){
			ref = irRenameEngine_get_register_ref(engine, operation_cursor->operation_type.in_reg.reg, operation_cursor->index, IR_OPERATION_DST_UNKOWN);
			if (ref == NULL){
				log_err("unable to register reference from the renaming engine");
			}
			else{
				graph_transfert_src_edge(&(ir_dst->graph), ref, (struct node*)node_cursor->ptr);
				ir_remove_node(ir_dst, (struct node*)node_cursor->ptr);
				node_cursor->ptr = ref;
			}
		}
	}

	irRenameEngine_propagate_alias(engine, ir_src->alias_buffer);
	irRenameEngine_set_mem_order(engine, copy_arg.next_mem_access);
	irRenameEngine_update_call_stack(engine, ir_src->stack, ir_src->stack_ptr);
	engine->func_id += copy_arg.max_dst - IR_CALL_STACK_PTR;

	ir_drop_range(ir_dst);

	return 0;
}