#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "ir.h"
#include "irImporterDynTrace.h"
#include "irImporterAsm.h"
#include "array.h"
#include "multiColumn.h"

void ir_dotPrint_node(void* data, FILE* file, void* arg);
void ir_dotPrint_edge(void* data, FILE* file, void* arg);

struct ir* ir_create(struct trace* trace, enum irCreateMethod create_method){
	struct ir* ir;

	ir =(struct ir*)malloc(sizeof(struct ir));
	if (ir != NULL){
		if(ir_init(ir, trace, create_method)){
			printf("ERROR: in %s, unable to init ir\n", __func__);
			free(ir);
			ir = NULL;
		}
	}
	else{
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
	}

	return ir;
}

int32_t ir_init(struct ir* ir, struct trace* trace, enum irCreateMethod create_method){
	ir->trace 				= trace;
	graph_init(&(ir->graph), sizeof(struct irOperation), sizeof(struct irDependence))
	graph_register_dotPrint_callback(&(ir->graph), NULL, ir_dotPrint_node, ir_dotPrint_edge, NULL)
	ir->input_linkedList 	= NULL;
	ir->output_linkedList 	= NULL;

	switch(create_method){
		case IR_CREATE_TRACE : {
			if (irImporterDynTrace_import(ir)){
				printf("ERROR: in %s, trace dynTrace import has failed\n", __func__);
				return -1;
			}
			break;
		}
		case IR_CREATE_ASM : {
			if (irImporterAsm_import(ir)){
				printf("ERROR: in %s, trace asm import has failed\n", __func__);
				return -1;
			}
			break;
		}
	}
	
	return 0;
}

struct node* ir_add_input(struct ir* ir, struct operand* operand){
	struct node* 			node;
	struct irOperation* 	operation;

	node = graph_add_node_(&(ir->graph));
	if (node == NULL){
		printf("ERROR: in %s, unable to add node to the graph\n", __func__);
	}
	else{
		operation = ir_node_get_operation(node);
		operation->type 							= IR_OPERATION_TYPE_INPUT;
		operation->operation_type.input.operand 	= operand;
		operation->operation_type.input.prev 		= NULL;
		operation->operation_type.input.next 		= ir->input_linkedList;

		if (ir->input_linkedList != NULL){
			ir_node_get_operation(ir->input_linkedList)->operation_type.input.prev = node;
		}

		ir->input_linkedList = node;
	}

	return node;
}

struct node* ir_add_output(struct ir* ir, enum irOpcode opcode, struct operand* operand){
	struct node* 			node;
	struct irOperation* 	operation;

	node = graph_add_node_(&(ir->graph));
	if (node == NULL){
		printf("ERROR: in %s, unable to add node to the graph\n", __func__);
	}
	else{
		operation = ir_node_get_operation(node);
		operation->type 							= IR_OPERATION_TYPE_OUTPUT;
		operation->operation_type.output.opcode 	= opcode;
		operation->operation_type.output.operand 	= operand;
		operation->operation_type.output.prev 		= NULL;
		operation->operation_type.output.next 		= ir->output_linkedList;

		if (ir->output_linkedList != NULL){
			ir_node_get_operation(ir->output_linkedList)->operation_type.output.prev = node;
		}

		ir->output_linkedList = node;
	}

	return node;
}

struct node* ir_add_immediate(struct ir* ir, uint16_t width, uint8_t signe, uint64_t value){
	struct node* 			node;
	struct irOperation* 	operation;

	node = graph_add_node_(&(ir->graph));
	if (node == NULL){
		printf("ERROR: in %s, unable to add node to the graph\n", __func__);
	}
	else{
		operation = ir_node_get_operation(node);
		operation->type 						= IR_OPERATION_TYPE_IMM;
		operation->operation_type.imm.width 	= width;
		operation->operation_type.imm.signe 	= signe;
		operation->operation_type.imm.value 	= value;
	}

	return node;
}

struct edge* ir_add_dependence(struct ir* ir, struct node* operation_src, struct node* operation_dst, enum irDependenceType type){
	struct edge* 			edge;
	struct irDependence* 	dependence;

	edge = graph_add_edge_(&(ir->graph), operation_src, operation_dst);
	if (edge == NULL){
		printf("ERROR: in %s, unable to add edge to the graph\n", __func__);
	}
	else{
		dependence = ir_edge_get_dependence(edge);
		dependence->type = type;
	}

	return edge;
}

void ir_remove_node(struct ir* ir, struct node* node){
	struct irOperation* operation;
	struct edge* 		edge_cursor;
	struct edge* 		edge_current;

	operation = ir_node_get_operation(node);
	switch(operation->type){
		case IR_OPERATION_TYPE_INPUT : {
			if (operation->operation_type.input.prev != NULL){
				ir_node_get_operation(operation->operation_type.input.prev)->operation_type.input.next = operation->operation_type.input.next;
			}
			else{
				ir->input_linkedList = operation->operation_type.input.next;
			}
			if (operation->operation_type.input.next != NULL){
				ir_node_get_operation(operation->operation_type.input.next)->operation_type.input.prev = operation->operation_type.input.prev;
			}
			break;
		}
		case IR_OPERATION_TYPE_OUTPUT : {
			if (operation->operation_type.output.prev != NULL){
				ir_node_get_operation(operation->operation_type.output.prev)->operation_type.output.next = operation->operation_type.output.next;
			}
			else{
				ir->output_linkedList = operation->operation_type.output.next;
			}
			if (operation->operation_type.output.next != NULL){
				ir_node_get_operation(operation->operation_type.output.next)->operation_type.output.prev = operation->operation_type.output.prev;
			}
			break;
		}
		default : {
			break;
		}
	}

	for (edge_cursor = node_get_head_edge_dst(node); edge_cursor != NULL; ){
		edge_current = edge_cursor;
		edge_cursor = edge_get_next_dst(edge_cursor);

		if (ir_node_get_operation(edge_get_src(edge_current))->type == IR_OPERATION_TYPE_IMM){
			graph_remove_node(&(ir->graph), edge_get_src(edge_current));
		}
	}

	graph_remove_node(&(ir->graph), node);
}

void ir_convert_output_to_inner(struct ir* ir, struct node* node){
	enum irOpcode 			opcode;
	struct irOperation* 	operation;

	operation = ir_node_get_operation(node);
	if (operation->type == IR_OPERATION_TYPE_OUTPUT){
		opcode = operation->operation_type.output.opcode;
		operation->type = IR_OPERATION_TYPE_INNER;
		operation->operation_type.inner.opcode = opcode;

		if (operation->operation_type.output.prev == NULL){
			ir->output_linkedList = operation->operation_type.output.next;
		}
		else{
			ir_node_get_operation(operation->operation_type.output.prev)->operation_type.output.next = operation->operation_type.output.next;
		}

		if (operation->operation_type.output.next != NULL){
			ir_node_get_operation(operation->operation_type.output.next)->operation_type.output.prev = operation->operation_type.output.prev;
		}
	}
}

void ir_convert_input_to_inner(struct ir* ir, struct node* node, enum irOpcode opcode){
	struct irOperation* operation;

	operation = ir_node_get_operation(node);
	if (operation->type == IR_OPERATION_TYPE_INPUT){
		operation->type = IR_OPERATION_TYPE_INNER;
		operation->operation_type.inner.opcode = opcode;

		if (operation->operation_type.input.prev == NULL){
			ir->output_linkedList = operation->operation_type.input.next;
		}
		else{
			ir_node_get_operation(operation->operation_type.input.prev)->operation_type.input.next = operation->operation_type.input.next;
		}

		if (operation->operation_type.input.next != NULL){
			ir_node_get_operation(operation->operation_type.input.next)->operation_type.input.prev = operation->operation_type.input.prev;
		}
	}
}

/* ===================================================================== */
/* Normalize functions						                             */
/* ===================================================================== */

#define IR_NORMALIZE_TRANSLATE_ROL_IMM 		1	/* IR must be obtained either by TRACE or ASM */
#define IR_NORMALIZE_TRANSLATE_SUB_IMM 		1 	/* IR must be obtained either by TRACE or ASM */
#define IR_NORMALIZE_REPLACE_XOR_FF 		1 	/* IR must be obtained by ASM */
#define IR_NORMALIZE_MERGE_TRANSITIVE_ADD 	1 	/* IR must be obtained either by TRACE or ASM */
#define IR_NORMALIZE_PROPAGATE_EXPRESSION 	1 	/* IR must be obtained either by TRACE or ASM */

void ir_normalize(struct ir* ir){
	#if IR_NORMALIZE_TRANSLATE_ROL_IMM == 1
	ir_normalize_translate_rol_imm(ir);
	#endif
	#if IR_NORMALIZE_TRANSLATE_SUB_IMM == 1
	ir_normalize_translate_sub_imm(ir);
	#endif
	#if IR_NORMALIZE_REPLACE_XOR_FF == 1
	ir_normalize_replace_xor_ff(ir);
	#endif
	#if IR_NORMALIZE_MERGE_TRANSITIVE_ADD == 1
	ir_normalize_merge_transitive_add(ir);
	#endif
	#if IR_NORMALIZE_PROPAGATE_EXPRESSION == 1
	ir_normalize_propagate_expression(ir);
	#endif
}

/* WARNING this routine might be incorrect (see below) */
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
				/* WARNING this is not perfect since we don't know the operand size */
				edge_cursor = node_get_head_edge_dst(node_cursor);
				while(edge_cursor != NULL && ir_node_get_operation(edge_get_src(edge_cursor))->type != IR_OPERATION_TYPE_IMM){
					edge_cursor = edge_get_next_dst(edge_cursor);
				}
				if (edge_cursor != NULL){
					imm_value = ir_node_get_operation(edge_get_src(edge_cursor));
					imm_value->operation_type.imm.value = 32 - imm_value->operation_type.imm.value;
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

/* WARNING this routine might be incorrect (see below) */
void ir_normalize_replace_xor_ff(struct ir* ir){
	struct node* 			node_cursor;
	struct edge* 			edge_cursor;
	struct irOperation* 	operation;
	enum irOpcode* 			opcode_ptr;

	for(node_cursor = graph_get_head_node(&(ir->graph)); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
		if (node_cursor->nb_edge_dst == 2){
			operation = ir_node_get_operation(node_cursor);
			if (operation->type == IR_OPERATION_TYPE_OUTPUT){
				if (operation->operation_type.output.opcode != IR_XOR){
					continue;
				}
				else{
					opcode_ptr = &(operation->operation_type.output.opcode);
				}
			}
			else if (operation->type == IR_OPERATION_TYPE_INNER){
				if (operation->operation_type.inner.opcode != IR_XOR){
					continue;
				}
				else{
					opcode_ptr = &(operation->operation_type.inner.opcode);
				}
			}
			else{
				continue;
			}

			for(edge_cursor = node_get_head_edge_dst(node_cursor); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
				operation = ir_node_get_operation(edge_get_src(edge_cursor));
				if (operation->type == IR_OPERATION_TYPE_IMM){
					/* This is not correct the size of the XOR must be known (signed unsigned stuff ?) */
					#pragma GCC diagnostic ignored "-Wlong-long" /* use of C99 long long integer constant */
					if (ir_imm_operation_get_unsigned_value(operation) == 0xffffffff){
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

void ir_normalize_merge_transitive_add(struct ir* ir){
	struct node* 			node_cursor1;
	struct node* 			node_cursor2;
	struct edge* 			edge_cursor2;
	struct irOperation* 	operation;

	for(node_cursor1 = graph_get_head_node(&(ir->graph)); node_cursor1 != NULL; node_cursor1 = node_get_next(node_cursor1)){
		operation = ir_node_get_operation(node_cursor1);
		if (operation->type == IR_OPERATION_TYPE_OUTPUT){
			if (operation->operation_type.output.opcode != IR_ADD){
				continue;
			}
		}
		else if (operation->type == IR_OPERATION_TYPE_INNER){
			if (operation->operation_type.inner.opcode != IR_ADD){
				continue;
			}
		}
		else{
			continue;
		}

		start:
		for(edge_cursor2 = node_get_head_edge_dst(node_cursor1); edge_cursor2 != NULL; edge_cursor2 = edge_get_next_dst(edge_cursor2)){
			node_cursor2  = edge_get_src(edge_cursor2);
			if (node_cursor2->nb_edge_src == 1){
				operation = ir_node_get_operation(node_cursor2);
				if (operation->type == IR_OPERATION_TYPE_OUTPUT){
					if (operation->operation_type.output.opcode != IR_ADD){
						continue;
					}
					else{
						if (operation->operation_type.output.prev == NULL){
							ir->output_linkedList = operation->operation_type.output.next;
						}
						else{
							ir_node_get_operation(operation->operation_type.output.prev)->operation_type.output.next = operation->operation_type.output.next;
						}

						if (operation->operation_type.output.next != NULL){
							ir_node_get_operation(operation->operation_type.output.next)->operation_type.output.prev = operation->operation_type.output.prev;
						}
					}
				}
				else if (operation->type == IR_OPERATION_TYPE_INNER){
					if (operation->operation_type.inner.opcode != IR_ADD){
						continue;
					}
				}
				else{
					continue;
				}

				graph_merge_node(&(ir->graph), node_cursor1, node_cursor2);
				ir_remove_node(ir, node_cursor2);
				goto start;
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

					if (*opcode_ptr1 == *opcode_ptr2 && node_cursor1->nb_edge_dst == node_cursor2->nb_edge_dst){
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

										if (operation11->operation_type.imm.width == operation22->operation_type.imm.width && operation11->operation_type.imm.signe == operation22->operation_type.imm.signe && operation11->operation_type.imm.value == operation22->operation_type.imm.value){
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
							return;
						}
					}
				}
			}
		}
	}
}

/* ===================================================================== */
/* Printing functions						                             */
/* ===================================================================== */

#pragma GCC diagnostic ignored "-Wunused-parameter"
void ir_dotPrint_node(void* data, FILE* file, void* arg){
	struct irOperation* operation = (struct irOperation*)data;

	switch(operation->type){
		case IR_OPERATION_TYPE_INPUT 		: {
			if (operation->operation_type.input.operand != NULL){
				if (OPERAND_IS_MEM(*(operation->operation_type.input.operand))){
					#if defined ARCH_32
					fprintf(file, "[shape=\"box\",label=\"@%08x\"]", operation->operation_type.input.operand->location.address);
					#elif defined ARCH_64
					#pragma GCC diagnostic ignored "-Wformat" /* ISO C90 does not support the ‘ll’ gnu_printf length modifier */
					fprintf(file, "[shape=\"box\",label=\"@%llx\"]", operation->operation_type.input.operand->location.address);
					#else
					#error Please specify an architecture {ARCH_32 or ARCH_64}
					#endif
				}
				else if (OPERAND_IS_REG(*(operation->operation_type.input.operand))){
					fprintf(file, "[shape=\"box\",label=\"%s\"]", reg_2_string(operation->operation_type.input.operand->location.reg));
				}
				else{
					printf("ERROR: in %s, unexpected data type (REG or MEM)\n", __func__);
					break;
				}
			}
			else{
				fprintf(file, "[shape=\"box\",label=\"NULL\"]");
			}
			break;
		}
		case IR_OPERATION_TYPE_OUTPUT 		: {
			fprintf(file, "[shape=\"invhouse\",label=\"%s\"]", irOpcode_2_string(operation->operation_type.output.opcode));
			break;
		}
		case IR_OPERATION_TYPE_INNER 		: {
			fprintf(file, "[label=\"%s\"]", irOpcode_2_string(operation->operation_type.inner.opcode));
			break;
		}
		case IR_OPERATION_TYPE_IMM 			: {
			if (operation->operation_type.imm.signe){
				#pragma GCC diagnostic ignored "-Wformat" /* ISO C90 does not support the ‘ll’ gnu_printf length modifier */
				#pragma GCC diagnostic ignored "-Wlong-long" /* use of C99 long long integer constant */
				fprintf(file, "[shape=\"diamond\",label=\"0x%x\"]", ir_imm_operation_get_signed_value(operation));
			}
			else{
				fprintf(file, "[shape=\"diamond\",label=\"0x%llx\"]", ir_imm_operation_get_unsigned_value(operation));
			}
			break;
		}
	}
}

#pragma GCC diagnostic ignored "-Wunused-parameter"
void ir_dotPrint_edge(void* data, FILE* file, void* arg){
	struct irDependence* dependence = (struct irDependence*)data;

	switch(dependence->type){
		case IR_DEPENDENCE_TYPE_DIRECT 	: {
			break;
		}
		case IR_DEPENDENCE_TYPE_BASE 	: {
			fprintf(file, "[label=\"base\"]");
			break;
		}
		case IR_DEPENDENCE_TYPE_INDEX 	: {
			fprintf(file, "[label=\"index\"]");
			break;
		}
		case IR_DEPENDENCE_TYPE_DISP 	: {
			fprintf(file, "[label=\"disp\"]");
			break;
		}
	}
}

char* irOpcode_2_string(enum irOpcode opcode){
	switch(opcode){
		case IR_ADD 	: {return "add";}
		case IR_AND 	: {return "and";}
		case IR_BSWAP 	: {return "bswap";}
		case IR_DEC 	: {return "dec";}
		case IR_MOVZX 	: {return "movzx";}
		case IR_NOT 	: {return "not";}
		case IR_OR 		: {return "or";}
		case IR_PART 	: {return "part";}
		case IR_ROL 	: {return "rol";}
		case IR_ROR 	: {return "ror";}
		case IR_SAR 	: {return "sar";}
		case IR_SHL 	: {return "shl";}
		case IR_SHR 	: {return "shr";}
		case IR_SUB 	: {return "sub";}
		case IR_XOR 	: {return "xor";}
	}

	return NULL;
}

void ir_print_io(struct ir* ir){
	struct node* 				node_cursor;
	struct irOperation* 		operation_cursor;
	struct multiColumnPrinter* 	printer;
	char 						value_str[20];
	char 						desc_str[20];

	printer = multiColumnPrinter_create(stdout, 3, NULL, NULL, NULL);
	if (printer != NULL){
		multiColumnPrinter_set_column_type(printer, 2, MULTICOLUMN_TYPE_UINT32);

		multiColumnPrinter_set_title(printer, 0, (char*)"VALUE");
		multiColumnPrinter_set_title(printer, 1, (char*)"DESC");
		multiColumnPrinter_set_title(printer, 2, (char*)"SIZE");

		printf("*** INPUT ***\n");
		multiColumnPrinter_print_header(printer);
		node_cursor = ir->input_linkedList;
		while(node_cursor != NULL){
			operation_cursor = ir_node_get_operation(node_cursor);

			if (operation_cursor->operation_type.input.operand != NULL){
				switch(operation_cursor->operation_type.input.operand->size){
				case 1 	: {snprintf(value_str, 20, "%02x", *(uint32_t*)(ir->trace->data + operation_cursor->operation_type.input.operand->data_offset) & 0x000000ff); break;}
				case 2 	: {snprintf(value_str, 20, "%04x", *(uint32_t*)(ir->trace->data + operation_cursor->operation_type.input.operand->data_offset) & 0x0000ffff); break;}
				case 4 	: {snprintf(value_str, 20, "%08x", *(uint32_t*)(ir->trace->data + operation_cursor->operation_type.input.operand->data_offset) & 0xffffffff); break;}
				default : {printf("WARNING: in %s, unexpected data size\n", __func__); break;}
				}

				if (OPERAND_IS_MEM(*(operation_cursor->operation_type.input.operand))){
					#if defined ARCH_32
					snprintf(desc_str, 20, "0x%08x", operation_cursor->operation_type.input.operand->location.address);
					#elif defined ARCH_64
					#pragma GCC diagnostic ignored "-Wformat" /* ISO C90 does not support the ‘ll’ gnu_printf length modifier */
					snprintf(desc_str, 20, "0x%llx", operation_cursor->operation_type.input.operand->location.address);
					#else
					#error Please specify an architecture {ARCH_32 or ARCH_64}
					#endif
				}
				else if (OPERAND_IS_REG(*(operation_cursor->operation_type.input.operand))){
					snprintf(desc_str, 20, "%s", reg_2_string(operation_cursor->operation_type.input.operand->location.reg));
				}
				else{
					printf("WARNING: in %s, unexpected operand type\n", __func__);
				}

				multiColumnPrinter_print(printer, value_str, desc_str, operation_cursor->operation_type.input.operand->size, NULL);
			}

			node_cursor = operation_cursor->operation_type.input.next;
		}

		printf("\n*** OUTPUT ***\n");
		multiColumnPrinter_print_header(printer);
		node_cursor = ir->output_linkedList;
		while(node_cursor != NULL){
			operation_cursor = ir_node_get_operation(node_cursor);

			if (operation_cursor->operation_type.input.operand != NULL){
				switch(operation_cursor->operation_type.output.operand->size){
				case 1 	: {snprintf(value_str, 20, "%02x", *(uint32_t*)(ir->trace->data + operation_cursor->operation_type.output.operand->data_offset) & 0x000000ff); break;}
				case 2 	: {snprintf(value_str, 20, "%04x", *(uint32_t*)(ir->trace->data + operation_cursor->operation_type.output.operand->data_offset) & 0x0000ffff); break;}
				case 4 	: {snprintf(value_str, 20, "%08x", *(uint32_t*)(ir->trace->data + operation_cursor->operation_type.output.operand->data_offset) & 0xffffffff); break;}
				default : {printf("WARNING: in %s, unexpected data size\n", __func__); break;}
				}

				if (OPERAND_IS_MEM(*(operation_cursor->operation_type.output.operand))){
					#if defined ARCH_32
					snprintf(desc_str, 20, "0x%08x", operation_cursor->operation_type.output.operand->location.address);
					#elif defined ARCH_64
					#pragma GCC diagnostic ignored "-Wformat" /* ISO C90 does not support the ‘ll’ gnu_printf length modifier */
					snprintf(desc_str, 20, "0x%llx", operation_cursor->operation_type.output.operand->location.address);
					#else
					#error Please specify an architecture {ARCH_32 or ARCH_64}
					#endif
				}
				else if (OPERAND_IS_REG(*(operation_cursor->operation_type.output.operand))){
					snprintf(desc_str, 20, "%s", reg_2_string(operation_cursor->operation_type.output.operand->location.reg));
				}
				else{
					printf("WARNING: in %s, unexpected operand type\n", __func__);
				}

				multiColumnPrinter_print(printer, value_str, desc_str, operation_cursor->operation_type.output.operand->size, NULL);
			}
			
			node_cursor = operation_cursor->operation_type.output.next;
		}

		multiColumnPrinter_delete(printer);
	}
	else{
		printf("ERROR: in %s, unable to create multi column printer\n", __func__);
	}
}