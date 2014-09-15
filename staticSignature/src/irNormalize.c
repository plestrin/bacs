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
#define IR_NORMALIZE_REMOVE_SUBEXPRESSION 			1
#define IR_NORMALIZE_SIMPLIFY_MEMORY_ACCESS 		1
#define IR_NORMALIZE_DISTRIBUTE_IMMEDIATE 			1
#define IR_NORMALIZE_DETECT_ROTATION 				0
#define IR_NORMALIZE_MERGE_ASSOCIATIVE_ADD 			1
#define IR_NORMALIZE_MERGE_ASSOCIATIVE_XOR 			1

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
	uint32_t a = 0;
	uint8_t modification_copy;
	INIT_TIMER
	
	{
		uint8_t modification = 1;
		#ifdef VERBOSE
		double timer_1_elapsed_time = 0.0;
		double timer_2_elapsed_time = 0.0;
		double timer_3_elapsed_time = 0.0;
		double timer_4_elapsed_time = 0.0;
		#endif

		while(modification){
			modification = 0;

			printf("*** ROUND %u ***\n", a);
			modification_copy = 0;

			#if IR_NORMALIZE_SIMPLIFY_INSTRUCTION == 1
			START_TIMER
			ir_normalize_simplify_instruction(ir, &modification, 0);
			STOP_TIMER
			#ifdef VERBOSE
			timer_1_elapsed_time += (timer_stop_time.tv_sec - timer_start_time.tv_sec) + (timer_stop_time.tv_nsec - timer_start_time.tv_nsec) / 1000000000.;
			#endif
			#endif

			if (modification){
				printf("INFO: in %s, modification simplify instruction @ %u\n", __func__, a);
				modification = 0;
				modification_copy = 1;
			}


			#if IR_NORMALIZE_REMOVE_SUBEXPRESSION == 1
			START_TIMER
			ir_normalize_remove_subexpression(ir, &modification);
			STOP_TIMER
			#ifdef VERBOSE
			timer_2_elapsed_time += (timer_stop_time.tv_sec - timer_start_time.tv_sec) + (timer_stop_time.tv_nsec - timer_start_time.tv_nsec) / 1000000000.;
			#endif
			#endif

			if (modification){
				printf("INFO: in %s, modification remove subexpression @ %u\n", __func__, a);
				modification = 0;
				modification_copy = 1;
			}

			#if IR_NORMALIZE_SIMPLIFY_MEMORY_ACCESS == 1
			START_TIMER
			ir_normalize_simplify_memory_access(ir, &modification);
			STOP_TIMER
			#ifdef VERBOSE
			timer_3_elapsed_time += (timer_stop_time.tv_sec - timer_start_time.tv_sec) + (timer_stop_time.tv_nsec - timer_start_time.tv_nsec) / 1000000000.;
			#endif
			#endif

			if (modification){
				printf("INFO: in %s, modification simplify memory @ %u\n", __func__, a);
				modification = 0;
				modification_copy = 1;
			}

			#if IR_NORMALIZE_DISTRIBUTE_IMMEDIATE == 1
			START_TIMER
			ir_normalize_distribute_immediate(ir, &modification);
			STOP_TIMER
			#ifdef VERBOSE
			timer_4_elapsed_time += (timer_stop_time.tv_sec - timer_start_time.tv_sec) + (timer_stop_time.tv_nsec - timer_start_time.tv_nsec) / 1000000000.;
			#endif
			#endif

			if (modification){
				printf("INFO: in %s, modification distribute immediate @ %u\n", __func__, a);
				modification_copy = 1;
			}

			modification = modification_copy;
			a++;
		}

		#if IR_NORMALIZE_SIMPLIFY_INSTRUCTION == 1
		START_TIMER
		ir_normalize_simplify_instruction(ir, &modification, 1);
		STOP_TIMER
		#ifdef VERBOSE
		timer_1_elapsed_time += (timer_stop_time.tv_sec - timer_start_time.tv_sec) + (timer_stop_time.tv_nsec - timer_start_time.tv_nsec) / 1000000000.;
		#endif
		#endif

		#ifdef VERBOSE
		multiColumnPrinter_print(printer, "Simplify instruction", timer_1_elapsed_time, NULL);
		multiColumnPrinter_print(printer, "Remove subexpression", timer_2_elapsed_time, NULL);
		multiColumnPrinter_print(printer, "Simplify memory access", timer_3_elapsed_time, NULL);
		multiColumnPrinter_print(printer, "Distribute immediate", timer_4_elapsed_time, NULL);
		#endif
	}

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

static void ir_normalize_simplify_instruction_numeric_add(struct ir* ir, struct node* node, uint8_t* modification);
static void ir_normalize_simplify_instruction_rewrite_add(struct ir* ir, struct node* node, uint8_t* modification, uint8_t final);
static void ir_normalize_simplify_instruction_numeric_and(struct ir* ir, struct node* node, uint8_t* modification);
static void ir_normalize_simplify_instruction_rewrite_and(struct ir* ir, struct node* node, uint8_t* modification, uint8_t final);
static void ir_normalize_simplify_instruction_numeric_imul(struct ir* ir, struct node* node, uint8_t* modification);
static void ir_normalize_simplify_instruction_rewrite_imul(struct ir* ir, struct node* node, uint8_t* modification, uint8_t final);
static void ir_normalize_simplify_instruction_rewrite_movzx(struct ir* ir, struct node* node, uint8_t* modification, uint8_t final);
static void ir_normalize_simplify_instruction_numeric_mul(struct ir* ir, struct node* node, uint8_t* modification);
static void ir_normalize_simplify_instruction_rewrite_mul(struct ir* ir, struct node* node, uint8_t* modification, uint8_t final);
static void ir_normalize_simplify_instruction_symbolic_or(struct ir* ir, struct node* node, uint8_t* modification);
static void ir_normalize_simplify_instruction_rewrite_or(struct ir* ir, struct node* node, uint8_t* modification, uint8_t final);
static void ir_normalize_simplify_instruction_rewrite_part1_8(struct ir* ir, struct node* node, uint8_t* modification, uint8_t final);
static void ir_normalize_simplify_instruction_rewrite_rol(struct ir* ir, struct node* node, uint8_t* modification, uint8_t final);
static void ir_normalize_simplify_instruction_numeric_shl(struct ir* ir, struct node* node, uint8_t* modification);
static void ir_normalize_simplify_instruction_rewrite_shl(struct ir* ir, struct node* node, uint8_t* modification, uint8_t final);
static void ir_normalize_simplify_instruction_rewrite_shr(struct ir* ir, struct node* node, uint8_t* modification, uint8_t final);
static void ir_normalize_simplify_instruction_rewrite_sub(struct ir* ir, struct node* node, uint8_t* modification, uint8_t final);
static void ir_normalize_simplify_instruction_symbolic_xor(struct ir* ir, struct node* node, uint8_t* modification);
static void ir_normalize_simplify_instruction_rewrite_xor(struct ir* ir, struct node* node, uint8_t* modification, uint8_t final);


void ir_normalize_simplify_instruction(struct ir* ir, uint8_t* modification, uint8_t final){
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
				case IR_ADD 		: {
					ir_normalize_simplify_instruction_numeric_add(ir, node_cursor, modification);
					ir_normalize_simplify_instruction_rewrite_add(ir, node_cursor, modification, final);
					break;
				}
				case IR_AND 		: {
					ir_normalize_simplify_instruction_numeric_and(ir, node_cursor, modification);
					ir_normalize_simplify_instruction_rewrite_and(ir, node_cursor, modification, final);
					break;
				}
				case IR_IMUL 		: {
					ir_normalize_simplify_instruction_numeric_imul(ir, node_cursor, modification);
					ir_normalize_simplify_instruction_rewrite_imul(ir, node_cursor, modification, final);
					break;
				}
				case IR_MOVZX 		: {
					ir_normalize_simplify_instruction_rewrite_movzx(ir, node_cursor, modification, final);
					break;
				}
				case IR_MUL 		: {
					ir_normalize_simplify_instruction_numeric_mul(ir, node_cursor, modification);
					ir_normalize_simplify_instruction_rewrite_mul(ir, node_cursor, modification, final);
					break;
				}
				case IR_OR 			: {
					ir_normalize_simplify_instruction_symbolic_or(ir, node_cursor, modification);
					ir_normalize_simplify_instruction_rewrite_or(ir, node_cursor, modification, final);
					break;
				}
				case IR_PART1_8 	: {
					ir_normalize_simplify_instruction_rewrite_part1_8(ir, node_cursor, modification, final);
					break;
				}
				case IR_ROL 		: {
					ir_normalize_simplify_instruction_rewrite_rol(ir, node_cursor, modification, final);
					break;
				}
				case IR_SHL 		: {
					ir_normalize_simplify_instruction_numeric_shl(ir, node_cursor, modification);
					ir_normalize_simplify_instruction_rewrite_shl(ir, node_cursor, modification, final);
					break;
				}
				case IR_SHR 		: {
					ir_normalize_simplify_instruction_rewrite_shr(ir, node_cursor, modification, final);
					break;
				}
				case IR_SUB 		: {
					ir_normalize_simplify_instruction_rewrite_sub(ir, node_cursor, modification, final);
					break;
				}
				case IR_XOR 		: {
					ir_normalize_simplify_instruction_symbolic_xor(ir, node_cursor, modification);
					ir_normalize_simplify_instruction_rewrite_xor(ir, node_cursor, modification, final);
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

static void ir_normalize_simplify_instruction_numeric_add(struct ir* ir, struct node* node, uint8_t* modification){
	uint32_t 			nb_imm_operand;
	struct edge* 		edge_cursor;
	struct edge* 		edge_cursor_next;
	struct node* 		operand;
	struct irOperation*	operand_operation;
	struct node* 		possible_rewrite;
	uint64_t 			value 	= 0;
	uint8_t 			size 	= 0;
	uint8_t 			signe 	= 0;

	if (node->nb_edge_dst > 1){
		for (edge_cursor = node_get_head_edge_dst(node), nb_imm_operand = 0, possible_rewrite = NULL; edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
			operand = edge_get_src(edge_cursor);
			operand_operation = ir_node_get_operation(operand);

			if (operand_operation->type == IR_OPERATION_TYPE_IMM){
				nb_imm_operand ++;
				if (operand_operation->operation_type.imm.signe){
					#pragma GCC diagnostic ignored "-Wlong-long" /* use of C99 long long integer constant */
					value += ir_imm_operation_get_signed_value(operand_operation);
				}
				else{
					#pragma GCC diagnostic ignored "-Wlong-long" /* use of C99 long long integer constant */
					value += ir_imm_operation_get_unsigned_value(operand_operation);
				}
				size = (size > operand_operation->size) ? size : operand_operation->size;
				signe = (operand_operation->operation_type.imm.signe) ? 1 : 0;

				if (operand->nb_edge_src == 1){
					possible_rewrite = operand;
				}
			}
		}

		if (nb_imm_operand > 1){
			if ((value & (0xffffffffffffffffULL >> (64 - size))) != 0){
				if (possible_rewrite == NULL){
					possible_rewrite = ir_add_immediate(ir, size, signe, value);
					if (possible_rewrite == NULL){
						printf("ERROR: in %s, unable to add immediate to IR\n", __func__);
						return;
					}
					else{
						if (ir_add_dependence(ir, possible_rewrite, node, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
							printf("ERROR: in %s, unable to add dependency to IR\n", __func__);
						}
					}
				}
				else{
					ir_node_get_operation(possible_rewrite)->operation_type.imm.signe = signe;
					ir_node_get_operation(possible_rewrite)->operation_type.imm.value = value;
					ir_node_get_operation(possible_rewrite)->size = size;
				}
			}
			else{
				possible_rewrite = NULL;
			}

			for (edge_cursor = node_get_head_edge_dst(node); edge_cursor != NULL; edge_cursor = edge_cursor_next){
				operand = edge_get_src(edge_cursor);
				operand_operation = ir_node_get_operation(operand);
				edge_cursor_next = edge_get_next_dst(edge_cursor);

				if (operand_operation->type == IR_OPERATION_TYPE_IMM && operand != possible_rewrite){
					ir_remove_dependence(ir, edge_cursor);
				}
			}
			*modification = 1;
		}
	}
}

#pragma GCC diagnostic ignored "-Wunused-parameter"
static void ir_normalize_simplify_instruction_rewrite_add(struct ir* ir, struct node* node, uint8_t* modification, uint8_t final){
	uint32_t 			nb_imm_operand;
	uint32_t 			nb_add_operand;
	struct edge* 		edge_cursor1;
	struct edge* 		edge_cursor2;
	struct irOperation*	operand_operation;
	struct edge* 		add_operand;

	if (node->nb_edge_dst == 1){
		graph_transfert_src_edge(&(ir->graph), edge_get_src(node_get_head_edge_dst(node)), node);
		ir_remove_node(ir, node);
		*modification = 1;
	}
	else{
		for (edge_cursor1 = node_get_head_edge_dst(node), nb_imm_operand = 0, nb_add_operand = 0, add_operand = NULL; edge_cursor1 != NULL; edge_cursor1 = edge_get_next_dst(edge_cursor1)){
			operand_operation = ir_node_get_operation(edge_get_src(edge_cursor1));

			if (operand_operation->type == IR_OPERATION_TYPE_IMM){
				nb_imm_operand ++;
			}
			else if (operand_operation->type == IR_OPERATION_TYPE_INST && operand_operation->operation_type.inst.opcode == IR_ADD){
				nb_add_operand ++;
				for (edge_cursor2 = node_get_head_edge_dst(edge_get_src(edge_cursor1)); edge_cursor2 != NULL; edge_cursor2 = edge_get_next_dst(edge_cursor2)){
					if (ir_node_get_operation(edge_get_src(edge_cursor2))->type == IR_OPERATION_TYPE_IMM){
						add_operand = edge_cursor1;
					}
				}
			}
			else{
				return;
			}
		}

		if (nb_imm_operand > 0 && nb_add_operand > 0 && add_operand != NULL){
			if (nb_add_operand > 1){
				printf("WARNING: in %s, multiple ADD operands, can't decide how to associate IMM\n", __func__);
				return;
			}

			if (graph_copy_dst_edge(&(ir->graph), node, edge_get_src(add_operand))){
				printf("ERROR: in %s, unable to copy dst edge\n", __func__);
			}
			ir_remove_dependence(ir, add_operand);
			*modification = 1;
		}
	}
}

static void ir_normalize_simplify_instruction_numeric_and(struct ir* ir, struct node* node, uint8_t* modification){
	uint32_t 			nb_imm_operand;
	struct edge* 		edge_cursor;
	struct edge* 		edge_cursor_next;
	struct node* 		operand;
	struct irOperation*	operand_operation;
	struct node* 		possible_rewrite;
	uint64_t 			value 	= 0xffffffffffffffff;
	uint8_t 			size 	= 0;

	if (node->nb_edge_dst > 1){
		for (edge_cursor = node_get_head_edge_dst(node), nb_imm_operand = 0, possible_rewrite = NULL; edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
			operand = edge_get_src(edge_cursor);
			operand_operation = ir_node_get_operation(operand);

			if (operand_operation->type == IR_OPERATION_TYPE_IMM){
				nb_imm_operand ++;
				#pragma GCC diagnostic ignored "-Wlong-long" /* use of C99 long long integer constant */
				value &= ir_imm_operation_get_unsigned_value(operand_operation);
				size = (size > operand_operation->size) ? size : operand_operation->size;

				if (operand->nb_edge_src == 1){
					possible_rewrite = operand;
				}
			}
		}

		if (nb_imm_operand > 1){
			if (possible_rewrite == NULL){
				possible_rewrite = ir_add_immediate(ir, size, 0, value);
				if (possible_rewrite == NULL){
					printf("ERROR: in %s, unable to add immediate to IR\n", __func__);
					return;
				}
				else{
					if (ir_add_dependence(ir, possible_rewrite, node, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
						printf("ERROR: in %s, unable to add dependency to IR\n", __func__);
					}
				}
			}
			else{
				ir_node_get_operation(possible_rewrite)->operation_type.imm.signe = 0;
				ir_node_get_operation(possible_rewrite)->operation_type.imm.value = value;
				ir_node_get_operation(possible_rewrite)->size = size;
			}

			for (edge_cursor = node_get_head_edge_dst(node); edge_cursor != NULL; edge_cursor = edge_cursor_next){
				operand = edge_get_src(edge_cursor);
				operand_operation = ir_node_get_operation(operand);
				edge_cursor_next = edge_get_next_dst(edge_cursor);

				if (operand_operation->type == IR_OPERATION_TYPE_IMM && operand != possible_rewrite){
					ir_remove_dependence(ir, edge_cursor);
				}
			}
			*modification = 1;
		}
	}
}

#pragma GCC diagnostic ignored "-Wunused-parameter"
static void ir_normalize_simplify_instruction_rewrite_and(struct ir* ir, struct node* node, uint8_t* modification, uint8_t final){
	uint32_t 			nb_imm_operand;
	uint32_t 			nb_and_operand;
	struct edge* 		edge_cursor1;
	struct edge* 		edge_cursor2;
	struct irOperation*	operand_operation;
	struct edge* 		and_operand;

	if (node->nb_edge_dst == 1){
		graph_transfert_src_edge(&(ir->graph), edge_get_src(node_get_head_edge_dst(node)), node);
		ir_remove_node(ir, node);
		*modification = 1;
	}
	else{
		for (edge_cursor1 = node_get_head_edge_dst(node), nb_imm_operand = 0, nb_and_operand = 0, and_operand = NULL; edge_cursor1 != NULL; edge_cursor1 = edge_get_next_dst(edge_cursor1)){
			operand_operation = ir_node_get_operation(edge_get_src(edge_cursor1));

			if (operand_operation->type == IR_OPERATION_TYPE_IMM){
				nb_imm_operand ++;
			}
			else if (operand_operation->type == IR_OPERATION_TYPE_INST && operand_operation->operation_type.inst.opcode == IR_AND){
				nb_and_operand ++;
				for (edge_cursor2 = node_get_head_edge_dst(edge_get_src(edge_cursor1)); edge_cursor2 != NULL; edge_cursor2 = edge_get_next_dst(edge_cursor2)){
					if (ir_node_get_operation(edge_get_src(edge_cursor2))->type == IR_OPERATION_TYPE_IMM){
						and_operand = edge_cursor1;
					}
				}
			}
			else{
				return;
			}
		}

		if (nb_imm_operand > 0 && nb_and_operand > 0 && and_operand != NULL){
			if (nb_and_operand > 1){
				printf("WARNING: in %s, multiple AND operands, can't decide how to associate IMM\n", __func__);
				return;
			}

			if (graph_copy_dst_edge(&(ir->graph), node, edge_get_src(and_operand))){
				printf("ERROR: in %s, unable to copy dst edge\n", __func__);
			}
			ir_remove_dependence(ir, and_operand);
			*modification = 1;
		}
	}
}

static void ir_normalize_simplify_instruction_numeric_imul(struct ir* ir, struct node* node, uint8_t* modification){
	uint32_t 			nb_imm_operand;
	struct edge* 		edge_cursor;
	struct edge* 		edge_cursor_next;
	struct node* 		operand;
	struct irOperation*	operand_operation;
	struct node* 		possible_rewrite;
	int64_t 			value 	= 1;
	uint8_t 			size 	= 0;
	uint8_t 			signe 	= 0;

	if (node->nb_edge_dst > 1){
		for (edge_cursor = node_get_head_edge_dst(node), nb_imm_operand = 0, possible_rewrite = NULL; edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
			operand = edge_get_src(edge_cursor);
			operand_operation = ir_node_get_operation(operand);

			if (operand_operation->type == IR_OPERATION_TYPE_IMM){
				nb_imm_operand ++;
				if (operand_operation->operation_type.imm.signe){
					#pragma GCC diagnostic ignored "-Wlong-long" /* use of C99 long long integer constant */
					value = value * ir_imm_operation_get_signed_value(operand_operation);
				}
				else{
					#pragma GCC diagnostic ignored "-Wlong-long" /* use of C99 long long integer constant */
					value = value * ir_imm_operation_get_unsigned_value(operand_operation);
				}
				size = (size > operand_operation->size) ? size : operand_operation->size;
				signe = (signe || operand_operation->operation_type.imm.signe) ? 1 : 0;

				if (operand->nb_edge_src == 1){
					possible_rewrite = operand;
				}
			}
		}

		if (nb_imm_operand > 1){
			if (value == 0){
				possible_rewrite = NULL;
			}
			else{
				if (possible_rewrite == NULL){
					possible_rewrite = ir_add_immediate(ir, size, signe, value);
					if (possible_rewrite == NULL){
						printf("ERROR: in %s, unable to add immediate to IR\n", __func__);
						return;
					}
					else{
						if (ir_add_dependence(ir, possible_rewrite, node, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
							printf("ERROR: in %s, unable to add dependency to IR\n", __func__);
						}
					}
				}
				else{
					ir_node_get_operation(possible_rewrite)->operation_type.imm.value = signe;
					ir_node_get_operation(possible_rewrite)->operation_type.imm.value = value;
					ir_node_get_operation(possible_rewrite)->size = size;
				}
			}

			for (edge_cursor = node_get_head_edge_dst(node); edge_cursor != NULL; edge_cursor = edge_cursor_next){
				operand = edge_get_src(edge_cursor);
				operand_operation = ir_node_get_operation(operand);
				edge_cursor_next = edge_get_next_dst(edge_cursor);

				if (operand_operation->type == IR_OPERATION_TYPE_IMM && operand != possible_rewrite){
					ir_remove_dependence(ir, edge_cursor);
				}
			}
			*modification = 1;
		}
	}
}

#pragma GCC diagnostic ignored "-Wunused-parameter"
static void ir_normalize_simplify_instruction_rewrite_imul(struct ir* ir, struct node* node, uint8_t* modification, uint8_t final){
	struct edge* 			edge_cursor;
	struct edge* 			edge_current;
	struct irOperation*		operation_cursor;

	if (node->nb_edge_dst == 0){
		ir_remove_node(ir, node);
		*modification = 1;
	}
	else if (node->nb_edge_dst == 1){
		graph_transfert_src_edge(&(ir->graph), edge_get_src(node_get_head_edge_dst(node)), node);
		ir_remove_node(ir, node);
		*modification = 1;
	}
	else{
		for (edge_cursor = node_get_head_edge_dst(node); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
			operation_cursor = ir_node_get_operation(edge_get_src(edge_cursor));

			if (operation_cursor->type == IR_OPERATION_TYPE_IMM && ir_imm_operation_get_unsigned_value(operation_cursor) == 0){
				for (edge_cursor = node_get_head_edge_dst(node); edge_cursor != NULL;){
					edge_current = edge_cursor;
					edge_cursor = edge_get_next_dst(edge_cursor);

					ir_remove_dependence(ir, edge_current);
				}

				ir_node_get_operation(node)->type = IR_OPERATION_TYPE_IMM;
				ir_node_get_operation(node)->operation_type.imm.signe = 0;
				ir_node_get_operation(node)->operation_type.imm.value = 0;
				ir_node_get_operation(node)->status_flag = IR_NODE_STATUS_FLAG_NONE;

				*modification = 1;
				break;
			}
		}
	}
}

static void ir_normalize_simplify_instruction_rewrite_movzx(struct ir* ir, struct node* node, uint8_t* modification, uint8_t final){
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
						*modification = 1;
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
					if (ir_add_dependence(ir, node_imm_new, operand, IR_DEPENDENCE_TYPE_SHIFT_DISP) == NULL){
						printf("ERROR: in %s, unable to add edge to IR\n", __func__);
					}
					else{
						*modification = 1;
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
		else if (final){
			for (edge_cursor = node_get_head_edge_dst(node); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
				if (graph_copy_src_edge(&(ir->graph), edge_get_src(edge_cursor), node)){
					printf("ERROR: in %s, unable to copy dst edge\n", __func__);
					break;
				}
			}
			ir_remove_node(ir, node);
		}
	}
}

static void ir_normalize_simplify_instruction_numeric_mul(struct ir* ir, struct node* node, uint8_t* modification){
	uint32_t 			nb_imm_operand;
	struct edge* 		edge_cursor;
	struct edge* 		edge_cursor_next;
	struct node* 		operand;
	struct irOperation*	operand_operation;
	struct node* 		possible_rewrite;
	uint64_t 			value 	= 1;
	uint8_t 			size 	= 0;
	uint8_t 			signe 	= 0;

	if (node->nb_edge_dst > 1){
		for (edge_cursor = node_get_head_edge_dst(node), nb_imm_operand = 0, possible_rewrite = NULL; edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
			operand = edge_get_src(edge_cursor);
			operand_operation = ir_node_get_operation(operand);

			if (operand_operation->type == IR_OPERATION_TYPE_IMM){
				nb_imm_operand ++;
				if (operand_operation->operation_type.imm.signe){
					#pragma GCC diagnostic ignored "-Wlong-long" /* use of C99 long long integer constant */
					value = value * ir_imm_operation_get_signed_value(operand_operation);
				}
				else{
					#pragma GCC diagnostic ignored "-Wlong-long" /* use of C99 long long integer constant */
					value = value * ir_imm_operation_get_unsigned_value(operand_operation);
				}
				size = (size > operand_operation->size) ? size : operand_operation->size;
				signe = (signe || operand_operation->operation_type.imm.signe) ? 1 : 0;

				if (operand->nb_edge_src == 1){
					possible_rewrite = operand;
				}
			}
		}

		if (nb_imm_operand > 1){
			if (value == 0){
				possible_rewrite = NULL;
			}
			else{
				if (possible_rewrite == NULL){
					possible_rewrite = ir_add_immediate(ir, size, signe, value);
					if (possible_rewrite == NULL){
						printf("ERROR: in %s, unable to add immediate to IR\n", __func__);
						return;
					}
					else{
						if (ir_add_dependence(ir, possible_rewrite, node, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
							printf("ERROR: in %s, unable to add dependency to IR\n", __func__);
						}
					}
				}
				else{
					ir_node_get_operation(possible_rewrite)->operation_type.imm.value = signe;
					ir_node_get_operation(possible_rewrite)->operation_type.imm.value = value;
					ir_node_get_operation(possible_rewrite)->size = size;
				}
			}

			for (edge_cursor = node_get_head_edge_dst(node); edge_cursor != NULL; edge_cursor = edge_cursor_next){
				operand = edge_get_src(edge_cursor);
				operand_operation = ir_node_get_operation(operand);
				edge_cursor_next = edge_get_next_dst(edge_cursor);

				if (operand_operation->type == IR_OPERATION_TYPE_IMM && operand != possible_rewrite){
					ir_remove_dependence(ir, edge_cursor);
				}
			}
			*modification = 1;
		}
	}
}

#pragma GCC diagnostic ignored "-Wunused-parameter"
static void ir_normalize_simplify_instruction_rewrite_mul(struct ir* ir, struct node* node, uint8_t* modification, uint8_t final){
	if (node->nb_edge_dst == 0){
		ir_remove_node(ir, node);
		*modification = 1;
	}
	else if (node->nb_edge_dst == 1){
		graph_transfert_src_edge(&(ir->graph), edge_get_src(node_get_head_edge_dst(node)), node);
		ir_remove_node(ir, node);
		*modification = 1;
	}
	else if (node->nb_edge_dst == 2){
		struct edge* 			edge_cursor;
		struct node* 			node_cursor;
		struct irOperation* 	operation_cursor;

		for (edge_cursor = node_get_head_edge_dst(node); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
			node_cursor = edge_get_src(edge_cursor);
			operation_cursor = ir_node_get_operation(node_cursor);

			if (operation_cursor->type == IR_OPERATION_TYPE_IMM && operation_cursor->operation_type.imm.signe == 0 && __builtin_popcount(ir_imm_operation_get_unsigned_value(operation_cursor)) == 1){
				ir_node_get_operation(node)->operation_type.inst.opcode = IR_SHL;
				operation_cursor->operation_type.imm.value = __builtin_ctz(ir_imm_operation_get_unsigned_value(operation_cursor));
				ir_edge_get_dependence(edge_cursor)->type = IR_DEPENDENCE_TYPE_SHIFT_DISP;

				*modification = 1;
				break;
			}
		}
	}
}

static void ir_normalize_simplify_instruction_symbolic_or(struct ir* ir, struct node* node, uint8_t* modification){
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
				*modification = 1;
			}
		}
	}
}

#pragma GCC diagnostic ignored "-Wunused-parameter"
static void ir_normalize_simplify_instruction_rewrite_or(struct ir* ir, struct node* node, uint8_t* modification, uint8_t final){
	if (node->nb_edge_dst == 1){
		graph_transfert_src_edge(&(ir->graph), edge_get_src(node_get_head_edge_dst(node)), node);
		ir_remove_node(ir, node);
		*modification = 1;
	}
}

#pragma GCC diagnostic ignored "-Wunused-parameter"
static void ir_normalize_simplify_instruction_rewrite_part1_8(struct ir* ir, struct node* node, uint8_t* modification, uint8_t final){
	struct node* 		operand;
	struct irOperation*	operand_operation;
	struct edge*		edge_cursor;

	if (node->nb_edge_dst == 1){
		operand = edge_get_src(node_get_head_edge_dst(node));
		operand_operation = ir_node_get_operation(operand);
		if (operand_operation->type == IR_OPERATION_TYPE_INST && operand_operation->operation_type.inst.opcode == IR_MOVZX){
			if (operand->nb_edge_src == 1){
				for (edge_cursor = node_get_head_edge_dst(node); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
					if (graph_copy_src_edge(&(ir->graph), edge_get_src(edge_cursor), operand)){
						printf("ERROR: in %s, unable to copy dst edge\n", __func__);
						break;
					}
				}

				ir_remove_node(ir, operand);
				ir_remove_node(ir, node);

				*modification = 1;
			}
			else{
				printf("WARNING: in %s, MOVZX instruction is shared, this case is not implemented yet -> skip\n", __func__);
			}
		}
	}
}

#pragma GCC diagnostic ignored "-Wunused-parameter"
static void ir_normalize_simplify_instruction_rewrite_rol(struct ir* ir, struct node* node, uint8_t* modification, uint8_t final){
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

					*modification = 1;
					break;
				}
				else{
					printf("WARNING: in %s, found IMM operand but it is shared -> skip\n", __func__);
				}
			}
		}
	}
}

static void ir_normalize_simplify_instruction_numeric_shl(struct ir* ir, struct node* node, uint8_t* modification){
	struct edge* 		edge_cursor;
	struct node* 		operand;
	struct irOperation*	operand_operation;
	struct node* 		possible_rewrite_node;
	struct edge* 		possible_rewrite_edge;

	struct edge* 		op1 = NULL;
	struct edge* 		op2 = NULL;

	if (node->nb_edge_dst == 2){
		for (edge_cursor = node_get_head_edge_dst(node), possible_rewrite_edge = NULL; edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
			operand = edge_get_src(edge_cursor);
			operand_operation = ir_node_get_operation(operand);

			if (operand_operation->type == IR_OPERATION_TYPE_IMM){
				if (ir_edge_get_dependence(edge_cursor)->type == IR_DEPENDENCE_TYPE_DIRECT){
					op1 = edge_cursor;
				}
				else if (ir_edge_get_dependence(edge_cursor)->type == IR_DEPENDENCE_TYPE_SHIFT_DISP){
					op2 = edge_cursor;
				}

				if (operand->nb_edge_src == 1){
					possible_rewrite_edge = edge_cursor;
				}
			}
			else{
				return;
			}
		}

		if (op1 != NULL && op2 != NULL){
			struct irOperation* operation1 = ir_node_get_operation(edge_get_src(op1));
			struct irOperation* operation2 = ir_node_get_operation(edge_get_src(op2));

			if (possible_rewrite_edge == NULL){
				possible_rewrite_node = ir_add_immediate(ir, ir_node_get_operation(node)->size, 0, ir_imm_operation_get_unsigned_value(operation1) << ir_imm_operation_get_unsigned_value(operation2));
				if (possible_rewrite_node == NULL){
					printf("ERROR: in %s, unable to add immediate to IR\n", __func__);
					return;
				}
				else{
					if (ir_add_dependence(ir, possible_rewrite_node, node, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
						printf("ERROR: in %s, unable to add dependency to IR\n", __func__);
					}
				}
			}
			else{
				possible_rewrite_node = edge_get_src(possible_rewrite_edge);
				ir_node_get_operation(possible_rewrite_node)->operation_type.imm.value = ir_imm_operation_get_unsigned_value(operation1) << ir_imm_operation_get_unsigned_value(operation2);
				ir_node_get_operation(possible_rewrite_node)->size = ir_node_get_operation(node)->size;
				ir_edge_get_dependence(possible_rewrite_edge)->type = IR_DEPENDENCE_TYPE_DIRECT;
			}

			if (op1 != possible_rewrite_edge){
				ir_remove_dependence(ir, op1);
			}

			if (op2 != possible_rewrite_edge){
				ir_remove_dependence(ir, op2);
			}

			*modification = 1;
		}
	}
	else if (node->nb_edge_dst > 2){
		printf("WARNING: in %s, incorrect format SHL: %u operand(s)\n", __func__, node->nb_edge_dst);
	}
}

#pragma GCC diagnostic ignored "-Wunused-parameter"
static void ir_normalize_simplify_instruction_rewrite_shl(struct ir* ir, struct node* node, uint8_t* modification, uint8_t final){
	struct edge* 		edge_cursor;
	struct node* 		node_cursor;
	struct irOperation*	operation_cursor;
	struct edge*		edge1;
	struct edge* 		edge2;
	struct edge* 		edge3;
	struct node* 		node1;
	struct node*		node2;
	struct node*		node3;
	uint8_t 			simplify;

	if (node->nb_edge_dst == 1){
		graph_transfert_src_edge(&(ir->graph), edge_get_src(node_get_head_edge_dst(node)), node);
		ir_remove_node(ir, node);
		*modification = 1;
	}
	else{
		for (edge_cursor = node_get_head_edge_dst(node), simplify = 0, node1 = NULL; edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
			if (ir_edge_get_dependence(edge_cursor)->type == IR_DEPENDENCE_TYPE_SHIFT_DISP && ir_node_get_operation(edge_get_src(edge_cursor))->type == IR_OPERATION_TYPE_IMM && ir_imm_operation_get_unsigned_value(ir_node_get_operation(edge_get_src(edge_cursor))) == 0){
				simplify = 1;
			}
			else if (ir_edge_get_dependence(edge_cursor)->type == IR_DEPENDENCE_TYPE_DIRECT){
				node1 = edge_get_src(edge_cursor);
			}
		}

		if (simplify && node1 != NULL){
			graph_transfert_src_edge(&(ir->graph), node1, node);
			ir_remove_node(ir, node);
			*modification = 1;
			return;
		}

		for (edge_cursor = node_get_head_edge_dst(node), edge1 = NULL, edge2 = NULL; edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
			node_cursor = edge_get_src(edge_cursor);
			operation_cursor = ir_node_get_operation(node_cursor);

			if (ir_edge_get_dependence(edge_cursor)->type == IR_DEPENDENCE_TYPE_DIRECT){
				if (operation_cursor->type == IR_OPERATION_TYPE_INST && (operation_cursor->operation_type.inst.opcode == IR_SHL || operation_cursor->operation_type.inst.opcode == IR_SHR)){
					edge1 = edge_cursor;
				}
			}
			else if (ir_edge_get_dependence(edge_cursor)->type == IR_DEPENDENCE_TYPE_SHIFT_DISP){
				if (operation_cursor->type == IR_OPERATION_TYPE_IMM){
					edge2 = edge_cursor;
				}
			}
		}

		if (edge1 != NULL && edge2 != NULL){
			node1 = edge_get_src(edge1);
			node2 = edge_get_src(edge2);

			if (node1->nb_edge_src > 1){
				struct node* node_temp;

				node_temp = ir_add_inst(ir, ir_node_get_operation(node1)->operation_type.inst.opcode,  ir_node_get_operation(node1)->size);
				if (node_temp == NULL){
					printf("ERROR: in %s, unable to add instruction to IR\n", __func__);
					return;
				}
				
				ir_remove_dependence(ir, edge1);
				edge1 = ir_add_dependence(ir, node_temp, node, IR_DEPENDENCE_TYPE_DIRECT);
				if (edge1 == NULL){
					printf("ERROR: in %s, unable to add dependency to IR\n", __func__);
					return;
				}

				if (graph_copy_dst_edge(&(ir->graph), node_temp, node1)){
					printf("ERROR: in %s, unable to copy dst edge\n", __func__);
				}

				node1 = node_temp;
			}

			for (edge_cursor = node_get_head_edge_dst(node1), edge3 = NULL; edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
				node_cursor = edge_get_src(edge_cursor);
				operation_cursor = ir_node_get_operation(node_cursor);

				if (ir_edge_get_dependence(edge_cursor)->type == IR_DEPENDENCE_TYPE_SHIFT_DISP){
					if (operation_cursor->type == IR_OPERATION_TYPE_IMM){
						edge3 = edge_cursor;
					}
				}
			}

			if (edge3 != NULL){
				uint64_t value3;
				uint64_t value2;

				node3 = edge_get_src(edge3);
				value2 = ir_imm_operation_get_unsigned_value(ir_node_get_operation(node2));
				value3 = ir_imm_operation_get_unsigned_value(ir_node_get_operation(node3));

				if (node3->nb_edge_src > 1){
					if (ir_node_get_operation(node1)->operation_type.inst.opcode == IR_SHL){
						node3 = ir_add_immediate(ir, ir_node_get_operation(node3)->size, 0, value3 + value2);
					}
					else if (value3 > value2){
						node3 = ir_add_immediate(ir, ir_node_get_operation(node3)->size, 0, value3 - value2);
						if (node3 == NULL){
							printf("ERROR: in %s, unable to add immediate node to IR\n", __func__);
						}
						else{
							ir_remove_dependence(ir, edge3);
							if (ir_add_dependence(ir, node3, node1, IR_DEPENDENCE_TYPE_SHIFT_DISP) == NULL){
								printf("ERROR: in %s, unable to add dependence to IR\n", __func__);
							}
						}
					}
					else if (value2 > value3){
						node3 = ir_add_immediate(ir, ir_node_get_operation(node3)->size, 0, value2 - value3);
						if (node3 == NULL){
							printf("ERROR: in %s, unable to add immediate node to IR\n", __func__);
						}
						else{
							ir_remove_dependence(ir, edge3);
							if (ir_add_dependence(ir, node3, node1, IR_DEPENDENCE_TYPE_SHIFT_DISP) == NULL){
								printf("ERROR: in %s, unable to add dependence to IR\n", __func__);
							}
						}
					}
					else{
						ir_remove_dependence(ir, edge3);
						graph_transfert_dst_edge(&(ir->graph), node, node1);
					}
				}
				else{
					if (ir_node_get_operation(node1)->operation_type.inst.opcode == IR_SHL){
						ir_node_get_operation(node3)->operation_type.imm.signe = 0;
						ir_node_get_operation(node3)->operation_type.imm.value = value3 + value2;
					}
					else if (value3 > value2){
						ir_node_get_operation(node3)->operation_type.imm.signe = 0;
						ir_node_get_operation(node3)->operation_type.imm.value = value3 - value2;
					}
					else if (value2 > value3){
						ir_node_get_operation(node3)->operation_type.imm.signe = 0;
						ir_node_get_operation(node3)->operation_type.imm.value = value2 - value3;
					}
					else{
						ir_remove_dependence(ir, edge3);
						graph_transfert_dst_edge(&(ir->graph), node, node1);
					}
				}

				if (ir_node_get_operation(node1)->operation_type.inst.opcode == IR_SHL){
					graph_transfert_src_edge(&(ir->graph), node1, node);
					ir_remove_node(ir, node);
				}
				else{
					if (value2 > value3){
						ir_node_get_operation(node1)->operation_type.inst.opcode = IR_SHL;
					}
					else if (value2 == value3){
						ir_remove_node(ir, node1);
					}

					ir_node_get_operation(node)->operation_type.inst.opcode = IR_AND;
					if (node2->nb_edge_src > 1){
						node2 = ir_add_immediate(ir, ir_node_get_operation(node)->size, 0, 0xffffffffffffffff << value2);
						if (node2 == NULL){
							printf("ERROR: in %s, unable to add immediate node to IR\n", __func__);
						}
						else{
							ir_remove_dependence(ir, edge2);
							if (ir_add_dependence(ir, node2, node, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
								printf("ERROR: in %s, unable to add dependence to IR\n", __func__);
							}
						}
					}
					else{
						ir_node_get_operation(node2)->operation_type.imm.value = 0xffffffffffffffff << value2;
						ir_node_get_operation(node2)->size = ir_node_get_operation(node)->size;
						ir_edge_get_dependence(edge2)->type = IR_DEPENDENCE_TYPE_DIRECT;
					}
				}

				*modification = 1;
			}
		}
	}
}

#pragma GCC diagnostic ignored "-Wunused-parameter"
static void ir_normalize_simplify_instruction_rewrite_shr(struct ir* ir, struct node* node, uint8_t* modification, uint8_t final){
	struct edge* 	edge_cursor;
	uint8_t 		simplify = 0;
	struct node* 	operand = NULL;

	for (edge_cursor = node_get_head_edge_dst(node); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
		if (ir_edge_get_dependence(edge_cursor)->type == IR_DEPENDENCE_TYPE_SHIFT_DISP && ir_node_get_operation(edge_get_src(edge_cursor))->type == IR_OPERATION_TYPE_IMM && ir_imm_operation_get_unsigned_value(ir_node_get_operation(edge_get_src(edge_cursor))) == 0){
			simplify = 1;
		}
		else if (ir_edge_get_dependence(edge_cursor)->type == IR_DEPENDENCE_TYPE_DIRECT){
			operand = edge_get_src(edge_cursor);
		}
	}

	if (simplify && operand != NULL){
		graph_transfert_src_edge(&(ir->graph), operand, node);
		ir_remove_node(ir, node);
		*modification = 1;
	}
}

#pragma GCC diagnostic ignored "-Wunused-parameter"
static void ir_normalize_simplify_instruction_rewrite_sub(struct ir* ir, struct node* node, uint8_t* modification, uint8_t final){
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

					*modification = 1;
					break;
				}
				else{
					printf("WARNING: in %s, found IMM operand but it is shared -> skip\n", __func__);
				}
			}
		}
	}
}

static void ir_normalize_simplify_instruction_symbolic_xor(struct ir* ir, struct node* node, uint8_t* modification){
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
						printf("ERROR: in %s, unable to add dependency to IR\n", __func__);
					}
				}
				else{
					printf("ERROR: in %s, unable to add immediate node to IR\n", __func__);
				}

				i++;
				*modification = 1;
			}
		}
	}
}

#pragma GCC diagnostic ignored "-Wunused-parameter"
static void ir_normalize_simplify_instruction_rewrite_xor(struct ir* ir, struct node* node, uint8_t* modification, uint8_t final){
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

						*modification = 1;
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
	
	for(node_cursor1 = graph_get_head_node(&(ir->graph)); node_cursor1 != NULL; node_cursor1 = node_get_next(node_cursor1)){
		if (node_cursor1->nb_edge_dst > max_nb_edge_dst){
			max_nb_edge_dst = node_cursor1->nb_edge_dst;
		}
	}

	if (max_nb_edge_dst == 0){
		return;
	}

	if (irNormalize_sort_node(ir)){
		printf("ERROR: in %s, unable to sort IR node(s)\n", __func__);
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

void ir_normalize_distribute_immediate(struct ir* ir, uint8_t* modification){
	struct node* 			node_cursor1;
	struct node* 			node_cursor2;
	struct node* 			node_cursor3;
	struct edge* 			edge_cursor;
	struct irOperation* 	operation_cursor1;
	struct irOperation* 	operation_cursor2;
	struct irOperation* 	operation_cursor3;
	struct edge* 			node1_imm_operand;
	struct edge*			node1_inst_operand;
	uint32_t 				node2_nb_imm_operand;
	
	for(node_cursor1 = graph_get_head_node(&(ir->graph)); node_cursor1 != NULL; node_cursor1 = node_get_next(node_cursor1)){
		operation_cursor1 = ir_node_get_operation(node_cursor1);

		if (operation_cursor1->type == IR_OPERATION_TYPE_INST && node_cursor1->nb_edge_dst == 2){
			for (edge_cursor = node_get_head_edge_dst(node_cursor1), node1_imm_operand = NULL, node1_inst_operand = NULL; edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
				node_cursor2 = edge_get_src(edge_cursor);
				operation_cursor2 = ir_node_get_operation(node_cursor2);

				switch(operation_cursor2->type){
					case IR_OPERATION_TYPE_IMM : {
						if (node1_imm_operand == NULL){
							node1_imm_operand = edge_cursor;
						}
						else{
							goto next;
						}
						break;
					}
					case IR_OPERATION_TYPE_INST : {
						if (node1_inst_operand == NULL){
							node1_inst_operand = edge_cursor;
						}
						else{
							goto next;
						}
						break;
					}
					default : {
						goto next;
					}
				}
			}

			if (node1_imm_operand != NULL && node1_inst_operand != NULL){
				node_cursor2 = edge_get_src(node1_inst_operand);
				operation_cursor2 = ir_node_get_operation(node_cursor2);

				if ((operation_cursor1->operation_type.inst.opcode == IR_MUL && operation_cursor2->operation_type.inst.opcode == IR_ADD) ||
					(operation_cursor1->operation_type.inst.opcode == IR_SHL && operation_cursor2->operation_type.inst.opcode == IR_ADD) ||
					(operation_cursor1->operation_type.inst.opcode == IR_SHL && operation_cursor2->operation_type.inst.opcode == IR_AND)){
					for (edge_cursor = node_get_head_edge_dst(node_cursor2), node2_nb_imm_operand = 0; edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
						node_cursor3 = edge_get_src(edge_cursor);
						operation_cursor3 = ir_node_get_operation(node_cursor3);

						if (operation_cursor3->type == IR_OPERATION_TYPE_IMM){
							node2_nb_imm_operand ++;
						}
					}

					if (node2_nb_imm_operand > 0){
						struct node* new_node;

						for (edge_cursor = node_get_head_edge_dst(node_cursor2); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
							node_cursor3 = edge_get_src(edge_cursor);
							operation_cursor3 = ir_node_get_operation(node_cursor3);

							new_node = ir_add_inst(ir, operation_cursor1->operation_type.inst.opcode, operation_cursor1->size);
							if (new_node == NULL){
								printf("ERROR: in %s, unable to add inst node to IR\n", __func__);
							}
							else{
								if (ir_add_dependence(ir, edge_get_src(node1_imm_operand), new_node, ir_edge_get_dependence(node1_imm_operand)->type) == NULL){
									printf("ERROR: in %s, unable to add dependency to IR\n", __func__);
								}

								if (ir_add_dependence(ir, node_cursor3, new_node, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
									printf("ERROR: in %s, unable to add dependency to IR\n", __func__);
								}

								if (ir_add_dependence(ir, new_node, node_cursor1, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
									printf("ERROR: in %s, unable to add dependency to IR\n", __func__);
								}
							}
						}
						operation_cursor1->operation_type.inst.opcode = operation_cursor2->operation_type.inst.opcode;
						ir_remove_dependence(ir, node1_imm_operand);
						ir_remove_dependence(ir, node1_inst_operand);

						*modification = 1;
					}
				}
			}
		}

		next:;
	}
}

void ir_normalize_simplify_memory_access(struct ir* ir, uint8_t* modification){
	struct node* 	node_cursor;
	struct edge* 	edge_cursor;
	uint32_t 		nb_mem_access;
	struct node** 	access_list;
	uint32_t 		i;

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
									printf("ERROR: in %s, unable to add new dependency to IR\n", __func__);
								}

								access_list[i] = access_list[i - 1];
							}
							else if (operation_prev->size < operation_next->size){
								printf("WARNING: in %s, simplification of memory access of different size (case STORE -> LOAD)\n", __func__);
								continue;
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
								printf("ERROR: in %s, unable to add new dependency to IR\n", __func__);
							}

							access_list[i] = access_list[i - 1];
						}
						else if (operation_prev->size < operation_next->size){
							ir_convert_node_to_inst(access_list[i - 1], ir_normalize_choose_part_opcode(operation_next->size, operation_prev->size), operation_prev->size)
							graph_remove_edge(&(ir->graph), graph_get_edge(node_cursor, access_list[i - 1]));

							if (ir_add_dependence(ir, access_list[i], access_list[i - 1], IR_DEPENDENCE_TYPE_DIRECT) == NULL){
								printf("ERROR: in %s, unable to add new dependency to IR\n", __func__);
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
								printf("ERROR: in %s, unable to add dependency to the IR\n", __func__);
							}

							if (ir_add_dependence(ir, node_immr, node_cursor, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
								printf("ERROR: in %s, unable to add dependency to the IR\n", __func__);
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