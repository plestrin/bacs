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

#define IR_NORMALIZE_TRANSLATE_ROL_IMM 		1	/* IR must be obtained either by TRACE or ASM */
#define IR_NORMALIZE_TRANSLATE_SUB_IMM 		1 	/* IR must be obtained either by TRACE or ASM */
#define IR_NORMALIZE_TRANSLATE_XOR_FF 		1 	/* IR must be obtained by ASM */
#define IR_NORMALIZE_MERGE_TRANSITIVE_ADD 	1 	/* IR must be obtained either by TRACE or ASM */
#define IR_NORMALIZE_MERGE_TRANSITIVE_XOR 	1 	/* IR must be obtained either by TRACE or ASM */
#define IR_NORMALIZE_PROPAGATE_EXPRESSION 	1 	/* IR must be obtained either by TRACE or ASM */
#define IR_NORMALIZE_DETECT_ROTATION 		1 	/* IR must be obtained by ASM */

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

#endif

void ir_normalize(struct ir* ir){
	INIT_TIMER

	#if IR_NORMALIZE_TRANSLATE_ROL_IMM == 1
	START_TIMER
	ir_normalize_translate_rol_imm(ir);
	STOP_TIMER
	PRINT_TIMER("Translate rol imm")
	#endif

	#if IR_NORMALIZE_TRANSLATE_SUB_IMM == 1
	START_TIMER
	ir_normalize_translate_sub_imm(ir);
	STOP_TIMER
	PRINT_TIMER("Translate sub imm")
	#endif

	#if IR_NORMALIZE_TRANSLATE_XOR_FF == 1
	START_TIMER
	ir_normalize_translate_xor_ff(ir);
	STOP_TIMER
	PRINT_TIMER("Translate xor ff")
	#endif

	#if IR_NORMALIZE_MERGE_TRANSITIVE_ADD == 1
	START_TIMER
	ir_normalize_merge_transitive_operation(ir, IR_ADD);
	STOP_TIMER
	PRINT_TIMER("Merge transitive add")
	#endif
	
	#if IR_NORMALIZE_MERGE_TRANSITIVE_XOR == 1
	START_TIMER
	ir_normalize_merge_transitive_operation(ir, IR_XOR);
	STOP_TIMER
	PRINT_TIMER("Merge transitive xor")
	#endif
	
	#if IR_NORMALIZE_PROPAGATE_EXPRESSION == 1
	START_TIMER
	ir_normalize_propagate_expression(ir);
	STOP_TIMER
	PRINT_TIMER("Propagate expression")
	#endif
	
	#if IR_NORMALIZE_DETECT_ROTATION == 1
	START_TIMER
	ir_normalize_detect_rotation(ir);
	STOP_TIMER
	PRINT_TIMER("Detect rotation")
	#endif

	CLEAN_TIMER
}

void ir_normalize_translate_rol_imm(struct ir* ir){
	struct node* 			node_cursor;
	struct edge* 			edge_cursor;
	struct irOperation* 	operation;
	struct irOperation* 	imm_value;
	
	for(node_cursor = graph_get_head_node(&(ir->graph)); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
		operation = ir_node_get_operation(node_cursor);
		if (operation->type == IR_OPERATION_TYPE_OUTPUT){
			if (operation->operation_type.output.opcode == IR_ROL){
				operation->operation_type.output.opcode = IR_ROR;
			}
			else{
				continue;
			}
		}
		else if (operation->type == IR_OPERATION_TYPE_INNER){
			if (operation->operation_type.inner.opcode == IR_ROL){
				operation->operation_type.inner.opcode = IR_ROR;
			}
			else{
				continue;
			}
		}
		else{
			continue;
		}
		
		switch(node_cursor->nb_edge_dst){
			case 1 : {
				break;
			}
			case 2 : {
				edge_cursor = node_get_head_edge_dst(node_cursor);
				while(edge_cursor != NULL && ir_node_get_operation(edge_get_src(edge_cursor))->type != IR_OPERATION_TYPE_IMM){
					edge_cursor = edge_get_next_dst(edge_cursor);
				}
				if (edge_cursor != NULL){
					imm_value = ir_node_get_operation(edge_get_src(edge_cursor));
					#pragma GCC diagnostic ignored "-Wlong-long" /* use of C99 long long integer constant */
					imm_value->operation_type.imm.value = operation->size - ir_imm_operation_get_unsigned_value(imm_value);
				}
				else{
					printf("WARNING: in %s, this case (ROL with 2 operands but no IMM) is not supposed to happen\n", __func__);
				}
				break;
			}
			default : {
				printf("WARNING: in %s, this case (ROL with %u operand(s)) is not supposed to happen\n", __func__, node_cursor->nb_edge_dst);
				break;
			}
		}
	}
}

/* WARNING this routine might be incorrect (see below) */
void ir_normalize_translate_sub_imm(struct ir* ir){
	struct node* 			node_cursor;
	struct edge* 			edge_cursor;
	struct irOperation* 	operation;
	struct irOperation* 	imm_value;
	enum irOpcode* 			opcode_ptr;
	
	for(node_cursor = graph_get_head_node(&(ir->graph)); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
		operation = ir_node_get_operation(node_cursor);
		if (operation->type == IR_OPERATION_TYPE_OUTPUT){
			if (operation->operation_type.output.opcode == IR_SUB){
				opcode_ptr = &(operation->operation_type.output.opcode);
			}
			else{
				continue;
			}
		}
		else if (operation->type == IR_OPERATION_TYPE_INNER){
			if (operation->operation_type.inner.opcode == IR_SUB){
				opcode_ptr = &(operation->operation_type.inner.opcode);
			}
			else{
				continue;
			}
		}
		else{
			continue;
		}
		
		switch(node_cursor->nb_edge_dst){
			case 1 : {
				*opcode_ptr = IR_ADD;
				break;
			}
			case 2 : {
				edge_cursor = node_get_head_edge_dst(node_cursor);
				while(edge_cursor != NULL && ir_node_get_operation(edge_get_src(edge_cursor))->type != IR_OPERATION_TYPE_IMM){
					edge_cursor = edge_get_next_dst(edge_cursor);
				}
				if (edge_cursor != NULL){
					imm_value = ir_node_get_operation(edge_get_src(edge_cursor));
					imm_value->operation_type.imm.value = (uint64_t)(-imm_value->operation_type.imm.value); /* I am not sure if this is correct */
					*opcode_ptr = IR_SUB;
				}
				break;
			}
			default : {
				printf("WARNING: in %s, this case (SUB with %u operand(s)) is not supposed to happen\n", __func__, node_cursor->nb_edge_dst);
				break;
			}
		}
	}
}

void ir_normalize_translate_xor_ff(struct ir* ir){
	struct node* 			node_cursor;
	struct edge* 			edge_cursor;
	struct irOperation* 	operation;
	struct irOperation* 	imm;
	enum irOpcode* 			opcode_ptr;

	for(node_cursor = graph_get_head_node(&(ir->graph)); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
		if (node_cursor->nb_edge_dst == 2){
			opcode_ptr = NULL;
			operation = ir_node_get_operation(node_cursor);

			if (operation->type == IR_OPERATION_TYPE_OUTPUT){
				if (operation->operation_type.output.opcode == IR_XOR){
					opcode_ptr = &(operation->operation_type.output.opcode);
				}
			}
			else if (operation->type == IR_OPERATION_TYPE_INNER){
				if (operation->operation_type.inner.opcode == IR_XOR){
					opcode_ptr = &(operation->operation_type.inner.opcode);
				}
			}

			if (opcode_ptr != NULL){
				for(edge_cursor = node_get_head_edge_dst(node_cursor); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
					imm = ir_node_get_operation(edge_get_src(edge_cursor));
					if (imm->type == IR_OPERATION_TYPE_IMM){
						#pragma GCC diagnostic ignored "-Wlong-long" /* use of C99 long long integer constant */
						if (ir_imm_operation_get_unsigned_value(imm) == (0xffffffffffffffffULL >> (64 - operation->size))){
							if (edge_get_src(edge_cursor)->nb_edge_src == 1){
								ir_remove_node(ir, edge_get_src(edge_cursor));
							}
							*opcode_ptr = IR_NOT;
							break;
						}
					}
				}
			}
		}
	}
}

void ir_normalize_merge_transitive_operation(struct ir* ir, enum irOpcode opcode){
	struct node* 			node_cursor;
	struct edge* 			edge_cursor;
	struct node* 			merge_node;
	struct irOperation* 	operation;

	for(node_cursor = graph_get_head_node(&(ir->graph)); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
		operation = ir_node_get_operation(node_cursor);
		if (operation->type == IR_OPERATION_TYPE_OUTPUT){
			if (operation->operation_type.output.opcode != opcode){
				continue;
			}
		}
		else if (operation->type == IR_OPERATION_TYPE_INNER){
			if (operation->operation_type.inner.opcode != opcode){
				continue;
			}
		}
		else{
			continue;
		}

		start:
		for(edge_cursor = node_get_head_edge_dst(node_cursor); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
			merge_node = edge_get_src(edge_cursor);
			if (merge_node->nb_edge_src == 1){
				operation = ir_node_get_operation(merge_node);
				if (operation->type == IR_OPERATION_TYPE_OUTPUT){
					if (operation->operation_type.output.opcode == opcode){
						graph_merge_node(&(ir->graph), node_cursor, merge_node);
						ir_remove_node(ir, merge_node);
						goto start;
					}
				}
				else if (operation->type == IR_OPERATION_TYPE_INNER){
					if (operation->operation_type.inner.opcode == opcode){
						graph_merge_node(&(ir->graph), node_cursor, merge_node);
						ir_remove_node(ir, merge_node);
						goto start;
					}
				}
			}
		}
	}
}

void ir_normalize_propagate_expression(struct ir* ir){
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
	enum irOpcode* 			opcode_ptr1;
	enum irOpcode* 			opcode_ptr2;

	for(node_cursor1 = graph_get_head_node(&(ir->graph)); node_cursor1 != NULL; node_cursor1 = node_get_next(node_cursor1)){
		if (node_cursor1->nb_edge_dst > max_nb_edge_dst){
			max_nb_edge_dst = node_cursor1->nb_edge_dst;
		}
	}

	if (max_nb_edge_dst == 0){
		return;
	}

	taken = (uint8_t*)alloca(sizeof(uint8_t) * max_nb_edge_dst);

	start:
	for(node_cursor1 = graph_get_head_node(&(ir->graph)); node_cursor1 != NULL; node_cursor1 = node_get_next(node_cursor1)){
		operation1 = ir_node_get_operation(node_cursor1);
		if (operation1->type == IR_OPERATION_TYPE_OUTPUT || operation1->type == IR_OPERATION_TYPE_INNER){

			if (operation1->type == IR_OPERATION_TYPE_OUTPUT){
				opcode_ptr1 = &(operation1->operation_type.output.opcode);
			}
			else{
				opcode_ptr1 = &(operation1->operation_type.inner.opcode);
			}

			for(node_cursor2 = node_get_next(node_cursor1); node_cursor2 != NULL; node_cursor2 = node_get_next(node_cursor2)){
				operation2 = ir_node_get_operation(node_cursor2);
				if (operation2->type == IR_OPERATION_TYPE_OUTPUT || operation2->type == IR_OPERATION_TYPE_INNER){

					if (operation2->type == IR_OPERATION_TYPE_OUTPUT){
						opcode_ptr2 = &(operation2->operation_type.output.opcode);
					}
					else{
						opcode_ptr2 = &(operation2->operation_type.inner.opcode);
					}

					if (*opcode_ptr1 == *opcode_ptr2 && node_cursor1->nb_edge_dst == node_cursor2->nb_edge_dst && operation1->size == operation2->size){
						memset(taken, 0, sizeof(uint8_t) * node_cursor1->nb_edge_dst);

						for (edge_cursor1 = node_get_head_edge_dst(node_cursor1); edge_cursor1 != NULL; edge_cursor1 = edge_get_next_dst(edge_cursor1)){
							for (edge_cursor2 = node_get_head_edge_dst(node_cursor2), i = 0; edge_cursor2 != NULL; edge_cursor2 = edge_get_next_dst(edge_cursor2), i++){
								if (!taken[i]){
									if (edge_get_src(edge_cursor1) == edge_get_src(edge_cursor2)){
										taken[i] = 1;
										break;
									}
									else if (ir_node_get_operation(edge_get_src(edge_cursor1))->type == IR_OPERATION_TYPE_IMM && ir_node_get_operation(edge_get_src(edge_cursor2))->type == IR_OPERATION_TYPE_IMM){
										struct irOperation* operation11;
										struct irOperation* operation22;

										operation11 = ir_node_get_operation(edge_get_src(edge_cursor1));
										operation22 = ir_node_get_operation(edge_get_src(edge_cursor2));

										if (operation11->size == operation22->size && operation11->operation_type.imm.signe == operation22->operation_type.imm.signe && operation11->operation_type.imm.value == operation22->operation_type.imm.value){
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
							if (operation1->type == IR_OPERATION_TYPE_OUTPUT){
								graph_transfert_src_edge(&(ir->graph), node_cursor1, node_cursor2);
								ir_remove_node(ir, node_cursor2);
							}
							else{
								graph_transfert_src_edge(&(ir->graph), node_cursor2, node_cursor1);
								ir_remove_node(ir, node_cursor1);
							}
							goto start;
						}
					}
				}
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
		if (node_cursor->nb_edge_dst == 2){
			operation = ir_node_get_operation(node_cursor);
			if (operation->type == IR_OPERATION_TYPE_OUTPUT){
				if (operation->operation_type.output.opcode != IR_OR){
					continue;
				}
				else{
					opcode_ptr = &(operation->operation_type.output.opcode);
				}
			}
			else if (operation->type == IR_OPERATION_TYPE_INNER){
				if (operation->operation_type.inner.opcode != IR_OR){
					continue;
				}
				else{
					opcode_ptr = &(operation->operation_type.inner.opcode);
				}
			}
			else{
				continue;
			}

			or_dep1 	= edge_get_src(node_get_head_edge_dst(node_cursor));
			or_dep2 	= edge_get_src(edge_get_next_dst(node_get_head_edge_dst(node_cursor)));
			node_shiftl = NULL;
			node_shiftr = NULL;

			operation = ir_node_get_operation(or_dep1);
			if (operation->type == IR_OPERATION_TYPE_OUTPUT){
				if (operation->operation_type.output.opcode == IR_SHR){
					node_shiftr = or_dep1;
				}
				else if (operation->operation_type.output.opcode == IR_SHL){
					node_shiftl = or_dep1;
				}
			}
			else if (operation->type == IR_OPERATION_TYPE_INNER){
				if (operation->operation_type.inner.opcode == IR_SHR){
					node_shiftr = or_dep1;
				}
				else if (operation->operation_type.inner.opcode == IR_SHL){
					node_shiftl = or_dep1;
				}
			}

			operation = ir_node_get_operation(or_dep2);
			if (operation->type == IR_OPERATION_TYPE_OUTPUT){
				if (operation->operation_type.output.opcode == IR_SHR){
					node_shiftr = or_dep2;
				}
				else if (operation->operation_type.output.opcode == IR_SHL){
					node_shiftl = or_dep2;
				}
			}
			else if (operation->type == IR_OPERATION_TYPE_INNER){
				if (operation->operation_type.inner.opcode == IR_SHR){
					node_shiftr = or_dep2;
				}
				else if (operation->operation_type.inner.opcode == IR_SHL){
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