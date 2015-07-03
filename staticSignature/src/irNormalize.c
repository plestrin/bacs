#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef VERBOSE
#include <time.h>
#endif

#include "irNormalize.h"

#include "irMemory.h"
#include "irVariableSize.h"
#include "irExpression.h"
#include "dagPartialOrder.h"

#ifdef VERBOSE
#include "multiColumn.h"
#endif

#define IR_NORMALIZE_REMOVE_DEAD_CODE 				1
#define IR_NORMALIZE_SIMPLIFY_INSTRUCTION			1
#define IR_NORMALIZE_REMOVE_SUBEXPRESSION 			1
#define IR_NORMALIZE_SIMPLIFY_MEMORY_ACCESS 		1
#define IR_NORMALIZE_FACTOR_INSTRUCTION 			1
#define IR_NORMALIZE_EXPAND_VARIABLE				1
#define IR_NORMALIZE_DISTRIBUTE_IMMEDIATE 			1
#define IR_NORMALIZE_MERGE_ASSOCIATIVE_XOR 			1
#define IR_NORMALIZE_ALIASING_SENSITIVITY 			1 /* 0=WEAK, 1=CHECK, 2+=STRICT */
#define IR_NORMALIZE_AFFINE_EXPRESSION 				1

struct irOperand{
	struct node*	node;
	struct edge* 	edge;
};

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
	#ifdef VERBOSE
	uint32_t 	round_counter = 0;
	uint8_t 	modification_copy;
	double 		timer_1_elapsed_time = 0.0;
	double 		timer_2_elapsed_time = 0.0;
	double 		timer_3_elapsed_time = 0.0;
	double 		timer_4_elapsed_time = 0.0;
	double 		timer_5_elapsed_time = 0.0;
	double 		timer_6_elapsed_time = 0.0;
	double 		timer_7_elapsed_time = 0.0;
	#endif
	uint8_t 	modification = 0;

	INIT_TIMER

	#if IR_NORMALIZE_REMOVE_DEAD_CODE == 1
	ir_normalize_remove_dead_code(ir, &modification);
	#ifdef VERBOSE
	if (modification){
		printf("INFO: in %s, modification remove dead code @ START\n", __func__);
	}
	#endif
	modification = 1;
	#else
	modification = 1;
	#endif

	
	while(modification){
		modification = 0;

		#ifdef VERBOSE
		printf("*** ROUND %u ***\n", round_counter);
		modification_copy = 0;
		#endif

		#if IR_NORMALIZE_SIMPLIFY_INSTRUCTION == 1
		START_TIMER
		ir_normalize_simplify_instruction(ir, &modification, 0);
		STOP_TIMER
		#ifdef VERBOSE
		timer_1_elapsed_time += (timer_stop_time.tv_sec - timer_start_time.tv_sec) + (timer_stop_time.tv_nsec - timer_start_time.tv_nsec) / 1000000000.;

		if (modification){
			printf("INFO: in %s, modification simplify instruction @ %u\n", __func__, round_counter);
			modification = 0;
			modification_copy = 1;
		}
		#endif
		#endif

		#if IR_NORMALIZE_REMOVE_SUBEXPRESSION == 1
		START_TIMER
		ir_normalize_remove_subexpression(ir, &modification);
		STOP_TIMER
		#ifdef VERBOSE
		timer_2_elapsed_time += (timer_stop_time.tv_sec - timer_start_time.tv_sec) + (timer_stop_time.tv_nsec - timer_start_time.tv_nsec) / 1000000000.;

		if (modification){
			printf("INFO: in %s, modification remove subexpression @ %u\n", __func__, round_counter);
			modification = 0;
			modification_copy = 1;
		}
		#endif
		#endif

		#if IR_NORMALIZE_SIMPLIFY_MEMORY_ACCESS == 1
		START_TIMER
		#if IR_NORMALIZE_ALIASING_SENSITIVITY == 0
		ir_normalize_simplify_memory_access(ir, &modification, ALIASING_STRATEGY_WEAK);
		#elif IR_NORMALIZE_ALIASING_SENSITIVITY == 1
		ir_normalize_simplify_memory_access(ir, &modification, ALIASING_STRATEGY_CHECK);
		#else
		ir_normalize_simplify_memory_access(ir, &modification, ALIASING_STRATEGY_STRICT);
		#endif
		STOP_TIMER
		#ifdef VERBOSE
		timer_3_elapsed_time += (timer_stop_time.tv_sec - timer_start_time.tv_sec) + (timer_stop_time.tv_nsec - timer_start_time.tv_nsec) / 1000000000.;

		if (modification){
			printf("INFO: in %s, modification simplify memory @ %u\n", __func__, round_counter);
			modification = 0;
			modification_copy = 1;
		}
		#endif
		#endif

		#if IR_NORMALIZE_FACTOR_INSTRUCTION == 1
		START_TIMER
		ir_normalize_factor_instruction(ir, &modification);
		STOP_TIMER
		#ifdef VERBOSE
		timer_4_elapsed_time += (timer_stop_time.tv_sec - timer_start_time.tv_sec) + (timer_stop_time.tv_nsec - timer_start_time.tv_nsec) / 1000000000.;

		if (modification){
			printf("INFO: in %s, modification factor instruction @ %u\n", __func__, round_counter);
			modification = 0;
			modification_copy = 1;
		}
		#endif
		#endif

		#if IR_NORMALIZE_DISTRIBUTE_IMMEDIATE == 1
		START_TIMER
		ir_normalize_distribute_immediate(ir, &modification);
		STOP_TIMER
		#ifdef VERBOSE
		timer_5_elapsed_time += (timer_stop_time.tv_sec - timer_start_time.tv_sec) + (timer_stop_time.tv_nsec - timer_start_time.tv_nsec) / 1000000000.;

		if (modification){
			printf("INFO: in %s, modification distribute immediate @ %u\n", __func__, round_counter);
			modification = 0;
			modification_copy = 1;
		}
		#endif
		#endif

		#if IR_NORMALIZE_EXPAND_VARIABLE == 1
		START_TIMER
		ir_normalize_expand_variable(ir, &modification);
		STOP_TIMER
		#ifdef VERBOSE
		timer_6_elapsed_time += (timer_stop_time.tv_sec - timer_start_time.tv_sec) + (timer_stop_time.tv_nsec - timer_start_time.tv_nsec) / 1000000000.;

		if (modification){
			printf("INFO: in %s, modification expand variable @ %u\n", __func__, round_counter);
			modification = 0;
			modification_copy = 1;
		}
		#endif
		#endif

		#if IR_NORMALIZE_AFFINE_EXPRESSION == 1
		START_TIMER
		ir_normalize_affine_expression(ir, &modification);
		STOP_TIMER
		#ifdef VERBOSE
		timer_7_elapsed_time += (timer_stop_time.tv_sec - timer_start_time.tv_sec) + (timer_stop_time.tv_nsec - timer_start_time.tv_nsec) / 1000000000.;

		if (modification){
			printf("INFO: in %s, modification affine expression @ %u\n", __func__, round_counter);
			modification_copy = 1;
		}
		#endif
		#endif

		#ifdef VERBOSE
		modification = modification_copy;
		round_counter ++;
		#endif
	}

	#if IR_NORMALIZE_SIMPLIFY_INSTRUCTION == 1
	START_TIMER
	ir_normalize_simplify_instruction(ir, &modification, 1);
	STOP_TIMER
	#ifdef VERBOSE
	timer_1_elapsed_time += (timer_stop_time.tv_sec - timer_start_time.tv_sec) + (timer_stop_time.tv_nsec - timer_start_time.tv_nsec) / 1000000000.;

	if (modification){
		printf("INFO: in %s, modification simplify instruction @ FINAL\n", __func__);
	}
	#endif
	#endif

	#ifdef VERBOSE
	multiColumnPrinter_print_header(printer);

	multiColumnPrinter_print(printer, "Simplify instruction", timer_1_elapsed_time, NULL);
	multiColumnPrinter_print(printer, "Remove subexpression", timer_2_elapsed_time, NULL);
	multiColumnPrinter_print(printer, "Simplify memory access", timer_3_elapsed_time, NULL);
	multiColumnPrinter_print(printer, "Factor instruction", timer_4_elapsed_time, NULL);
	multiColumnPrinter_print(printer, "Distribute immediate", timer_5_elapsed_time, NULL);
	multiColumnPrinter_print(printer, "Expand variable", timer_6_elapsed_time, NULL);
	multiColumnPrinter_print(printer, "Affine expression", timer_7_elapsed_time, NULL);
	#endif
	
	#if IR_NORMALIZE_MERGE_ASSOCIATIVE_XOR == 1
	{
		START_TIMER
		ir_normalize_merge_associative_operation(ir, IR_XOR);
		STOP_TIMER
		PRINT_TIMER("Merge associative xor")
	}
	#endif

	CLEAN_TIMER
}

void ir_normalize_remove_dead_code(struct ir* ir,  uint8_t* modification){
	struct node* 			node_cursor;
	struct node* 			next_cursor;
	struct irOperation* 	operation_cursor;
	uint8_t 				local_modification = 0;

	if (dagPartialOrder_sort_src_dst(&(ir->graph))){
		printf("ERROR: in %s, unable to sort DAG\n", __func__);
	}

	for (node_cursor = graph_get_head_node(&(ir->graph)); node_cursor != NULL; node_cursor = next_cursor){
		next_cursor = node_get_next(node_cursor);
		operation_cursor = ir_node_get_operation(node_cursor);

		switch (operation_cursor->type){
			case IR_OPERATION_TYPE_IN_REG 	: {
				if (!node_cursor->nb_edge_src && (operation_cursor->status_flag & IR_NODE_STATUS_FLAG_FINAL) == 0){
					ir_remove_node(ir, node_cursor);
					local_modification = 1;
				}
				break;
			}
			case IR_OPERATION_TYPE_IN_MEM 	: {
				if (!node_cursor->nb_edge_src && (operation_cursor->status_flag & IR_NODE_STATUS_FLAG_FINAL) == 0){
					ir_remove_node(ir, node_cursor);
					local_modification = 1;
				}
				break;
			}
			case IR_OPERATION_TYPE_OUT_MEM  : {
				break;
			}
			case IR_OPERATION_TYPE_IMM 		: {
				if (!node_cursor->nb_edge_src && (operation_cursor->status_flag & IR_NODE_STATUS_FLAG_FINAL) == 0){
					ir_remove_node(ir, node_cursor);
					local_modification = 1;
				}
				break;
			}
			case IR_OPERATION_TYPE_INST 	: {
				if (!node_cursor->nb_edge_src && (operation_cursor->status_flag & IR_NODE_STATUS_FLAG_FINAL) == 0){
					ir_remove_node(ir, node_cursor);
					local_modification = 1;
				}
				break;
			}
			case IR_OPERATION_TYPE_SYMBOL 	: {
				break;
			}
		}
	}

	if (modification != NULL){
		*modification = local_modification;
	}

	ir_check_order(ir);
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
static void ir_normalize_simplify_instruction_numeric_shr(struct ir* ir, struct node* node, uint8_t* modification);
static void ir_normalize_simplify_instruction_rewrite_shr(struct ir* ir, struct node* node, uint8_t* modification, uint8_t final);
static void ir_normalize_simplify_instruction_rewrite_sub(struct ir* ir, struct node* node, uint8_t* modification, uint8_t final);
static void ir_normalize_simplify_instruction_symbolic_xor(struct ir* ir, struct node* node, uint8_t* modification);
static void ir_normalize_simplify_instruction_rewrite_xor(struct ir* ir, struct node* node, uint8_t* modification, uint8_t final);

typedef void(*simplify_numeric_instruction_ptr)(struct ir*,struct node*,uint8_t*);
typedef void(*simplify_symbolic_instruction_ptr)(struct ir*,struct node*,uint8_t*);
typedef void(*simplify_rewrite_instruction_ptr)(struct ir*,struct node*,uint8_t*,uint8_t);

static const simplify_numeric_instruction_ptr numeric_simplify[NB_IR_OPCODE] = {
	ir_normalize_simplify_instruction_numeric_add, 					/* 0  IR_ADD 			*/
	ir_normalize_simplify_instruction_numeric_and, 					/* 1  IR_AND 			*/
	NULL, 															/* 2  IR_CMOV 			*/
	NULL, 															/* 3  IR_DIV  			*/
	NULL, 															/* 4  IR_IDIV 			*/
	ir_normalize_simplify_instruction_numeric_imul, 				/* 5  IR_IMUL 			*/
	NULL, 															/* 6  IR_LEA 			*/
	NULL, 															/* 7  IR_MOV 			*/
	NULL, 															/* 8  IR_MOVZX 			*/
	ir_normalize_simplify_instruction_numeric_mul, 					/* 9  IR_MUL 			*/
	NULL, 															/* 10 IR_NEG 			*/
	NULL, 															/* 11 IR_NOT 			*/
	NULL, 															/* 12 IR_OR 			*/
	NULL, 															/* 13 IR_PART1_8 		*/
	NULL, 															/* 14 IR_PART2_8 		*/
	NULL, 															/* 15 IR_PART1_16 		*/
	NULL, 															/* 16 IR_ROL 			*/
	NULL, 															/* 17 IR_ROR 			*/
	ir_normalize_simplify_instruction_numeric_shl, 					/* 18 IR_SHL 			*/
	NULL, 															/* 19 IR_SHLD 			*/
	ir_normalize_simplify_instruction_numeric_shr, 					/* 20 IR_SHR 			*/
	NULL, 															/* 21 IR_SHRD 			*/
	NULL, 															/* 22 IR_SUB 			*/
	NULL, 															/* 23 IR_XOR 			*/
	NULL, 															/* 24 IR_LOAD 			*/
	NULL, 															/* 25 IR_STORE 			*/
	NULL, 															/* 26 IR_JOKER 			*/
	NULL 															/* 27 IR_INVALID 		*/
};

static const simplify_symbolic_instruction_ptr symbolic_simplify[NB_IR_OPCODE] = {
	NULL, 															/* 0  IR_ADD 			*/
	NULL, 															/* 1  IR_AND 			*/
	NULL, 															/* 2  IR_CMOV 			*/
	NULL, 															/* 3  IR_DIV  			*/
	NULL, 															/* 4  IR_IDIV 			*/
	NULL, 															/* 5  IR_IMUL 			*/
	NULL, 															/* 6  IR_LEA 			*/
	NULL, 															/* 7  IR_MOV 			*/
	NULL, 															/* 8  IR_MOVZX 			*/
	NULL, 															/* 9  IR_MUL 			*/
	NULL, 															/* 10 IR_NEG 			*/
	NULL, 															/* 11 IR_NOT 			*/
	ir_normalize_simplify_instruction_symbolic_or, 					/* 12 IR_OR 			*/
	NULL, 															/* 13 IR_PART1_8 		*/
	NULL, 															/* 14 IR_PART2_8 		*/
	NULL, 															/* 15 IR_PART1_16 		*/
	NULL, 															/* 16 IR_ROL 			*/
	NULL, 															/* 17 IR_ROR 			*/
	NULL, 															/* 18 IR_SHL 			*/
	NULL, 															/* 19 IR_SHLD 			*/
	NULL, 															/* 20 IR_SHR 			*/
	NULL, 															/* 21 IR_SHRD 			*/
	NULL, 															/* 22 IR_SUB 			*/
	ir_normalize_simplify_instruction_symbolic_xor, 				/* 23 IR_XOR 			*/
	NULL, 															/* 24 IR_LOAD 			*/
	NULL, 															/* 25 IR_STORE 			*/
	NULL, 															/* 26 IR_JOKER 			*/
	NULL 															/* 27 IR_INVALID 		*/
};

static const simplify_rewrite_instruction_ptr rewrite_simplify[NB_IR_OPCODE] = {
	ir_normalize_simplify_instruction_rewrite_add, 					/* 0  IR_ADD 			*/
	ir_normalize_simplify_instruction_rewrite_and, 					/* 1  IR_AND 			*/
	NULL, 															/* 2  IR_CMOV 			*/
	NULL, 															/* 3  IR_DIV  			*/
	NULL, 															/* 4  IR_IDIV 			*/
	ir_normalize_simplify_instruction_rewrite_imul, 				/* 5  IR_IMUL 			*/
	NULL, 															/* 6  IR_LEA 			*/
	NULL, 															/* 7  IR_MOV 			*/
	ir_normalize_simplify_instruction_rewrite_movzx, 				/* 8  IR_MOVZX 			*/
	ir_normalize_simplify_instruction_rewrite_mul, 					/* 9  IR_MUL 			*/
	NULL, 															/* 10 IR_NEG 			*/
	NULL, 															/* 11 IR_NOT 			*/
	ir_normalize_simplify_instruction_rewrite_or, 					/* 12 IR_OR 			*/
	ir_normalize_simplify_instruction_rewrite_part1_8, 				/* 13 IR_PART1_8 		*/
	NULL, 															/* 14 IR_PART2_8 		*/
	NULL, 															/* 15 IR_PART1_16 		*/
	ir_normalize_simplify_instruction_rewrite_rol, 					/* 16 IR_ROL 			*/
	NULL, 															/* 17 IR_ROR 			*/
	ir_normalize_simplify_instruction_rewrite_shl, 					/* 18 IR_SHL 			*/
	NULL, 															/* 19 IR_SHLD 			*/
	ir_normalize_simplify_instruction_rewrite_shr, 					/* 20 IR_SHR 			*/
	NULL, 															/* 21 IR_SHRD 			*/
	ir_normalize_simplify_instruction_rewrite_sub, 					/* 22 IR_SUB 			*/
	ir_normalize_simplify_instruction_rewrite_xor, 					/* 23 IR_XOR 			*/
	NULL, 															/* 24 IR_LOAD 			*/
	NULL, 															/* 25 IR_STORE 			*/
	NULL, 															/* 26 IR_JOKER 			*/
	NULL 															/* 27 IR_INVALID 		*/
};

void ir_normalize_simplify_instruction(struct ir* ir, uint8_t* modification, uint8_t final){
	struct node* 			node_cursor;
	struct node* 			next_node_cursor;
	struct irOperation* 	operation;

	if (dagPartialOrder_sort_src_dst(&(ir->graph))){
		printf("ERROR: in %s, unable to sort ir node(s)\n", __func__);
		return;
	}

	for(node_cursor = graph_get_tail_node(&(ir->graph)), next_node_cursor = NULL; node_cursor != NULL;){
		operation = ir_node_get_operation(node_cursor);
		if (operation->type == IR_OPERATION_TYPE_INST){
			if (numeric_simplify[operation->operation_type.inst.opcode] != NULL){
				numeric_simplify[operation->operation_type.inst.opcode](ir, node_cursor, modification);
			}
			if (symbolic_simplify[operation->operation_type.inst.opcode] != NULL){
				symbolic_simplify[operation->operation_type.inst.opcode](ir, node_cursor, modification);
			}
			if (rewrite_simplify[operation->operation_type.inst.opcode] != NULL){
				rewrite_simplify[operation->operation_type.inst.opcode](ir, node_cursor, modification, final);
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

/* I would be good if I can use this stub for every numeric simplifciation. IMUL SHR */
#define ir_normalize_simplify_instruction_numeric_generic(ir, node, modification, identity_el, operation) 													\
	{ 																																						\
		uint32_t 			nb_imm_operand 		= 0; 																										\
		uint32_t 			nb_sym_operand 		= 0; 																										\
		struct edge* 		edge_cursor; 																													\
		struct edge* 		edge_cursor_next; 																												\
		struct node* 		operand; 																														\
		struct irOperation*	operand_operation; 																												\
		struct node* 		possible_rewrite 	= NULL; 																									\
		uint64_t 			value 				= (identity_el); 																							\
 																																							\
		if ((node)->nb_edge_dst > 1){ 																														\
			for (edge_cursor = node_get_head_edge_dst(node); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){ 							\
				operand = edge_get_src(edge_cursor); 																										\
				operand_operation = ir_node_get_operation(operand); 																						\
																																							\
				if (operand_operation->type == IR_OPERATION_TYPE_IMM){ 																						\
					nb_imm_operand ++; 																														\
																																							\
					value = value operation ir_imm_operation_get_unsigned_value(operand_operation); 														\
																																							\
					if (operand->nb_edge_src == 1){ 																										\
						possible_rewrite = operand; 																										\
					} 																																		\
				} 																																			\
				else{ 																																		\
					nb_sym_operand ++; 																														\
				} 																																			\
			} 																																				\
																																							\
			value = value & (0xffffffffffffffff >> (64 - ir_node_get_operation(node)->size)); 																\
																																							\
			if ((nb_imm_operand == 1 && value == (identity_el)) || nb_imm_operand > 1){ 																	\
				if (value != (identity_el) || nb_sym_operand == 0){ 																						\
					if (possible_rewrite == NULL){ 																											\
						possible_rewrite = ir_add_immediate(ir, ir_node_get_operation(node)->size, value); 													\
						if (possible_rewrite == NULL){ 																										\
							printf("ERROR: in %s, unable to add immediate to IR\n", __func__); 																\
							return; 																														\
						} 																																	\
						else{ 																																\
							if (ir_add_dependence(ir, possible_rewrite, node, IR_DEPENDENCE_TYPE_DIRECT) == NULL){ 											\
								printf("ERROR: in %s, unable to add dependency to IR\n", __func__); 														\
							} 																																\
						} 																																	\
					} 																																		\
					else{ 																																	\
						ir_node_get_operation(possible_rewrite)->operation_type.imm.value = value; 															\
						ir_node_get_operation(possible_rewrite)->size = ir_node_get_operation(node)->size; 													\
					} 																																		\
				} 																																			\
				else{ 																																		\
					possible_rewrite = NULL; 																												\
				} 																																			\
																																							\
				for (edge_cursor = node_get_head_edge_dst(node); edge_cursor != NULL; edge_cursor = edge_cursor_next){ 										\
					operand = edge_get_src(edge_cursor); 																									\
					operand_operation = ir_node_get_operation(operand); 																					\
					edge_cursor_next = edge_get_next_dst(edge_cursor); 																						\
																																							\
					if (operand_operation->type == IR_OPERATION_TYPE_IMM && operand != possible_rewrite){ 													\
						ir_remove_dependence(ir, edge_cursor); 																								\
					} 																																		\
				} 																																			\
				*(modification) = 1; 																														\
			} 																																				\
		} 																																					\
	}

static void ir_normalize_simplify_instruction_numeric_add(struct ir* ir, struct node* node, uint8_t* modification){
	ir_normalize_simplify_instruction_numeric_generic(ir, node, modification, 0, +)
}

static void ir_normalize_simplify_instruction_rewrite_add(struct ir* ir, struct node* node, uint8_t* modification, uint8_t final){
	uint32_t 			nb_imm_operand;
	struct edge* 		edge_cursor1;
	struct edge* 		edge_cursor2;
	struct edge* 		current_edge;
	struct irOperation*	operand_operation;

	if (node->nb_edge_dst == 1){
		graph_transfert_src_edge(&(ir->graph), edge_get_src(node_get_head_edge_dst(node)), node);
		ir_remove_node(ir, node);

		*modification = 1;
		return;
	}

	for (edge_cursor1 = node_get_head_edge_dst(node), nb_imm_operand = 0; edge_cursor1 != NULL; edge_cursor1 = edge_get_next_dst(edge_cursor1)){
		operand_operation = ir_node_get_operation(edge_get_src(edge_cursor1));

		if (operand_operation->type == IR_OPERATION_TYPE_IMM){
			nb_imm_operand ++;
		}
	}

	for (edge_cursor1 = node_get_head_edge_dst(node); edge_cursor1 != NULL;){
		current_edge = edge_cursor1;
		edge_cursor1 = edge_get_next_dst(edge_cursor1);
		operand_operation = ir_node_get_operation(edge_get_src(current_edge));

		if (operand_operation->type == IR_OPERATION_TYPE_INST && operand_operation->operation_type.inst.opcode == IR_ADD){
			if (edge_get_src(current_edge)->nb_edge_src == 1){
				graph_transfert_dst_edge(&(ir->graph), node, edge_get_src(current_edge));
				ir_remove_node(ir, edge_get_src(current_edge));

				*modification = 1;
				continue;
			}
			else if (final){
				for (edge_cursor2 = node_get_head_edge_src(edge_get_src(current_edge)); edge_cursor2 != NULL; edge_cursor2 = edge_get_next_src(edge_cursor2)){
					if (edge_cursor2 != current_edge && (ir_node_get_operation(edge_get_dst(edge_cursor2))->type != IR_OPERATION_TYPE_OUT_MEM || ir_edge_get_dependence(edge_cursor2)->type == IR_DEPENDENCE_TYPE_ADDRESS)){
						break;
					}
				}
				if (edge_cursor2 == NULL){
					if (graph_copy_dst_edge(&(ir->graph), node, edge_get_src(current_edge))){
						printf("ERROR: in %s, unable to copy dst edge\n", __func__);
					}
					ir_remove_dependence(ir, current_edge);

					*modification = 1;
					continue;
				}
			}

			if (nb_imm_operand > 0){
				for (edge_cursor2 = node_get_head_edge_dst(edge_get_src(current_edge)); edge_cursor2 != NULL; edge_cursor2 = edge_get_next_dst(edge_cursor2)){
					if (ir_node_get_operation(edge_get_src(edge_cursor2))->type == IR_OPERATION_TYPE_IMM){
						if (graph_copy_dst_edge(&(ir->graph), node, edge_get_src(current_edge))){
							printf("ERROR: in %s, unable to copy dst edge\n", __func__);
						}
						ir_remove_dependence(ir, current_edge);

						*modification = 1;
						break;
					}
				}
			}
		}
	}
}

static void ir_normalize_simplify_instruction_numeric_and(struct ir* ir, struct node* node, uint8_t* modification){
	ir_normalize_simplify_instruction_numeric_generic(ir, node, modification, (0xffffffffffffffff >> (64 - ir_node_get_operation(node)->size)), &)
}

#pragma GCC diagnostic ignored "-Wunused-parameter"
static void ir_normalize_simplify_instruction_rewrite_and(struct ir* ir, struct node* node, uint8_t* modification, uint8_t final){
	struct node* 		imm_operand 	= NULL;
	struct edge* 		and_operand 	= NULL;
	struct node*		movzx_operand 	= NULL;
	struct edge* 		edge_cursor1;
	struct edge* 		edge_cursor2;
	struct irOperation*	operand_operation;

	if (node->nb_edge_dst == 1){
		graph_transfert_src_edge(&(ir->graph), edge_get_src(node_get_head_edge_dst(node)), node);
		ir_remove_node(ir, node);
		
		*modification = 1;
		return;
	}

	for (edge_cursor1 = node_get_head_edge_dst(node); edge_cursor1 != NULL; edge_cursor1 = edge_get_next_dst(edge_cursor1)){
		operand_operation = ir_node_get_operation(edge_get_src(edge_cursor1));

		if (operand_operation->type == IR_OPERATION_TYPE_IMM){
			imm_operand = edge_get_src(edge_cursor1);
		}
		else if (operand_operation->type == IR_OPERATION_TYPE_INST){
			if (operand_operation->operation_type.inst.opcode == IR_AND){
				if (and_operand != NULL){
					printf("WARNING: in %s, multiple AND operands, can't decide how to associate IMM\n", __func__);
					return;
				}
				else{
					for (edge_cursor2 = node_get_head_edge_dst(edge_get_src(edge_cursor1)); edge_cursor2 != NULL; edge_cursor2 = edge_get_next_dst(edge_cursor2)){
						if (ir_node_get_operation(edge_get_src(edge_cursor2))->type == IR_OPERATION_TYPE_IMM){
							and_operand = edge_cursor1;
						}
					}
				}
			}
			else if (operand_operation->operation_type.inst.opcode == IR_MOVZX){
				movzx_operand = edge_get_src(edge_cursor1);
			}
		}
	}

	if (node->nb_edge_dst == 2 && imm_operand != NULL && movzx_operand != NULL){
		if (movzx_operand->nb_edge_dst == 1){
			if ((~ir_imm_operation_get_unsigned_value(ir_node_get_operation(imm_operand)) & (0xffffffffffffffff >> (64 - ir_node_get_operation(edge_get_src(node_get_head_edge_dst(movzx_operand)))->size))) == 0){
				graph_transfert_src_edge(&(ir->graph), movzx_operand, node);
				ir_remove_node(ir, node);
		
				*modification = 1;
				return;
			}
		}
		else{
			printf("ERROR: in %s, MOVZX has a wrong number of dst edge: %u\n", __func__, movzx_operand->nb_edge_dst);
		}
	}

	if (imm_operand != NULL && and_operand != NULL){
		if (graph_copy_dst_edge(&(ir->graph), node, edge_get_src(and_operand))){
			printf("ERROR: in %s, unable to copy dst edge\n", __func__);
		}
		ir_remove_dependence(ir, and_operand);

		*modification = 1;
	}
}

static void ir_normalize_simplify_instruction_numeric_imul(struct ir* ir, struct node* node, uint8_t* modification){
	uint32_t 			nb_imm_operand;
	struct edge* 		edge_cursor;
	struct edge* 		edge_cursor_next;
	struct node* 		operand;
	struct irOperation*	operand_operation;
	struct node* 		possible_rewrite;
	int64_t 			value;
	uint8_t 			size;

	if (node->nb_edge_dst <= 1){
		return;
	}

	size = ir_node_get_operation(node)->size;

	for (edge_cursor = node_get_head_edge_dst(node), nb_imm_operand = 0, value = 1, possible_rewrite = NULL; edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
		operand = edge_get_src(edge_cursor);
		operand_operation = ir_node_get_operation(operand);

		if (operand_operation->type == IR_OPERATION_TYPE_IMM){
			nb_imm_operand ++;
			value = value * ir_imm_operation_get_signed_value(operand_operation);

			if (operand->nb_edge_src == 1){
				possible_rewrite = operand;
			}
		}
	}

	if (nb_imm_operand <= 1 && (value & (0xffffffffffffffffULL >> (64 - size))) != 1){
		return;
	}

	if ((value & (0xffffffffffffffffULL >> (64 - size))) != 1){
		if (possible_rewrite == NULL){
			possible_rewrite = ir_add_immediate(ir, size, value);
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

static void ir_normalize_simplify_instruction_rewrite_imul(struct ir* ir, struct node* node, uint8_t* modification, uint8_t final){
	struct edge* 			edge_cursor;
	struct edge* 			edge_current;
	struct irOperation*		operation_cursor;

	if (node->nb_edge_dst == 1){
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
				ir_node_get_operation(node)->operation_type.imm.value = 0;
				ir_node_get_operation(node)->status_flag = IR_NODE_STATUS_FLAG_NONE;

				*modification = 1;
				return;
			}
		}
		if (node->nb_edge_dst == 2){
			for (edge_cursor = node_get_head_edge_dst(node); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
			operation_cursor = ir_node_get_operation(edge_get_src(edge_cursor));

			if (operation_cursor->type == IR_OPERATION_TYPE_IMM && ((ir_imm_operation_get_unsigned_value(operation_cursor) >> (operation_cursor->size - 1) & 0x0000000000000001ULL) == 0)){
				ir_node_get_operation(node)->operation_type.inst.opcode = IR_MUL;

				*modification = 1;
				return;
			}
		}
		}
	}
}

static void ir_normalize_simplify_instruction_rewrite_movzx(struct ir* ir, struct node* node, uint8_t* modification, uint8_t final){
	struct edge* 		edge;
	struct node* 		operand;
	struct irOperation*	operand_operation;
	struct node* 		node_imm_new;

	if (node->nb_edge_dst == 1){
		edge = node_get_head_edge_dst(node);
		operand = edge_get_src(edge);
		operand_operation = ir_node_get_operation(operand);
		if (operand_operation->type == IR_OPERATION_TYPE_INST && operand_operation->operation_type.inst.opcode == IR_PART1_8){
			if (operand->nb_edge_dst == 1){
				if (ir_node_get_operation(edge_get_src(node_get_head_edge_dst(operand)))->size != ir_node_get_operation(node)->size){
					return;
				}
			}

			node_imm_new = ir_add_immediate(ir, ir_node_get_operation(node)->size, 0x000000ff);
			if (node_imm_new != NULL){
				if (ir_add_dependence(ir, node_imm_new, node, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
					printf("ERROR: in %s, unable to add dependency to IR\n", __func__);
				}
				else{
					ir_node_get_operation(node)->operation_type.inst.opcode = IR_AND;
					if (operand->nb_edge_src == 1){
						graph_transfert_dst_edge(&(ir->graph), node, operand);
						ir_remove_node(ir, operand);
						*modification = 1;
					}
					else{
						graph_copy_dst_edge(&(ir->graph), node, operand);
						ir_remove_dependence(ir, edge);
						*modification = 1;
					}
				}
			}
			else{
				printf("ERROR: in %s, unable to add immediate to IR\n", __func__);
			}
		}
		else if (operand_operation->type == IR_OPERATION_TYPE_INST && operand_operation->operation_type.inst.opcode == IR_PART2_8){
			if (operand->nb_edge_dst == 1){
				if (ir_node_get_operation(edge_get_src(node_get_head_edge_dst(operand)))->size != ir_node_get_operation(node)->size){
					return;
				}
			}

			if (operand->nb_edge_src == 1){
				ir_node_get_operation(node)->operation_type.inst.opcode = IR_AND;

				node_imm_new = ir_add_immediate(ir, ir_node_get_operation(node)->size, 0x000000ff);
				if (node_imm_new != NULL){
					if (ir_add_dependence(ir, node_imm_new, node, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
						printf("ERROR: in %s, unable to add dependency to IR\n", __func__);
					}
				}
				else{
					printf("ERROR: in %s, unable to add immediate to IR\n", __func__);
				}

				operand_operation->operation_type.inst.opcode = IR_SHR;
				operand_operation->size = ir_node_get_operation(node)->size;

				node_imm_new = ir_add_immediate(ir, 8, 8);
				if (node_imm_new != NULL){
					if (ir_add_dependence(ir, node_imm_new, operand, IR_DEPENDENCE_TYPE_SHIFT_DISP) == NULL){
						printf("ERROR: in %s, unable to add dependency to IR\n", __func__);
					}
					else{
						*modification = 1;
					}
				}
				else{
					printf("ERROR: in %s, unable to add immediate to IR\n", __func__);
				}

			}
			else{
				printf("WARNING: in %s, PART2_8 instruction is shared, this case is not implemented yet -> skip\n", __func__);
			}
		}
	}
}

static void ir_normalize_simplify_instruction_numeric_mul(struct ir* ir, struct node* node, uint8_t* modification){
	ir_normalize_simplify_instruction_numeric_generic(ir, node, modification, 1, *)
}

static void ir_normalize_simplify_instruction_rewrite_mul(struct ir* ir, struct node* node, uint8_t* modification, uint8_t final){
	struct edge* 			edge_cursor;
	struct edge* 			edge_current;
	struct irOperation*		operation_cursor;

	if (node->nb_edge_dst == 1){
		graph_transfert_src_edge(&(ir->graph), edge_get_src(node_get_head_edge_dst(node)), node);
		ir_remove_node(ir, node);
		*modification = 1;
	}
	else if (node->nb_edge_dst == 2){
		for (edge_cursor = node_get_head_edge_dst(node); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
			operation_cursor = ir_node_get_operation(edge_get_src(edge_cursor));

			if (operation_cursor->type == IR_OPERATION_TYPE_IMM){
				if (ir_imm_operation_get_unsigned_value(operation_cursor) == 0){
					for (edge_cursor = node_get_head_edge_dst(node); edge_cursor != NULL;){
						edge_current = edge_cursor;
						edge_cursor = edge_get_next_dst(edge_cursor);

						ir_remove_dependence(ir, edge_current);
					}

					ir_node_get_operation(node)->type = IR_OPERATION_TYPE_IMM;
					ir_node_get_operation(node)->operation_type.imm.value = 0;
					ir_node_get_operation(node)->status_flag = IR_NODE_STATUS_FLAG_NONE;

					*modification = 1;
					break;

				}
				else if (__builtin_popcount(ir_imm_operation_get_unsigned_value(operation_cursor)) == 1){
					ir_node_get_operation(node)->operation_type.inst.opcode = IR_SHL;
					operation_cursor->operation_type.imm.value = __builtin_ctz(ir_imm_operation_get_unsigned_value(operation_cursor));
					operation_cursor->size = 8;
					ir_edge_get_dependence(edge_cursor)->type = IR_DEPENDENCE_TYPE_SHIFT_DISP;

					*modification = 1;
					break;
				}
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

static void ir_normalize_simplify_instruction_rewrite_or(struct ir* ir, struct node* node, uint8_t* modification, uint8_t final){
	if (node->nb_edge_dst == 1){
		graph_transfert_src_edge(&(ir->graph), edge_get_src(node_get_head_edge_dst(node)), node);
		ir_remove_node(ir, node);
		*modification = 1;
	}
}

static void ir_normalize_simplify_instruction_rewrite_part1_8(struct ir* ir, struct node* node, uint8_t* modification, uint8_t final){
	struct node* 		operand;
	struct irOperation*	operand_operation;

	if (node->nb_edge_dst == 1){
		operand = edge_get_src(node_get_head_edge_dst(node));
		operand_operation = ir_node_get_operation(operand);
		if (operand_operation->type == IR_OPERATION_TYPE_INST){
			if (operand_operation->operation_type.inst.opcode == IR_MOVZX){
				if (operand->nb_edge_dst == 1){
					if (ir_node_get_operation(edge_get_src(node_get_head_edge_dst(operand)))->size == ir_node_get_operation(node)->size){
						graph_transfert_src_edge(&(ir->graph), edge_get_src(node_get_head_edge_dst(operand)), node);

						ir_remove_node(ir, node);

						*modification = 1;
					}
				}
				else{
					printf("ERROR: in %s, MOVZX instruction has %u operand(s)\n", __func__, operand->nb_edge_dst);
				}
			}
		}
		else if (operand_operation->type == IR_OPERATION_TYPE_IMM){
			ir_convert_node_to_imm(node, operand_operation->operation_type.imm.value, 8);
			ir_remove_dependence(ir, node_get_head_edge_dst(node));

			*modification = 1;
		}
	}
}

static void ir_normalize_simplify_instruction_rewrite_rol(struct ir* ir, struct node* node, uint8_t* modification, uint8_t final){
	struct edge* 		edge_cursor;
	struct irOperation*	operand_operation;
	struct node* 		new_imm;

	if (node->nb_edge_dst == 2){
		for (edge_cursor = node_get_head_edge_dst(node); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
			operand_operation = ir_node_get_operation(edge_get_src(edge_cursor));
			if (operand_operation->type == IR_OPERATION_TYPE_IMM && ir_edge_get_dependence(edge_cursor)->type == IR_DEPENDENCE_TYPE_SHIFT_DISP){
				if (edge_get_src(edge_cursor)->nb_edge_src == 1){
					operand_operation->operation_type.imm.value = ir_node_get_operation(node)->size - ir_imm_operation_get_unsigned_value(operand_operation);
					ir_node_get_operation(node)->operation_type.inst.opcode = IR_ROR;

					*modification = 1;
					break;
				}
				else{
					new_imm = ir_add_immediate(ir, ir_node_get_operation(node)->size, ir_node_get_operation(node)->size - ir_imm_operation_get_unsigned_value(operand_operation));
					if (new_imm == NULL){
						printf("ERROR: in %s, unable to add immediate to IR\n", __func__);
					}
					else{
						if (ir_add_dependence(ir, new_imm, node, IR_DEPENDENCE_TYPE_SHIFT_DISP) == NULL){
							printf("ERROR: in %s, unable to add dependency to IR\n", __func__);
						}
						else{
							ir_remove_dependence(ir, edge_cursor);
							ir_node_get_operation(node)->operation_type.inst.opcode = IR_ROR;

							*modification = 1;
						}
					}
				}
			}
		}
	}
}

static void ir_normalize_simplify_instruction_numeric_shl(struct ir* ir, struct node* node, uint8_t* modification){
	struct edge* 	edge_cursor;
	struct node* 	operand;
	struct node* 	possible_rewrite_node;
	struct edge* 	possible_rewrite_edge 	= NULL;
	struct edge* 	op1 					= NULL;
	struct edge* 	op2 					= NULL;
	uint64_t 		value;

	if (node->nb_edge_dst == 2){
		for (edge_cursor = node_get_head_edge_dst(node); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
			operand = edge_get_src(edge_cursor);

			if (ir_node_get_operation(operand)->type == IR_OPERATION_TYPE_IMM){
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

		if ((op1 != NULL && op2 != NULL) || (op2 != NULL && ir_imm_operation_get_unsigned_value(ir_node_get_operation(edge_get_src(op2))) == ir_node_get_operation(node)->size)){
			if (op1 != NULL){
				value = ir_imm_operation_get_unsigned_value(ir_node_get_operation(edge_get_src(op1))) << ir_imm_operation_get_unsigned_value(ir_node_get_operation(edge_get_src(op2)));
			}
			else{
				value = 0;
			}

			if (possible_rewrite_edge == NULL){
				possible_rewrite_node = ir_add_immediate(ir, ir_node_get_operation(node)->size, value);
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
				ir_node_get_operation(possible_rewrite_node)->operation_type.imm.value 	= value;
				ir_node_get_operation(possible_rewrite_node)->size 						= ir_node_get_operation(node)->size;
				ir_edge_get_dependence(possible_rewrite_edge)->type 					= IR_DEPENDENCE_TYPE_DIRECT;
			}

			if (op1 != possible_rewrite_edge){
				ir_remove_dependence(ir, op1);
			}

			if (op2 != possible_rewrite_edge){
				ir_remove_dependence(ir, op2);
			}

			*modification = 1;
		}
		else if (op2 != NULL && ir_imm_operation_get_unsigned_value(ir_node_get_operation(edge_get_src(op2))) == 0){
			ir_remove_dependence(ir, op2);

			*modification = 1;
		}
	}
	else if (node->nb_edge_dst > 2){
		printf("WARNING: in %s, incorrect format SHL: %u operand(s)\n", __func__, node->nb_edge_dst);
	}
}

static void ir_normalize_simplify_instruction_rewrite_shl(struct ir* ir, struct node* node, uint8_t* modification, uint8_t final){
	struct edge* 		edge_cursor;
	struct node* 		node_cursor;
	struct irOperation*	operation_cursor;
	struct edge*		edge1 				= NULL;
	struct edge* 		edge2 				= NULL;
	struct edge* 		edge3 				= NULL;
	struct node* 		node1 				= NULL;
	struct node*		node2;
	struct node*		node3;

	if (node->nb_edge_dst == 1){
		graph_transfert_src_edge(&(ir->graph), edge_get_src(node_get_head_edge_dst(node)), node);
		ir_remove_node(ir, node);
		*modification = 1;

		return;
	}

	for (edge_cursor = node_get_head_edge_dst(node), node1 = NULL; edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
		node_cursor = edge_get_src(edge_cursor);
		operation_cursor = ir_node_get_operation(node_cursor);

		if (ir_edge_get_dependence(edge_cursor)->type == IR_DEPENDENCE_TYPE_DIRECT){
			node1 = edge_get_src(edge_cursor);
			if (operation_cursor->type == IR_OPERATION_TYPE_INST && (operation_cursor->operation_type.inst.opcode == IR_SHL || operation_cursor->operation_type.inst.opcode == IR_SHR)){
				edge1 = edge_cursor;
			}
		}
		else if (ir_edge_get_dependence(edge_cursor)->type == IR_DEPENDENCE_TYPE_SHIFT_DISP && operation_cursor->type == IR_OPERATION_TYPE_IMM){
			edge2 = edge_cursor;
		}
	}

	if (edge1 != NULL && edge2 != NULL){
		node1 = edge_get_src(edge1);
		node2 = edge_get_src(edge2);

		if (node1->nb_edge_src > 1){
			struct node* node_temp;

			node_temp = ir_add_inst(ir, IR_INSTRUCTION_INDEX_UNKOWN, ir_node_get_operation(node1)->size, ir_node_get_operation(node1)->operation_type.inst.opcode);
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

		for (edge_cursor = node_get_head_edge_dst(node1); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
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
					node3 = ir_add_immediate(ir, 8, value3 + value2);
				}
				else if (value3 > value2){
					node3 = ir_add_immediate(ir, 8, value3 - value2);
					if (node3 == NULL){
						printf("ERROR: in %s, unable to add immediate to IR\n", __func__);
					}
					else{
						ir_remove_dependence(ir, edge3);
						if (ir_add_dependence(ir, node3, node1, IR_DEPENDENCE_TYPE_SHIFT_DISP) == NULL){
							printf("ERROR: in %s, unable to add dependency to IR\n", __func__);
						}
					}
				}
				else if (value2 > value3){
					node3 = ir_add_immediate(ir, 8, value2 - value3);
					if (node3 == NULL){
						printf("ERROR: in %s, unable to add immediate to IR\n", __func__);
					}
					else{
						ir_remove_dependence(ir, edge3);
						if (ir_add_dependence(ir, node3, node1, IR_DEPENDENCE_TYPE_SHIFT_DISP) == NULL){
							printf("ERROR: in %s, unable to add dependency to IR\n", __func__);
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
					ir_node_get_operation(node3)->operation_type.imm.value = value3 + value2;
					ir_node_get_operation(node3)->size = 8;
				}
				else if (value3 > value2){
					ir_node_get_operation(node3)->operation_type.imm.value = value3 - value2;
					ir_node_get_operation(node3)->size = 8;
				}
				else if (value2 > value3){
					ir_node_get_operation(node3)->operation_type.imm.value = value2 - value3;
					ir_node_get_operation(node3)->size = 8;
				}
				else{
					ir_remove_dependence(ir, edge3);
					graph_transfert_dst_edge(&(ir->graph), node, node1);
				}
			}

			if (ir_node_get_operation(node1)->operation_type.inst.opcode == IR_SHL){
				graph_transfert_src_edge(&(ir->graph), node1, node);
				ir_node_get_operation(node1)->status_flag = ir_node_get_operation(node)->status_flag & IR_NODE_STATUS_FLAG_FINAL;
				ir_remove_node(ir, node);
			}
			else{
				if (value2 > value3){
					ir_node_get_operation(node1)->operation_type.inst.opcode = IR_SHL;
				}
				else if (value2 == value3){
					graph_transfert_dst_edge(&(ir->graph), node, node1);
					ir_remove_node(ir, node1);
				}

				ir_node_get_operation(node)->operation_type.inst.opcode = IR_AND;
				if (node2->nb_edge_src > 1){
					node2 = ir_add_immediate(ir, ir_node_get_operation(node)->size, (0xffffffffffffffff >> (64 - ir_node_get_operation(node)->size + value3)) << value2);
					if (node2 == NULL){
						printf("ERROR: in %s, unable to add immediate to IR\n", __func__);
					}
					else{
						ir_remove_dependence(ir, edge2);
						if (ir_add_dependence(ir, node2, node, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
							printf("ERROR: in %s, unable to add dependency to IR\n", __func__);
						}
					}
				}
				else{
					ir_node_get_operation(node2)->operation_type.imm.value = (0xffffffffffffffff >> (64 - ir_node_get_operation(node)->size + value3)) << value2;
					ir_node_get_operation(node2)->size = ir_node_get_operation(node)->size;
					ir_edge_get_dependence(edge2)->type = IR_DEPENDENCE_TYPE_DIRECT;
				}
			}

			*modification = 1;
		}
	}
}

static void ir_normalize_simplify_instruction_numeric_shr(struct ir* ir, struct node* node, uint8_t* modification){
	struct edge* 	edge_cursor;
	struct node* 	operand;
	struct node* 	possible_rewrite_node;
	struct edge* 	possible_rewrite_edge 	= NULL;
	struct edge* 	op1 					= NULL;
	struct edge* 	op2 					= NULL;
	uint64_t 		value;

	if (node->nb_edge_dst == 2){
		for (edge_cursor = node_get_head_edge_dst(node); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
			operand = edge_get_src(edge_cursor);

			if (ir_node_get_operation(operand)->type == IR_OPERATION_TYPE_IMM){
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

		if ((op1 != NULL && op2 != NULL) || (op2 != NULL && ir_imm_operation_get_unsigned_value(ir_node_get_operation(edge_get_src(op2))) == ir_node_get_operation(node)->size)){
			if (op1 != NULL){
				value = ir_imm_operation_get_unsigned_value(ir_node_get_operation(edge_get_src(op1))) >> ir_imm_operation_get_unsigned_value(ir_node_get_operation(edge_get_src(op2)));
			}
			else{
				value = 0;
			}

			if (possible_rewrite_edge == NULL){
				possible_rewrite_node = ir_add_immediate(ir, ir_node_get_operation(node)->size, value);
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
				ir_node_get_operation(possible_rewrite_node)->operation_type.imm.value 	= value;
				ir_node_get_operation(possible_rewrite_node)->size 						= ir_node_get_operation(node)->size;
				ir_edge_get_dependence(possible_rewrite_edge)->type 					= IR_DEPENDENCE_TYPE_DIRECT;
			}

			if (op1 != possible_rewrite_edge){
				ir_remove_dependence(ir, op1);
			}

			if (op2 != possible_rewrite_edge){
				ir_remove_dependence(ir, op2);
			}

			*modification = 1;
		}
		else if (op2 != NULL && ir_imm_operation_get_unsigned_value(ir_node_get_operation(edge_get_src(op2))) == 0){
			ir_remove_dependence(ir, op2);

			*modification = 1;
		}
	}
	else if (node->nb_edge_dst > 2){
		printf("WARNING: in %s, incorrect format SHR: %u operand(s)\n", __func__, node->nb_edge_dst);
	}
}

static void ir_normalize_simplify_instruction_rewrite_shr(struct ir* ir, struct node* node, uint8_t* modification, uint8_t final){
	if (node->nb_edge_dst == 1){
		graph_transfert_src_edge(&(ir->graph), edge_get_src(node_get_head_edge_dst(node)), node);
		ir_remove_node(ir, node);
		*modification = 1;
	}
}

#pragma GCC diagnostic ignored "-Wunused-parameter"
static void ir_normalize_simplify_instruction_rewrite_sub(struct ir* ir, struct node* node, uint8_t* modification, uint8_t final){
	struct edge* 		edge_cursor;
	struct irOperation*	operation_cursor;
	struct edge* 		operand1 = NULL;
	struct edge* 		operand2 = NULL;

	for (edge_cursor = node_get_head_edge_dst(node); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
		switch(ir_edge_get_dependence(edge_cursor)->type){
			case IR_DEPENDENCE_TYPE_DIRECT 			: {
				operand1 = edge_cursor;
				break;
			}
			case IR_DEPENDENCE_TYPE_SUBSTITUTE 		: {
				operand2 = edge_cursor;
				break;
			}
			default 								: {
				break;
			}
		}
	}

	if (operand1 != NULL && operand2 != NULL){
		operation_cursor = ir_node_get_operation(edge_get_src(operand1));
		if (operation_cursor->type == IR_OPERATION_TYPE_INST && operation_cursor->operation_type.inst.opcode == IR_ADD && edge_get_src(operand2)->nb_edge_src > 1){
			for (edge_cursor = node_get_head_edge_dst(edge_get_src(operand1)); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
				if (edge_get_src(edge_cursor) == edge_get_src(operand2)){
					struct node* add_node = NULL;

					if (edge_get_src(operand1)->nb_edge_src > 1){
						add_node = ir_add_inst(ir, IR_INSTRUCTION_INDEX_UNKOWN, operation_cursor->size, IR_ADD);
						if (add_node != NULL){
							for (edge_cursor = node_get_head_edge_dst(edge_get_src(operand1)); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
								if (edge_get_src(edge_cursor) != edge_get_src(operand2)){
									if (ir_add_dependence(ir, edge_get_src(edge_cursor), add_node, ir_edge_get_dependence(edge_cursor)->type) == NULL){
										printf("ERROR: in %s, unable to add dependency to IR\n", __func__);
									}
								}
							}
						}
						else{
							printf("ERROR: in %s, unable to add instruction to IR\n", __func__);
							return;
						}
					}
					else{
						ir_remove_dependence(ir, edge_cursor);
						add_node = edge_get_src(operand1);
					}

					graph_transfert_src_edge(&(ir->graph), add_node, node);
					ir_remove_node(ir, node);

					*modification = 1;
					return;
				}
			}
		}

		operation_cursor = ir_node_get_operation(edge_get_src(operand2));
		if (operation_cursor->type == IR_OPERATION_TYPE_INST && operation_cursor->operation_type.inst.opcode == IR_ADD && edge_get_src(operand1)->nb_edge_src > 1){
			for (edge_cursor = node_get_head_edge_dst(edge_get_src(operand2)); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
				if (edge_get_src(edge_cursor) == edge_get_src(operand1)){
					printf("WARNING: in %s, this case is not implemented yet (sub by a sum)\n", __func__);
					break;
				}
			}
		}

		if (operation_cursor->type == IR_OPERATION_TYPE_IMM){
			if (edge_get_src(operand2)->nb_edge_src == 1){
				operation_cursor->operation_type.imm.value = (uint64_t)(-ir_imm_operation_get_signed_value(operation_cursor));
				ir_edge_get_dependence(operand2)->type = IR_DEPENDENCE_TYPE_DIRECT;
				ir_node_get_operation(node)->operation_type.inst.opcode = IR_ADD;

				*modification = 1;
				return;
			}
			else{
				printf("WARNING: in %s, this case is not implemented yet (shared imm)\n", __func__);
			}
		}
	}
	else{
		printf("ERROR: in %s, the connectivity is wrong. Run the check procedure for more details\n", __func__);
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
				imm_zero = ir_add_immediate(ir, size, 0);
				if (imm_zero != NULL){
					if (ir_add_dependence(ir, imm_zero, node, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
						printf("ERROR: in %s, unable to add dependency to IR\n", __func__);
					}
				}
				else{
					printf("ERROR: in %s, unable to add immediate to IR\n", __func__);
				}

				i++;
				*modification = 1;
			}
		}
	}
}

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

struct commonOperand{
	struct edge* 	edge1;
	struct edge* 	edge2;
};

void ir_normalize_remove_subexpression(struct ir* ir, uint8_t* modification){
	struct node* 			main_cursor;
	#define IR_NORMALIZE_REMOVE_SUBEXPRESSION_MAX_NB_OPERAND 512
	struct commonOperand 	operand_buffer[IR_NORMALIZE_REMOVE_SUBEXPRESSION_MAX_NB_OPERAND];
	uint32_t 				i;
	uint32_t 				nb_match;
	struct node* 			new_intermediate_inst;
	uint32_t 				nb_edge_dst;
	uint32_t 				nb_operand_imm_node1;
	uint32_t 				nb_operand_imm_node2;
	struct edge* 			edge_cursor1;
	struct edge* 			edge_cursor2;
	struct node* 			node1;
	struct node* 			node2;
	struct irOperation* 	operation1;
	struct irOperation* 	operation2;
	struct edge* 			operand_cursor;
	struct edge* 			prev_edge_cursor1;
	struct edge* 			prev_edge_cursor2;

	if (dagPartialOrder_sort_src_dst(&(ir->graph))){
		printf("ERROR: in %s, unable to sort IR node(s)\n", __func__);
		return;
	}

	for (main_cursor = graph_get_head_node(&(ir->graph)); main_cursor != NULL; main_cursor = node_get_next(main_cursor)){
		if (main_cursor->nb_edge_src < 2){
			continue;
		}

		for (edge_cursor1 = node_get_head_edge_src(main_cursor), prev_edge_cursor1 = NULL; edge_cursor1 != NULL;){
			node1 = edge_get_dst(edge_cursor1);
			operation1 = ir_node_get_operation(node1);

			if (operation1->type != IR_OPERATION_TYPE_INST){
				goto next_cursor1;
			}
			
			for (edge_cursor2 = edge_get_next_src(edge_cursor1), prev_edge_cursor2 = NULL; edge_cursor2 != NULL;){
				node2 = edge_get_dst(edge_cursor2);
				operation2 = ir_node_get_operation(node2);

				if (operation2->type != IR_OPERATION_TYPE_INST || operation1->size != operation2->size || operation1->operation_type.inst.opcode != operation2->operation_type.inst.opcode || node1 == node2 || ir_edge_get_dependence(edge_cursor1)->type != ir_edge_get_dependence(edge_cursor2)->type){
					goto next_cursor2;
				}
					
				if (node1->nb_edge_dst > IR_NORMALIZE_REMOVE_SUBEXPRESSION_MAX_NB_OPERAND){
					printf("WARNING: in %s, IR_NORMALIZE_REMOVE_SUBEXPRESSION_MAX_NB_OPERAND has been reached: %u for %s\n", __func__, node1->nb_edge_dst, irOpcode_2_string(operation1->operation_type.inst.opcode));
					goto next_cursor2;
				}

				for (operand_cursor = node_get_head_edge_dst(node1), i = 0, nb_operand_imm_node1 = 0; operand_cursor != NULL; operand_cursor = edge_get_next_dst(operand_cursor), i++){
					operand_buffer[i].edge1 = operand_cursor;
					if (operand_cursor == edge_cursor1){
						operand_buffer[i].edge2 = edge_cursor2;
					}
					else{
						operand_buffer[i].edge2 = NULL;
					}
					if (numeric_simplify[operation1->operation_type.inst.opcode] != NULL){
						if (ir_node_get_operation(edge_get_src(operand_cursor))->type == IR_OPERATION_TYPE_IMM){
							nb_operand_imm_node1 ++;
							if (nb_operand_imm_node1 > 1){
								goto next_cursor2;
							}
						}
					}
				}

				for (operand_cursor = node_get_head_edge_dst(node2), nb_match = 1, nb_operand_imm_node2 = 0; operand_cursor != NULL; operand_cursor = edge_get_next_dst(operand_cursor)){
					if (numeric_simplify[operation2->operation_type.inst.opcode] != NULL){
						if (ir_node_get_operation(edge_get_src(operand_cursor))->type == IR_OPERATION_TYPE_IMM){
							nb_operand_imm_node2 ++;
							if (nb_operand_imm_node2 > 1){
								goto next_cursor2;
							}
						}
					}

					if (operand_cursor == edge_cursor2){
						continue;
					}

					for (i = 0; i < node1->nb_edge_dst; i++){
						if (operand_buffer[i].edge2 == NULL){
							if (ir_edge_get_dependence(operand_buffer[i].edge1)->type == ir_edge_get_dependence(operand_cursor)->type){
								if (edge_get_src(operand_cursor) == edge_get_src(operand_buffer[i].edge1)){
									operand_buffer[i].edge2 = operand_cursor;
									nb_match ++;
									break;
								}
								else if (ir_node_get_operation(edge_get_src(operand_cursor))->type == IR_OPERATION_TYPE_IMM && ir_node_get_operation(edge_get_src(operand_buffer[i].edge1))->type == IR_OPERATION_TYPE_IMM){
									if (ir_imm_operation_get_unsigned_value(ir_node_get_operation(edge_get_src(operand_cursor))) == ir_imm_operation_get_unsigned_value(ir_node_get_operation(edge_get_src(operand_buffer[i].edge1)))){
										operand_buffer[i].edge2 = operand_cursor;
										nb_match ++;
										break;
									}
								}
							}
						}
					}
				}

				/* CASE 1 */
				if (nb_match == node1->nb_edge_dst && nb_match == node2->nb_edge_dst){
					graph_transfert_src_edge(&(ir->graph), node1, node2);
					if (operation2->index < operation1->index){
						operation1->index = operation2->index;
					}
					ir_remove_node(ir, node2);

					*modification = 1;
					edge_cursor2 = prev_edge_cursor2;
					goto next_cursor2;
				}
				/* CASE 2 */
				else if (nb_match > 1 && nb_match == node1->nb_edge_dst){
					for (i = 0; i < node1->nb_edge_dst; i++){
						ir_remove_dependence(ir, operand_buffer[i].edge2);
					}

					if (ir_add_dependence(ir, node1, node2, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
						printf("ERROR: in %s, unable to add dependency to IR\n", __func__);
					}

					*modification = 1;
					edge_cursor2 = prev_edge_cursor2;
					goto next_cursor2;
				}
				/* CASE 3 */
				else if (nb_match > 1 && nb_match == node2->nb_edge_dst){
					for (i = 0, nb_edge_dst = node1->nb_edge_dst; i < nb_edge_dst; i++){
						if (operand_buffer[i].edge2 != NULL){
							ir_remove_dependence(ir, operand_buffer[i].edge1);
						}
					}

					if (ir_add_dependence(ir, node2, node1, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
						printf("ERROR: in %s, unable to add dependency to IR\n", __func__);
					}

					*modification = 1;
					edge_cursor1 = prev_edge_cursor1;
					goto next_cursor1;
				}
				/* CASE 4 */
				else if (nb_match > 1 && (operation1->operation_type.inst.opcode == IR_ADD || operation1->operation_type.inst.opcode == IR_XOR)){
					new_intermediate_inst = ir_add_inst(ir, IR_INSTRUCTION_INDEX_UNKOWN, operation1->size, operation1->operation_type.inst.opcode);
					if (new_intermediate_inst == NULL){
						printf("ERROR: in %s, unable to add instruction to IR\n", __func__);
					}
					else{
						for (i = 0, nb_edge_dst = node1->nb_edge_dst; i < nb_edge_dst; i++){
							if (operand_buffer[i].edge2 != NULL){
								if (ir_add_dependence(ir, edge_get_src(operand_buffer[i].edge1), new_intermediate_inst, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
									printf("ERROR: in %s, unable to add dependency to IR\n", __func__);
								}

								ir_remove_dependence(ir, operand_buffer[i].edge2);
								ir_remove_dependence(ir, operand_buffer[i].edge1);
							}
						}

						if (ir_add_dependence(ir, new_intermediate_inst, node1, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
							printf("ERROR: in %s, unable to add dependency to IR\n", __func__);
						}

						if (ir_add_dependence(ir, new_intermediate_inst, node2, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
							printf("ERROR: in %s, unable to add dependency to IR\n", __func__);
						}

						*modification = 1;
						edge_cursor1 = prev_edge_cursor1;
						goto next_cursor1;
					}
				}

				next_cursor2:
				prev_edge_cursor2 = edge_cursor2;
				if (edge_cursor2 == NULL){
					edge_cursor2 = edge_get_next_src(edge_cursor1);
				}
				else{
					edge_cursor2 = edge_get_next_src(edge_cursor2);
				}
			}

			next_cursor1:
			prev_edge_cursor1 = edge_cursor1;
			if (edge_cursor1 == NULL){
				edge_cursor1 = node_get_head_edge_src(main_cursor);
			}
			else{
				edge_cursor1 = edge_get_next_src(edge_cursor1);
			}
		}
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
						/* What happens if node1_inst_operand has other children? Nothing is broken but is it wise to perform this so called simplification? */
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

							new_node = ir_add_inst(ir, IR_INSTRUCTION_INDEX_UNKOWN, operation_cursor1->size, operation_cursor1->operation_type.inst.opcode);
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

								if (ir_add_dependence(ir, new_node, node_cursor1, ir_edge_get_dependence(edge_cursor)->type) == NULL){
									printf("ERROR: in %s, unable to add dependency to IR\n", __func__);
								}
							}
						}
						operation_cursor1->operation_type.inst.opcode = operation_cursor2->operation_type.inst.opcode;
						operation_cursor1->size = operation_cursor2->size;

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

void ir_normalize_factor_instruction(struct ir* ir, uint8_t* modification){
	struct node* 			node_cursor1;
	struct node* 			node_cursor2;
	struct node* 			node_cursor3;
	struct edge* 			edge_cursor2;
	struct edge* 			edge_current2;
	struct edge* 			edge_cursor3;
	struct irOperation* 	operation_cursor1;
	struct irOperation* 	operation_cursor2;
	uint32_t 				opcode_counter[NB_IR_OPCODE];
	uint32_t 				i;
	enum irOpcode 			opcode;
	#define IR_NORMALIZE_FACTOR_MAX_NB_OPERAND 16
	struct node* 			operand[IR_NORMALIZE_FACTOR_MAX_NB_OPERAND];
	uint8_t 				operand_count[IR_NORMALIZE_FACTOR_MAX_NB_OPERAND];
	uint8_t 				nb_operand;
	struct node*			common_operand;


	for(node_cursor1 = graph_get_head_node(&(ir->graph)); node_cursor1 != NULL; node_cursor1 = node_get_next(node_cursor1)){
		operation_cursor1 = ir_node_get_operation(node_cursor1);

		if (operation_cursor1->type == IR_OPERATION_TYPE_INST && node_cursor1->nb_edge_dst > 1){
			memset(opcode_counter, 0, sizeof(opcode_counter));
			for (edge_cursor2 = node_get_head_edge_dst(node_cursor1); edge_cursor2 != NULL; edge_cursor2 = edge_get_next_dst(edge_cursor2)){
				node_cursor2 = edge_get_src(edge_cursor2);
				operation_cursor2 = ir_node_get_operation(node_cursor2);

				if (operation_cursor2->type == IR_OPERATION_TYPE_INST && node_cursor2->nb_edge_dst == 2){
					opcode_counter[operation_cursor2->operation_type.inst.opcode] ++;
				}
			}

			for (i = 0, opcode = IR_INVALID; i < NB_IR_OPCODE; i++){
				if (opcode_counter[i] > 1){
					if ((operation_cursor1->operation_type.inst.opcode == IR_ADD && (enum irOpcode)i == IR_MUL) || 
						(operation_cursor1->operation_type.inst.opcode == IR_ADD && (enum irOpcode)i == IR_SHL) ||
						(operation_cursor1->operation_type.inst.opcode == IR_AND && (enum irOpcode)i == IR_SHL)){
						if (opcode == IR_INVALID){
							opcode = (enum irOpcode)i;
						}
						else{
							printf("WARNING: in %s, multiple scenario for factoring instruction (1/2)\n", __func__);
							goto next;
						}
					}
				}
			}

			if (opcode != IR_INVALID){
				if (opcode_counter[opcode] >= IR_NORMALIZE_FACTOR_MAX_NB_OPERAND){
					printf("WARNING: in %s, IR_NORMALIZE_FACTOR_MAX_NB_OPERAND has been reached\n", __func__);
					goto next;
				}

				for (edge_cursor2 = node_get_head_edge_dst(node_cursor1), nb_operand = 0; edge_cursor2 != NULL; edge_cursor2 = edge_get_next_dst(edge_cursor2)){
					node_cursor2 = edge_get_src(edge_cursor2);
					operation_cursor2 = ir_node_get_operation(node_cursor2);

					if (operation_cursor2->type == IR_OPERATION_TYPE_INST && node_cursor2->nb_edge_dst == 2 && operation_cursor2->operation_type.inst.opcode == opcode){
						for (edge_cursor3 = node_get_head_edge_dst(node_cursor2); edge_cursor3 != NULL; edge_cursor3 = edge_get_next_dst(edge_cursor3)){
							node_cursor3 = edge_get_src(edge_cursor3);

							for (i = 0; i < nb_operand; i++){
								if (operand[i] == node_cursor3){ /* This is a weak test: for IMM test the value */
									operand_count[i] ++;
									break;
								}
							}
							if (i == nb_operand){
								if (nb_operand == IR_NORMALIZE_FACTOR_MAX_NB_OPERAND){
									printf("WARNING: in %s, IR_NORMALIZE_FACTOR_MAX_NB_OPERAND has been reached\n", __func__);
									goto next;
								}
								operand[i] = node_cursor3;
								operand_count[i] = 1;
								nb_operand ++;
							}
						}
					}
				}

				for (i = 0, common_operand = NULL; i < nb_operand; i++){
					if (operand_count[i] > 1){
						if (common_operand == NULL){
							common_operand = operand[i];
						}
						else{
							printf("WARNING: in %s, multiple scenario for factoring instruction (2/2)\n", __func__);
							goto next;
						}
					}
				}

				if (common_operand != NULL){
					struct node* 			new_node1;
					struct node* 			new_node2;
					struct edge* 			op1;
					struct edge* 			op2;
					enum irDependenceType 	common_operand_dependence_type = IR_DEPENDENCE_TYPE_DIRECT;

					new_node1 = ir_add_inst(ir, IR_INSTRUCTION_INDEX_UNKOWN, operation_cursor1->size, operation_cursor1->operation_type.inst.opcode);
					if (new_node1 == NULL){
						printf("ERROR: in %s, unable to add inst to IR\n", __func__);
						goto next;
					}

					for (edge_cursor2 = node_get_head_edge_dst(node_cursor1), new_node2 = NULL; edge_cursor2 != NULL; edge_cursor2 = edge_get_next_dst(edge_cursor2)){
						node_cursor2 = edge_get_src(edge_cursor2);
						operation_cursor2 = ir_node_get_operation(node_cursor2);

						if (operation_cursor2->type == IR_OPERATION_TYPE_INST && node_cursor2->nb_edge_dst == 2 && operation_cursor2->operation_type.inst.opcode == opcode){
							if (node_cursor2->nb_edge_src == 1){
								new_node2 = node_cursor2;
							}

							op1 = node_get_head_edge_dst(node_cursor2);
							op2 = edge_get_next_dst(op1);

							if (edge_get_src(op1) == common_operand){
								common_operand_dependence_type = ir_edge_get_dependence(op1)->type;
								if (ir_add_dependence(ir, edge_get_src(op2), new_node1, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
									printf("ERROR: in %s, unable to add dependency to IR\n", __func__);
								}
								ir_remove_dependence(ir, op2);
							}
							else if (edge_get_src(op2) == common_operand){
								common_operand_dependence_type = ir_edge_get_dependence(op1)->type;
								if (ir_add_dependence(ir, edge_get_src(op1), new_node1, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
									printf("ERROR: in %s, unable to add dependency to IR\n", __func__);
								}
								ir_remove_dependence(ir, op1);
							}
						}
					}

					if (new_node2 == NULL){
						new_node2 = ir_add_inst(ir, IR_INSTRUCTION_INDEX_UNKOWN, operation_cursor1->size, opcode);
						if (new_node2 == NULL){
							printf("ERROR: in %s, unable to add instruction to IR\n", __func__);
							goto next;
						}
						else{
							if (ir_add_dependence(ir, new_node2, node_cursor1, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
								printf("ERROR: in %s, unable to add dependency to IR\n", __func__);
							}
							if (ir_add_dependence(ir, common_operand, new_node2, common_operand_dependence_type) == NULL){
								printf("ERROR: in %s, unable to add dependency to IR\n", __func__);
							}
						}
					}

					if (ir_add_dependence(ir, new_node1, new_node2, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
						printf("ERROR: in %s, unable to add dependency to IR\n", __func__);
					}

					for (edge_cursor2 = node_get_head_edge_dst(node_cursor1); edge_cursor2 != NULL;){
						edge_current2 = edge_cursor2;
						edge_cursor2 = edge_get_next_dst(edge_cursor2);
						node_cursor2 = edge_get_src(edge_current2);
						operation_cursor2 = ir_node_get_operation(node_cursor2);

						if (operation_cursor2->type == IR_OPERATION_TYPE_INST && node_cursor2->nb_edge_dst == 1 && operation_cursor2->operation_type.inst.opcode == opcode && node_cursor2 != new_node2){
							ir_remove_dependence(ir, edge_current2);
						}
					}

					*modification = 1;
				}
			}
		}

		next:;
	}
}

/* I wish I could delete this routine */
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
									printf("ERROR: in %s, unable to add dependency to IR\n", __func__);
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
										printf("ERROR: in %s, unable to add dependency to IR\n", __func__);
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
/* Sorting routines						                                 */
/* ===================================================================== */

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