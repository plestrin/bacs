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

struct node* ir_add_input(struct ir* ir, struct operand* operand, uint8_t size){
	struct node* 			node;
	struct irOperation* 	operation;

	node = graph_add_node_(&(ir->graph));
	if (node == NULL){
		printf("ERROR: in %s, unable to add node to the graph\n", __func__);
	}
	else{
		operation = ir_node_get_operation(node);
		operation->type 							= IR_OPERATION_TYPE_INPUT;
		operation->size 							= size;
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

struct node* ir_add_output(struct ir* ir, enum irOpcode opcode, struct operand* operand, uint8_t size){
	struct node* 			node;
	struct irOperation* 	operation;

	node = graph_add_node_(&(ir->graph));
	if (node == NULL){
		printf("ERROR: in %s, unable to add node to the graph\n", __func__);
	}
	else{
		operation = ir_node_get_operation(node);
		operation->type 							= IR_OPERATION_TYPE_OUTPUT;
		operation->size 							= size;
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

struct node* ir_add_immediate(struct ir* ir, uint8_t size, uint8_t signe, uint64_t value){
	struct node* 			node;
	struct irOperation* 	operation;

	node = graph_add_node_(&(ir->graph));
	if (node == NULL){
		printf("ERROR: in %s, unable to add node to the graph\n", __func__);
	}
	else{
		operation = ir_node_get_operation(node);
		operation->type 						= IR_OPERATION_TYPE_IMM;
		operation->size 						= size;
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

		if (ir_node_get_operation(edge_get_src(edge_current))->type == IR_OPERATION_TYPE_IMM && edge_get_src(edge_current)->nb_edge_src == 1){
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