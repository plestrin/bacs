#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "ir.h"
#include "irImporterAsm.h"
#include "result.h"
#include "multiColumn.h"
#include "base.h"

struct ir* ir_create(struct assembly* assembly, struct memTrace* mem_trace){
	struct ir* ir;

	ir =(struct ir*)malloc(sizeof(struct ir));
	if (ir != NULL){
		if(ir_init(ir, assembly, mem_trace)){
			log_err("unable to init ir");
			free(ir);
			ir = NULL;
		}
	}
	else{
		log_err("unable to allocate memory");
	}

	return ir;
}

int32_t ir_init(struct ir* ir, struct assembly* assembly, struct memTrace* mem_trace){
	graph_init(&(ir->graph), sizeof(struct irOperation), sizeof(struct irDependence))

	graph_register_dotPrint_callback(&(ir->graph), NULL, ir_dotPrint_node, ir_dotPrint_edge, NULL)
	
	#ifdef VERBOSE
	if (mem_trace != NULL){
		log_info("found memory concrete values");
	}
	#endif

	if (irImporterAsm_import(ir, assembly, mem_trace)){
		log_err("trace asm import has failed");
		return -1;
	}

	return 0;
}

struct node* ir_add_in_reg(struct ir* ir, uint32_t index, enum irRegister reg){
	struct node* 			node;
	struct irOperation* 	operation;

	node = graph_add_node_(&(ir->graph));
	if (node == NULL){
		log_err("unable to add node to the graph");
	}
	else{
		operation = ir_node_get_operation(node);
		operation->type 								= IR_OPERATION_TYPE_IN_REG;
		operation->operation_type.in_reg.reg 			= reg;
		operation->size 								= irRegister_get_size(reg);
		operation->index 								= index;
		operation->status_flag 							= IR_NODE_STATUS_FLAG_NONE;
	}

	return node;
}

struct node* ir_add_in_mem_(struct ir* ir, uint32_t index, uint8_t size, struct node* address, struct node* prev, ADDRESS concrete_address){
	struct node* 			node;
	struct irOperation* 	operation;

	node = graph_add_node_(&(ir->graph));
	if (node == NULL){
		log_err("unable to add node to the graph");
	}
	else{
		operation = ir_node_get_operation(node);
		operation->type 								= IR_OPERATION_TYPE_IN_MEM;
		operation->operation_type.mem.access.prev 		= prev;
		operation->operation_type.mem.access.next 		= NULL;
		if (prev == NULL){
			operation->operation_type.mem.access.order 	= 1;
		}
		else{
			ir_node_get_operation(prev)->operation_type.mem.access.next = node;
			operation->operation_type.mem.access.order 	= ir_node_get_operation(prev)->operation_type.mem.access.order + 1;
		}
		operation->operation_type.mem.access.con_addr 	= concrete_address;
		operation->size 								= size;
		operation->index 								= index;
		operation->status_flag 							= IR_NODE_STATUS_FLAG_NONE;

		if (ir_add_dependence(ir, address, node, IR_DEPENDENCE_TYPE_ADDRESS) == NULL){
			log_err("unable to add address dependence");
		}
	}

	return node;
}

struct node* ir_add_out_mem_(struct ir* ir, uint32_t index, uint8_t size, struct node* address, struct node* prev, ADDRESS concrete_address){
	struct node* 			node;
	struct irOperation* 	operation;

	node = graph_add_node_(&(ir->graph));
	if (node == NULL){
		log_err("unable to add node to the graph");
	}
	else{
		operation = ir_node_get_operation(node);
		operation->type 								= IR_OPERATION_TYPE_OUT_MEM;
		operation->operation_type.mem.access.prev 		= prev;
		operation->operation_type.mem.access.next 		= NULL;
		if (prev == NULL){
			operation->operation_type.mem.access.order 	= 1;
		}
		else{
			ir_node_get_operation(prev)->operation_type.mem.access.next = node;
			operation->operation_type.mem.access.order 	= ir_node_get_operation(prev)->operation_type.mem.access.order + 1;
		}
		operation->operation_type.mem.access.con_addr 	= concrete_address;
		operation->size 								= size;
		operation->index 								= index;
		operation->status_flag 							= IR_NODE_STATUS_FLAG_FINAL;

		if (ir_add_dependence(ir, address, node, IR_DEPENDENCE_TYPE_ADDRESS) == NULL){
			log_err("unable to add address dependence");
		}
	}

	return node;
}

struct node* ir_add_immediate(struct ir* ir, uint8_t size, uint64_t value){
	struct node* 			node;
	struct irOperation* 	operation;

	node = graph_add_node_(&(ir->graph));
	if (node == NULL){
		log_err("unable to add node to the graph");
	}
	else{
		operation = ir_node_get_operation(node);
		operation->type 								= IR_OPERATION_TYPE_IMM;
		operation->operation_type.imm.value 			= value;
		operation->size 								= size;
		operation->index 								= IR_INSTRUCTION_INDEX_IMMEDIATE;
		operation->status_flag 							= IR_NODE_STATUS_FLAG_NONE;
	}

	return node;
}

struct node* ir_add_inst(struct ir* ir, uint32_t index, uint8_t size, enum irOpcode opcode){
	struct node* 			node;
	struct irOperation* 	operation;

	node = graph_add_node_(&(ir->graph));
	if (node == NULL){
		log_err("unable to add node to the graph");
	}
	else{
		operation = ir_node_get_operation(node);
		operation->type 								= IR_OPERATION_TYPE_INST;
		operation->operation_type.inst.opcode 			= opcode;
		operation->size 								= size;
		operation->index 								= index;
		operation->status_flag 							= IR_NODE_STATUS_FLAG_NONE;
	}

	return node;
}

struct node* ir_add_symbol(struct ir* ir, void* result_ptr, uint32_t index){
	struct node* 			node;
	struct irOperation* 	operation;

	node = graph_add_node_(&(ir->graph));
	if (node == NULL){
		log_err("unable to add node to the graph");
	}
	else{
		operation = ir_node_get_operation(node);
		operation->type 								= IR_OPERATION_TYPE_SYMBOL;
		operation->operation_type.symbol.result_ptr 	= result_ptr;
		operation->operation_type.symbol.index 			= index;
		operation->size 								= 1;
		operation->index 								= IR_INSTRUCTION_INDEX_UNKOWN;
		operation->status_flag 							= IR_NODE_STATUS_FLAG_NONE;
	}

	return node;
}

struct node* ir_insert_immediate(struct ir* ir, struct node* root, uint8_t size, uint64_t value){
	struct node* 			node;
	struct irOperation* 	operation;

	node = graph_insert_node_(&(ir->graph), root);
	if (node == NULL){
		log_err("unable to add node to the graph");
	}
	else{
		operation = ir_node_get_operation(node);
		operation->type 								= IR_OPERATION_TYPE_IMM;
		operation->operation_type.imm.value 			= value;
		operation->size 								= size;
		operation->index 								= IR_INSTRUCTION_INDEX_IMMEDIATE;
		operation->status_flag 							= IR_NODE_STATUS_FLAG_NONE;
	}

	return node;
}

struct node* ir_insert_inst(struct ir* ir, struct node* root, uint32_t index, uint8_t size, enum irOpcode opcode){
	struct node* 			node;
	struct irOperation* 	operation;

	node = graph_insert_node_(&(ir->graph), root);
	if (node == NULL){
		log_err("unable to add node to the graph");
	}
	else{
		operation = ir_node_get_operation(node);
		operation->type 								= IR_OPERATION_TYPE_INST;
		operation->operation_type.inst.opcode 			= opcode;
		operation->size 								= size;
		operation->index 								= index;
		operation->status_flag 							= IR_NODE_STATUS_FLAG_NONE;
	}

	return node;
}

struct edge* ir_add_dependence(struct ir* ir, struct node* operation_src, struct node* operation_dst, enum irDependenceType type){
	struct edge* 			edge;
	struct irDependence* 	dependence;

	edge = graph_add_edge_(&(ir->graph), operation_src, operation_dst);
	if (edge == NULL){
		log_err("unable to add edge to the graph");
	}
	else{
		dependence = ir_edge_get_dependence(edge);
		dependence->type 					= type;
	}

	return edge;
}

struct edge* ir_add_macro_dependence(struct ir* ir, struct node* operation_src, struct node* operation_dst, uint32_t desc){
	struct edge* 			edge;
	struct irDependence* 	dependence;

	edge = graph_add_edge_(&(ir->graph), operation_src, operation_dst);
	if (edge == NULL){
		log_err("unable to add edge to the graph");
	}
	else{
		dependence = ir_edge_get_dependence(edge);
		dependence->type 					= IR_DEPENDENCE_TYPE_MACRO;
		dependence->dependence_type.macro 	= desc;
	}

	return edge;
}

void ir_remove_node(struct ir* ir, struct node* node){
	struct edge* 	edge_cursor;
	uint32_t 		nb_parent;
	struct node** 	parent;
	uint32_t 		i;

	if (ir_node_get_operation(node)->type == IR_OPERATION_TYPE_IN_MEM || ir_node_get_operation(node)->type == IR_OPERATION_TYPE_OUT_MEM){
		ir_mem_remove(ir_node_get_operation(node));
	}

	for (edge_cursor = node_get_head_edge_dst(node), nb_parent = 0; edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
		if (!(ir_node_get_operation(edge_get_src(edge_cursor))->status_flag & IR_NODE_STATUS_FLAG_FINAL) && (edge_get_src(edge_cursor)->nb_edge_src == 1)){
			nb_parent ++;
		}
	}

	if (nb_parent != 0){
		parent = (struct node**)malloc(sizeof(struct node*) * nb_parent);
		if (parent == NULL){
			log_err("unable to allocate memory");
			graph_remove_node(&(ir->graph), node);
		}
		else{
			for (edge_cursor = node_get_head_edge_dst(node), i = 0; edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
				if (!(ir_node_get_operation(edge_get_src(edge_cursor))->status_flag & IR_NODE_STATUS_FLAG_FINAL) && (edge_get_src(edge_cursor)->nb_edge_src == 1)){
					parent[i ++] = edge_get_src(edge_cursor);
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

void ir_remove_footprint(struct ir* ir, struct node** node_buffer, uint32_t nb_node){
	uint32_t 		i;
	struct edge* 	edge_cursor;

	for (i = 0; i < nb_node; i++){
		for (edge_cursor = node_get_head_edge_dst(node_buffer[i]); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
			if (ir_edge_get_dependence(edge_cursor)->type == IR_DEPENDENCE_TYPE_MACRO){
				break;
			}
		}
		if (edge_cursor != NULL){
			continue;
		}
		for (edge_cursor = node_get_head_edge_src(node_buffer[i]); edge_cursor != NULL; edge_cursor = edge_get_next_src(edge_cursor)){
			if (ir_edge_get_dependence(edge_cursor)->type == IR_DEPENDENCE_TYPE_MACRO){
				break;
			}
		}
		if (edge_cursor != NULL){
			continue;
		}

		if (ir_node_get_operation(node_buffer[i])->type == IR_OPERATION_TYPE_IN_MEM || ir_node_get_operation(node_buffer[i])->type == IR_OPERATION_TYPE_OUT_MEM){
			ir_mem_remove(ir_node_get_operation(node_buffer[i]));
		}

		graph_remove_node(&(ir->graph), node_buffer[i]);
	}
}

void ir_remove_dependence(struct ir* ir, struct edge* edge){
	struct node* node_src;

	node_src = edge_get_src(edge);
	graph_remove_edge(&(ir->graph), edge);
	if (node_src->nb_edge_src == 0 && !(ir_node_get_operation(node_src)->status_flag & IR_NODE_STATUS_FLAG_FINAL)){
		ir_remove_node(ir, node_src);
	}
}

/* ===================================================================== */
/* Printing functions						                             */
/* ===================================================================== */

#pragma GCC diagnostic ignored "-Wunused-parameter"
void ir_print_location_node(struct node* node, struct assembly* assembly){
	struct irOperation* operation;
	struct edge* 		edge_cursor;

	operation = ir_node_get_operation(node);
	switch(operation->index){
		case IR_INSTRUCTION_INDEX_IMMEDIATE 	: {
			printf("IMM=%llx", ir_imm_operation_get_unsigned_value(operation));
			break;
		}
		case IR_INSTRUCTION_INDEX_ADDRESS 		: 
		case IR_INSTRUCTION_INDEX_UNKOWN 		: {
			switch(operation->type){
				case IR_OPERATION_TYPE_IN_REG 	: {
					printf("%s@??", irRegister_2_string(operation->operation_type.in_reg.reg));
					break;
				}
				case IR_OPERATION_TYPE_IN_MEM 	: {
					printf("IMEM@??");
					break;
				}
				case IR_OPERATION_TYPE_OUT_MEM 	: {
					printf("OMEM@??");
					break;
				}
				case IR_OPERATION_TYPE_INST 	: {
					printf("%s(", irOpcode_2_string(operation->operation_type.inst.opcode));
					for (edge_cursor = node_get_head_edge_dst(node); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
						ir_print_location_node(edge_get_src(edge_cursor), assembly);
						printf(",");
					}
					printf("\b)");

					break;
				}
				default 						: {
					printf("??");
					break;
				}
			}
			break;
		}
		default 								: {
			switch(operation->type){
				case IR_OPERATION_TYPE_IN_REG 	: {
					printf("%s@%u", irRegister_2_string(operation->operation_type.in_reg.reg), operation->index);
					break;
				}
				case IR_OPERATION_TYPE_IN_MEM 	: {
					printf("IMEM@%u", operation->index);
					break;
				}
				case IR_OPERATION_TYPE_OUT_MEM 	: {
					printf("OMEM@%u", operation->index);
					break;
				}
				default 						: {
					printf("%u", operation->index);
					break;
				}
			}
		}
	}
}

#pragma GCC diagnostic ignored "-Wunused-parameter"
void ir_dotPrint_node(void* data, FILE* file, void* arg){
	struct irOperation* operation = (struct irOperation*)data;

	switch(operation->type){
		case IR_OPERATION_TYPE_IN_REG 		: {
			fprintf(file, "[shape=\"box\",label=\"%s\"", irRegister_2_string(operation->operation_type.in_reg.reg));
			break;
		}
		case IR_OPERATION_TYPE_IN_MEM 		: {
			fprintf(file, "[shape=\"box\",label=\"LOAD\"");
			break;
		}
		case IR_OPERATION_TYPE_OUT_MEM 		: {
			fprintf(file, "[shape=\"box\",label=\"STORE\"");
			break;
		}
		case IR_OPERATION_TYPE_IMM 			: {
			if (operation->status_flag & IR_NODE_STATUS_FLAG_FINAL){
				fprintf(file, "[shape=\"Mdiamond\"");
			}
			else{
				fprintf(file, "[shape=\"diamond\"");
			}
			switch(operation->size){
				case 8 	: {
					fprintf(file, ",label=\"0x%02x\"", (uint32_t)(operation->operation_type.imm.value & 0xff));
					break;
				}
				case 16 : {
					fprintf(file, ",label=\"0x%04x\"", (uint32_t)(operation->operation_type.imm.value & 0xffff));
					break;
				}
				case 32 : {
					fprintf(file, ",label=\"0x%08x\"", (uint32_t)(operation->operation_type.imm.value & 0xffffffff));
					break;
				}
				default : {
					printf("ERROR: in %s, this case is not implemented, size: %u\n", __func__, operation->size);
					break;
				}
			}
			break;
		}
		case IR_OPERATION_TYPE_INST 		: {
			if (operation->status_flag & IR_NODE_STATUS_FLAG_FINAL){
				fprintf(file, "[shape=\"octagon\",label=\"%s\"", irOpcode_2_string(operation->operation_type.inst.opcode));
			}
			else{
				fprintf(file, "[label=\"%s\"", irOpcode_2_string(operation->operation_type.inst.opcode));
			}
			break;
		}
		case IR_OPERATION_TYPE_SYMBOL 		: {
			fprintf(file, "[label=\"%s\"", ((struct result*)(operation->operation_type.symbol.result_ptr))->signature->name);
			break;
		}
	}
	if (operation->status_flag & IR_NODE_STATUS_FLAG_ERROR){
		fprintf(file, ",color=\"red\"]");
	}
	else{
		fprintf(file, "]");
	}
}

#pragma GCC diagnostic ignored "-Wunused-parameter"
void ir_dotPrint_edge(void* data, FILE* file, void* arg){
	struct irDependence* dependence = (struct irDependence*)data;

	switch(dependence->type){
		case IR_DEPENDENCE_TYPE_DIRECT 		: {
			break;
		}
		case IR_DEPENDENCE_TYPE_ADDRESS 	: {
			fprintf(file, "[label=\"@\"]");
			break;
		}
		case IR_DEPENDENCE_TYPE_SHIFT_DISP 	: {
			fprintf(file, "[label=\"disp\"]");
			break;
		}
		case IR_DEPENDENCE_TYPE_DIVISOR 	: {
			fprintf(file, "[label=\"/\"]");
			break;
		}
		case IR_DEPENDENCE_TYPE_ROUND_OFF 	:
		case IR_DEPENDENCE_TYPE_SUBSTITUTE  : {
			fprintf(file, "[label=\"s\"]"); 		/* the s is used to tag special operands */
			break;
		}
		case IR_DEPENDENCE_TYPE_MACRO 		: {
			if (IR_DEPENDENCE_MACRO_DESC_IS_INPUT(dependence->dependence_type.macro)){
				fprintf(file, "[label=\"I%uF%u\"]", IR_DEPENDENCE_MACRO_DESC_GET_ARG(dependence->dependence_type.macro), IR_DEPENDENCE_MACRO_DESC_GET_FRAG(dependence->dependence_type.macro));
			}
			else{
				fprintf(file, "[label=\"O%uF%u\"]", IR_DEPENDENCE_MACRO_DESC_GET_ARG(dependence->dependence_type.macro), IR_DEPENDENCE_MACRO_DESC_GET_FRAG(dependence->dependence_type.macro));
			}
			break;
		}
	}
}

#pragma GCC diagnostic ignored "-Wunused-parameter"
int32_t ir_printDot_filter_macro_node(struct node* node, void* arg){
	struct edge* 			edge_cursor;
	struct irDependence* 	dependence;

	for (edge_cursor = node_get_head_edge_src(node); edge_cursor != NULL; edge_cursor = edge_get_next_src(edge_cursor)){
		dependence = ir_edge_get_dependence(edge_cursor);
		if (dependence->type == IR_DEPENDENCE_TYPE_MACRO){
			return 1;
		}
	}

	for (edge_cursor = node_get_head_edge_dst(node); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
		dependence = ir_edge_get_dependence(edge_cursor);
		if (dependence->type == IR_DEPENDENCE_TYPE_MACRO){
			return 1;
		}
	}

	return 0;
}

#pragma GCC diagnostic ignored "-Wunused-parameter"
int32_t ir_printDot_filter_macro_edge(struct edge* edge, void* arg){
	struct irDependence* dependence;

	dependence = ir_edge_get_dependence(edge);
	if (dependence->type == IR_DEPENDENCE_TYPE_MACRO){
		return 1;
	}

	return 0;
}

char* irOpcode_2_string(enum irOpcode opcode){
	switch(opcode){
		case IR_ADD 		: {return "add";}
		case IR_AND 		: {return "and";}
		case IR_CMOV 		: {return "cmov";}
		case IR_DIV 		: {return "div";}
		case IR_IDIV 		: {return "idiv";}
		case IR_IMUL 		: {return "imul";}
		case IR_LEA 		: {return "lea";}
		case IR_MOV 		: {return "mov";}
		case IR_MOVZX 		: {return "movzx";}
		case IR_MUL 		: {return "mul";}
		case IR_NEG 		: {return "neg";}
		case IR_NOT 		: {return "not";}
		case IR_OR 			: {return "or";}
		case IR_PART1_8 	: {return "part(1/8)";}
		case IR_PART2_8 	: {return "part(2/8)";}
		case IR_PART1_16 	: {return "part(1/16)";}
		case IR_ROL 		: {return "rol";}
		case IR_ROR 		: {return "ror";}
		case IR_SHL 		: {return "shl";}
		case IR_SHLD 		: {return "shld";}
		case IR_SHR 		: {return "shr";}
		case IR_SHRD 		: {return "shrd";}
		case IR_SUB 		: {return "sub";}
		case IR_XOR 		: {return "xor";}
		case IR_LOAD 		: {return "load";}
		case IR_STORE 		: {return "store";}
		case IR_JOKER 		: {return "*";}
		case IR_INVALID 	: {return "?";}
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
		case IR_REG_SP 		: {return "SP";}
		case IR_REG_EBP 	: {return "EBP";}
		case IR_REG_BP 		: {return "BP";}
		case IR_REG_ESI 	: {return "ESI";}
		case IR_REG_SI 		: {return "SI";}
		case IR_REG_EDI 	: {return "EDI";}
		case IR_REG_DI 		: {return "DI";}
	}

	return NULL;
}