#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef VERBOSE
#include <time.h>
#endif

#include "irNormalize.h"

#ifdef VERBOSE
#include "multiColumn.h"
#endif

#define IR_NORMALIZE_SIMPLIFY_INSTRUCTION			1
#define IR_NORMALIZE_MERGE_ASSOCIATIVE_ADD 			1
#define IR_NORMALIZE_MERGE_ASSOCIATIVE_XOR 			1
#define IR_NORMALIZE_REMOVE_SUBEXPRESSION 			1
#define IR_NORMALIZE_SIMPLIFY_MEMORY_ACCESS 		1
#define IR_NORMALIZE_DETECT_ROTATION 				1

static int32_t irNormalize_sort_node(struct ir* ir);
static void irNormalize_recursive_sort(struct node** node_buffer, struct node* node, uint32_t* generator);

struct irOperand{
	struct node*	node;
	struct edge* 	edge;
};

int32_t compare_order_memoryNode(const void* arg1, const void* arg2);
int32_t compare_address_node_irOperand(const void* arg1, const void* arg2);

#ifdef VERBOSE

#define INIT_TIMER																																			\
	struct timespec 			timer_start_time; 																											\
	struct timespec 			timer_stop_time; 																											\
	double 						timer_elapsed_time; 																										\
	struct multiColumnPrinter* 	printer; 																													\
																																							\
	printer = multiColumnPrinter_create(stdout, 2, NULL, NULL, NULL); 																						\
	if (printer != NULL){ 																																	\
		multiColumnPrinter_set_column_type(printer, 1, MULTICOLUMN_TYPE_DOUBLE); 																			\
		multiColumnPrinter_set_column_size(printer, 0, 32); 																								\
		multiColumnPrinter_set_title(printer, 0, "RULE"); 																									\
		multiColumnPrinter_set_title(printer, 1, "TIME"); 																									\
																																							\
		multiColumnPrinter_print_header(printer); 																											\
	} 																																						\
	else{ 																																					\
		printf("ERROR: in %s, unable to init multiColumnPrinter\n", __func__); 																				\
		return; 																																			\
	}

#define START_TIMER 																																		\
	if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &timer_start_time)){ 																						\
		printf("ERROR: in %s, clock_gettime fails\n", __func__); 																							\
	}

#define STOP_TIMER 																																			\
	if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &timer_stop_time)){ 																						\
		printf("ERROR: in %s, clock_gettime fails\n", __func__); 																							\
	}

#define PRINT_TIMER(ctx_string) 																															\
	timer_elapsed_time = (timer_stop_time.tv_sec - timer_start_time.tv_sec) + (timer_stop_time.tv_nsec - timer_start_time.tv_nsec) / 1000000000.; 			\
	multiColumnPrinter_print(printer, (ctx_string), timer_elapsed_time, NULL);

#define CLEAN_TIMER 																																		\
		multiColumnPrinter_delete(printer);

#else

#define INIT_TIMER
#define START_TIMER
#define STOP_TIMER
#define PRINT_TIMER(ctx_string)
#define CLEAN_TIMER

#endif

void ir_normalize(struct ir* ir){
	INIT_TIMER


	#if IR_NORMALIZE_SIMPLIFY_INSTRUCTION == 1
	{
		START_TIMER
		ir_normalize_simplify_instruction(ir);
		STOP_TIMER
		PRINT_TIMER("Simplify instruction")
	}
	#endif
	
	#if IR_NORMALIZE_REMOVE_SUBEXPRESSION == 1 && IR_NORMALIZE_SIMPLIFY_MEMORY_ACCESS == 1
	{
		uint8_t modification;
		#ifdef VERBOSE
		double timer_1_elapsed_time = 0.0;
		double timer_2_elapsed_time = 0.0;
		#endif

		START_TIMER
		ir_normalize_remove_subexpression(ir, &modification);
		STOP_TIMER
		#ifdef VERBOSE
		timer_1_elapsed_time += (timer_stop_time.tv_sec - timer_start_time.tv_sec) + (timer_stop_time.tv_nsec - timer_start_time.tv_nsec) / 1000000000.;
		#endif

		START_TIMER
		ir_normalize_simplify_memory_access(ir, &modification);
		STOP_TIMER
		#ifdef VERBOSE
		timer_2_elapsed_time += (timer_stop_time.tv_sec - timer_start_time.tv_sec) + (timer_stop_time.tv_nsec - timer_start_time.tv_nsec) / 1000000000.;
		#endif

		while(modification){
			START_TIMER
			ir_normalize_remove_subexpression(ir, &modification);
			STOP_TIMER
			#ifdef VERBOSE
			timer_1_elapsed_time += (timer_stop_time.tv_sec - timer_start_time.tv_sec) + (timer_stop_time.tv_nsec - timer_start_time.tv_nsec) / 1000000000.;
			#endif

			if (!modification){
				continue;
			}

			START_TIMER
			ir_normalize_simplify_memory_access(ir, &modification);
			STOP_TIMER
			#ifdef VERBOSE
			timer_2_elapsed_time += (timer_stop_time.tv_sec - timer_start_time.tv_sec) + (timer_stop_time.tv_nsec - timer_start_time.tv_nsec) / 1000000000.;
			#endif
		}

		#ifdef VERBOSE
		multiColumnPrinter_print(printer, "Remove subexpression", timer_1_elapsed_time, NULL);
		multiColumnPrinter_print(printer, "Simplify memory access", timer_2_elapsed_time, NULL);
		#endif
	}
	#elif IR_NORMALIZE_REMOVE_SUBEXPRESSION == 1
	{
		uint8_t modification;
		START_TIMER
		ir_normalize_remove_subexpression(ir, &modification);
		STOP_TIMER
		PRINT_TIMER("Remove subexpression")
	}
	#elif IR_NORMALIZE_SIMPLIFY_MEMORY_ACCESS == 1
	{
		uint8_t modification;
		START_TIMER
		ir_normalize_simplify_memory_access(ir, &modification);
		STOP_TIMER
		PRINT_TIMER("Simplify memory access")
	}
	#endif

	#if IR_NORMALIZE_MERGE_ASSOCIATIVE_ADD == 1
	{
		START_TIMER
		ir_normalize_merge_associative_operation(ir, IR_ADD);
		STOP_TIMER
		PRINT_TIMER("Merge associative add")
	}
	#endif
	
	#if IR_NORMALIZE_MERGE_ASSOCIATIVE_XOR == 1
	{
		START_TIMER
		ir_normalize_merge_associative_operation(ir, IR_XOR);
		STOP_TIMER
		PRINT_TIMER("Merge associative xor")
	}
	#endif

	#if IR_NORMALIZE_DETECT_ROTATION == 1
	{
		START_TIMER
		ir_normalize_detect_rotation(ir);
		STOP_TIMER
		PRINT_TIMER("Detect rotation")
	}
	#endif

	CLEAN_TIMER
}

static void ir_normalize_simplify_instruction_rewrite_movzx(struct ir* ir, struct node* node);
static void ir_normalize_simplify_instruction_symbolic_or(struct ir* ir, struct node* node);
static void ir_normalize_simplify_instruction_rewrite_or(struct ir* ir, struct node* node);
static void ir_normalize_simplify_instruction_rewrite_part1_8(struct ir* ir, struct node* node);
static void ir_normalize_simplify_instruction_rewrite_rol(struct ir* ir, struct node* node);
static void ir_normalize_simplify_instruction_rewrite_sub(struct ir* ir, struct node* node);
static void ir_normalize_simplify_instruction_symbolic_xor(struct ir* ir, struct node* node);
static void ir_normalize_simplify_instruction_rewrite_xor(struct ir* ir, struct node* node);


void ir_normalize_simplify_instruction(struct ir* ir){
	struct node* 			node_cursor;
	struct node* 			next_node_cursor;
	struct irOperation* 	operation;

	if (irNormalize_sort_node(ir)){
		printf("ERROR: in %s, unable to sort ir node(s)\n", __func__);
		return;
	}

	for(node_cursor = graph_get_tail_node(&(ir->graph)), next_node_cursor = NULL; node_cursor != NULL;){
		
		operation = ir_node_get_operation(node_cursor);
		if (operation->type == IR_OPERATION_TYPE_INST){
			switch(operation->operation_type.inst.opcode){
				case IR_MOVZX 		: {
					ir_normalize_simplify_instruction_rewrite_movzx(ir, node_cursor);
					break;
				}
				case IR_OR 			: {
					ir_normalize_simplify_instruction_symbolic_or(ir, node_cursor);
					ir_normalize_simplify_instruction_rewrite_or(ir, node_cursor);
					break;
				}
				case IR_PART1_8 	: {
					ir_normalize_simplify_instruction_rewrite_part1_8(ir, node_cursor);
					break;
				}
				case IR_ROL 		: {
					ir_normalize_simplify_instruction_rewrite_rol(ir, node_cursor);
					break;
				}
				case IR_SUB 		: {
					ir_normalize_simplify_instruction_rewrite_sub(ir, node_cursor);
					break;
				}
				case IR_XOR 		: {
					ir_normalize_simplify_instruction_symbolic_xor(ir, node_cursor);
					ir_normalize_simplify_instruction_rewrite_xor(ir, node_cursor);
					break;
				}
				default 			: {
					break;
				}
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

static void ir_normalize_simplify_instruction_rewrite_movzx(struct ir* ir, struct node* node){
	struct node* 		operand;
	struct irOperation*	operand_operation;
	struct node* 		node_imm_new;
	struct edge* 		edge_cursor;

	if (node->nb_edge_dst == 1){
		operand = edge_get_src(node_get_head_edge_dst(node));
		operand_operation = ir_node_get_operation(operand);
		if (operand_operation->type == IR_OPERATION_TYPE_INST && operand_operation->operation_type.inst.opcode == IR_PART1_8){
			if (operand->nb_edge_src == 1){
				node_imm_new = ir_add_immediate(ir, ir_node_get_operation(node)->size, 0, 0x000000ff);
				if (node_imm_new != NULL){
					if (ir_add_dependence(ir, node_imm_new, node, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
						printf("ERROR: in %s, unable to add edge to IR\n", __func__);
					}
					else{
						ir_node_get_operation(node)->operation_type.inst.opcode = IR_AND;

						graph_transfert_dst_edge(&(ir->graph), node, operand);
						ir_remove_node(ir, operand);
					}
				}
				else{
					printf("ERROR: in %s, unable to add immediate node to IR\n", __func__);
				}
			}
			else{
				printf("WARNING: in %s, PART1_8 instruction is shared, this case is not implemented yet -> skip\n", __func__);
			}
		}
		else if (operand_operation->type == IR_OPERATION_TYPE_INST && operand_operation->operation_type.inst.opcode == IR_PART2_8){
			if (operand->nb_edge_src == 1){
				ir_node_get_operation(node)->operation_type.inst.opcode = IR_AND;

				node_imm_new = ir_add_immediate(ir, ir_node_get_operation(node)->size, 0, 0x000000ff);
				if (node_imm_new != NULL){
					if (ir_add_dependence(ir, node_imm_new, node, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
						printf("ERROR: in %s, unable to add edge to IR\n", __func__);
					}
				}
				else{
					printf("ERROR: in %s, unable to add immediate node to IR\n", __func__);
				}

				operand_operation->operation_type.inst.opcode = IR_SHR;

				node_imm_new = ir_add_immediate(ir, ir_node_get_operation(node)->size, 0, ir_node_get_operation(node)->size - 8);
				if (node_imm_new != NULL){
					if (ir_add_dependence(ir, node_imm_new, operand, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
						printf("ERROR: in %s, unable to add edge to IR\n", __func__);
					}
				}
				else{
					printf("ERROR: in %s, unable to add immediate node to IR\n", __func__);
				}
			}
			else{
				printf("WARNING: in %s, PART2_8 instruction is shared, this case is not implemented yet -> skip\n", __func__);
			}
		}
		else{
			for (edge_cursor = node_get_head_edge_src(node); edge_cursor != NULL; edge_cursor = edge_get_next_src(edge_cursor)){
				if (graph_copy_dst_edge(&(ir->graph), edge_get_dst(edge_cursor), node)){
					printf("ERROR: in %s, unable to copy dst edge\n", __func__);
					break;
				}
			}
			ir_remove_node(ir, node);
		}
	}
}

static void ir_normalize_simplify_instruction_symbolic_or(struct ir* ir, struct node* node){
	struct edge* 			edge_cursor;
	struct irOperand*		operand_list;
	uint32_t 				nb_operand;
	uint32_t 				i;

	if (node->nb_edge_dst >= 2){
		operand_list = (struct irOperand*)alloca(sizeof(struct irOperand) * node->nb_edge_dst);

		for (edge_cursor = node_get_head_edge_dst(node), i = 0; edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor), i++){
			operand_list[i].node = edge_get_src(edge_cursor);
			operand_list[i].edge = edge_cursor;
		}

		nb_operand = node->nb_edge_dst;
		qsort(operand_list, nb_operand, sizeof(struct irOperand), compare_address_node_irOperand);

		for (i = 1; i < nb_operand; i++){
			if (operand_list[i - 1].node == operand_list[i].node){
				ir_remove_dependence(ir, operand_list[i - 1].edge);
			}
		}
	}
}

static void ir_normalize_simplify_instruction_rewrite_or(struct ir* ir, struct node* node){
	if (node->nb_edge_dst == 1){
		graph_transfert_src_edge(&(ir->graph), edge_get_src(node_get_head_edge_dst(node)), node);
		ir_remove_node(ir, node);
	}
}

static void ir_normalize_simplify_instruction_rewrite_part1_8(struct ir* ir, struct node* node){
	struct node* 		operand;
	struct irOperation*	operand_operation;
	struct edge*		edge_cursor;

	if (node->nb_edge_dst == 1){
		operand = edge_get_src(node_get_head_edge_dst(node));
		operand_operation = ir_node_get_operation(operand);
		if (operand_operation->type == IR_OPERATION_TYPE_INST && operand_operation->operation_type.inst.opcode == IR_MOVZX){
			if (operand->nb_edge_src == 1){
				for (edge_cursor = node_get_head_edge_src(node); edge_cursor != NULL; edge_cursor = edge_get_next_src(edge_cursor)){
					if (graph_copy_dst_edge(&(ir->graph), edge_get_dst(edge_cursor), operand)){
						printf("ERROR: in %s, unable to copy dst edge\n", __func__);
						break;
					}
				}

				ir_remove_node(ir, operand);
				ir_remove_node(ir, node);
			}
			else{
				printf("WARNING: in %s, MOVZX instruction is shared, this case is not implemented yet -> skip\n", __func__);
			}
		}
	}
}

#pragma GCC diagnostic ignored "-Wunused-parameter"
static void ir_normalize_simplify_instruction_rewrite_rol(struct ir* ir, struct node* node){
	struct edge* 		edge_cursor;
	struct irOperation*	operand_operation;

	if (node->nb_edge_dst == 2){
		for (edge_cursor = node_get_head_edge_dst(node); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
			operand_operation = ir_node_get_operation(edge_get_src(edge_cursor));
			if (operand_operation->type == IR_OPERATION_TYPE_IMM){
				if (edge_get_src(edge_cursor)->nb_edge_src == 1){
					#pragma GCC diagnostic ignored "-Wlong-long" /* use of C99 long long integer constant */
					operand_operation->operation_type.imm.value = ir_node_get_operation(node)->size - ir_imm_operation_get_unsigned_value(operand_operation);
					ir_node_get_operation(node)->operation_type.inst.opcode = IR_ROR;
					break;
				}
				else{
					printf("WARNING: in %s, found IMM operand but it is shared -> skip\n", __func__);
				}
			}
		}
	}
}

#pragma GCC diagnostic ignored "-Wunused-parameter"
static void ir_normalize_simplify_instruction_rewrite_sub(struct ir* ir, struct node* node){
	struct edge* 		edge_cursor;
	struct irOperation*	operand_operation;

	if (node->nb_edge_dst == 2){
		for (edge_cursor = node_get_head_edge_dst(node); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
			operand_operation = ir_node_get_operation(edge_get_src(edge_cursor));
			if (operand_operation->type == IR_OPERATION_TYPE_IMM){
				if (edge_get_src(edge_cursor)->nb_edge_src == 1){
					operand_operation->operation_type.imm.value = (uint64_t)(-operand_operation->operation_type.imm.value); /* I am not sure if this is correct */
					operand_operation->operation_type.imm.signe = 1;
					ir_node_get_operation(node)->operation_type.inst.opcode = IR_ADD;
					break;
				}
				else{
					printf("WARNING: in %s, found IMM operand but it is shared -> skip\n", __func__);
				}
			}
		}
	}
}

static void ir_normalize_simplify_instruction_symbolic_xor(struct ir* ir, struct node* node){
	struct edge* 			edge_cursor;
	struct irOperand*		operand_list;
	uint32_t 				nb_operand;
	uint32_t 				i;

	if (node->nb_edge_dst >= 2){
		operand_list = (struct irOperand*)alloca(sizeof(struct irOperand) * node->nb_edge_dst);

		for (edge_cursor = node_get_head_edge_dst(node), i = 0; edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor), i++){
			operand_list[i].node = edge_get_src(edge_cursor);
			operand_list[i].edge = edge_cursor;
		}

		nb_operand = node->nb_edge_dst;
		qsort(operand_list, nb_operand, sizeof(struct irOperand), compare_address_node_irOperand);

		for (i = 1; i < nb_operand; i++){
			if (operand_list[i - 1].node == operand_list[i].node){
				struct node* 	imm_zero;
				uint8_t 		size;

				size = ir_node_get_operation(operand_list[i].node)->size;
				ir_remove_dependence(ir, operand_list[i - 1].edge);
				ir_remove_dependence(ir, operand_list[i].edge);
				imm_zero = ir_add_immediate(ir, size, 0, 0);
				if (imm_zero != NULL){
					if (ir_add_dependence(ir, imm_zero, node, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
						printf("ERROR: in %s, unable to add dependence to IR\n", __func__);
					}
				}
				else{
					printf("ERROR: in %s, unable to add immediate node to IR\n", __func__);
				}

				i++;
			}
		}
	}
}

static void ir_normalize_simplify_instruction_rewrite_xor(struct ir* ir, struct node* node){
	struct edge* 		edge_cursor;
	struct irOperation*	operand_operation;

	if (node->nb_edge_dst == 1){
		graph_transfert_src_edge(&(ir->graph), edge_get_src(node_get_head_edge_dst(node)), node);
		ir_remove_node(ir, node);
	}
	else if(node->nb_edge_dst == 2){
		for(edge_cursor = node_get_head_edge_dst(node); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
			operand_operation = ir_node_get_operation(edge_get_src(edge_cursor));
			if (operand_operation->type == IR_OPERATION_TYPE_IMM){
				if (edge_get_src(edge_cursor)->nb_edge_src){
					#pragma GCC diagnostic ignored "-Wlong-long" /* use of C99 long long integer constant */
					if (ir_imm_operation_get_unsigned_value(operand_operation) == (0xffffffffffffffffULL >> (64 - ir_node_get_operation(node)->size))){
						ir_remove_dependence(ir, edge_cursor);
						ir_node_get_operation(node)->operation_type.inst.opcode = IR_NOT;
						break;
					}
				}
				else{
					printf("WARNING: in %s, found IMM operand but it is shared -> skip\n", __func__);
				}
			}
		}
	}
}

void ir_normalize_remove_subexpression(struct ir* ir, uint8_t* modification){
	struct node* 			node_cursor1;
	struct node* 			node_cursor2;
	struct edge* 			edge_cursor1;
	struct edge* 			edge_cursor2;
	struct irOperation* 	operation1;
	struct irOperation* 	operation2;
	uint32_t 				i;
	uint8_t* 				taken;
	uint32_t 				max_nb_edge_dst = 0;
	uint32_t 				nb_match;
	struct node*			next_node_cursor2;

	*modification = 0;
	
	for(node_cursor1 = graph_get_head_node(&(ir->graph)); node_cursor1 != NULL; node_cursor1 = node_get_next(node_cursor1)){
		if (node_cursor1->nb_edge_dst > max_nb_edge_dst){
			max_nb_edge_dst = node_cursor1->nb_edge_dst;
		}
	}

	if (max_nb_edge_dst == 0){
		return;
	}

	if (irNormalize_sort_node(ir)){
		printf("ERROR: in %s, unable to sort ir node(s)\n", __func__);
		return;
	}

	taken = (uint8_t*)alloca(sizeof(uint8_t) * max_nb_edge_dst);

	for(node_cursor1 = graph_get_head_node(&(ir->graph)); node_cursor1 != NULL; node_cursor1 = node_get_next(node_cursor1)){
		operation1 = ir_node_get_operation(node_cursor1);
		if (operation1->type == IR_OPERATION_TYPE_INST){
			for(node_cursor2 = node_get_next(node_cursor1); node_cursor2 != NULL; node_cursor2 = next_node_cursor2){
				next_node_cursor2 = node_get_next(node_cursor2);

				operation2 = ir_node_get_operation(node_cursor2);
				if (operation2->type == IR_OPERATION_TYPE_INST){

					if (node_cursor1->nb_edge_dst == node_cursor2->nb_edge_dst && irOperation_equal(operation1, operation2)){
						memset(taken, 0, sizeof(uint8_t) * node_cursor1->nb_edge_dst);

						for (edge_cursor1 = node_get_head_edge_dst(node_cursor1); edge_cursor1 != NULL; edge_cursor1 = edge_get_next_dst(edge_cursor1)){
							for (edge_cursor2 = node_get_head_edge_dst(node_cursor2), i = 0; edge_cursor2 != NULL; edge_cursor2 = edge_get_next_dst(edge_cursor2), i++){
								if (!taken[i]){
									if (edge_get_src(edge_cursor1) == edge_get_src(edge_cursor2)){
										taken[i] = 1;
										break;
									}
									else if (ir_node_get_operation(edge_get_src(edge_cursor1))->type == IR_OPERATION_TYPE_IMM && ir_node_get_operation(edge_get_src(edge_cursor2))->type == IR_OPERATION_TYPE_IMM){
										if (irOperation_equal(ir_node_get_operation(edge_get_src(edge_cursor1)), ir_node_get_operation(edge_get_src(edge_cursor2)))){
											taken[i] = 1;
											break;
										}
									}
								}
							}

							if (edge_cursor2 == NULL){
								break;
							}
						}

						for (i = 0, nb_match = 0; i < node_cursor1->nb_edge_dst; i++){
							nb_match += taken[i];
						}

						if (nb_match == node_cursor1->nb_edge_dst){
							graph_transfert_src_edge(&(ir->graph), node_cursor1, node_cursor2);
							ir_remove_node(ir, node_cursor2);
							*modification = 1;
						}
					}
				}
			}
		}
	}
}

static enum irOpcode ir_normalize_choose_part_opcode(uint8_t size_src, uint8_t size_dst){
	if (size_src == 32 && size_dst == 8){
		return IR_PART1_8;
	}
	else{
		printf("ERROR: in %s, this case is not implemented (src=%u, dst=%u)\n", __func__, size_src, size_dst);
		return IR_PART1_8;
	}
}

void ir_normalize_simplify_memory_access(struct ir* ir, uint8_t* modification){
	struct node* 	node_cursor;
	struct edge* 	edge_cursor;
	uint32_t 		nb_mem_access;
	struct node** 	access_list;
	uint32_t 		i;

	*modification = 0;

	for(node_cursor = graph_get_head_node(&(ir->graph)); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
		if (node_cursor->nb_edge_src > 1){
			for (edge_cursor = node_get_head_edge_src(node_cursor), nb_mem_access = 0; edge_cursor != NULL; edge_cursor = edge_get_next_src(edge_cursor)){
				if (ir_edge_get_dependence(edge_cursor)->type == IR_DEPENDENCE_TYPE_ADDRESS && (ir_node_get_operation(edge_get_dst(edge_cursor))->type == IR_OPERATION_TYPE_IN_MEM || ir_node_get_operation(edge_get_dst(edge_cursor))->type == IR_OPERATION_TYPE_OUT_MEM)){
					nb_mem_access ++;
				}
			}
			if (nb_mem_access > 1){
				access_list = (struct node**)malloc(sizeof(struct node*) * nb_mem_access);
				if (access_list == NULL){
					printf("ERROR: in %s, unable to allocate memory\n", __func__);
					continue;
				}

				for (edge_cursor = node_get_head_edge_src(node_cursor), i = 0; edge_cursor != NULL; edge_cursor = edge_get_next_src(edge_cursor)){
					if (ir_edge_get_dependence(edge_cursor)->type == IR_DEPENDENCE_TYPE_ADDRESS && (ir_node_get_operation(edge_get_dst(edge_cursor))->type == IR_OPERATION_TYPE_IN_MEM || ir_node_get_operation(edge_get_dst(edge_cursor))->type == IR_OPERATION_TYPE_OUT_MEM)){
						access_list[i++] = edge_get_dst(edge_cursor);
					}
				}

				qsort(access_list, nb_mem_access, sizeof(struct node*), compare_order_memoryNode);

				for (i = 1; i < nb_mem_access; i++){
					struct irOperation* 		operation_prev;
					struct irOperation* 		operation_next;

					operation_prev = ir_node_get_operation(access_list[i - 1]);
					operation_next = ir_node_get_operation(access_list[i]);

					/* STORE -> LOAD */
					if (operation_prev->type == IR_OPERATION_TYPE_OUT_MEM && operation_next->type == IR_OPERATION_TYPE_IN_MEM){
						struct node* stored_value = NULL;

						for (edge_cursor = node_get_head_edge_dst(access_list[i - 1]); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
							if (ir_edge_get_dependence(edge_cursor)->type != IR_DEPENDENCE_TYPE_ADDRESS){
								stored_value = edge_get_src(edge_cursor);
							}
						}

						if (stored_value != NULL){
							if (operation_prev->size > operation_next->size){
								ir_convert_node_to_inst(access_list[i], ir_normalize_choose_part_opcode(operation_prev->size, operation_next->size), operation_next->size)
								graph_remove_edge(&(ir->graph), graph_get_edge(node_cursor, access_list[i]));

								if (ir_add_dependence(ir, stored_value, access_list[i], IR_DEPENDENCE_TYPE_DIRECT) == NULL){
									printf("ERROR: in %s, unable to add new dependance to IR\n", __func__);
								}

								access_list[i] = access_list[i - 1];
							}
							else if (operation_prev->size < operation_next->size){
								printf("WARNING: in %s, simplification of memory access of different size (case STORE -> LOAD)\n", __func__);
							}
							else{
								graph_transfert_src_edge(&(ir->graph), stored_value, access_list[i]);
								ir_remove_node(ir, access_list[i]);

								access_list[i] = access_list[i - 1];
							}
							*modification = 1;
							continue;
						}
						else{
							printf("ERROR: in %s, incorrect memory access pattern in STORE -> LOAD\n", __func__);
						}
					}

					/* LOAD -> LOAD */
					if (operation_prev->type == IR_OPERATION_TYPE_IN_MEM && operation_next->type == IR_OPERATION_TYPE_IN_MEM){
						if (operation_prev->size > operation_next->size){
							ir_convert_node_to_inst(access_list[i], ir_normalize_choose_part_opcode(operation_prev->size, operation_next->size), operation_next->size)
							graph_remove_edge(&(ir->graph), graph_get_edge(node_cursor, access_list[i]));

							if (ir_add_dependence(ir, access_list[i - 1], access_list[i], IR_DEPENDENCE_TYPE_DIRECT) == NULL){
								printf("ERROR: in %s, unable to add new dependance to IR\n", __func__);
							}

							access_list[i] = access_list[i - 1];
						}
						else if (operation_prev->size < operation_next->size){
							ir_convert_node_to_inst(access_list[i - 1], ir_normalize_choose_part_opcode(operation_next->size, operation_prev->size), operation_prev->size)
							graph_remove_edge(&(ir->graph), graph_get_edge(node_cursor, access_list[i - 1]));

							if (ir_add_dependence(ir, access_list[i], access_list[i - 1], IR_DEPENDENCE_TYPE_DIRECT) == NULL){
								printf("ERROR: in %s, unable to add new dependance to IR\n", __func__);
							}
						}
						else{
							graph_transfert_src_edge(&(ir->graph), access_list[i - 1], access_list[i]);
							ir_remove_node(ir, access_list[i]);

							access_list[i] = access_list[i - 1];
						}
						*modification = 1;
						continue;
					}

					/* STORE -> STORE */
					if (operation_prev->type == IR_OPERATION_TYPE_OUT_MEM && operation_next->type == IR_OPERATION_TYPE_OUT_MEM){
						if (operation_prev->size > operation_next->size){
							printf("WARNING: in %s, simplification of memory access of different size (case STORE (%u bits) -> STORE (%u bits))\n", __func__, operation_prev->size, operation_next->size);
						}

						ir_remove_node(ir, access_list[i - 1]);
						*modification = 1;
						continue;
					}
				}

				free(access_list);
			}
		}
	}
}

void ir_normalize_detect_rotation(struct ir* ir){
	struct node* 		node_cursor;
	struct node* 		or_dep1;
	struct node* 		or_dep2;
	struct node* 		node_shiftr;
	struct node* 		node_shiftl;
	struct node* 		shiftr_dep1;
	struct node* 		shiftr_dep2;
	struct node* 		shiftl_dep1;
	struct node* 		shiftl_dep2;
	struct node* 		node_immr;
	struct node* 		node_imml;
	struct node* 		node_common;
	struct irOperation* operation;
	enum irOpcode* 		opcode_ptr;

	for(node_cursor = graph_get_head_node(&(ir->graph)); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
		operation = ir_node_get_operation(node_cursor);
		if (node_cursor->nb_edge_dst == 2 && operation->type == IR_OPERATION_TYPE_INST && operation->operation_type.inst.opcode == IR_OR){
			opcode_ptr 	= &(operation->operation_type.inst.opcode);
			or_dep1 	= edge_get_src(node_get_head_edge_dst(node_cursor));
			or_dep2 	= edge_get_src(edge_get_next_dst(node_get_head_edge_dst(node_cursor)));
			node_shiftl = NULL;
			node_shiftr = NULL;

			operation = ir_node_get_operation(or_dep1);
			if (operation->type == IR_OPERATION_TYPE_INST){
				if (operation->operation_type.inst.opcode == IR_SHR){
					node_shiftr = or_dep1;
				}
				else if (operation->operation_type.inst.opcode == IR_SHL){
					node_shiftl = or_dep1;
				}
			}

			operation = ir_node_get_operation(or_dep2);
			if (operation->type == IR_OPERATION_TYPE_INST){
				if (operation->operation_type.inst.opcode == IR_SHR){
					node_shiftr = or_dep2;
				}
				else if (operation->operation_type.inst.opcode == IR_SHL){
					node_shiftl = or_dep2;
				}
			}

			if (node_shiftl != NULL && node_shiftr != NULL){
				if (node_shiftl->nb_edge_dst == 2 && node_shiftr->nb_edge_dst == 2){

					shiftr_dep1 	= edge_get_src(node_get_head_edge_dst(node_shiftr));
					shiftr_dep2 	= edge_get_src(edge_get_next_dst(node_get_head_edge_dst(node_shiftr)));
					shiftl_dep1 	= edge_get_src(node_get_head_edge_dst(node_shiftl));
					shiftl_dep2 	= edge_get_src(edge_get_next_dst(node_get_head_edge_dst(node_shiftl)));
					node_immr 		= NULL;
					node_imml 		= NULL;
					node_common 	= NULL;

					if (shiftl_dep1 == shiftr_dep1 && shiftl_dep2 != shiftr_dep2){
						if (ir_node_get_operation(shiftl_dep2)->type == IR_OPERATION_TYPE_IMM && ir_node_get_operation(shiftr_dep2)->type == IR_OPERATION_TYPE_IMM){
							node_immr = shiftr_dep2;
							node_imml = shiftl_dep2;
							node_common = shiftl_dep1;
						}
					}
					else if (shiftl_dep1 == shiftr_dep2 && shiftl_dep2 != shiftr_dep1){
						if (ir_node_get_operation(shiftl_dep2)->type == IR_OPERATION_TYPE_IMM && ir_node_get_operation(shiftr_dep1)->type == IR_OPERATION_TYPE_IMM){
							node_immr = shiftr_dep1;
							node_imml = shiftl_dep2;
							node_common = shiftl_dep1;
						}
					}
					else if (shiftl_dep2 == shiftr_dep2 && shiftl_dep1 != shiftr_dep1){
						if (ir_node_get_operation(shiftl_dep1)->type == IR_OPERATION_TYPE_IMM && ir_node_get_operation(shiftr_dep1)->type == IR_OPERATION_TYPE_IMM){
							node_immr = shiftr_dep1;
							node_imml = shiftl_dep1;
							node_common = shiftl_dep2;
						}
					}

					if (node_immr != NULL && node_imml != NULL && node_common != NULL){
						#pragma GCC diagnostic ignored "-Wlong-long" /* use of C99 long long integer constant */
						if (ir_imm_operation_get_unsigned_value(ir_node_get_operation(node_immr)) + ir_imm_operation_get_unsigned_value(ir_node_get_operation(node_imml)) == ir_node_get_operation(node_cursor)->size){

							*opcode_ptr = IR_ROR;
							if (ir_add_dependence(ir, node_common, node_cursor, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
								printf("ERROR: in %s, unable to add dependence to the IR\n", __func__);
							}

							if (ir_add_dependence(ir, node_immr, node_cursor, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
								printf("ERROR: in %s, unable to add dependence to the IR\n", __func__);
							}

							if (node_shiftl->nb_edge_src == 1){
								ir_remove_node(ir, node_shiftl);
							}
							else{
								graph_remove_edge(&(ir->graph), graph_get_edge(node_shiftl, node_cursor));
							}

							if (node_shiftr->nb_edge_src == 1){
								ir_remove_node(ir, node_shiftr);
							}
							else{
								graph_remove_edge(&(ir->graph), graph_get_edge(node_shiftr, node_cursor));
							}
						}
					}
				}
			}
		}
	}
}

void ir_normalize_merge_associative_operation(struct ir* ir, enum irOpcode opcode){
	struct node* 			node_cursor;
	struct edge* 			edge_cursor;
	struct node* 			merge_node;
	struct irOperation* 	operation;

	for(node_cursor = graph_get_head_node(&(ir->graph)); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
		operation = ir_node_get_operation(node_cursor);
		if (operation->type == IR_OPERATION_TYPE_INST && operation->operation_type.inst.opcode == opcode){

			start:
			for(edge_cursor = node_get_head_edge_dst(node_cursor); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
				merge_node = edge_get_src(edge_cursor);
				operation = ir_node_get_operation(merge_node);
				if (operation->type == IR_OPERATION_TYPE_INST && operation->operation_type.inst.opcode == opcode){
					if (merge_node->nb_edge_src == 1){
						graph_merge_node(&(ir->graph), node_cursor, merge_node);
						ir_remove_node(ir, merge_node);
						goto start;
					}
					/* Here starts the swamp of doom and death */
					else if (opcode == IR_XOR){
						if (merge_node->nb_edge_dst > 100){
							printf("WARNING: in %s, uncommon number of dst edge (%u), do not develop\n", __func__, merge_node->nb_edge_dst);
						}
						else{
							graph_remove_edge(&(ir->graph), edge_cursor);
							for(edge_cursor = node_get_head_edge_dst(merge_node); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
								if (graph_add_edge(&(ir->graph), edge_get_src(edge_cursor), node_cursor, &(edge_cursor->data)) == NULL){
									printf("ERROR: in %s, unable to add edge to IR\n", __func__);
								}
							}
							goto start;
						}
					}
					else if (opcode == IR_ADD && merge_node->nb_edge_src == 2){
						struct edge* edge_cursor123;
						struct node* node_cursor123;

						for(edge_cursor123 = node_get_head_edge_src(merge_node); edge_cursor123 != NULL; edge_cursor123 = edge_get_next_src(edge_cursor123)){
							node_cursor123 = edge_get_dst(edge_cursor123);

							if (ir_node_get_operation(node_cursor123)->type == IR_OPERATION_TYPE_OUT_MEM){
								graph_remove_edge(&(ir->graph), edge_cursor);
								for(edge_cursor = node_get_head_edge_dst(merge_node); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
									if (graph_add_edge(&(ir->graph), edge_get_src(edge_cursor), node_cursor, &(edge_cursor->data)) == NULL){
										printf("ERROR: in %s, unable to add edge to IR\n", __func__);
									}
								}
								goto start;
							}
						}
					}
				}
			}
		}
	}
}

/* ===================================================================== */
/* sorting routines						                                 */
/* ===================================================================== */

static int32_t irNormalize_sort_node(struct ir* ir){
	struct node** 	node_buffer;
	uint32_t 		i;
	struct node* 	primary_cursor;
	uint32_t 		generator = IR_NODE_INDEX_FIRST;

	node_buffer = (struct node**)malloc(ir->graph.nb_node * sizeof(struct node*));
	if (node_buffer == NULL){
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
		return -1;
	}

	for(primary_cursor = graph_get_head_node(&(ir->graph)); primary_cursor != NULL; primary_cursor = node_get_next(primary_cursor)){
		ir_node_get_operation(primary_cursor)->index = IR_NODE_INDEX_UNSET;
	}

	for(primary_cursor = graph_get_head_node(&(ir->graph)); primary_cursor != NULL; primary_cursor = node_get_next(primary_cursor)){
		if (!irOperation_is_index_set(ir_node_get_operation(primary_cursor))){
			irNormalize_recursive_sort(node_buffer, primary_cursor, &generator);
		}
	}

	ir->graph.node_linkedList_head = node_buffer[0];
	node_buffer[0]->prev = NULL;
	if (ir->graph.nb_node > 1){
		node_buffer[0]->next = node_buffer[1];

		for (i = 1; i < ir->graph.nb_node - 1; i++){
			node_buffer[i]->prev = node_buffer[i - 1];
			node_buffer[i]->next = node_buffer[i + 1];
		}
			
		node_buffer[i]->prev = node_buffer[i - 1];
		node_buffer[i]->next = NULL;
		ir->graph.node_linkedList_tail = node_buffer[i];
	}
	else{
		node_buffer[0]->next = NULL;
		ir->graph.node_linkedList_tail = node_buffer[0];
	}

	free(node_buffer);

	return 0;
}

static void irNormalize_recursive_sort(struct node** node_buffer, struct node* node, uint32_t* generator){
	struct edge* edge_cursor;
	struct node* parent_node;

	ir_node_get_operation(node)->index = IR_NODE_INDEX_SETTING;
	for (edge_cursor = node_get_head_edge_dst(node); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
		parent_node = edge_get_src(edge_cursor);

		if (ir_node_get_operation(parent_node)->index == IR_NODE_INDEX_UNSET){
			irNormalize_recursive_sort(node_buffer, parent_node, generator);
		}
		else if (ir_node_get_operation(parent_node)->index == IR_NODE_INDEX_SETTING){
			printf("ERROR: in %s, cycle detected in graph\n", __func__);
		}
	}

	ir_node_get_operation(node)->index = *generator;
	node_buffer[*generator - IR_NODE_INDEX_FIRST] = node;
	*generator = *generator + 1;
}

int32_t compare_order_memoryNode(const void* arg1, const void* arg2){
	struct node* access1 = *(struct node**)arg1;
	struct node* access2 = *(struct node**)arg2;
	struct irOperation* op1 = ir_node_get_operation(access1);
	struct irOperation* op2 = ir_node_get_operation(access2);
	uint32_t order1;
	uint32_t order2;

	if (op1->type == IR_OPERATION_TYPE_IN_MEM){
		order1 = op1->operation_type.in_mem.order;
	}
	else{
		order1 = op1->operation_type.out_mem.order;	
	}

	if (op2->type == IR_OPERATION_TYPE_IN_MEM){
		order2 = op2->operation_type.in_mem.order;
	}
	else{
		order2 = op2->operation_type.out_mem.order;	
	}

	if (order1 < order2){
		return -1;
	}
	else if (order1 > order2){
		return 1;
	}
	else{
		return 0;
	}
}

int32_t compare_address_node_irOperand(const void* arg1, const void* arg2){
	struct irOperand* operand1 = (struct irOperand*)arg1;
	struct irOperand* operand2 = (struct irOperand*)arg2;

	if (operand1->node > operand2->node){
		return 1;
	}
	else if (operand1->node < operand2->node){
		return -1;
	}
	else{
		return 0;
	}
}