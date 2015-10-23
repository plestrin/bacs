#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef VERBOSE
#include <time.h>
#endif

#include "irNormalize.h"
#include "irExpression.h"
#include "irVariableRange.h"
#include "dagPartialOrder.h"
#include "base.h"

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
#define IR_NORMALIZE_ALIASING_SENSITIVITY 			1 /* 0=WEAK, 1=CHECK, 2+=STRICT */
#define IR_NORMALIZE_AFFINE_EXPRESSION 				1
#define IR_NORMALIZE_REGROUP_MEM_ACCESS 			1

struct irOperand{
	struct node*	node;
	struct edge* 	edge;
};

int32_t compare_address_node_irOperand(const void* arg1, const void* arg2);

#ifdef VERBOSE

#define INIT_TIMER																																			\
	struct timespec 			timer_start_time; 																											\
	struct timespec 			timer_stop_time; 																											\
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
		log_err("unable to init multiColumnPrinter"); 																										\
		return; 																																			\
	}

#define START_TIMER 																																		\
	if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &timer_start_time)){ 																						\
		log_err("clock_gettime fails"); 																													\
	}

#define STOP_TIMER 																																			\
	if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &timer_stop_time)){ 																						\
		log_err("clock_gettime fails"); 																													\
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
	double 		timer_8_elapsed_time = 0.0;
	#endif
	uint8_t 	modification = 0;

	INIT_TIMER

	#if IR_NORMALIZE_REMOVE_DEAD_CODE == 1
	ir_normalize_remove_dead_code(ir, &modification);
	#ifdef VERBOSE
	if (modification){
		log_info("modification remove dead code @ START");
	}
	#endif
	modification = 1;
	#ifdef IR_FULL_CHECK
	ir_check(ir);
	#endif
	#else
	modification = 1;
	#endif

	while(modification){
		/*char 	file_name[256];
		snprintf(file_name, 256, "%u.dot", round_counter);
		graphPrintDot_print(&(ir->graph), file_name, NULL);*/

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
			log_info_m("modification simplify instruction @ %u", round_counter);
			modification = 0;
			modification_copy = 1;
		}
		#endif
		#ifdef IR_FULL_CHECK
		ir_check(ir);
		#endif
		#endif

		#if IR_NORMALIZE_REMOVE_SUBEXPRESSION == 1
		START_TIMER
		ir_normalize_remove_common_subexpression(ir, &modification);
		STOP_TIMER
		#ifdef VERBOSE
		timer_2_elapsed_time += (timer_stop_time.tv_sec - timer_start_time.tv_sec) + (timer_stop_time.tv_nsec - timer_start_time.tv_nsec) / 1000000000.;

		if (modification){
			log_info_m("modification remove subexpression @ %u", round_counter);
			modification = 0;
			modification_copy = 1;
		}
		#endif
		#ifdef IR_FULL_CHECK
		ir_check(ir);
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
			log_info_m("modification simplify memory @ %u", round_counter);
			modification = 0;
			modification_copy = 1;
		}
		#endif
		#ifdef IR_FULL_CHECK
		ir_check(ir);
		#endif
		#endif

		#if IR_NORMALIZE_FACTOR_INSTRUCTION == 1
		START_TIMER
		ir_normalize_factor_instruction(ir, &modification);
		STOP_TIMER
		#ifdef VERBOSE
		timer_4_elapsed_time += (timer_stop_time.tv_sec - timer_start_time.tv_sec) + (timer_stop_time.tv_nsec - timer_start_time.tv_nsec) / 1000000000.;

		if (modification){
			log_info_m("modification factor instruction @ %u", round_counter);
			modification = 0;
			modification_copy = 1;
		}
		#endif
		#ifdef IR_FULL_CHECK
		ir_check(ir);
		#endif
		#endif

		#if IR_NORMALIZE_DISTRIBUTE_IMMEDIATE == 1
		START_TIMER
		ir_normalize_distribute_immediate(ir, &modification);
		STOP_TIMER
		#ifdef VERBOSE
		timer_5_elapsed_time += (timer_stop_time.tv_sec - timer_start_time.tv_sec) + (timer_stop_time.tv_nsec - timer_start_time.tv_nsec) / 1000000000.;

		if (modification){
			log_info_m("modification distribute immediate @ %u", round_counter);
			modification = 0;
			modification_copy = 1;
		}
		#endif
		#ifdef IR_FULL_CHECK
		ir_check(ir);
		#endif
		#endif

		#if IR_NORMALIZE_EXPAND_VARIABLE == 1
		START_TIMER
		ir_normalize_expand_variable(ir, &modification);
		STOP_TIMER
		#ifdef VERBOSE
		timer_6_elapsed_time += (timer_stop_time.tv_sec - timer_start_time.tv_sec) + (timer_stop_time.tv_nsec - timer_start_time.tv_nsec) / 1000000000.;

		if (modification){
			log_info_m("modification expand variable @ %u", round_counter);
			modification = 0;
			modification_copy = 1;
		}
		#endif
		#ifdef IR_FULL_CHECK
		ir_check(ir);
		#endif
		#endif

		#if IR_NORMALIZE_REGROUP_MEM_ACCESS == 1
		START_TIMER
		ir_normalize_regroup_mem_access(ir, &modification);
		STOP_TIMER
		#ifdef VERBOSE
		timer_7_elapsed_time += (timer_stop_time.tv_sec - timer_start_time.tv_sec) + (timer_stop_time.tv_nsec - timer_start_time.tv_nsec) / 1000000000.;

		if (modification){
			log_info_m("modification regroup mem access @ %u", round_counter);
			modification = 0;
			modification_copy = 1;
		}
		#endif
		#ifdef IR_FULL_CHECK
		ir_check(ir);
		#endif
		#endif

		#if IR_NORMALIZE_AFFINE_EXPRESSION == 1
		START_TIMER
		ir_normalize_affine_expression(ir, &modification);
		STOP_TIMER
		#ifdef VERBOSE
		timer_8_elapsed_time += (timer_stop_time.tv_sec - timer_start_time.tv_sec) + (timer_stop_time.tv_nsec - timer_start_time.tv_nsec) / 1000000000.;

		if (modification){
			log_info_m("modification affine expression @ %u", round_counter);
			modification_copy = 1;
		}
		#endif
		#ifdef IR_FULL_CHECK
		ir_check(ir);
		#endif
		#endif

		#ifdef VERBOSE
		modification = modification_copy;
		round_counter ++;
		#endif
	}

	#if IR_NORMALIZE_SIMPLIFY_INSTRUCTION == 1
	do {
		modification = 0;

		START_TIMER
		ir_normalize_simplify_instruction(ir, &modification, 1);
		STOP_TIMER
		#ifdef VERBOSE
		timer_1_elapsed_time += (timer_stop_time.tv_sec - timer_start_time.tv_sec) + (timer_stop_time.tv_nsec - timer_start_time.tv_nsec) / 1000000000.;

		if (modification){
			log_info("modification simplify instruction @ FINAL");
		}
		#endif
	} while(modification);
	#endif

	#ifdef VERBOSE
	multiColumnPrinter_print_header(printer);

	multiColumnPrinter_print(printer, "Simplify instruction", timer_1_elapsed_time, NULL);
	multiColumnPrinter_print(printer, "Remove subexpression", timer_2_elapsed_time, NULL);
	multiColumnPrinter_print(printer, "Simplify memory access", timer_3_elapsed_time, NULL);
	multiColumnPrinter_print(printer, "Factor instruction", timer_4_elapsed_time, NULL);
	multiColumnPrinter_print(printer, "Distribute immediate", timer_5_elapsed_time, NULL);
	multiColumnPrinter_print(printer, "Expand variable", timer_6_elapsed_time, NULL);
	multiColumnPrinter_print(printer, "Regroup mem access", timer_7_elapsed_time, NULL);
	multiColumnPrinter_print(printer, "Affine expression", timer_8_elapsed_time, NULL);
	#endif

	CLEAN_TIMER
}

void ir_normalize_concrete(struct ir* ir){
	#ifdef VERBOSE
	uint32_t 	round_counter = 0;
	uint8_t 	modification_copy;
	double 		timer_1_elapsed_time = 0.0;
	#endif
	uint8_t 	modification = 0;

	INIT_TIMER

	#if IR_NORMALIZE_REMOVE_DEAD_CODE == 1
	ir_normalize_remove_dead_code(ir, &modification);
	#ifdef VERBOSE
	if (modification){
		log_info("modification remove dead code @ START");
	}
	#endif
	modification = 1;
	#ifdef IR_FULL_CHECK
	ir_check(ir);
	#endif
	#else
	modification = 1;
	#endif

	ir_simplify_concrete_memory_access(ir);

	#ifdef IR_FULL_CHECK
	ir_check(ir);
	#endif

	while(modification){
		modification = 0;

		#ifdef VERBOSE
		printf("*** ROUND %u ***\n", round_counter);
		modification_copy = 0;
		#endif

		#if IR_NORMALIZE_REMOVE_SUBEXPRESSION == 1
		START_TIMER
		ir_normalize_remove_common_subexpression(ir, &modification);
		STOP_TIMER
		#ifdef VERBOSE
		timer_1_elapsed_time += (timer_stop_time.tv_sec - timer_start_time.tv_sec) + (timer_stop_time.tv_nsec - timer_start_time.tv_nsec) / 1000000000.;

		if (modification){
			log_info_m("modification remove subexpression @ %u", round_counter);
			modification = 0;
			modification_copy = 1;
		}
		#endif
		#ifdef IR_FULL_CHECK
		ir_check(ir);
		#endif
		#endif

		#ifdef VERBOSE
		modification = modification_copy;
		round_counter ++;
		#endif
	}

	#ifdef VERBOSE
	multiColumnPrinter_print_header(printer);
	multiColumnPrinter_print(printer, "Remove subexpression", timer_1_elapsed_time, NULL);
	#endif

	CLEAN_TIMER
}

void ir_normalize_remove_dead_code(struct ir* ir,  uint8_t* modification){
	struct node* 			node_cursor;
	struct node* 			next_cursor;
	struct irOperation* 	operation_cursor;
	uint8_t 				local_modification = 0;

	if (dagPartialOrder_sort_src_dst(&(ir->graph))){
		log_err("unable to sort DAG");
	}

	for (node_cursor = graph_get_head_node(&(ir->graph)); node_cursor != NULL; node_cursor = next_cursor){
		next_cursor = node_get_next(node_cursor);
		operation_cursor = ir_node_get_operation(node_cursor);

		switch (operation_cursor->type){
			case IR_OPERATION_TYPE_IN_REG 	: {
				if (!node_cursor->nb_edge_src && (operation_cursor->status_flag & IR_OPERATION_STATUS_FLAG_FINAL) == 0){
					ir_remove_node(ir, node_cursor);
					local_modification = 1;
				}
				break;
			}
			case IR_OPERATION_TYPE_IN_MEM 	: {
				if (!node_cursor->nb_edge_src && (operation_cursor->status_flag & IR_OPERATION_STATUS_FLAG_FINAL) == 0){
					ir_remove_node(ir, node_cursor);
					local_modification = 1;
				}
				break;
			}
			case IR_OPERATION_TYPE_OUT_MEM  : {
				break;
			}
			case IR_OPERATION_TYPE_IMM 		: {
				if (!node_cursor->nb_edge_src && (operation_cursor->status_flag & IR_OPERATION_STATUS_FLAG_FINAL) == 0){
					ir_remove_node(ir, node_cursor);
					local_modification = 1;
				}
				break;
			}
			case IR_OPERATION_TYPE_INST 	: {
				if (!node_cursor->nb_edge_src && (operation_cursor->status_flag & IR_OPERATION_STATUS_FLAG_FINAL) == 0){
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

	#ifdef IR_FULL_CHECK
	ir_check_order(ir);
	#endif
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
static void ir_normalize_simplify_instruction_numeric_or(struct ir* ir, struct node* node, uint8_t* modification);
static void ir_normalize_simplify_instruction_symbolic_or(struct ir* ir, struct node* node, uint8_t* modification);
static void ir_normalize_simplify_instruction_rewrite_or(struct ir* ir, struct node* node, uint8_t* modification, uint8_t final);
static void ir_normalize_simplify_instruction_rewrite_part1_8(struct ir* ir, struct node* node, uint8_t* modification, uint8_t final);
static void ir_normalize_simplify_instruction_rewrite_rol(struct ir* ir, struct node* node, uint8_t* modification, uint8_t final);
static void ir_normalize_simplify_instruction_numeric_shl(struct ir* ir, struct node* node, uint8_t* modification);
static void ir_normalize_simplify_instruction_rewrite_shl(struct ir* ir, struct node* node, uint8_t* modification, uint8_t final);
static void ir_normalize_simplify_instruction_numeric_shr(struct ir* ir, struct node* node, uint8_t* modification);
static void ir_normalize_simplify_instruction_rewrite_shr(struct ir* ir, struct node* node, uint8_t* modification, uint8_t final);
static void ir_normalize_simplify_instruction_rewrite_sub(struct ir* ir, struct node* node, uint8_t* modification, uint8_t final);
static void ir_normalize_simplify_instruction_numeric_xor(struct ir* ir, struct node* node, uint8_t* modification);
static void ir_normalize_simplify_instruction_symbolic_xor(struct ir* ir, struct node* node, uint8_t* modification);
static void ir_normalize_simplify_instruction_rewrite_xor(struct ir* ir, struct node* node, uint8_t* modification, uint8_t final);

typedef void(*simplify_numeric_instruction_ptr)(struct ir*,struct node*,uint8_t*);
typedef void(*simplify_symbolic_instruction_ptr)(struct ir*,struct node*,uint8_t*);
typedef void(*simplify_rewrite_instruction_ptr)(struct ir*,struct node*,uint8_t*,uint8_t);

static const simplify_numeric_instruction_ptr numeric_simplify[NB_IR_OPCODE] = {
	NULL, 															/* 0  IR_ADC 			*/
	ir_normalize_simplify_instruction_numeric_add, 					/* 1  IR_ADD 			*/
	ir_normalize_simplify_instruction_numeric_and, 					/* 2  IR_AND 			*/
	NULL, 															/* 3  IR_CMOV 			*/
	NULL, 															/* 4  IR_DIVQ  			*/
	NULL, 															/* 5  IR_DIVR  			*/
	NULL, 															/* 6  IR_IDIV 			*/
	ir_normalize_simplify_instruction_numeric_imul, 				/* 7  IR_IMUL 			*/
	NULL, 															/* 8  IR_LEA 			*/
	NULL, 															/* 9  IR_MOV 			*/
	NULL, 															/* 10 IR_MOVZX 			*/
	ir_normalize_simplify_instruction_numeric_mul, 					/* 11 IR_MUL 			*/
	NULL, 															/* 12 IR_NEG 			*/
	NULL, 															/* 13 IR_NOT 			*/
	ir_normalize_simplify_instruction_numeric_or, 					/* 14 IR_OR 			*/
	NULL, 															/* 15 IR_PART1_8 		*/
	NULL, 															/* 16 IR_PART2_8 		*/
	NULL, 															/* 17 IR_PART1_16 		*/
	NULL, 															/* 18 IR_ROL 			*/
	NULL, 															/* 19 IR_ROR 			*/
	ir_normalize_simplify_instruction_numeric_shl, 					/* 20 IR_SHL 			*/
	NULL, 															/* 21 IR_SHLD 			*/
	ir_normalize_simplify_instruction_numeric_shr, 					/* 22 IR_SHR 			*/
	NULL, 															/* 23 IR_SHRD 			*/
	NULL, 															/* 24 IR_SUB 			*/
	ir_normalize_simplify_instruction_numeric_xor, 					/* 25 IR_XOR 			*/
	NULL, 															/* 26 IR_LOAD 			*/
	NULL, 															/* 27 IR_STORE 			*/
	NULL, 															/* 28 IR_JOKER 			*/
	NULL 															/* 29 IR_INVALID 		*/
};

static const simplify_symbolic_instruction_ptr symbolic_simplify[NB_IR_OPCODE] = {
	NULL, 															/* 0  IR_ADC 			*/
	NULL, 															/* 1  IR_ADD 			*/
	NULL, 															/* 2  IR_AND 			*/
	NULL, 															/* 3  IR_CMOV 			*/
	NULL, 															/* 4  IR_DIVQ  			*/
	NULL, 															/* 5  IR_DIVR  			*/
	NULL, 															/* 6  IR_IDIV 			*/
	NULL, 															/* 7  IR_IMUL 			*/
	NULL, 															/* 8  IR_LEA 			*/
	NULL, 															/* 9  IR_MOV 			*/
	NULL, 															/* 10 IR_MOVZX 			*/
	NULL, 															/* 11 IR_MUL 			*/
	NULL, 															/* 12 IR_NEG 			*/
	NULL, 															/* 13 IR_NOT 			*/
	ir_normalize_simplify_instruction_symbolic_or, 					/* 14 IR_OR 			*/
	NULL, 															/* 15 IR_PART1_8 		*/
	NULL, 															/* 16 IR_PART2_8 		*/
	NULL, 															/* 17 IR_PART1_16 		*/
	NULL, 															/* 18 IR_ROL 			*/
	NULL, 															/* 19 IR_ROR 			*/
	NULL, 															/* 20 IR_SHL 			*/
	NULL, 															/* 21 IR_SHLD 			*/
	NULL, 															/* 22 IR_SHR 			*/
	NULL, 															/* 23 IR_SHRD 			*/
	NULL, 															/* 24 IR_SUB 			*/
	ir_normalize_simplify_instruction_symbolic_xor, 				/* 25 IR_XOR 			*/
	NULL, 															/* 26 IR_LOAD 			*/
	NULL, 															/* 27 IR_STORE 			*/
	NULL, 															/* 28 IR_JOKER 			*/
	NULL 															/* 29 IR_INVALID 		*/
};

static const simplify_rewrite_instruction_ptr rewrite_simplify[NB_IR_OPCODE] = {
	NULL, 															/* 0  IR_ADC 			*/
	ir_normalize_simplify_instruction_rewrite_add, 					/* 1  IR_ADD 			*/
	ir_normalize_simplify_instruction_rewrite_and, 					/* 2  IR_AND 			*/
	NULL, 															/* 3  IR_CMOV 			*/
	NULL, 															/* 4  IR_DIVQ  			*/
	NULL, 															/* 5  IR_DIVR  			*/
	NULL, 															/* 6  IR_IDIV 			*/
	ir_normalize_simplify_instruction_rewrite_imul, 				/* 7  IR_IMUL 			*/
	NULL, 															/* 8  IR_LEA 			*/
	NULL, 															/* 9  IR_MOV 			*/
	ir_normalize_simplify_instruction_rewrite_movzx, 				/* 10 IR_MOVZX 			*/
	ir_normalize_simplify_instruction_rewrite_mul, 					/* 11 IR_MUL 			*/
	NULL, 															/* 12 IR_NEG 			*/
	NULL, 															/* 13 IR_NOT 			*/
	ir_normalize_simplify_instruction_rewrite_or, 					/* 14 IR_OR 			*/
	ir_normalize_simplify_instruction_rewrite_part1_8, 				/* 15 IR_PART1_8 		*/
	NULL, 															/* 16 IR_PART2_8 		*/
	NULL, 															/* 17 IR_PART1_16 		*/
	ir_normalize_simplify_instruction_rewrite_rol, 					/* 18 IR_ROL 			*/
	NULL, 															/* 19 IR_ROR 			*/
	ir_normalize_simplify_instruction_rewrite_shl, 					/* 20 IR_SHL 			*/
	NULL, 															/* 21 IR_SHLD 			*/
	ir_normalize_simplify_instruction_rewrite_shr, 					/* 22 IR_SHR 			*/
	NULL, 															/* 23 IR_SHRD 			*/
	ir_normalize_simplify_instruction_rewrite_sub, 					/* 24 IR_SUB 			*/
	ir_normalize_simplify_instruction_rewrite_xor, 					/* 25 IR_XOR 			*/
	NULL, 															/* 26 IR_LOAD 			*/
	NULL, 															/* 27 IR_STORE 			*/
	NULL, 															/* 28 IR_JOKER 			*/
	NULL 															/* 29 IR_INVALID 		*/
};

void ir_normalize_simplify_instruction(struct ir* ir, uint8_t* modification, uint8_t final){
	struct node* 			node_cursor;
	struct node* 			next_node_cursor;
	struct irOperation* 	operation;
	#ifdef IR_FULL_CHECK
	uint8_t 				local_modification = 0;
	enum irOpcode 			local_opcode;
	#endif

	if (dagPartialOrder_sort_src_dst(&(ir->graph))){
		log_err("unable to sort ir node(s)");
		return;
	}

	for(node_cursor = graph_get_tail_node(&(ir->graph)), next_node_cursor = NULL; node_cursor != NULL;){
		operation = ir_node_get_operation(node_cursor);
		if (operation->type == IR_OPERATION_TYPE_INST){
			#ifdef IR_FULL_CHECK
			local_opcode = operation->operation_type.inst.opcode;

			if (symbolic_simplify[operation->operation_type.inst.opcode] != NULL){
				symbolic_simplify[operation->operation_type.inst.opcode](ir, node_cursor, &local_modification);
			}
			if (numeric_simplify[operation->operation_type.inst.opcode] != NULL){
				numeric_simplify[operation->operation_type.inst.opcode](ir, node_cursor, &local_modification);
			}
			if (rewrite_simplify[operation->operation_type.inst.opcode] != NULL){
				rewrite_simplify[operation->operation_type.inst.opcode](ir, node_cursor, &local_modification, final);
			}

			if (local_modification){
				*modification = 1;
				if (ir_check(ir)){
					log_debug_m("IR check failed after simplification of instruction %s", irOpcode_2_string(local_opcode));
				}
				local_modification = 0;
			}
			#else
			if (numeric_simplify[operation->operation_type.inst.opcode] != NULL){
				numeric_simplify[operation->operation_type.inst.opcode](ir, node_cursor, modification);
			}
			if (symbolic_simplify[operation->operation_type.inst.opcode] != NULL){
				symbolic_simplify[operation->operation_type.inst.opcode](ir, node_cursor, modification);
			}
			if (rewrite_simplify[operation->operation_type.inst.opcode] != NULL){
				rewrite_simplify[operation->operation_type.inst.opcode](ir, node_cursor, modification, final);
			}
			#endif
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
							log_err("unable to add immediate to IR"); 																						\
							return; 																														\
						} 																																	\
						else{ 																																\
							if (ir_add_dependence(ir, possible_rewrite, node, IR_DEPENDENCE_TYPE_DIRECT) == NULL){ 											\
								log_err("unable to add dependency to IR"); 																					\
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

#define ir_normalize_simplify_instruction_rewrite_generic_1op(ir, node, modification) 																		\
	if ((node)->nb_edge_dst == 1){ 																															\
		ir_merge_equivalent_node((ir), edge_get_src(node_get_head_edge_dst(node)), (node)); 																\
		*(modification) = 1; 																																\
		return; 																																			\
	}

#define ir_normalize_simplify_instruction_rewrite_absorbing_element(ir, node, absorbing_el, modification) 													\
	{ 																																						\
		struct edge* 		edge_cursor_; 																													\
		struct irOperation* operation_cursor_; 																												\
																																							\
		for (edge_cursor_ = node_get_head_edge_dst(node); edge_cursor_ != NULL; edge_cursor_ = edge_get_next_dst(edge_cursor_)){ 							\
			operation_cursor_ = ir_node_get_operation(edge_get_src(edge_cursor_)); 																			\
																																							\
			if (operation_cursor_->type == IR_OPERATION_TYPE_IMM && ir_imm_operation_get_unsigned_value(operation_cursor_) == (absorbing_el)){ 				\
				ir_convert_node_to_imm(ir, node, ir_node_get_operation(node)->size, absorbing_el); 															\
																																							\
				*(modification) = 1; 																														\
				return; 																																	\
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
	int32_t 			diff;

	ir_normalize_simplify_instruction_rewrite_generic_1op(ir, node, modification)

	for (edge_cursor1 = node_get_head_edge_dst(node), nb_imm_operand = 0; edge_cursor1 != NULL; edge_cursor1 = edge_get_next_dst(edge_cursor1)){
		operand_operation = ir_node_get_operation(edge_get_src(edge_cursor1));

		if (operand_operation->type == IR_OPERATION_TYPE_IMM){
			nb_imm_operand ++;
		}
	}

	for (edge_cursor1 = node_get_head_edge_dst(node); edge_cursor1 != NULL; ){
		current_edge = edge_cursor1;
		edge_cursor1 = edge_get_next_dst(edge_cursor1);
		operand_operation = ir_node_get_operation(edge_get_src(current_edge));

		if (operand_operation->type == IR_OPERATION_TYPE_INST && operand_operation->operation_type.inst.opcode == IR_ADD){
			if (operand_operation->operation_type.inst.dst == IR_OPERATION_DST_UNKOWN || ir_node_get_operation(node)->operation_type.inst.dst == IR_OPERATION_DST_UNKOWN){
				log_warn("unkown instruction dst");
				diff = 0;
			}
			else{
				diff = operand_operation->operation_type.inst.dst - ir_node_get_operation(node)->operation_type.inst.dst;
			}

			if (!diff){
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
							log_err("unable to copy dst edge");
						}
						ir_remove_dependence(ir, current_edge);

						*modification = 1;
						continue;
					}
				}
			}

			if (nb_imm_operand > 0){
				for (edge_cursor2 = node_get_head_edge_dst(edge_get_src(current_edge)); edge_cursor2 != NULL; edge_cursor2 = edge_get_next_dst(edge_cursor2)){
					if (ir_node_get_operation(edge_get_src(edge_cursor2))->type == IR_OPERATION_TYPE_IMM){
						if (graph_copy_dst_edge(&(ir->graph), node, edge_get_src(current_edge))){
							log_err("unable to copy dst edge");
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

static void ir_normalize_simplify_instruction_rewrite_and(struct ir* ir, struct node* node, uint8_t* modification, uint8_t final){
	struct edge* 			imm_operand 	= NULL;
	struct edge* 			and_operand 	= NULL;
	struct edge* 			edge_cursor1;
	struct edge* 			edge_cursor2;
	struct irOperation*		operand_operation;
	struct node* 			operand;
	uint64_t 				imm_value;
	struct node** 			operand_buffer;
	uint32_t 				nb_operand;

	ir_normalize_simplify_instruction_rewrite_generic_1op(ir, node, modification)
	ir_normalize_simplify_instruction_rewrite_absorbing_element(ir, node, 0, modification)

	operand_buffer = (struct node**)alloca(sizeof(struct node*) * node->nb_edge_dst);

	for (edge_cursor1 = node_get_head_edge_dst(node), nb_operand = 0; edge_cursor1 != NULL; edge_cursor1 = edge_get_next_dst(edge_cursor1)){
		operand = edge_get_src(edge_cursor1);

		if (ir_node_get_operation(operand)->type == IR_OPERATION_TYPE_IMM){
			imm_operand = edge_cursor1;
		}
		else{
			operand_buffer[nb_operand ++] = operand;
		}
	}

	if (imm_operand != NULL){
		imm_value = ir_imm_operation_get_unsigned_value(ir_node_get_operation(edge_get_src(imm_operand)));
		if (variableRange_is_mask_compact(imm_value)){
			struct variableRange 	range;
			struct variableRange 	mask_range;

			ir_drop_range(ir); /* We are not sure what have been done before. Might be removed later */

			irVariableRange_get_range_and_buffer(&range, operand_buffer, nb_operand, ir_node_get_operation(node)->size, ir->range_seed);
			variableRange_init_mask(&mask_range, imm_value, ir_node_get_operation(edge_get_src(imm_operand))->size);

			if (variableRange_include(&mask_range, &range)){
				ir_remove_dependence(ir, imm_operand);

				*modification = 1;
				ir_normalize_simplify_instruction_rewrite_and(ir, node, modification, final);
				return;
			}
		}

		for (edge_cursor1 = node_get_head_edge_dst(node); edge_cursor1 != NULL; edge_cursor1 = edge_get_next_dst(edge_cursor1)){
			operand = edge_get_src(edge_cursor1);
			operand_operation = ir_node_get_operation(operand);

			if (operand_operation->type == IR_OPERATION_TYPE_INST && operand_operation->operation_type.inst.opcode == IR_AND){
				for (edge_cursor2 = node_get_head_edge_dst(operand); edge_cursor2 != NULL; edge_cursor2 = edge_get_next_dst(edge_cursor2)){
					if (ir_node_get_operation(edge_get_src(edge_cursor2))->type == IR_OPERATION_TYPE_IMM){
						if (and_operand != NULL){
							log_warn("multiple AND operands, can't decide how to associate IMM");
							return;
						}
						and_operand = edge_cursor1;
					}
				}
			}
		}

		if (and_operand != NULL){
			if (graph_copy_dst_edge(&(ir->graph), node, edge_get_src(and_operand))){
				log_err("unable to copy dst edge");
			}
			ir_remove_dependence(ir, and_operand);

			*modification = 1;
		}
	}
}

#pragma GCC diagnostic ignored "-Wunused-parameter"
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

	if (nb_imm_operand < 1 || (nb_imm_operand == 1 && (value & (0xffffffffffffffffULL >> (64 - size))) != 1)){
		return;
	}

	if ((value & (0xffffffffffffffffULL >> (64 - size))) != 1){
		if (possible_rewrite == NULL){
			possible_rewrite = ir_add_immediate(ir, size, value);
			if (possible_rewrite == NULL){
				log_err("unable to add immediate to IR");
				return;
			}
			else{
				if (ir_add_dependence(ir, possible_rewrite, node, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
					log_err("unable to add dependency to IR");
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
	struct irOperation*		operation_cursor;

	ir_normalize_simplify_instruction_rewrite_generic_1op(ir, node, modification)
	ir_normalize_simplify_instruction_rewrite_absorbing_element(ir, node, 0, modification)

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

static void ir_normalize_simplify_instruction_rewrite_movzx(struct ir* ir, struct node* node, uint8_t* modification, uint8_t final){
	struct edge* 		edge;
	struct node* 		operand;
	struct irOperation*	operand_operation;
	struct node* 		node_imm_new;

	if (node->nb_edge_dst == 1){
		edge = node_get_head_edge_dst(node);
		operand = edge_get_src(edge);
		operand_operation = ir_node_get_operation(operand);
		if (operand_operation->type == IR_OPERATION_TYPE_INST && (operand_operation->operation_type.inst.opcode == IR_PART1_8 || operand_operation->operation_type.inst.opcode == IR_PART1_16)){
			if (operand->nb_edge_dst == 1){
				if (ir_node_get_operation(edge_get_src(node_get_head_edge_dst(operand)))->size != ir_node_get_operation(node)->size){
					return;
				}
			}

			node_imm_new = ir_add_immediate(ir, ir_node_get_operation(node)->size, 0xffffffffffffffff >> (64 - operand_operation->size));
			if (node_imm_new != NULL){
				if (ir_add_dependence(ir, node_imm_new, node, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
					log_err("unable to add dependency to IR");
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
				log_err("unable to add immediate to IR");
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
						log_err("unable to add dependency to IR");
					}
				}
				else{
					log_err("unable to add immediate to IR");
				}

				operand_operation->operation_type.inst.opcode = IR_SHR;
				operand_operation->size = ir_node_get_operation(node)->size;

				node_imm_new = ir_add_immediate(ir, 8, 8);
				if (node_imm_new != NULL){
					if (ir_add_dependence(ir, node_imm_new, operand, IR_DEPENDENCE_TYPE_SHIFT_DISP) == NULL){
						log_err("unable to add dependency to IR");
					}
					else{
						*modification = 1;
					}
				}
				else{
					log_err("unable to add immediate to IR");
				}

			}
			else{
				log_warn("PART2_8 instruction is shared, this case is not implemented yet -> skip");
			}
		}
	}
}

static void ir_normalize_simplify_instruction_numeric_mul(struct ir* ir, struct node* node, uint8_t* modification){
	ir_normalize_simplify_instruction_numeric_generic(ir, node, modification, 1, *)
}

static void ir_normalize_simplify_instruction_rewrite_mul(struct ir* ir, struct node* node, uint8_t* modification, uint8_t final){
	struct edge* 			edge_cursor;
	struct irOperation*		operation_cursor;
	struct node* 			imm_node;

	ir_normalize_simplify_instruction_rewrite_generic_1op(ir, node, modification)
	ir_normalize_simplify_instruction_rewrite_absorbing_element(ir, node, 0, modification)

	if (node->nb_edge_dst == 2){
		for (edge_cursor = node_get_head_edge_dst(node); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
			operation_cursor = ir_node_get_operation(edge_get_src(edge_cursor));

			if (operation_cursor->type == IR_OPERATION_TYPE_IMM && __builtin_popcount(ir_imm_operation_get_unsigned_value(operation_cursor)) == 1){
				ir_node_get_operation(node)->operation_type.inst.opcode = IR_SHL;
				if (edge_get_src(edge_cursor)->nb_edge_src > 1){
					imm_node = ir_add_immediate(ir, 8, __builtin_ctz(ir_imm_operation_get_unsigned_value(operation_cursor)));
					if (imm_node == NULL){
						log_err("unable to add immediate to IR");
					}
					else{
						if (ir_add_dependence(ir, imm_node, node, IR_DEPENDENCE_TYPE_SHIFT_DISP) == NULL){
							log_err("unable to add depedence to IR");
						}

						ir_remove_dependence(ir, edge_cursor);

						*modification = 1;
						return;
					}
				}
				else{
					operation_cursor->operation_type.imm.value = __builtin_ctz(ir_imm_operation_get_unsigned_value(operation_cursor));
					ir_edge_get_dependence(edge_cursor)->type = IR_DEPENDENCE_TYPE_SHIFT_DISP;

					*modification = 1;
					break;
				}
			}
		}
	}
}

static void ir_normalize_simplify_instruction_numeric_or(struct ir* ir, struct node* node, uint8_t* modification){
	ir_normalize_simplify_instruction_numeric_generic(ir, node, modification, 0, |)
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
	struct edge* 			edge_cursor;
	struct edge* 			current_edge;
	struct irOperation* 	operand_operation;
	#if 0
	/* This was a good idea but it broke TEST_SHA1_WR. I coul dbe interesting to try the opposite */
	struct variableRange* 	range;
	uint32_t 				i;
	uint32_t 				j;
	#endif

	ir_normalize_simplify_instruction_rewrite_generic_1op(ir, node, modification)
	ir_normalize_simplify_instruction_rewrite_absorbing_element(ir, node, (0xffffffffffffffff >> (64 - ir_node_get_operation(node)->size)), modification)

	for (edge_cursor = node_get_head_edge_dst(node); edge_cursor != NULL; ){
		current_edge = edge_cursor;
		edge_cursor = edge_get_next_dst(edge_cursor);
		operand_operation = ir_node_get_operation(edge_get_src(current_edge));

		if (operand_operation->type == IR_OPERATION_TYPE_INST && operand_operation->operation_type.inst.opcode == IR_OR){
			if (edge_get_src(current_edge)->nb_edge_src == 1){
				graph_transfert_dst_edge(&(ir->graph), node, edge_get_src(current_edge));
				ir_remove_node(ir, edge_get_src(current_edge));

				*modification = 1;
			}
		}
	}

	#if 0
	ir_drop_range(ir); /* We are not sure what have been done before. Might be removed later */

	range = (struct variableRange*)alloca(node->nb_edge_dst * sizeof(struct variableRange));
	for (edge_cursor = node_get_head_edge_dst(node), i = 0; edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor), i++){
		irVariableRange_compute(edge_get_src(edge_cursor), range + i, ir->range_seed);
		if (ir_operation_get_range(ir_node_get_operation(edge_get_src(edge_cursor)))){
			memcpy(range + i, ir_operation_get_range(ir_node_get_operation(edge_get_src(edge_cursor))), sizeof(struct variableRange));
		}
	}

	for (i = 0; i < node->nb_edge_dst; i++){
		if (range[i].disp == 0 && range[i].index_lo < range[i].index_up){
			if (range[i].scale != 0xffffffff){
				for (j = i + 1; j < node->nb_edge_dst; j++){
					if ((range[i].index_up << range[i].scale) >= (0x1ULL << range[j].scale) && (range[j].index_up << range[j].scale) >= (0x1ULL << range[i].scale)){
						break;
					}
				}
				if (j != node->nb_edge_dst){
					break;
				}
			}
		}
		else{
			break;
		}
	}
	if (i == node->nb_edge_dst){
		ir_node_get_operation(node)->operation_type.inst.opcode = IR_XOR;
		*modification = 1;
	}
	#endif
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
					log_err_m("MOVZX instruction has %u operand(s)", operand->nb_edge_dst);
				}
			}
		}
		else if (operand_operation->type == IR_OPERATION_TYPE_IMM){
			ir_convert_node_to_imm(ir, node, 8, operand_operation->operation_type.imm.value);

			*modification = 1;
		}
	}
}

static void ir_normalize_simplify_instruction_rewrite_rol(struct ir* ir, struct node* node, uint8_t* modification, uint8_t final){
	struct edge* 		edge_cursor;
	struct edge* 		edge_next;
	struct irOperation*	operand_operation;
	struct node* 		new_imm;

	if (node->nb_edge_dst == 2){
		for (edge_cursor = node_get_head_edge_dst(node); edge_cursor != NULL; edge_cursor = edge_next){
			edge_next = edge_get_next_dst(edge_cursor);
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
						log_err("unable to add immediate to IR");
					}
					else{
						if (ir_add_dependence(ir, new_imm, node, IR_DEPENDENCE_TYPE_SHIFT_DISP) == NULL){
							log_err("unable to add dependency to IR");
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
					log_err("unable to add immediate to IR");
					return;
				}
				else{
					if (ir_add_dependence(ir, possible_rewrite_node, node, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
						log_err("unable to add dependency to IR");
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
		log_warn_m("incorrect format SHL: %u operand(s)", node->nb_edge_dst);
	}
}

static void ir_normalize_simplify_instruction_rewrite_shl(struct ir* ir, struct node* node, uint8_t* modification, uint8_t final){
	struct edge* 		edge_cursor;
	struct irOperation*	operation_cursor;
	struct edge*		edge1 				= NULL;
	struct edge* 		edge2 				= NULL;
	struct edge* 		edge3 				= NULL;
	struct node* 		node1;
	struct node*		node2;
	struct node*		node3;

	ir_normalize_simplify_instruction_rewrite_generic_1op(ir, node, modification)

	for (edge_cursor = node_get_head_edge_dst(node); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
		operation_cursor = ir_node_get_operation(edge_get_src(edge_cursor));

		if (ir_edge_get_dependence(edge_cursor)->type == IR_DEPENDENCE_TYPE_DIRECT){
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

			node_temp = ir_add_inst(ir, IR_OPERATION_INDEX_UNKOWN, ir_node_get_operation(node1)->size, ir_node_get_operation(node1)->operation_type.inst.opcode, IR_OPERATION_DST_UNKOWN);
			if (node_temp == NULL){
				log_err("unable to add instruction to IR");
				return;
			}

			ir_remove_dependence(ir, edge1);
			edge1 = ir_add_dependence(ir, node_temp, node, IR_DEPENDENCE_TYPE_DIRECT);
			if (edge1 == NULL){
				log_err("unable to add dependency to IR");
				return;
			}

			if (graph_copy_dst_edge(&(ir->graph), node_temp, node1)){
				log_err("unable to copy dst edge");
			}

			node1 = node_temp;
		}

		for (edge_cursor = node_get_head_edge_dst(node1); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
			operation_cursor = ir_node_get_operation(edge_get_src(edge_cursor));

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
					if (node3 == NULL){
						log_err("unable to add immediate to IR");
					}
					else{
						ir_remove_dependence(ir, edge3);
						if (ir_add_dependence(ir, node3, node1, IR_DEPENDENCE_TYPE_SHIFT_DISP) == NULL){
							log_err("unable to add dependency to IR");
						}
					}
				}
				else if (value3 > value2){
					node3 = ir_add_immediate(ir, 8, value3 - value2);
					if (node3 == NULL){
						log_err("unable to add immediate to IR");
					}
					else{
						ir_remove_dependence(ir, edge3);
						if (ir_add_dependence(ir, node3, node1, IR_DEPENDENCE_TYPE_SHIFT_DISP) == NULL){
							log_err("unable to add dependency to IR");
						}
					}
				}
				else if (value2 > value3){
					node3 = ir_add_immediate(ir, 8, value2 - value3);
					if (node3 == NULL){
						log_err("unable to add immediate to IR");
					}
					else{
						ir_remove_dependence(ir, edge3);
						if (ir_add_dependence(ir, node3, node1, IR_DEPENDENCE_TYPE_SHIFT_DISP) == NULL){
							log_err("unable to add dependency to IR");
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
				ir_node_get_operation(node1)->status_flag = ir_node_get_operation(node)->status_flag & IR_OPERATION_STATUS_FLAG_FINAL;
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
						log_err("unable to add immediate to IR");
					}
					else{
						ir_remove_dependence(ir, edge2);
						if (ir_add_dependence(ir, node2, node, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
							log_err("unable to add dependency to IR");
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
					log_err("unable to add immediate to IR");
					return;
				}
				else{
					if (ir_add_dependence(ir, possible_rewrite_node, node, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
						log_err("unable to add dependency to IR");
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
		log_warn_m("incorrect format SHR: %u operand(s)", node->nb_edge_dst);
	}
}

static void ir_normalize_simplify_instruction_rewrite_shr(struct ir* ir, struct node* node, uint8_t* modification, uint8_t final){
	struct edge* 	edge_cursor;
	struct node* 	direct_operand 	= NULL;
	struct node* 	disp_operand;
	uint64_t 		value 			= 0;

	ir_normalize_simplify_instruction_rewrite_generic_1op(ir, node, modification)

	for (edge_cursor = node_get_head_edge_dst(node); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
		if (ir_edge_get_dependence(edge_cursor)->type == IR_DEPENDENCE_TYPE_SHIFT_DISP){
			disp_operand = edge_get_src(edge_cursor);
			if (ir_node_get_operation(disp_operand)->type == IR_OPERATION_TYPE_IMM){
				value = ir_imm_operation_get_unsigned_value(ir_node_get_operation(disp_operand));
			}
		}
		else if (ir_edge_get_dependence(edge_cursor)->type == IR_DEPENDENCE_TYPE_DIRECT){
			direct_operand = edge_get_src(edge_cursor);
		}
	}

	if (value != 0 && direct_operand != NULL && direct_operand->nb_edge_src == 1 && ir_node_get_operation(direct_operand)->type == IR_OPERATION_TYPE_INST && ir_node_get_operation(direct_operand)->operation_type.inst.opcode == IR_SHR){

		for (edge_cursor = node_get_head_edge_dst(direct_operand); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
			if (ir_edge_get_dependence(edge_cursor)->type == IR_DEPENDENCE_TYPE_SHIFT_DISP){
				disp_operand = edge_get_src(edge_cursor);
				if (ir_node_get_operation(disp_operand)->type == IR_OPERATION_TYPE_IMM){
					if (disp_operand->nb_edge_src == 1){
						ir_node_get_operation(disp_operand)->operation_type.imm.value += value;
					}
					else{
						disp_operand = ir_add_immediate(ir, ir_node_get_operation(node)->size, value + ir_imm_operation_get_unsigned_value(ir_node_get_operation(disp_operand)));
						if (disp_operand == NULL){
							log_err("unable to add immediate to IR");
						}
						else{
							if (ir_add_dependence(ir, disp_operand, direct_operand, IR_DEPENDENCE_TYPE_SHIFT_DISP) == NULL){
								log_err("unable to add dependency to IR");
							}
							else{
								ir_remove_dependence(ir, edge_cursor);
							}
						}
					}
					graph_transfert_src_edge(&(ir->graph), direct_operand, node);
					ir_remove_node(ir, node);

					*modification = 1;
					return;
				}
			}
		}
	}


}

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

	if (operand1 == NULL || operand2 == NULL){
		log_err("the connectivity is wrong. Run the check procedure for more details");
		return;
	}

	operation_cursor = ir_node_get_operation(edge_get_src(operand1));
	if (operation_cursor->type == IR_OPERATION_TYPE_INST && operation_cursor->operation_type.inst.opcode == IR_ADD && edge_get_src(operand2)->nb_edge_src > 1){
		for (edge_cursor = node_get_head_edge_dst(edge_get_src(operand1)); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
			if (edge_get_src(edge_cursor) == edge_get_src(operand2)){

				for (edge_cursor = node_get_head_edge_dst(edge_get_src(operand1)); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
					if (edge_get_src(edge_cursor) != edge_get_src(operand2)){
						if (ir_add_dependence(ir, edge_get_src(edge_cursor), node, ir_edge_get_dependence(edge_cursor)->type) == NULL){
							log_err("unable to add dependency to IR");
						}
					}
				}

				ir_remove_dependence(ir, operand1);
				ir_remove_dependence(ir, operand2);

				ir_node_get_operation(node)->operation_type.inst.opcode = IR_ADD;
				ir_normalize_simplify_instruction_rewrite_add(ir, node, modification, final);

				*modification = 1;
				return;
			}
		}
	}

	operation_cursor = ir_node_get_operation(edge_get_src(operand2));
	if (operation_cursor->type == IR_OPERATION_TYPE_INST && operation_cursor->operation_type.inst.opcode == IR_ADD && edge_get_src(operand1)->nb_edge_src > 1){
		for (edge_cursor = node_get_head_edge_dst(edge_get_src(operand2)); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
			if (edge_get_src(edge_cursor) == edge_get_src(operand1)){
				log_warn("this case is not implemented yet (sub by a sum)");
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
			struct node* imm;

			imm = ir_add_immediate(ir, operation_cursor->size, (uint64_t)(-ir_imm_operation_get_signed_value(operation_cursor)));
			if (imm == NULL){
				log_err("unable to add immediate to IR");
			}
			else{
				if (ir_add_dependence(ir, imm, node, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
					log_err("unable to add dependency to IR");
				}
				ir_remove_dependence(ir, operand2);
				ir_node_get_operation(node)->operation_type.inst.opcode = IR_ADD;

				*modification = 1;
				return;
			}
		}
	}
}

static void ir_normalize_simplify_instruction_numeric_xor(struct ir* ir, struct node* node, uint8_t* modification){
	ir_normalize_simplify_instruction_numeric_generic(ir, node, modification, 0, ^)
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
						log_err("unable to add dependency to IR");
					}
				}
				else{
					log_err("unable to add immediate to IR");
				}

				i++;
				*modification = 1;
			}
		}
	}
}

static void ir_normalize_simplify_instruction_rewrite_xor(struct ir* ir, struct node* node, uint8_t* modification, uint8_t final){
	struct edge* 		edge_cursor;
	struct edge* 		current_edge;
	struct irOperation*	operand_operation;
	int32_t 			diff;

	ir_normalize_simplify_instruction_rewrite_generic_1op(ir, node, modification)

	if(node->nb_edge_dst == 2){
		for(edge_cursor = node_get_head_edge_dst(node); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
			operand_operation = ir_node_get_operation(edge_get_src(edge_cursor));
			if (operand_operation->type == IR_OPERATION_TYPE_IMM){
				if (ir_imm_operation_get_unsigned_value(operand_operation) == (0xffffffffffffffffULL >> (64 - ir_node_get_operation(node)->size))){
					ir_remove_dependence(ir, edge_cursor);
					ir_node_get_operation(node)->operation_type.inst.opcode = IR_NOT;

					*modification = 1;
					return;
				}
			}
		}
	}

	for (edge_cursor = node_get_head_edge_dst(node); edge_cursor != NULL; ){
		current_edge = edge_cursor;
		edge_cursor = edge_get_next_dst(edge_cursor);
		operand_operation = ir_node_get_operation(edge_get_src(current_edge));

		if (operand_operation->type == IR_OPERATION_TYPE_INST && operand_operation->operation_type.inst.opcode == IR_XOR){
			if (operand_operation->operation_type.inst.dst == IR_OPERATION_DST_UNKOWN || ir_node_get_operation(node)->operation_type.inst.dst == IR_OPERATION_DST_UNKOWN){
				log_warn("unkown instruction dst");
				diff = 0;
			}
			else{
				diff = operand_operation->operation_type.inst.dst - ir_node_get_operation(node)->operation_type.inst.dst;
			}

			if (!diff){
				if (edge_get_src(current_edge)->nb_edge_src == 1){
					graph_transfert_dst_edge(&(ir->graph), node, edge_get_src(current_edge));
					ir_remove_node(ir, edge_get_src(current_edge));

					*modification = 1;
					continue;
				}
				else if (final){
					if (edge_get_src(current_edge)->nb_edge_dst > 100){
						log_warn_m("uncommon number of dst edge (%u), do not develop", edge_get_src(current_edge)->nb_edge_dst);
					}
					else{
						if (graph_copy_dst_edge(&(ir->graph), node, edge_get_src(current_edge))){
							log_err("unable to copy dst edge");
						}
						ir_remove_dependence(ir, current_edge);

						*modification = 1;
						continue;
					}
				}
			}
		}
	}
/*
	if (*modification == 1 && final){
		ir_normalize_simplify_instruction_symbolic_xor(ir, node, modification);
		ir_normalize_simplify_instruction_numeric_xor(ir, node, modification);
		ir_normalize_simplify_instruction_rewrite_xor(ir, node, modification, 0);
	}
*/
}

void ir_normalize_remove_common_subexpression(struct ir* ir, uint8_t* modification){
	struct commonOperand{
		struct edge* 	edge1;
		struct edge* 	edge2;
	};

	#define commonOperand_swap(cop1, cop2) 			\
		(cop1)->edge2 = (cop2)->edge1; 				\
		(cop2)->edge1 = (cop1)->edge1; 				\
		(cop1)->edge1 = (cop1)->edge2;

	struct node* 			main_cursor;
	#define IR_NORMALIZE_REMOVE_SUBEXPRESSION_MAX_NB_OPERAND 512
	struct commonOperand 	operand_buffer[IR_NORMALIZE_REMOVE_SUBEXPRESSION_MAX_NB_OPERAND];
	uint32_t 				i;
	uint32_t 				nb_match;
	struct node* 			new_intermediate_inst;
	uint32_t 				nb_imm_node;
	struct edge* 			edge_cursor1;
	struct edge* 			edge_cursor2;
	struct node* 			node1;
	struct node* 			node2;
	struct irOperation* 	operation1;
	struct irOperation* 	operation2;
	struct edge* 			operand_cursor;
	struct edge* 			prev_edge_cursor1;
	struct edge* 			prev_edge_cursor2;
	uint32_t 				init_operand_buffer;
	uint32_t 				nb_edge_node1;
	uint32_t 				nb_edge_node2;

	if (dagPartialOrder_sort_src_dst(&(ir->graph))){
		log_err("unable to sort IR node(s)");
		return;
	}

	for (main_cursor = graph_get_head_node(&(ir->graph)); main_cursor != NULL; main_cursor = node_get_next(main_cursor)){
		if (main_cursor->nb_edge_src < 2 && ir_node_get_operation(main_cursor)->type == IR_OPERATION_TYPE_SYMBOL){
			continue;
		}

		for (edge_cursor1 = node_get_head_edge_src(main_cursor), prev_edge_cursor1 = NULL; edge_cursor1 != NULL;){
			node1 = edge_get_dst(edge_cursor1);
			operation1 = ir_node_get_operation(node1);
			init_operand_buffer = 0;

			if (operation1->type != IR_OPERATION_TYPE_INST){
				goto next_cursor1;
			}

			for (edge_cursor2 = edge_get_next_src(edge_cursor1), prev_edge_cursor2 = NULL; edge_cursor2 != NULL;){
				node2 = edge_get_dst(edge_cursor2);
				operation2 = ir_node_get_operation(node2);

				if (operation2->type != IR_OPERATION_TYPE_INST || operation1->size != operation2->size || operation1->operation_type.inst.opcode != operation2->operation_type.inst.opcode || node1 == node2 || ir_edge_get_dependence(edge_cursor1)->type != ir_edge_get_dependence(edge_cursor2)->type){
					goto next_cursor2;
				}

				if (!init_operand_buffer){
					for (operand_cursor = node_get_head_edge_dst(node1), nb_imm_node = 0, nb_edge_node1 = 0; operand_cursor != NULL; operand_cursor = edge_get_next_dst(operand_cursor)){
						if (ir_edge_get_dependence(operand_cursor)->type == IR_DEPENDENCE_TYPE_MACRO){
							continue;
						}

						if (nb_edge_node1 == IR_NORMALIZE_REMOVE_SUBEXPRESSION_MAX_NB_OPERAND){
							log_warn_m("IR_NORMALIZE_REMOVE_SUBEXPRESSION_MAX_NB_OPERAND has been reached: %u for %s", IR_NORMALIZE_REMOVE_SUBEXPRESSION_MAX_NB_OPERAND, irOpcode_2_string(operation1->operation_type.inst.opcode));
							goto next_cursor1;
						}

						operand_buffer[nb_edge_node1 ++].edge1 = operand_cursor;

						if (numeric_simplify[operation1->operation_type.inst.opcode] != NULL && ir_node_get_operation(edge_get_src(operand_cursor))->type == IR_OPERATION_TYPE_IMM){
							nb_imm_node ++;
							if (nb_imm_node > 1){
								goto next_cursor1;
							}
						}
					}
					init_operand_buffer = 1;
				}

				for (operand_cursor = node_get_head_edge_dst(node2), nb_match = 0, nb_imm_node = 0, nb_edge_node2 = 0; operand_cursor != NULL; operand_cursor = edge_get_next_dst(operand_cursor)){
					if (ir_edge_get_dependence(operand_cursor)->type == IR_DEPENDENCE_TYPE_MACRO){
						continue;
					}

					if (numeric_simplify[operation2->operation_type.inst.opcode] != NULL){
						if (ir_node_get_operation(edge_get_src(operand_cursor))->type == IR_OPERATION_TYPE_IMM){
							nb_imm_node ++;
							if (nb_imm_node > 1){
								goto next_cursor2;
							}
						}
					}

					for (i = nb_match, nb_edge_node2 ++; i < nb_edge_node1; i++){
						if (ir_edge_get_dependence(operand_buffer[i].edge1)->type == ir_edge_get_dependence(operand_cursor)->type){
							if (edge_get_src(operand_cursor) == edge_get_src(operand_buffer[i].edge1)){
								commonOperand_swap(operand_buffer + nb_match, operand_buffer + i);
								operand_buffer[nb_match ++].edge2 = operand_cursor;
								break;
							}
							else if (ir_node_get_operation(edge_get_src(operand_cursor))->type == IR_OPERATION_TYPE_IMM && ir_node_get_operation(edge_get_src(operand_buffer[i].edge1))->type == IR_OPERATION_TYPE_IMM && ir_imm_operation_get_unsigned_value(ir_node_get_operation(edge_get_src(operand_cursor))) == ir_imm_operation_get_unsigned_value(ir_node_get_operation(edge_get_src(operand_buffer[i].edge1)))){
								commonOperand_swap(operand_buffer + nb_match, operand_buffer + i);
								operand_buffer[nb_match ++].edge2 = operand_cursor;
								break;
							}
						}
					}
				}

				/* CASE 1 */
				if (nb_match > 0 && nb_match == nb_edge_node1 && nb_match == nb_edge_node2){
					graph_transfert_src_edge(&(ir->graph), node1, node2);
					operation1->index = min(operation2->index, operation1->index);
					ir_remove_node(ir, node2);

					*modification = 1;
					edge_cursor2 = prev_edge_cursor2;
					goto next_cursor2;
				}
				/* CASE 2 */
				else if (nb_match > 1 && nb_match == nb_edge_node1){
					for (i = 0; i < nb_match; i++){
						ir_remove_dependence(ir, operand_buffer[i].edge2);
					}

					if (ir_add_dependence(ir, node1, node2, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
						log_err("unable to add dependency to IR");
					}

					operation2->index = (operation1->index > operation2->index) ? operation2->index : IR_OPERATION_INDEX_UNKOWN;

					*modification = 1;
					edge_cursor2 = prev_edge_cursor2;
					goto next_cursor2;
				}
				/* CASE 3 */
				else if (nb_match > 1 && nb_match == nb_edge_node2){
					for (i = 0; i < nb_match; i++){
						ir_remove_dependence(ir, operand_buffer[i].edge1);
					}

					if (ir_add_dependence(ir, node2, node1, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
						log_err("unable to add dependency to IR");
					}

					operation1->index = (operation2->index > operation1->index) ? operation1->index : IR_OPERATION_INDEX_UNKOWN;

					*modification = 1;
					edge_cursor1 = prev_edge_cursor1;
					goto next_cursor1;
				}
				/* CASE 4 */
				else if (nb_match > 1 && (operation1->operation_type.inst.opcode == IR_ADD || operation1->operation_type.inst.opcode == IR_XOR)){
					new_intermediate_inst = ir_add_inst(ir, IR_OPERATION_INDEX_UNKOWN, operation1->size, operation1->operation_type.inst.opcode, (operation1->operation_type.inst.dst == operation2->operation_type.inst.dst) ? operation1->operation_type.inst.dst : IR_OPERATION_DST_UNKOWN);
					if (new_intermediate_inst == NULL){
						log_err("unable to add instruction to IR");
					}
					else{
						for (i = 0; i < nb_match; i++){
							if (ir_add_dependence(ir, edge_get_src(operand_buffer[i].edge1), new_intermediate_inst, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
								log_err("unable to add dependency to IR");
							}

							ir_remove_dependence(ir, operand_buffer[i].edge2);
							ir_remove_dependence(ir, operand_buffer[i].edge1);
						}

						if (ir_add_dependence(ir, new_intermediate_inst, node1, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
							log_err("unable to add dependency to IR");
						}

						if (ir_add_dependence(ir, new_intermediate_inst, node2, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
							log_err("unable to add dependency to IR");
						}

						operation1->index = IR_OPERATION_INDEX_UNKOWN;
						operation2->index = IR_OPERATION_INDEX_UNKOWN;

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

							new_node = ir_add_inst(ir, IR_OPERATION_INDEX_UNKOWN, operation_cursor1->size, operation_cursor1->operation_type.inst.opcode, IR_OPERATION_DST_UNKOWN);
							if (new_node == NULL){
								log_err("unable to add inst node to IR");
							}
							else{
								if (ir_add_dependence(ir, edge_get_src(node1_imm_operand), new_node, ir_edge_get_dependence(node1_imm_operand)->type) == NULL){
									log_err("unable to add dependency to IR");
								}

								if (ir_add_dependence(ir, node_cursor3, new_node, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
									log_err("unable to add dependency to IR");
								}

								if (ir_add_dependence(ir, new_node, node_cursor1, ir_edge_get_dependence(edge_cursor)->type) == NULL){
									log_err("unable to add dependency to IR");
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
							log_warn("multiple scenario for factoring instruction (1/2)");
							goto next;
						}
					}
				}
			}

			if (opcode != IR_INVALID){
				if (opcode_counter[opcode] >= IR_NORMALIZE_FACTOR_MAX_NB_OPERAND){
					log_warn("IR_NORMALIZE_FACTOR_MAX_NB_OPERAND has been reached");
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
									log_warn("IR_NORMALIZE_FACTOR_MAX_NB_OPERAND has been reached");
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
							log_warn("multiple scenario for factoring instruction (2/2)");
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

					new_node1 = ir_add_inst(ir, IR_OPERATION_INDEX_UNKOWN, operation_cursor1->size, operation_cursor1->operation_type.inst.opcode, IR_OPERATION_DST_UNKOWN);
					if (new_node1 == NULL){
						log_err("unable to add inst to IR");
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
									log_err("unable to add dependency to IR");
								}
								ir_remove_dependence(ir, op2);
							}
							else if (edge_get_src(op2) == common_operand){
								common_operand_dependence_type = ir_edge_get_dependence(op1)->type;
								if (ir_add_dependence(ir, edge_get_src(op1), new_node1, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
									log_err("unable to add dependency to IR");
								}
								ir_remove_dependence(ir, op1);
							}
						}
					}

					if (new_node2 == NULL){
						new_node2 = ir_add_inst(ir, IR_OPERATION_INDEX_UNKOWN, operation_cursor1->size, opcode, IR_OPERATION_DST_UNKOWN);
						if (new_node2 == NULL){
							log_err("unable to add instruction to IR");
							goto next;
						}
						else{
							if (ir_add_dependence(ir, new_node2, node_cursor1, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
								log_err("unable to add dependency to IR");
							}
							if (ir_add_dependence(ir, common_operand, new_node2, common_operand_dependence_type) == NULL){
								log_err("unable to add dependency to IR");
							}
						}
					}

					if (ir_add_dependence(ir, new_node1, new_node2, IR_DEPENDENCE_TYPE_DIRECT) == NULL){
						log_err("unable to add dependency to IR");
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

/* ===================================================================== */
/* Sorting routines													     */
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