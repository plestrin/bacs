#include <stdlib.h>
#include <stdio.h>

#include "irVariableSize.h"

#include "dagPartialOrder.h"

const uint8_t irRegisterSize[NB_IR_REGISTER] = {
	32, 	/* IR_REG_EAX 	*/
	16, 	/* IR_REG_AX 	*/
	8, 		/* IR_REG_AH 	*/
	8, 		/* IR_REG_AL 	*/
	32, 	/* IR_REG_EBX 	*/
	16, 	/* IR_REG_BX 	*/
	8, 		/* IR_REG_BH 	*/
	8, 		/* IR_REG_BL 	*/
	32, 	/* IR_REG_ECX 	*/
	16, 	/* IR_REG_CX 	*/
	8, 		/* IR_REG_CH 	*/
	8, 		/* IR_REG_CL 	*/
	32, 	/* IR_REG_EDX 	*/
	16, 	/* IR_REG_DX 	*/
	8, 		/* IR_REG_DH 	*/
	8, 		/* IR_REG_DL 	*/
	32, 	/* IR_REG_ESP 	*/
	32, 	/* IR_REG_EBP 	*/
	32, 	/* IR_REG_ESI 	*/
	32 		/* IR_REG_EDI 	*/
};

static enum irRegister irRegister_resize(enum irRegister reg, uint8_t size){
	switch(reg){
		case IR_REG_EAX  : {
			if (size == 8){
				return IR_REG_AL; /* IR_REG_AH ? */
			}
			else if (size == 16){
				return IR_REG_AX;
			}
			else{
				printf("ERROR: in %s, %u is an incorrect size for EAX\n", __func__, size);
			}
			break;
		}
		case IR_REG_AX : {
			if (size == 8){
				return IR_REG_AL; /* IR_REG_AH ? */
			}
			else{
				printf("ERROR: in %s, %u is an incorrect size for AX\n", __func__, size);
			}
			break;
		}
		case IR_REG_EBX  : {
			if (size == 8){
				return IR_REG_BL; /* IR_REG_BH ? */
			}
			else if (size == 16){
				return IR_REG_BX;
			}
			else{
				printf("ERROR: in %s, %u is an incorrect size for EBX\n", __func__, size);
			}
			break;
		}
		case IR_REG_BX : {
			if (size == 8){
				return IR_REG_BL; /* IR_REG_BH ? */
			}
			else{
				printf("ERROR: in %s, %u is an incorrect size for BX\n", __func__, size);
			}
			break;
		}
		case IR_REG_ECX  : {
			if (size == 8){
				return IR_REG_CL; /* IR_REG_CH ? */
			}
			else if (size == 16){
				return IR_REG_CX;
			}
			else{
				printf("ERROR: in %s, %u is an incorrect size for ECX\n", __func__, size);
			}
			break;
		}
		case IR_REG_CX : {
			if (size == 8){
				return IR_REG_CL; /* IR_REG_CH ? */
			}
			else{
				printf("ERROR: in %s, %u is an incorrect size for CX\n", __func__, size);
			}
			break;
		}
		case IR_REG_EDX  : {
			if (size == 8){
				return IR_REG_DL; /* IR_REG_DH ? */
			}
			else if (size == 16){
				return IR_REG_DX;
			}
			else{
				printf("ERROR: in %s, %u is an incorrect size for EDX\n", __func__, size);
			}
			break;
		}
		case IR_REG_DX : {
			if (size == 8){
				return IR_REG_DL; /* IR_REG_DH ? */
			}
			else{
				printf("ERROR: in %s, %u is an incorrect size for DX\n", __func__, size);
			}
			break;
		}
		default : {
			printf("ERROR: in %s, register %s cannot be resized\n", __func__, irRegister_2_string(reg));
		}
	}

	return reg;
}

#define NB_TEMPLATE 4

static const uint64_t template[NB_TEMPLATE][2] = {
	{0x0000000000000000, 0x0000000000000000},
	{0x0000000000000000, 0x00000000000000ff},
	{0x0000000000000000, 0x000000000000ffff},
	{0x0000000000000000, 0x00000000ffffffff}
};

#define test_template(mask, template, index) (((mask)->p1 | template[index][1]) == template[index][1] && ((mask)->p2 | template[index][0]) == template[index][0])

static const uint8_t templateSize[NB_TEMPLATE] = {0, 8, 16, 32};

static inline void mask_set_full_size(struct mask* mask, uint8_t size){
	if (size > 64){
		mask->p1 = 0xffffffffffffffff;
		mask->p2 = 0xffffffffffffffff >> (128 - size);
	}
	else{
		mask->p1 = 0xffffffffffffffff >> (64 - size);
		mask->p2 = 0x0000000000000000;
	}
}

static inline void mask_limit_to_size(struct mask* mask, uint8_t size){
	if (size > 64){
		mask->p2 &= 0xffffffffffffffff >> (128 - size);
	}
	else{
		mask->p1 &= 0xffffffffffffffff >> (64 - size);
		mask->p2 = 0x0000000000000000;
	}
}

static void mask_get_src(struct node* node, struct mask* mask);

static void modify_mask_ins_and(struct node* node, struct mask* mask);
static void modify_mask_ins_shl(struct node* node, struct mask* mask);
static void modify_mask_ins_shr(struct node* node, struct mask* mask);

struct mask* irVariableSize_propogate_mask(struct ir* ir){
	uint32_t 				i;
	struct node* 			node_cursor;
	struct irOperation* 	operation_cursor;
	struct mask* 			mask_buffer;

	mask_buffer = (struct mask*)calloc(ir->graph.nb_node, sizeof(struct mask));
	if (mask_buffer == NULL){
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
		return NULL;
	}

	if (dagPartialOrder_sort_dst_src(&(ir->graph))){
		printf("ERROR: in %s, unable to sort DAG\n", __func__);
	}

	for (node_cursor = graph_get_head_node(&(ir->graph)), i = 0; node_cursor != NULL; node_cursor = node_get_next(node_cursor), i++){
		operation_cursor = ir_node_get_operation(node_cursor);

		node_cursor->ptr = mask_buffer + i;

		if (operation_cursor->status_flag & IR_NODE_STATUS_FLAG_FINAL){
			mask_set_full_size(mask_buffer + i, operation_cursor->size);
		}

		switch(operation_cursor->type){
			case IR_OPERATION_TYPE_IMM 		:
			case IR_OPERATION_TYPE_IN_REG 	: {
				mask_get_src(node_cursor, mask_buffer + i);
				break;
			}
			case IR_OPERATION_TYPE_IN_MEM 	: {
				mask_get_src(node_cursor, mask_buffer + i);
				break;
			}
			case IR_OPERATION_TYPE_OUT_MEM 	: {
				mask_set_full_size(mask_buffer + i, operation_cursor->size);
				break;
			}
			case IR_OPERATION_TYPE_INST 	: {
				mask_get_src(node_cursor, mask_buffer + i);
				switch(operation_cursor->operation_type.inst.opcode){
					case IR_AND : {
						modify_mask_ins_and(node_cursor, mask_buffer + i);
						break;
					}
					case IR_SHL : {
						modify_mask_ins_shl(node_cursor, mask_buffer + i);
						break;
					}
					case IR_SHR : {
						modify_mask_ins_shr(node_cursor, mask_buffer + i);
						break;
					}
					default  	: {
						break;
					}
				}
				mask_limit_to_size(mask_buffer + i, operation_cursor->size);
				break;
			}
			case IR_OPERATION_TYPE_SYMBOL 	: {
				printf("WARNING: in %s, this case is not supported SYMBOL_NODE\n", __func__);
				break;
			}
		}
	}

	return mask_buffer;
}

static void mask_get_src(struct node* node, struct mask* mask){
	struct edge* 		edge_cursor;
	struct irOperation* operation_cursor;
	struct mask* 		dst_mask;

	for (edge_cursor = node_get_head_edge_src(node); edge_cursor != NULL; edge_cursor = edge_get_next_src(edge_cursor)){
		if (ir_edge_get_dependence(edge_cursor)->type != IR_DEPENDENCE_TYPE_ADDRESS){
			operation_cursor = ir_node_get_operation(edge_get_dst(edge_cursor));
			dst_mask = (struct mask*)(edge_get_dst(edge_cursor)->ptr);
			if (operation_cursor->type == IR_OPERATION_TYPE_INST){
				switch(operation_cursor->operation_type.inst.opcode){
					case IR_PART2_8 : {
						mask->p1 |= dst_mask->p1 << 8;
						break;
					}
					default 		: {
						mask->p1 |= dst_mask->p1;
						mask->p2 |= dst_mask->p2;
						break;
					}
				}
			}
			else{
				mask->p1 |= dst_mask->p1;
				mask->p2 |= dst_mask->p2;
			}
		}
		else{
			mask->p1 |= 0x00000000ffffffff;
		}
	}
}

static void modify_mask_ins_and(struct node* node, struct mask* mask){
	struct edge* 		edge_cursor;
	struct irOperation* operation_cursor;

	for (edge_cursor = node_get_head_edge_dst(node); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
		operation_cursor = ir_node_get_operation(edge_get_src(edge_cursor));
		if (operation_cursor->type == IR_OPERATION_TYPE_IMM){
			mask->p1 &= ir_imm_operation_get_unsigned_value(operation_cursor);
		}
	}
}

static void modify_mask_ins_shl(struct node* node, struct mask* mask){
	struct edge* 		edge_cursor;
	struct irOperation* operation_cursor;
	uint8_t 			shift;

	for (edge_cursor = node_get_head_edge_dst(node); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
		if (ir_edge_get_dependence(edge_cursor)->type == IR_DEPENDENCE_TYPE_SHIFT_DISP){
			operation_cursor = ir_node_get_operation(edge_get_src(edge_cursor));
			if (operation_cursor->type == IR_OPERATION_TYPE_IMM){
				shift = ir_imm_operation_get_unsigned_value(operation_cursor);
				mask->p2 = (mask->p2 << shift) | (mask->p1 >> (64 - shift));
				mask->p1 = mask->p1 << shift;

				return;
			}
		}
	}
}

static void modify_mask_ins_shr(struct node* node, struct mask* mask){
	struct edge* 		edge_cursor;
	struct irOperation* operation_cursor;
	uint8_t 			shift;

	for (edge_cursor = node_get_head_edge_dst(node); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
		if (ir_edge_get_dependence(edge_cursor)->type == IR_DEPENDENCE_TYPE_SHIFT_DISP){
			operation_cursor = ir_node_get_operation(edge_get_src(edge_cursor));
			if (operation_cursor->type == IR_OPERATION_TYPE_IMM){
				shift = ir_imm_operation_get_unsigned_value(operation_cursor);
				mask->p1 = (mask->p1 >> shift) | (mask->p2 << (64 - shift));
				mask->p2 = mask->p2 >> shift;

				return;
			}
		}
	}
}

void irVariableSize_shrink(struct ir* ir){
	struct node* 		node_cursor;
	struct node* 		next_node_cursor;
	struct mask* 		mask_cursor;
	struct irOperation* operation_cursor;
	uint8_t 			i;

	for(node_cursor = graph_get_tail_node(&(ir->graph)), next_node_cursor = NULL; node_cursor != NULL;){
		mask_cursor = (struct mask*)node_cursor->ptr;
		operation_cursor = ir_node_get_operation(node_cursor);

		for (i = 0; i < NB_TEMPLATE; i++){
			if (test_template(mask_cursor, template, i)){
				if (templateSize[i] == 0){
					ir_remove_node(ir, node_cursor);
				}
				else if (templateSize[i] < operation_cursor->size){
					switch(operation_cursor->type){
						case IR_OPERATION_TYPE_IN_REG 	: {
							operation_cursor->operation_type.in_reg.reg = irRegister_resize(operation_cursor->operation_type.in_reg.reg, templateSize[i]);
							operation_cursor->size = irRegister_get_size(operation_cursor->operation_type.in_reg.reg);
							break;
						}
						case IR_OPERATION_TYPE_IN_MEM 	: {
							operation_cursor->size = templateSize[i];
							break;
						}
						case IR_OPERATION_TYPE_OUT_MEM 	: {
							printf("ERROR: in %s, this case is not supposed to happen (shrink OUT MEM)\n", __func__);
							break;
						}
						case IR_OPERATION_TYPE_IMM  	: {
							operation_cursor->size = templateSize[i];
							break;
						}
						case IR_OPERATION_TYPE_INST 	: {
							switch(operation_cursor->operation_type.inst.opcode){
								case IR_MOVZX 		: {
									printf("WARNING: in %s, this case is not implemented, trying to resize MOVZX\n", __func__);
									break;
								}
								case IR_PART1_16 	: {
									printf("WARNING: in %s, this case is not implemented, trying to resize PART1_16\n", __func__);
									break;
								}
								default 			: {
									operation_cursor->size = templateSize[i];
									break;
								}
							}
							break;
						}
						case IR_OPERATION_TYPE_SYMBOL 	: {
							printf("ERROR: in %s, this case is not supposed to happen (shrink SYMBOL)\n", __func__);
							break;
						}
					}
				}
				break;
			}
		}

		if (next_node_cursor != NULL){
			if (node_get_prev(next_node_cursor) != node_cursor){
				node_cursor = node_get_prev(next_node_cursor);
			}
			else{
				next_node_cursor = node_cursor;
				node_cursor = node_get_prev(node_cursor);
			}
		}
		else{
			if (graph_get_tail_node(&(ir->graph)) != node_cursor){
				node_cursor = graph_get_tail_node(&(ir->graph));
			}
			else{
				next_node_cursor = node_cursor;
				node_cursor = node_get_prev(node_cursor);
			}
		}
	}
}

void irVariableSize_adapt(struct ir* ir){
	struct node* 		node_cursor;
	struct irOperation* operation_cursor;
	struct edge* 		edge_cursor;
	struct edge* 		edge_current;
	struct node* 		parent_node;
	struct irOperation* parent_operation;
	struct node* 		new_ins;

	for (node_cursor = graph_get_head_node(&(ir->graph)); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
		operation_cursor = ir_node_get_operation(node_cursor);

		if (operation_cursor->type != IR_OPERATION_TYPE_INST && operation_cursor->type != IR_OPERATION_TYPE_OUT_MEM){
			continue;
		}

		for (edge_cursor = node_get_head_edge_dst(node_cursor); edge_cursor != NULL;){
			edge_current = edge_cursor;
			edge_cursor = edge_get_next_dst(edge_cursor);

			parent_node = edge_get_src(edge_current);
			parent_operation = ir_node_get_operation(parent_node);

			if (operation_cursor->size != parent_operation->size){
				/* Discard case where size mismatch is OK */
				if (operation_cursor->type == IR_OPERATION_TYPE_OUT_MEM && ir_edge_get_dependence(edge_current)->type == IR_DEPENDENCE_TYPE_ADDRESS && parent_operation->size == 32){
					continue;
				}

				if (operation_cursor->type == IR_OPERATION_TYPE_INST && (operation_cursor->operation_type.inst.opcode == IR_SHL || operation_cursor->operation_type.inst.opcode == IR_SHR || operation_cursor->operation_type.inst.opcode == IR_ROR) && ir_edge_get_dependence(edge_current)->type == IR_DEPENDENCE_TYPE_SHIFT_DISP && parent_operation->size == 8){
					continue;
				}

				if (operation_cursor->type == IR_OPERATION_TYPE_INST && operation_cursor->operation_type.inst.opcode == IR_MOVZX && operation_cursor->size == 32 && parent_operation->size == 8){
					continue;
				}

				if (operation_cursor->type == IR_OPERATION_TYPE_INST && (operation_cursor->operation_type.inst.opcode == IR_PART1_8 || operation_cursor->operation_type.inst.opcode == IR_PART2_8) && operation_cursor->size == 8 && parent_operation->size == 32){
					continue;
				}

				if (operation_cursor->type == IR_OPERATION_TYPE_INST && operation_cursor->operation_type.inst.opcode == IR_PART1_16 && operation_cursor->size == 16 && parent_operation->size == 32){
					continue;
				}

				/* Add instruction to convert size */
				if (operation_cursor->size == 32 && parent_operation->size == 8){
					new_ins = ir_add_inst(ir, IR_MOVZX, 32);
				}
				else if (operation_cursor->size == 8 && parent_operation->size == 32){
					new_ins = ir_add_inst(ir, IR_PART1_8, 8);
				}
				else{
					printf("ERROR: in %s, this case is not implemented, size mismatch %u -> %u\n", __func__, parent_operation->size, operation_cursor->size);
					continue;
				}

				if (new_ins == NULL){
					printf("ERROR: in %s, unable to add operation to IR\n", __func__);
				}
				else{
					ir_add_dependence(ir, parent_node, new_ins, IR_DEPENDENCE_TYPE_DIRECT);
					ir_add_dependence(ir, new_ins, node_cursor, IR_DEPENDENCE_TYPE_DIRECT);
					ir_remove_dependence(ir, edge_current);
				}
			}
		}
	}
}

