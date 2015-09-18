#include <stdlib.h>
#include <stdio.h>
#include <search.h>
#include <string.h>

#include "irVariableSize.h"
#include "dagPartialOrder.h"
#include "base.h"

const uint8_t irRegisterSize[NB_IR_REGISTER] = {
	32, 	/* IR_REG_EAX 		*/
	16, 	/* IR_REG_AX 		*/
	8, 		/* IR_REG_AH 		*/
	8, 		/* IR_REG_AL 		*/
	32, 	/* IR_REG_EBX 		*/
	16, 	/* IR_REG_BX 		*/
	8, 		/* IR_REG_BH 		*/
	8, 		/* IR_REG_BL 		*/
	32, 	/* IR_REG_ECX 		*/
	16, 	/* IR_REG_CX 		*/
	8, 		/* IR_REG_CH 		*/
	8, 		/* IR_REG_CL 		*/
	32, 	/* IR_REG_EDX 		*/
	16, 	/* IR_REG_DX 		*/
	8, 		/* IR_REG_DH 		*/
	8, 		/* IR_REG_DL 		*/
	32, 	/* IR_REG_ESP 		*/
	16, 	/* IR_REG_SP 		*/
	32, 	/* IR_REG_EBP 		*/
	16, 	/* IR_REG_BP 		*/
	32, 	/* IR_REG_ESI 		*/
	16, 	/* IR_REG_SI 		*/
	32, 	/* IR_REG_EDI 		*/
	16, 	/* IR_REG_DI 		*/
	32, 	/* IR_REG_XMM1_1 	*/
	32, 	/* IR_REG_XMM1_2 	*/
	32, 	/* IR_REG_XMM1_3 	*/
	32, 	/* IR_REG_XMM1_4 	*/
	32, 	/* IR_REG_XMM2_1 	*/
	32, 	/* IR_REG_XMM2_2 	*/
	32, 	/* IR_REG_XMM2_3 	*/
	32, 	/* IR_REG_XMM2_4 	*/
	32, 	/* IR_REG_XMM3_1 	*/
	32, 	/* IR_REG_XMM3_2 	*/
	32, 	/* IR_REG_XMM3_3 	*/
	32, 	/* IR_REG_XMM3_4 	*/
	32, 	/* IR_REG_XMM4_1 	*/
	32, 	/* IR_REG_XMM4_2 	*/
	32, 	/* IR_REG_XMM4_3 	*/
	32, 	/* IR_REG_XMM4_4 	*/
	32, 	/* IR_REG_XMM5_1 	*/
	32, 	/* IR_REG_XMM5_2 	*/
	32, 	/* IR_REG_XMM5_3 	*/
	32, 	/* IR_REG_XMM5_4 	*/
	32, 	/* IR_REG_XMM6_1 	*/
	32, 	/* IR_REG_XMM6_2 	*/
	32, 	/* IR_REG_XMM6_3 	*/
	32, 	/* IR_REG_XMM6_4 	*/
	32, 	/* IR_REG_XMM7_1 	*/
	32, 	/* IR_REG_XMM7_2 	*/
	32, 	/* IR_REG_XMM7_3 	*/
	32, 	/* IR_REG_XMM7_4 	*/
	32, 	/* IR_REG_XMM8_1 	*/
	32, 	/* IR_REG_XMM8_2 	*/
	32, 	/* IR_REG_XMM8_3 	*/
	32, 	/* IR_REG_XMM8_4 	*/
	32, 	/* IR_REG_XMM9_1 	*/
	32, 	/* IR_REG_XMM9_2 	*/
	32, 	/* IR_REG_XMM9_3 	*/
	32, 	/* IR_REG_XMM9_4 	*/
	32, 	/* IR_REG_XMM10_1 	*/
	32, 	/* IR_REG_XMM10_2 	*/
	32, 	/* IR_REG_XMM10_3 	*/
	32, 	/* IR_REG_XMM10_4 	*/
	32, 	/* IR_REG_XMM11_1 	*/
	32, 	/* IR_REG_XMM11_2 	*/
	32, 	/* IR_REG_XMM11_3 	*/
	32, 	/* IR_REG_XMM11_4 	*/
	32, 	/* IR_REG_XMM12_1 	*/
	32, 	/* IR_REG_XMM12_2 	*/
	32, 	/* IR_REG_XMM12_3 	*/
	32, 	/* IR_REG_XMM12_4 	*/
	32, 	/* IR_REG_XMM13_1 	*/
	32, 	/* IR_REG_XMM13_2 	*/
	32, 	/* IR_REG_XMM13_3 	*/
	32, 	/* IR_REG_XMM13_4 	*/
	32, 	/* IR_REG_XMM14_1 	*/
	32, 	/* IR_REG_XMM14_2 	*/
	32, 	/* IR_REG_XMM14_3 	*/
	32, 	/* IR_REG_XMM14_4 	*/
	32, 	/* IR_REG_XMM15_1 	*/
	32, 	/* IR_REG_XMM15_2 	*/
	32, 	/* IR_REG_XMM15_3 	*/
	32, 	/* IR_REG_XMM15_4 	*/
	32, 	/* IR_REG_XMM16_1 	*/
	32, 	/* IR_REG_XMM16_2 	*/
	32, 	/* IR_REG_XMM16_3 	*/
	32, 	/* IR_REG_XMM16_4 	*/
	32, 	/* IR_REG_MMX1_1 	*/
	32, 	/* IR_REG_MMX1_2 	*/
	32, 	/* IR_REG_MMX2_1 	*/
	32, 	/* IR_REG_MMX2_2 	*/
	32, 	/* IR_REG_MMX3_1 	*/
	32, 	/* IR_REG_MMX3_2 	*/
	32, 	/* IR_REG_MMX4_1 	*/
	32, 	/* IR_REG_MMX4_2 	*/
	32, 	/* IR_REG_MMX5_1 	*/
	32, 	/* IR_REG_MMX5_2 	*/
	32, 	/* IR_REG_MMX6_1 	*/
	32, 	/* IR_REG_MMX6_2 	*/
	32, 	/* IR_REG_MMX7_1  	*/
	32, 	/* IR_REG_MMX7_2  	*/
	32, 	/* IR_REG_MMX8_1  	*/
	32, 	/* IR_REG_MMX8_2  	*/
	0 		/* IR_REG_TMP 		*/
};

static void irVariableSize_remove_size_convertor(struct ir* ir);
static void irVariableSize_add_size_convertor(struct ir* ir);

static void irVariableSize_remove_size_convertor(struct ir* ir){
	struct node* 		node_cursor;
	struct node* 		prev_node_cursor;
	struct irOperation* operation_cursor;

	for (node_cursor = graph_get_head_node(&(ir->graph)), prev_node_cursor = NULL; node_cursor != NULL;){
		operation_cursor = ir_node_get_operation(node_cursor);

		if (operation_cursor->type != IR_OPERATION_TYPE_INST){
			goto next;
		}

		switch (operation_cursor->operation_type.inst.opcode){
			case IR_MOVZX 		: {
				struct node* operand;
				struct node* mask;

				operand = edge_get_src(node_get_head_edge_dst(node_cursor));
				if (ir_node_get_operation(operand)->size == operation_cursor->size){
					if ((uint32_t)(operand->ptr) != operation_cursor->size){
						mask = ir_add_immediate(ir, operation_cursor->size, 0xffffffffffffffff >> (64 - (uint32_t)(operand->ptr)));
						if (mask){
							if (ir_add_dependence(ir, mask, node_cursor, IR_DEPENDENCE_TYPE_DIRECT)){
								operation_cursor->operation_type.inst.opcode = IR_AND;
							}
							else{
								log_err("unable to add dependency to IR");
							}
						}
						else{
							log_err("unable to add immediate to IR");
						}
					}
					else{
						log_err_m("trying to remove unnecessary MOVZX instruction but operand size was incorrect %p", (void*)ir_node_get_operation(operand));
					}
				}

				break;
			}
			case IR_PART1_8 	:
			case IR_PART1_16 	: {
				struct edge* 			edge_cursor;
				struct edge* 			tmp;
				struct irOperation* 	child;
				struct node* 			operand;

				operand = edge_get_src(node_get_head_edge_dst(node_cursor));

				for (edge_cursor = node_get_head_edge_src(node_cursor); edge_cursor != NULL; ){
					child = ir_node_get_operation(edge_get_dst(edge_cursor));

					if (child->size == ir_node_get_operation(operand)->size && !(child->type == IR_OPERATION_TYPE_INST && child->operation_type.inst.opcode == IR_MOVZX)){
						if (ir_add_dependence(ir, operand, edge_get_dst(edge_cursor), ir_edge_get_dependence(edge_cursor)->type) == NULL){
							log_err("unable to add dependency to IR");
						}

						tmp = edge_cursor;
						edge_cursor = edge_get_next_src(edge_cursor);
						ir_remove_dependence(ir, tmp);
					}
					else{
						edge_cursor = edge_get_next_src(edge_cursor);
					}
				}

				break;
			}
			case IR_PART2_8 	: {
				struct edge* 			edge_cursor;
				struct edge* 			tmp;
				struct irOperation* 	child;
				struct node* 			operand;
				struct node* 			shift = NULL;

				operand = edge_get_src(node_get_head_edge_dst(node_cursor));

				for (edge_cursor = node_get_head_edge_src(node_cursor); edge_cursor != NULL;){
					child = ir_node_get_operation(edge_get_dst(edge_cursor));

					if (child->size == ir_node_get_operation(operand)->size && !(child->type == IR_OPERATION_TYPE_INST && child->operation_type.inst.opcode == IR_MOVZX)){
						if (shift == NULL){
							shift = ir_add_inst(ir, IR_OPERATION_INDEX_UNKOWN, child->size, IR_SHR, IR_OPERATION_DST_UNKOWN);
							if (shift){
								struct node* disp;

								disp = ir_add_immediate(ir, child->size, 8);
								if (disp){
									if (ir_add_dependence(ir, disp, shift, IR_DEPENDENCE_TYPE_SHIFT_DISP) == NULL){
										log_err("unable to add dependency to IR");
									}
								}
								else{
									log_err("unable to add immediate to IR");
								}
							}
							else{
								log_err("unable to add instruction to IR");
								break;
							}
						}

						if (ir_add_dependence(ir, operand, shift, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
							log_err("unable to add dependency to IR");
						}

						if (ir_add_dependence(ir, shift, edge_get_dst(edge_cursor), ir_edge_get_dependence(edge_cursor)->type) == NULL){
							log_err("unable to add dependency to IR");
						}

						tmp = edge_cursor;
						edge_cursor = edge_get_next_src(edge_cursor);
						ir_remove_dependence(ir, tmp);
					}
					else{
						edge_cursor = edge_get_next_src(edge_cursor);
					}
				}

				break;
			}
			default 			: {
				break;
			}
		}

		next:
		if (prev_node_cursor != NULL){
			if (node_get_next(prev_node_cursor) != node_cursor){
				node_cursor = node_get_next(prev_node_cursor);
			}
			else{
				prev_node_cursor = node_cursor;
				node_cursor = node_get_next(node_cursor);
			}
		}
		else{
			if (graph_get_head_node(&(ir->graph)) != node_cursor){
				node_cursor = graph_get_head_node(&(ir->graph));
			}
			else{
				prev_node_cursor = node_cursor;
				node_cursor = node_get_next(node_cursor);
			}
		}
	}
}

static void irVariableSize_add_size_convertor(struct ir* ir){
	struct node* 		node_cursor;
	struct irOperation* operation_cursor;
	struct edge* 		edge_cursor;
	struct edge* 		edge_current;
	struct node* 		operand_node;
	struct irOperation* operand_operation;
	struct node* 		new_ins;

	for (node_cursor = graph_get_head_node(&(ir->graph)); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
		operation_cursor = ir_node_get_operation(node_cursor);

		if (operation_cursor->type != IR_OPERATION_TYPE_INST && operation_cursor->type != IR_OPERATION_TYPE_OUT_MEM){
			continue;
		}

		for (edge_cursor = node_get_head_edge_dst(node_cursor); edge_cursor != NULL;){
			edge_current = edge_cursor;
			edge_cursor = edge_get_next_dst(edge_cursor);

			operand_node = edge_get_src(edge_current);
			operand_operation = ir_node_get_operation(operand_node);

			if (operation_cursor->size != operand_operation->size){
				if (operation_cursor->type == IR_OPERATION_TYPE_OUT_MEM && ir_edge_get_dependence(edge_current)->type == IR_DEPENDENCE_TYPE_ADDRESS && operand_operation->size == 32){
					continue;
				}

				if (operation_cursor->type == IR_OPERATION_TYPE_INST && (operation_cursor->operation_type.inst.opcode == IR_SHL || operation_cursor->operation_type.inst.opcode == IR_SHR || operation_cursor->operation_type.inst.opcode == IR_ROR) && ir_edge_get_dependence(edge_current)->type == IR_DEPENDENCE_TYPE_SHIFT_DISP && operand_operation->size == 8){
					continue;
				}

				if (operation_cursor->type == IR_OPERATION_TYPE_INST && operation_cursor->operation_type.inst.opcode == IR_MOVZX && valid_operand_size_ins_movzx(operation_cursor, operand_operation)){
					continue;
				}

				if (operation_cursor->type == IR_OPERATION_TYPE_INST && (operation_cursor->operation_type.inst.opcode == IR_PART1_8 || operation_cursor->operation_type.inst.opcode == IR_PART2_8) && valid_operand_size_ins_partX_8(operation_cursor, operand_operation)){
					continue;
				}

				if (operation_cursor->type == IR_OPERATION_TYPE_INST && operation_cursor->operation_type.inst.opcode == IR_PART1_16 && valid_operand_size_ins_partX_16(operation_cursor, operand_operation)){
					continue;
				}

				/* Add instruction to convert size */
				if (operation_cursor->size == 32 && operand_operation->size == 8){
					new_ins = ir_add_inst(ir, IR_OPERATION_INDEX_UNKOWN, 32, IR_MOVZX, IR_OPERATION_DST_UNKOWN);
				}
				else if (operation_cursor->size == 8 && operand_operation->size == 32){
					new_ins = ir_add_inst(ir, IR_OPERATION_INDEX_UNKOWN, 8, IR_PART1_8, IR_OPERATION_DST_UNKOWN);
				}
				else if (operation_cursor->size == 32 && operand_operation->size == 16){
					new_ins = ir_add_inst(ir, IR_OPERATION_INDEX_UNKOWN, 32, IR_MOVZX, IR_OPERATION_DST_UNKOWN);
				}
				else if (operation_cursor->size == 16 && operand_operation->size == 32){
					new_ins = ir_add_inst(ir, IR_OPERATION_INDEX_UNKOWN, 16, IR_PART1_16, IR_OPERATION_DST_UNKOWN);
				}
				else if (operation_cursor->size == 16 && operand_operation->size == 8){
					new_ins = ir_add_inst(ir, IR_OPERATION_INDEX_UNKOWN, 16, IR_MOVZX, IR_OPERATION_DST_UNKOWN);
				}
				else{
					log_err_m("this case is not implemented, size mismatch %u -> %u", operand_operation->size, operation_cursor->size);
					continue;
				}

				if (new_ins == NULL){
					log_err("unable to add operation to IR");
				}
				else{
					if (ir_add_dependence(ir, operand_node, new_ins, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
						log_err("unable to add dependency to IR");
					}

					if (ir_add_dependence(ir, new_ins, node_cursor, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
						log_err("unable to add dependency to IR");
					}

					ir_remove_dependence(ir, edge_current);
				}
			}
		}
	}
}

enum paddingStrategy{
	PADDING_OK,
	PADDING_COMPLAIN,
	PADDING_SECURE,
	PADDING_IGNORE
};

static const enum paddingStrategy insPaddingStrategy[NB_IR_OPCODE] = {
	PADDING_OK, 		/* 0  IR_ADC 		*/
	PADDING_OK, 		/* 1  IR_ADD 		*/
	PADDING_OK, 		/* 2  IR_AND 		*/
	PADDING_OK, 		/* 3  IR_CMOV 		*/
	PADDING_SECURE, 	/* 4  IR_DIVQ 		*/
	PADDING_SECURE, 	/* 5  IR_DIVR 		*/
	PADDING_SECURE, 	/* 6  IR_IDIV 		*/
	PADDING_COMPLAIN, 	/* 7  IR_IMUL 		*/
	PADDING_IGNORE, 	/* 8  IR_LEA 		- not important */
	PADDING_IGNORE, 	/* 9  IR_MOV 		- not important */
	PADDING_IGNORE, 	/* 10  IR_MOVZX 		*/
	PADDING_COMPLAIN, 	/* 11  IR_MUL 		*/
	PADDING_COMPLAIN, 	/* 12 IR_NEG 		*/
	PADDING_OK, 		/* 13 IR_NOT 		*/
	PADDING_OK, 		/* 14 IR_OR 		*/
	PADDING_IGNORE, 	/* 15 IR_PART1_8 	*/
	PADDING_IGNORE, 	/* 16 IR_PART2_8 	*/
	PADDING_IGNORE, 	/* 17 IR_PART1_16 	*/
	PADDING_COMPLAIN, 	/* 18 IR_ROL 		*/
	PADDING_COMPLAIN, 	/* 19 IR_ROR 		*/
	PADDING_COMPLAIN, 	/* 20 IR_SHL 		*/
	PADDING_COMPLAIN, 	/* 21 IR_SHLD 		*/
	PADDING_COMPLAIN, 	/* 22 IR_SHR 		*/
	PADDING_COMPLAIN, 	/* 23 IR_SHRD 		*/
	PADDING_COMPLAIN, 	/* 24 IR_SUB 		*/
	PADDING_OK, 		/* 25 IR_XOR 		*/
	PADDING_IGNORE, 	/* 26 IR_LOAD 		- not important */
	PADDING_IGNORE, 	/* 27 IR_STORE 		- not important */
	PADDING_IGNORE, 	/* 28 IR_JOKER 		- not important */
	PADDING_IGNORE, 	/* 29 IR_INVALID 	- not important */
};

void ir_normalize_expand_variable(struct ir* ir, uint8_t* modification){
	struct node* 			node_cursor;
	struct irOperation* 	operation_cursor;
	uint8_t 				local_modification = 0;

	if (dagPartialOrder_sort_dst_src(&(ir->graph))){
		log_err("unable to sort DAG");
		return;
	}

	for (node_cursor = graph_get_head_node(&(ir->graph)); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
		operation_cursor = ir_node_get_operation(node_cursor);
		node_cursor->ptr = (void*)((uint32_t)(operation_cursor->size));

		if (operation_cursor->size < 32){
			switch (operation_cursor->type){
				case IR_OPERATION_TYPE_IN_REG 	: {
					log_warn_m("register of size: %u, this case is not implemented", operation_cursor->size);
					break;
				}
				case IR_OPERATION_TYPE_IN_MEM 	: {
					break;
				}
				case IR_OPERATION_TYPE_OUT_MEM 	: {
					break;
				}
				case IR_OPERATION_TYPE_IMM 		: {
					struct edge* edge_cursor;

					for (edge_cursor = node_get_head_edge_src(node_cursor); edge_cursor != NULL; edge_cursor = edge_get_next_src(edge_cursor)){
						if (ir_node_get_operation(edge_get_dst(edge_cursor))->size == 32){
							operation_cursor->operation_type.imm.value &= 0xffffffffffffffff >> (64 - operation_cursor->size);
							operation_cursor->size = 32;
							
							local_modification = 1;
							break;
						}
					}
					break;
				}
				case IR_OPERATION_TYPE_INST 	: {
					switch (insPaddingStrategy[operation_cursor->operation_type.inst.opcode]){
						case PADDING_OK 		: {
							operation_cursor->size = 32;

							local_modification = 1;
							break;
						}
						case PADDING_COMPLAIN 	: {
							log_warn_m("%s of size: %u, this case is not implemented", irOpcode_2_string(operation_cursor->operation_type.inst.opcode), operation_cursor->size);
							break;
						}
						case PADDING_SECURE 	: {
							#define IRVARIABLESIZE_NB_OPERAND_MAX 16
							struct edge* 	edge_cursor;
							struct node* 	operand;
							struct node* 	mask;
							struct node* 	and_tab[IRVARIABLESIZE_NB_OPERAND_MAX];
							struct edge* 	edge_tab[IRVARIABLESIZE_NB_OPERAND_MAX];
							uint32_t 		i;
							uint32_t 		nb_edge_dst;

							if (node_cursor->nb_edge_dst > IRVARIABLESIZE_NB_OPERAND_MAX){
								log_err("number of operand exceed a static array upper bound");
							}
							else{
								for (edge_cursor = node_get_head_edge_dst(node_cursor), i = 0; edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor), i++){
									operand = edge_get_src(edge_cursor);

									edge_tab[i] = edge_cursor;

									mask = ir_insert_immediate(ir, node_cursor, 32, 0xffffffffffffffff >> (64 - ir_node_get_operation(node_cursor)->size));
									if (mask){
										and_tab[i] = ir_insert_inst(ir, node_cursor, IR_OPERATION_INDEX_UNKOWN, 32, IR_AND, IR_OPERATION_DST_UNKOWN);
										if (and_tab[i]){
											if (ir_add_dependence(ir, mask, and_tab[i], IR_DEPENDENCE_TYPE_DIRECT) == NULL){
												log_err("unable to add dependency to IR");
											}
											if (ir_add_dependence(ir, operand, and_tab[i], IR_DEPENDENCE_TYPE_DIRECT) == NULL){
												log_err("unable to add dependency to IR");
											}
										}
										else{
											log_err("unable to add instruction to IR");
										}
									}
									else{
										log_err("unable to add immediate to IR");
									}
								}

								nb_edge_dst = node_cursor->nb_edge_dst;

								for (i = 0; i < nb_edge_dst; i++){
									if (ir_add_dependence(ir, and_tab[i], node_cursor,ir_edge_get_dependence(edge_tab[i])->type) == NULL){
										log_err("unable to add dependency to IR");
									}
									ir_remove_dependence(ir, edge_tab[i]);
								}

								operation_cursor->size = 32;

								local_modification = 1;
							}
							break;
						}
						case PADDING_IGNORE 	: {
							break;
						}
					}
					break;
				}
				case IR_OPERATION_TYPE_SYMBOL 	: {
					break;
				}
			}
		}
	}

	if (local_modification){
		irVariableSize_remove_size_convertor(ir);
		irVariableSize_add_size_convertor(ir);

		*modification = 1;
	}
}

struct accessEntry{
	uint64_t 		offset;
	uint32_t 		size;
	struct node* 	address;
	struct edge* 	access;
};

static int32_t accessEntry_compare_offset_and_type(void* element1, void* element2){
	struct accessEntry* entry1 = (struct accessEntry*)element1;
	struct accessEntry* entry2 = (struct accessEntry*)element2;
	struct irOperation* mem_access1;
	struct irOperation* mem_access2;

	mem_access1 = ir_node_get_operation(edge_get_dst(entry1->access));
	mem_access2 = ir_node_get_operation(edge_get_dst(entry2->access));

	if (mem_access1->type < mem_access2->type){
		return -1;
	}
	else if (mem_access1->type > mem_access2->type){
		return 1;
	}
	else{
		if (entry1->offset < entry2->offset){
			return -1;
		}
		else if (entry1->offset > entry2->offset){
			return 1;
		}
		else{
			return 0;
		}
	}
}

static int32_t accessEntry_compare_order(const void* element1, const void* element2){
	struct accessEntry* entry1 = *(struct accessEntry**)element1;
	struct accessEntry* entry2 = *(struct accessEntry**)element2;
	struct irOperation* mem_access1;
	struct irOperation* mem_access2;

	mem_access1 = ir_node_get_operation(edge_get_dst(entry1->access));
	mem_access2 = ir_node_get_operation(edge_get_dst(entry2->access));

	if (mem_access1->operation_type.mem.access.order < mem_access2->operation_type.mem.access.order){
		return -1;
	}
	else if (mem_access1->operation_type.mem.access.order > mem_access2->operation_type.mem.access.order){
		return 1;
	}
	else{
		return 0;
	}
}

#define IRVARIABLESIZE_GROUP_MAX_SIZE 4

struct accessGroup{
	uint32_t 			nb_entry;
	struct accessEntry* entries[IRVARIABLESIZE_GROUP_MAX_SIZE];
};

void accessGroup_print(const struct accessGroup* group, FILE* file){
	uint32_t i;

	fputc('{', file);
	for (i = 0; i < group->nb_entry; i++){
		if (ir_node_get_operation(edge_get_dst(group->entries[i]->access))->type == IR_OPERATION_TYPE_IN_MEM){
			fputc('R', file);
		}
		else{
			fputc('W', file);
		}
		fprintf(file, ":0x%llx:%u:%p", group->entries[i]->offset, group->entries[i]->size, (void*)group->entries[i]);
		if (i != group->nb_entry - 1){
			fputc(' ', file);
		}
	}
	fputc('}', file);
}

static uint32_t accessGroup_check_aliasing(const struct accessGroup* group, struct ir* ir){
	uint32_t 			i;
	enum aliasType 		alias_type;
	struct accessGroup 	local_group;

	memcpy(&local_group, group, sizeof(struct accessGroup));
	qsort(local_group.entries, local_group.nb_entry, sizeof(struct accessEntry*), accessEntry_compare_order);

	if (ir_node_get_operation(edge_get_dst(local_group.entries[0]->access))->type == IR_OPERATION_TYPE_IN_MEM){
		alias_type = ALIAS_OUT;
	}
	else{
		alias_type = ALIAS_ALL;
	}

	for (i = 0; i + 1 < local_group.nb_entry; i++){
		if(ir_normalize_search_alias_conflict(edge_get_dst(local_group.entries[i]->access), edge_get_dst(local_group.entries[local_group.nb_entry - 1]->access), alias_type, ALIASING_STRATEGY_CHECK, ir->range_seed)){
			return 0;
		}
	}

	return 1;
}

static enum irOpcode accessGroup_choose_part_opcode(uint32_t offset, uint32_t size){
	if (offset == 0){
		if (size == 8){
			return IR_PART1_8;
		}
		else if (size == 16){
			return IR_PART1_16;
		}
	}
	else if (offset == 8){
		if (size == 8){
			return IR_PART2_8;
		}
	}

	log_err_m("this case is not handled yet (offset=%u, size=%u)", offset, size);
	return IR_INVALID;
}

static void accessGroup_commit(struct accessGroup* group, struct ir* ir, uint8_t* modification){
	uint32_t 		i;
	uint32_t 		offset;

	if (ir_node_get_operation(edge_get_dst(group->entries[0]->access))->type == IR_OPERATION_TYPE_IN_MEM){
		enum irOpcode 	opcode;
		struct node*	part_node;
		struct node*	root;

		root = edge_get_dst(group->entries[0]->access);
		for (i = 0, offset = 0; i < group->nb_entry; i++){
			opcode = accessGroup_choose_part_opcode(offset, group->entries[i]->size * 8);
			if (opcode != IR_INVALID){
				part_node = ir_add_inst(ir, IR_OPERATION_INDEX_UNKOWN, group->entries[i]->size * 8, opcode, IR_OPERATION_DST_UNKOWN);
				if (part_node == NULL){
					log_err("unable to add instruction to IR");
				}
				else{
					graph_transfert_src_edge(&(ir->graph), part_node, edge_get_dst(group->entries[i]->access));
					if (ir_add_dependence(ir, root, part_node, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
						log_err("unable to add dependency to IR");
					}
				}
			}
			else{
				log_err("unable to choose part opcode");
			}

			offset += group->entries[i]->size * 8;
			if (offset == 16 && i + 1 != group->nb_entry){
				offset = 0;

				root = ir_add_inst(ir, IR_OPERATION_INDEX_UNKOWN, 32, IR_SHR, IR_OPERATION_DST_UNKOWN);
				if (root == NULL){
					log_err("unable to add instruction to IR");
					return;
				}

				part_node = ir_add_immediate(ir, 32, 16);
				if (part_node == NULL){
					log_err("unable to add immediate to IR");
				}
				else{
					if (ir_add_dependence(ir, part_node, root, IR_DEPENDENCE_TYPE_SHIFT_DISP) == NULL){
						log_err("unable to add dependency to IR");
					}
				}

				if (ir_add_dependence(ir, edge_get_dst(group->entries[0]->access), root, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
					log_err("unable to add dependency to IR");
				}
			}
		}

		for (i = 1; i < group->nb_entry; i++){
			ir_remove_node(ir, edge_get_dst(group->entries[i]->access));
		}

		ir_node_get_operation(edge_get_dst(group->entries[0]->access))->size = 32;
	}
	else{
		struct node* shl_node;
		struct node* exp_node;
		struct node* or_node;
		struct node* imm_node;
		struct edge* edge_cursor;

		or_node = ir_add_inst(ir, IR_OPERATION_INDEX_UNKOWN, 32, IR_OR, IR_OPERATION_DST_UNKOWN);
		if (or_node == NULL){
			log_err("unable to add instruction to IR");
		}
		else{
			for (i = 0, offset = 0; i < group->nb_entry; i++){
				for (edge_cursor = node_get_head_edge_dst(edge_get_dst(group->entries[i]->access)); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
					if (ir_edge_get_dependence(edge_cursor)->type == IR_DEPENDENCE_TYPE_DIRECT){
						break;
					}
				}
				if (edge_cursor == NULL){
					log_err("unable to locate direct operand");
					continue;
				}

				exp_node = ir_add_inst(ir, IR_OPERATION_INDEX_UNKOWN, 32, IR_MOVZX, IR_OPERATION_DST_UNKOWN);
				if (exp_node == NULL){
					log_err("unable to add instruction to IR");
				}
				else{
					if (ir_add_dependence(ir, edge_get_src(edge_cursor), exp_node, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
						log_err("unable to add dependency to IR");
					}
					if (i == 0){
						if (ir_add_dependence(ir, exp_node, or_node, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
							log_err("unable to add dependency to IR");
						}
						ir_remove_dependence(ir, edge_cursor);
					}
					else{
						shl_node = ir_add_inst(ir, IR_OPERATION_INDEX_UNKOWN, 32, IR_SHL, IR_OPERATION_DST_UNKOWN);
						if (shl_node == NULL){
							log_err("unable to add instruction to IR");
						}
						else{
							imm_node = ir_add_immediate(ir, 32, offset);
							if (imm_node == NULL){
								log_err("unable to add immediate to IR");
							}
							else{
								if (ir_add_dependence(ir, imm_node, shl_node, IR_DEPENDENCE_TYPE_SHIFT_DISP) == NULL){
									log_err("unable to add dependency to IR");
								}
							}
							if (ir_add_dependence(ir, exp_node, shl_node, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
								log_err("unable to add dependency to IR");
							}
							if (ir_add_dependence(ir, shl_node, or_node, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
								log_err("unable to add dependency to IR");
							}

							ir_remove_node(ir, edge_get_dst(group->entries[i]->access));
						}
					}
				}
				offset += group->entries[i]->size * 8;
			}

			if (ir_add_dependence(ir, or_node, edge_get_dst(group->entries[0]->access), IR_DEPENDENCE_TYPE_DIRECT) == NULL){
				log_err("unable to add dependency to IR");
			}
			ir_node_get_operation(edge_get_dst(group->entries[0]->access))->size = 32;
		}
	}

	*modification = 1;
}

struct accessTable{
	struct node* 	var;
	struct array* 	group_array;
	struct ir* 		ir;
	struct array 	access_array;
};

void accessTable_print(const struct accessTable* table, uint32_t* mapping, FILE* file){
	uint32_t 			i;
	struct accessEntry* entry;

	fputs("{", file);
	for (i = 0; i < array_get_length(&(table->access_array)); i++){
		if (mapping != NULL){
			entry = (struct accessEntry*)array_get(&(table->access_array), mapping[i]);
		}
		else{
			entry = (struct accessEntry*)array_get(&(table->access_array), i);
		}

		if (i == array_get_length(&(table->access_array)) - 1){
			fprintf(file, "%c:0x%llx:%u", (ir_node_get_operation(edge_get_dst(entry->access))->type == IR_OPERATION_TYPE_IN_MEM) ? 'R' : 'W', entry->offset, entry->size);
		}
		else{
			fprintf(file, "%c:0x%llx:%u ", (ir_node_get_operation(edge_get_dst(entry->access))->type == IR_OPERATION_TYPE_IN_MEM) ? 'R' : 'W', entry->offset, entry->size);
		}
	}
	fputs("}", file);
}

static uint32_t accessTable_generate_recursive_group(struct accessTable* table, uint32_t* mapping, struct accessGroup* group, uint32_t offset_start, uint32_t nb_entry, uint32_t size){
	uint32_t i;

	#ifdef IR_FULL_CHECK
	#define accessGroup_check() 																																	\
		{ 																																							\
			struct node* 		node_mem_access1 = edge_get_dst(group->entries[0]->access); 																		\
			struct node* 		node_mem_access2 = edge_get_dst(group->entries[nb_entry]->access); 																	\
			struct irOperation* op_mem_access1 = ir_node_get_operation(node_mem_access1); 																			\
			struct irOperation* op_mem_access2 = ir_node_get_operation(node_mem_access2); 																			\
			ADDRESS 			address1 = op_mem_access1->operation_type.mem.access.con_addr; 																		\
			ADDRESS 			address2 = op_mem_access2->operation_type.mem.access.con_addr; 																		\
																																									\
			if (address1 != MEMADDRESS_INVALID && address2 != MEMADDRESS_INVALID){ 																					\
				if (address1 + size != address2){ 																													\
					log_err_m("found group with concrete address incoherence, expected 0x%08x but get 0x%08x", address1 + size, address2); 							\
					printf("  - %p ", (void*)node_mem_access1); ir_print_node(op_mem_access1, stdout); putchar('\n'); 												\
					printf("  - %p ", (void*)node_mem_access2); ir_print_node(op_mem_access2, stdout); putchar('\n'); 												\
																																									\
					op_mem_access1->status_flag |= IR_OPERATION_STATUS_FLAG_ERROR; 																					\
					op_mem_access2->status_flag |= IR_OPERATION_STATUS_FLAG_ERROR; 																					\
																																									\
					continue; 																																		\
				} 																																					\
			} 																																						\
		}
	#else
	#define accessGroup_check()
	#endif

	for (i = offset_start; i < array_get_length(&(table->access_array)); i++){
		group->entries[nb_entry] = (struct accessEntry*)array_get(&(table->access_array), mapping[i]);
		if (ir_node_get_operation(edge_get_dst(group->entries[0]->access))->type != ir_node_get_operation(edge_get_dst(group->entries[nb_entry]->access))->type){
			continue;
		}
		if (group->entries[nb_entry - 1]->offset + group->entries[nb_entry - 1]->size != group->entries[nb_entry]->offset){
			continue;
		}

		if (group->entries[nb_entry]->size + size > 4){
			continue;
		}
		else if (group->entries[nb_entry]->size + size == 4){
			accessGroup_check()

			group->nb_entry = nb_entry + 1;

			#ifdef IR_FULL_CHECK
			printf("\tscheduling group i=%u ", i); accessGroup_print(group, stdout); putchar('\n'); /* pour le debug */
			#endif
			if (accessGroup_check_aliasing(group, table->ir)){
				if (array_add(table->group_array, group) < 0){
					log_err("unable to add element to array");
				}
				else{
					return 1;
				}
			}
		}
		else if (nb_entry != IRVARIABLESIZE_GROUP_MAX_SIZE - 1){
			accessGroup_check()

			if (accessTable_generate_recursive_group(table, mapping, group, i + 1, nb_entry + 1, size + group->entries[nb_entry]->size)){
				return 1;
			}
		}
	}

	return 0;
}

#pragma GCC diagnostic ignored "-Wunused-parameter"
static void accessTable_regroup(const void* data, const VISIT which, int32_t depth){
	struct accessTable* table = *(struct accessTable**)data;
	uint32_t* 			mapping;
	uint32_t 			i;
	struct accessGroup 	group;

	if (which != leaf && which != preorder){
		return;
	}

	if (array_get_length(&(table->access_array)) < 2){
		return;
	}

	mapping = array_create_mapping(&(table->access_array), accessEntry_compare_offset_and_type);
	if (mapping == NULL){
		log_err("unable to create mapping");
		return;
	}

	#ifdef IR_FULL_CHECK
	printf("Table: "); accessTable_print(table, mapping, stdout); putchar('\n'); /* pour le debug */
	#endif

	for (i = 0; i < array_get_length(&(table->access_array)); i++){	
		group.entries[0] = (struct accessEntry*)array_get(&(table->access_array), mapping[i]);
		if (group.entries[0]->offset & 0x0000000000000003){
			continue;
		}

		#ifdef IR_FULL_CHECK
		{
			struct irOperation* op_mem_access = ir_node_get_operation(edge_get_dst(group.entries[0]->access));

			if (op_mem_access->operation_type.mem.access.con_addr != MEMADDRESS_INVALID && op_mem_access->operation_type.mem.access.con_addr & 0x0000000000000003){
				log_warn("memory access seems to be aligned but concrete address is not");
			}
		}
		#endif

		if (accessTable_generate_recursive_group(table, mapping, &group, i + 1, 1, group.entries[0]->size)){
			break;
		}
	}

	free(mapping);
}

static int32_t accessTable_compare_var(const void* arg1, const void* arg2){
	struct accessTable* table1 = (struct accessTable*)arg1;
	struct accessTable* table2 = (struct accessTable*)arg2;

	if (table1->var < table2->var){
		return -1;
	}
	else if (table1->var > table2->var){
		return 1;
	}
	else{
		return 0;
	}
}

static void accessTable_delete(struct accessTable* table){
	array_clean(&(table->access_array));
	free(table);
}

void ir_normalize_regroup_mem_access(struct ir* ir, uint8_t* modification){
	struct node* 		node_cursor;
	struct irOperation* operation_cursor;
	struct edge*		edge_cursor;
	struct node*		var;
	struct node* 		imm;
	void* 				tree_root 			= NULL;
	struct accessTable* new_table 			= NULL;
	void* 				ptr;
	struct accessTable* table_ptr;
	struct accessEntry 	new_entry;
	struct array 		group_array;

	if (array_init(&group_array, sizeof(struct accessGroup))){
		log_err("unable to init array");
		return;
	}

	ir_drop_range(ir); /* We are not sure what have been done before. Might be removed later */

	for (node_cursor = graph_get_head_node(&(ir->graph)); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
		if (node_cursor->nb_edge_dst != 2 || node_cursor->nb_edge_src == 0){
			continue;
		}

		operation_cursor = ir_node_get_operation(node_cursor);

		if (operation_cursor->type != IR_OPERATION_TYPE_INST || operation_cursor->operation_type.inst.opcode != IR_ADD){
			continue;
		}

		for (edge_cursor = node_get_head_edge_src(node_cursor); edge_cursor != NULL; edge_cursor = edge_get_next_src(edge_cursor)){
			if (ir_edge_get_dependence(edge_cursor)->type == IR_DEPENDENCE_TYPE_ADDRESS){
				break;
			}
		}
		if (edge_cursor == NULL){
			continue;
		}

		var = NULL;
		imm = NULL;

		edge_cursor = node_get_head_edge_dst(node_cursor);
		switch (ir_node_get_operation(edge_get_src(edge_cursor))->type){
			case IR_OPERATION_TYPE_IN_REG 	:
			case IR_OPERATION_TYPE_IN_MEM 	:
			case IR_OPERATION_TYPE_INST 	: {
				var = edge_get_src(edge_cursor);
				break;
			}
			case IR_OPERATION_TYPE_IMM 		: {
				imm = edge_get_src(edge_cursor);
				break;
			}
			default 						: {
				continue;
			}
		}

		edge_cursor = edge_get_next_dst(edge_cursor);
		switch (ir_node_get_operation(edge_get_src(edge_cursor))->type){
			case IR_OPERATION_TYPE_IN_REG 	:
			case IR_OPERATION_TYPE_IN_MEM 	:
			case IR_OPERATION_TYPE_INST 	: {
				if (var == NULL){
					var = edge_get_src(edge_cursor);
					break;
				}
				else{
					continue;
				}
			}
			case IR_OPERATION_TYPE_IMM 		: {
				if (imm == NULL){
					imm = edge_get_src(edge_cursor);
					break;
				}
				else{
					continue;
				}
			}
			default 						: {
				continue;
			}
		}

		if (new_table == NULL){
			new_table = (struct accessTable*)malloc(sizeof(struct accessTable));
			if (new_table == NULL){
				log_err("unable to allocate memory\n");
				continue;
			}
			else{
				if (array_init(&(new_table->access_array), sizeof(struct accessEntry))){
					log_err("unable to init array");
					free(new_table);
					new_table = NULL;
					continue;
				}
			}
		}

		new_table->var 			= var;
		new_table->group_array 	= &group_array;
		new_table->ir 			= ir;

		if ((ptr = tsearch(new_table, &tree_root, accessTable_compare_var)) == NULL){
			log_err("unbale to allocate memory in tsearch");
			continue;
		}

		table_ptr = *(struct accessTable**)ptr;
		if (table_ptr == new_table){
			for (edge_cursor = node_get_head_edge_src(var); edge_cursor != NULL; edge_cursor = edge_get_next_src(edge_cursor)){
				if (ir_edge_get_dependence(edge_cursor)->type == IR_DEPENDENCE_TYPE_ADDRESS){

					new_entry.offset 	= 0;
					new_entry.size 		= ir_node_get_operation(edge_get_dst(edge_cursor))->size / 8;
					new_entry.address 	= var;
					new_entry.access 	= edge_cursor;

					if (array_add(&(table_ptr->access_array), &new_entry) < 0){
						log_err("unable to add element to array\n");
					}
				}
			}
			new_table = NULL;
		}

		for (edge_cursor = node_get_head_edge_src(node_cursor); edge_cursor != NULL; edge_cursor = edge_get_next_src(edge_cursor)){
			if (ir_edge_get_dependence(edge_cursor)->type == IR_DEPENDENCE_TYPE_ADDRESS){
				new_entry.offset 	= ir_imm_operation_get_unsigned_value(ir_node_get_operation(imm));
				new_entry.size 		= ir_node_get_operation(edge_get_dst(edge_cursor))->size / 8;
				new_entry.address 	= node_cursor;
				new_entry.access 	= edge_cursor;

				if (array_add(&(table_ptr->access_array), &new_entry) < 0){
					log_err("unable to add element to array\n");
				}
			}
		}
	}

	if (new_table != NULL){
		accessTable_delete(new_table);
	}

	twalk(tree_root, accessTable_regroup);

	if (array_get_length(&group_array) > 0){
		uint32_t 			i;
		struct accessGroup* group;

		for (i = 0; i < array_get_length(&group_array); i++){
			group = (struct accessGroup*)array_get(&group_array, i);
			accessGroup_commit(group, ir, modification);
		}
	}

	array_clean(&group_array);

	tdestroy(tree_root, (void(*)(void*))accessTable_delete);
}


