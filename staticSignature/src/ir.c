#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "ir.h"
#include "irImporterAsm.h"
#include "array.h"
#include "multiColumn.h"

void ir_dotPrint_node(void* data, FILE* file, void* arg);
void ir_dotPrint_edge(void* data, FILE* file, void* arg);


int32_t irOperation_equal(const struct irOperation* op1, const  struct irOperation* op2){
	if (op1->type == op2->type && op1->size == op2->size){
		switch(op1->type){
			case IR_OPERATION_TYPE_IN_REG 	: {
				return (op1->operation_type.in_reg.reg == op2->operation_type.in_reg.reg);
			}
			case IR_OPERATION_TYPE_IN_MEM 	: {
				return 1;

			}
			case IR_OPERATION_TYPE_OUT_MEM 	: {
				return 1;
			}
			case IR_OPERATION_TYPE_IMM 		: {
				return (op1->operation_type.imm.signe == op2->operation_type.imm.signe && op1->operation_type.imm.value == op2->operation_type.imm.value);
			}
			case IR_OPERATION_TYPE_INST 	: {
				return (op1->operation_type.inst.opcode == op2->operation_type.inst.opcode);
			}
		}
	}

	return 0;
}

struct ir* ir_create(struct trace* trace){
	struct ir* ir;

	ir =(struct ir*)malloc(sizeof(struct ir));
	if (ir != NULL){
		if(ir_init(ir, trace)){
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

int32_t ir_init(struct ir* ir, struct trace* trace){
	ir->trace 				= trace;
	graph_init(&(ir->graph), sizeof(struct irOperation), sizeof(struct irDependence))
	graph_register_dotPrint_callback(&(ir->graph), NULL, ir_dotPrint_node, ir_dotPrint_edge, NULL)
	
	if (irImporterAsm_import(ir)){
		printf("ERROR: in %s, trace asm import has failed\n", __func__);
		return -1;
	}
	
	return 0;
}

struct node* ir_add_in_reg(struct ir* ir, enum irRegister reg){
	struct node* 			node;
	struct irOperation* 	operation;

	node = graph_add_node_(&(ir->graph));
	if (node == NULL){
		printf("ERROR: in %s, unable to add node to the graph\n", __func__);
	}
	else{
		operation = ir_node_get_operation(node);
		operation->type 						= IR_OPERATION_TYPE_IN_REG;
		operation->operation_type.in_reg.reg 	= reg;
		operation->size 						= irRegister_get_size(reg);
		operation->status_flag 					= IR_NODE_STATUS_FLAG_NONE;
	}

	return node;
}

struct node* ir_add_in_mem(struct ir* ir, struct node* address, uint8_t size, uint32_t order){
	struct node* 			node;
	struct irOperation* 	operation;

	node = graph_add_node_(&(ir->graph));
	if (node == NULL){
		printf("ERROR: in %s, unable to add node to the graph\n", __func__);
	}
	else{
		operation = ir_node_get_operation(node);
		operation->type 						= IR_OPERATION_TYPE_IN_MEM;
		operation->operation_type.in_mem.order 	= order;
		operation->size 						= size;
		operation->status_flag 					= IR_NODE_STATUS_FLAG_NONE;

		if (ir_add_dependence(ir, address, node, IR_DEPENDENCE_TYPE_ADDRESS) == NULL){
			printf("ERROR: in %s, unable to add address dependence\n", __func__);
		}
	}

	return node;
}

struct node* ir_add_out_mem(struct ir* ir, struct node* address, uint8_t size, uint32_t order){
	struct node* 			node;
	struct irOperation* 	operation;

	node = graph_add_node_(&(ir->graph));
	if (node == NULL){
		printf("ERROR: in %s, unable to add node to the graph\n", __func__);
	}
	else{
		operation = ir_node_get_operation(node);
		operation->type 						= IR_OPERATION_TYPE_OUT_MEM;
		operation->operation_type.out_mem.order = order;
		operation->size 						= size;
		operation->status_flag 					= IR_NODE_STATUS_FLAG_FINAL;

		if (ir_add_dependence(ir, address, node, IR_DEPENDENCE_TYPE_ADDRESS) == NULL){
			printf("ERROR: in %s, unable to add address dependence\n", __func__);
		}
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
		operation->operation_type.imm.signe 	= signe;
		operation->operation_type.imm.value 	= value;
		operation->size 						= size;
		operation->status_flag 					= IR_NODE_STATUS_FLAG_NONE;
	}

	return node;
}

struct node* ir_add_inst(struct ir* ir, enum irOpcode opcode, uint8_t size){
	struct node* 			node;
	struct irOperation* 	operation;

	node = graph_add_node_(&(ir->graph));
	if (node == NULL){
		printf("ERROR: in %s, unable to add node to the graph\n", __func__);
	}
	else{
		operation = ir_node_get_operation(node);
		operation->type 						= IR_OPERATION_TYPE_INST;
		operation->operation_type.inst.opcode 	= opcode;
		operation->size 						= size;
		operation->status_flag 					= IR_NODE_STATUS_FLAG_NONE;
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
	struct edge* 	edge_cursor;
	uint32_t 		nb_parent;
	struct node** 	parent;
	uint32_t 		i;

	for (edge_cursor = node_get_head_edge_dst(node), nb_parent = 0; edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
		if (!(ir_node_get_operation(edge_get_src(edge_cursor))->status_flag & IR_NODE_STATUS_FLAG_FINAL) && (edge_get_src(edge_cursor)->nb_edge_src == 1)){
			nb_parent ++;
		}
	}

	if (nb_parent != 0){
		parent = (struct node**)malloc(sizeof(struct node*) * nb_parent);
		if (parent == NULL){
			printf("ERROR: in %s, unable to allocate memory\n", __func__);
			graph_remove_node(&(ir->graph), node);
		}
		else{
			for (edge_cursor = node_get_head_edge_dst(node), i = 0; edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
				if (!(ir_node_get_operation(edge_get_src(edge_cursor))->status_flag & IR_NODE_STATUS_FLAG_FINAL) && (edge_get_src(edge_cursor)->nb_edge_src == 1)){
					parent[i++] = edge_get_src(edge_cursor);
				}
			}

			graph_remove_node(&(ir->graph), node);

			for (i = 0; i < nb_parent; i++){
				ir_remove_node(ir, parent[i]);
			}

			free(parent);
		}
	}
	else{
		graph_remove_node(&(ir->graph), node);
	}
}

uint8_t irRegister_get_size(enum irRegister reg){
	switch(reg){
		case IR_REG_EAX 	: {return 32;}
		case IR_REG_AX 		: {return 16;}
		case IR_REG_AH 		: {return 8;}
		case IR_REG_AL 		: {return 8;}
		case IR_REG_EBX 	: {return 32;}
		case IR_REG_BX 		: {return 16;}
		case IR_REG_BH 		: {return 8;}
		case IR_REG_BL 		: {return 8;}
		case IR_REG_ECX 	: {return 32;}
		case IR_REG_CX 		: {return 16;}
		case IR_REG_CH 		: {return 8;}
		case IR_REG_CL 		: {return 8;}
		case IR_REG_EDX 	: {return 32;}
		case IR_REG_DX 		: {return 16;}
		case IR_REG_DH 		: {return 8;}
		case IR_REG_DL 		: {return 8;}
		case IR_REG_ESP 	: {return 32;}
		case IR_REG_EBP 	: {return 32;}
		case IR_REG_ESI 	: {return 32;}
		case IR_REG_EDI 	: {return 32;}
	}

	return 0;
}

/* ===================================================================== */
/* Printing functions						                             */
/* ===================================================================== */

#pragma GCC diagnostic ignored "-Wunused-parameter"
void ir_dotPrint_node(void* data, FILE* file, void* arg){
	struct irOperation* operation = (struct irOperation*)data;

	switch(operation->type){
		case IR_OPERATION_TYPE_IN_REG 		: {
			fprintf(file, "[shape=\"box\",label=\"%s\"]", irRegister_2_string(operation->operation_type.in_reg.reg));
			break;
		}
		case IR_OPERATION_TYPE_IN_MEM 		: {
			fprintf(file, "[shape=\"box\",label=\"LOAD\"]");
			break;
		}
		case IR_OPERATION_TYPE_OUT_MEM 		: {
			fprintf(file, "[shape=\"box\",label=\"STORE\"]");
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
		case IR_OPERATION_TYPE_INST 		: {
			fprintf(file, "[label=\"%s\"]", irOpcode_2_string(operation->operation_type.inst.opcode));
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
		case IR_DEPENDENCE_TYPE_ADDRESS : {
			fprintf(file, "[label=\"@\"]");
			break;
		}
	}
}

char* irOpcode_2_string(enum irOpcode opcode){
	switch(opcode){
		case IR_ADD 		: {return "add";}
		case IR_AND 		: {return "and";}
		case IR_MOVZX 		: {return "movzx";}
		case IR_NOT 		: {return "not";}
		case IR_OR 			: {return "or";}
		case IR_PART1_8 	: {return "part(1/8)";}
		case IR_PART2_8 	: {return "part(2/8)";}
		case IR_PART1_16 	: {return "part(1/16)";}
		case IR_ROL 		: {return "rol";}
		case IR_ROR 		: {return "ror";}
		case IR_SAR 		: {return "sar";}
		case IR_SHL 		: {return "shl";}
		case IR_SHR 		: {return "shr";}
		case IR_SUB 		: {return "sub";}
		case IR_XOR 		: {return "xor";}
		case IR_INPUT 		: {return "input";}
		case IR_JOKER 		: {return "*";}
	}

	return NULL;
}

char* irRegister_2_string(enum irRegister reg){
	switch(reg){
		case IR_REG_EAX 	: {return "EAX";}
		case IR_REG_AX 		: {return "AX";}
		case IR_REG_AH 		: {return "AH";}
		case IR_REG_AL 		: {return "AL";}
		case IR_REG_EBX 	: {return "EBX";}
		case IR_REG_BX 		: {return "BX";}
		case IR_REG_BH 		: {return "BH";}
		case IR_REG_BL 		: {return "BL";}
		case IR_REG_ECX 	: {return "ECX";}
		case IR_REG_CX 		: {return "CX";}
		case IR_REG_CH 		: {return "CH";}
		case IR_REG_CL 		: {return "CL";}
		case IR_REG_EDX 	: {return "EDX";}
		case IR_REG_DX 		: {return "DX";}
		case IR_REG_DH 		: {return "DH";}
		case IR_REG_DL 		: {return "DL";}
		case IR_REG_ESP 	: {return "ESP";}
		case IR_REG_EBP 	: {return "EBP";}
		case IR_REG_ESI 	: {return "ESI";}
		case IR_REG_EDI 	: {return "EDI";}
	}

	return NULL;
}