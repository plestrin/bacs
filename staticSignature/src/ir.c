#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "ir.h"
#include "irImporterAsm.h"
#include "irRenameEngine.h"
#include "codeSignature.h"
#include "base.h"

struct ir* ir_create(struct assembly* assembly, struct memTrace* mem_trace){
	struct ir* ir;

	ir = (struct ir*)malloc(sizeof(struct ir));
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

	ir->range_seed = 1;

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

struct ir* ir_create_compound(struct assembly* assembly, struct memTrace* mem_trace, struct irComponent** ir_component_buffer, uint32_t nb_ir_component){
	struct ir* ir;

	ir = (struct ir*)malloc(sizeof(struct ir));
	if (ir != NULL){
		if(ir_init_compound(ir, assembly, mem_trace, ir_component_buffer, nb_ir_component)){
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

int32_t ir_init_compound(struct ir* ir, struct assembly* assembly, struct memTrace* mem_trace, struct irComponent** ir_component_buffer, uint32_t nb_ir_component){
	graph_init(&(ir->graph), sizeof(struct irOperation), sizeof(struct irDependence))

	ir->range_seed = 1;

	graph_register_dotPrint_callback(&(ir->graph), NULL, ir_dotPrint_node, ir_dotPrint_edge, NULL)
	
	#ifdef VERBOSE
	if (mem_trace != NULL){
		log_info("found memory concrete values");
	}
	#endif

	if (irImporterAsm_import_compound(ir, assembly, mem_trace, ir_component_buffer, nb_ir_component)){
		log_err("trace asm import has failed");
		return -1;
	}

	return 0;
}

struct node* ir_add_in_reg(struct ir* ir, uint32_t index, enum irRegister reg, uint32_t primer){
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
		operation->operation_type.in_reg.primer 		= primer;
		operation->size 								= irRegister_get_size(reg);
		operation->index 								= index;
		operation->status_flag 							= IR_OPERATION_STATUS_FLAG_NONE;
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
		operation->operation_type.mem.prev 				= prev;
		operation->operation_type.mem.next 				= NULL;
		if (prev == NULL){
			operation->operation_type.mem.order 		= 1;
		}
		else{
			ir_node_get_operation(prev)->operation_type.mem.next = node;
			operation->operation_type.mem.order 		= ir_node_get_operation(prev)->operation_type.mem.order + 1;
		}
		operation->operation_type.mem.con_addr 			= concrete_address;
		operation->size 								= size;
		operation->index 								= index;
		operation->status_flag 							= IR_OPERATION_STATUS_FLAG_NONE;

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
		operation->operation_type.mem.prev 				= prev;
		operation->operation_type.mem.next 				= NULL;
		if (prev == NULL){
			operation->operation_type.mem.order 		= 1;
		}
		else{
			ir_node_get_operation(prev)->operation_type.mem.next = node;
			operation->operation_type.mem.order 		= ir_node_get_operation(prev)->operation_type.mem.order + 1;
		}
		operation->operation_type.mem.con_addr 			= concrete_address;
		operation->size 								= size;
		operation->index 								= index;
		operation->status_flag 							= IR_OPERATION_STATUS_FLAG_FINAL;

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
		operation->index 								= IR_OPERATION_INDEX_IMMEDIATE;
		operation->status_flag 							= IR_OPERATION_STATUS_FLAG_NONE;
	}

	return node;
}

struct node* ir_add_inst(struct ir* ir, uint32_t index, uint8_t size, enum irOpcode opcode, uint32_t dst){
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
		operation->operation_type.inst.dst 				= dst;
		operation->operation_type.inst.seed 			= IR_INVALID_RANGE_SEED;
		operation->size 								= size;
		operation->index 								= index;
		operation->status_flag 							= IR_OPERATION_STATUS_FLAG_NONE;
	}

	return node;
}

struct node* ir_add_symbol(struct ir* ir, void* code_signature, void* result, uint32_t index){
	struct node* 			node;
	struct irOperation* 	operation;

	node = graph_add_node_(&(ir->graph));
	if (node == NULL){
		log_err("unable to add node to the graph");
	}
	else{
		operation = ir_node_get_operation(node);
		operation->type 								= IR_OPERATION_TYPE_SYMBOL;
		operation->operation_type.symbol.code_signature = code_signature;
		operation->operation_type.symbol.result 		= result;
		operation->operation_type.symbol.index 			= index;
		operation->size 								= 1;
		operation->index 								= IR_OPERATION_INDEX_UNKOWN;
		operation->status_flag 							= IR_OPERATION_STATUS_FLAG_NONE;
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
		operation->index 								= IR_OPERATION_INDEX_IMMEDIATE;
		operation->status_flag 							= IR_OPERATION_STATUS_FLAG_NONE;
	}

	return node;
}

void ir_convert_node_to_imm(struct ir* ir, struct node* node, uint8_t size, uint64_t value){
	struct irOperation* operation = ir_node_get_operation(node);
	struct edge* 		edge_cursor;
	struct edge* 		edge_current;

	for (edge_cursor = node_get_head_edge_dst(node); edge_cursor != NULL; ){
		edge_current = edge_cursor;
		edge_cursor = edge_get_next_dst(edge_cursor);

		ir_remove_dependence(ir, edge_current);
	}

	if (operation->type == IR_OPERATION_TYPE_IN_MEM || operation->type == IR_OPERATION_TYPE_OUT_MEM){
		ir_mem_remove(operation);
	}

	operation->type 						= IR_OPERATION_TYPE_IMM;
	operation->operation_type.imm.value 	= value;
	operation->size 						= size;
	operation->index 						= IR_OPERATION_INDEX_IMMEDIATE;
	operation->status_flag 					= IR_OPERATION_STATUS_FLAG_NONE;
}

struct node* ir_insert_inst(struct ir* ir, struct node* root, uint32_t index, uint8_t size, enum irOpcode opcode, uint32_t dst){
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
		operation->operation_type.inst.dst 				= dst;
		operation->operation_type.inst.seed 			= IR_INVALID_RANGE_SEED;
		operation->size 								= size;
		operation->index 								= index;
		operation->status_flag 							= IR_OPERATION_STATUS_FLAG_NONE;
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
	struct edge* 	edge_curr;
	struct edge* 	edge_next;

	if (ir_node_get_operation(node)->type == IR_OPERATION_TYPE_IN_MEM || ir_node_get_operation(node)->type == IR_OPERATION_TYPE_OUT_MEM){
		ir_mem_remove(ir_node_get_operation(node));
	}

	if (ir_node_get_operation(node)->status_flag & IR_OPERATION_STATUS_FLAG_FINAL){
		irRenameEngine_delete_node(ir->alias_buffer, node);
	}

	for (edge_curr = node_get_head_edge_dst(node); edge_curr != NULL; edge_curr = edge_next){
		edge_next = edge_get_next_dst(edge_curr);
		ir_remove_dependence(ir, edge_curr);
	}

	graph_remove_node(&(ir->graph), node);
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

		if (ir_node_get_operation(node_buffer[i])->status_flag & IR_OPERATION_STATUS_FLAG_FINAL){
			irRenameEngine_delete_node(ir->alias_buffer, node_buffer[i]);
		}

		graph_remove_node(&(ir->graph), node_buffer[i]);
	}
}

void ir_remove_dependence(struct ir* ir, struct edge* edge){
	struct node* node_src;

	node_src = edge_get_src(edge);
	graph_remove_edge(&(ir->graph), edge);
	if (!(ir_node_get_operation(node_src)->status_flag & IR_OPERATION_STATUS_FLAG_FINAL) && (node_src->nb_edge_src == 0 || ir_node_get_operation(node_src)->type == IR_OPERATION_TYPE_SYMBOL)){
		ir_remove_node(ir, node_src);
	}
}

/* ===================================================================== */
/* Printing functions						                             */
/* ===================================================================== */

void ir_print_location_node(struct node* node, struct assembly* assembly){
	struct irOperation* operation;
	struct edge* 		edge_cursor;

	operation = ir_node_get_operation(node);
	switch(operation->index){
		case IR_OPERATION_INDEX_IMMEDIATE 	: {
			printf("IMM=%llx", ir_imm_operation_get_unsigned_value(operation));
			break;
		}
		case IR_OPERATION_INDEX_ADDRESS 		: 
		case IR_OPERATION_INDEX_UNKOWN 		: {
			switch(operation->type){
				case IR_OPERATION_TYPE_IN_REG 	: {
					printf("%s@??", irRegister_2_string(operation->operation_type.in_reg.reg));
					break;
				}
				case IR_OPERATION_TYPE_IN_MEM 	: {
					fputs("IMEM@??", stdout);
					break;
				}
				case IR_OPERATION_TYPE_OUT_MEM 	: {
					fputs("OMEM@??", stdout);
					break;
				}
				case IR_OPERATION_TYPE_INST 	: {
					printf("%s(", irOpcode_2_string(operation->operation_type.inst.opcode));
					for (edge_cursor = node_get_head_edge_dst(node); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
						ir_print_location_node(edge_get_src(edge_cursor), assembly);
						fputs(",", stdout);
					}
					fputs("\b)", stdout);

					break;
				}
				default 						: {
					fputs("??", stdout);
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

void ir_print_node(struct irOperation* operation, FILE* file){
	switch(operation->type){
		case IR_OPERATION_TYPE_IN_REG 		: {
			fputs(irRegister_2_string(operation->operation_type.in_reg.reg), file);
			break;
		}
		case IR_OPERATION_TYPE_IN_MEM 		: {
			fputs("LOAD", file);
			break;
		}
		case IR_OPERATION_TYPE_OUT_MEM 		: {
			fputs("STORE", file);
			break;
		}
		case IR_OPERATION_TYPE_IMM 			: {
			fprintf(file, "0x%llx", ir_imm_operation_get_unsigned_value(operation));
			break;
		}
		case IR_OPERATION_TYPE_INST 		: {
			fputs(irOpcode_2_string(operation->operation_type.inst.opcode), file);
			break;
		}
		case IR_OPERATION_TYPE_SYMBOL 		: {
			fputs(((struct codeSignature*)(operation->operation_type.symbol.code_signature))->signature.name, file);
			break;
		}
	}
	fprintf(file, ":{size=%u, flag=0x%08x, index=%u}", operation->size, operation->status_flag, operation->index);
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
			fputs("[shape=\"box\",label=\"LOAD\"", file);
			break;
		}
		case IR_OPERATION_TYPE_OUT_MEM 		: {
			fputs("[shape=\"box\",label=\"STORE\"", file);
			break;
		}
		case IR_OPERATION_TYPE_IMM 			: {
			if (operation->status_flag & IR_OPERATION_STATUS_FLAG_FINAL){
				fputs("[shape=\"Mdiamond\"", file);
			}
			else{
				fputs("[shape=\"diamond\"", file);
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
					log_err_m("this case is not implemented, size: %u", operation->size);
					break;
				}
			}
			break;
		}
		case IR_OPERATION_TYPE_INST 		: {
			if (operation->status_flag & IR_OPERATION_STATUS_FLAG_FINAL){
				fprintf(file, "[shape=\"octagon\",label=\"%s\"", irOpcode_2_string(operation->operation_type.inst.opcode));
			}
			else{
				fprintf(file, "[label=\"%s\"", irOpcode_2_string(operation->operation_type.inst.opcode));
			}
			break;
		}
		case IR_OPERATION_TYPE_SYMBOL 		: {
			fprintf(file, "[label=\"%s\"", ((struct codeSignature*)(operation->operation_type.symbol.code_signature))->signature.name);
			break;
		}
	}
	if (operation->status_flag & IR_OPERATION_STATUS_FLAG_ERROR){
		fputs(",color=\"red\"]", file);
	}
	else{
		fputs("]", file);
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
			fputs("[label=\"@\"]", file);
			break;
		}
		case IR_DEPENDENCE_TYPE_SHIFT_DISP 	: {
			fputs("[label=\"disp\"]", file);
			break;
		}
		case IR_DEPENDENCE_TYPE_DIVISOR 	: {
			fputs("[label=\"/\"]", file);
			break;
		}
		case IR_DEPENDENCE_TYPE_ROUND_OFF 	:
		case IR_DEPENDENCE_TYPE_SUBSTITUTE  : {
			fputs("[label=\"s\"]", file); 		/* the s is used to tag special operands */
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

char* irOpcode_2_string(enum irOpcode opcode){
	switch(opcode){
		case IR_ADC 		: {return "adc";}
		case IR_ADD 		: {return "add";}
		case IR_AND 		: {return "and";}
		case IR_CMOV 		: {return "cmov";}
		case IR_DIVQ 		: {return "divq";}
		case IR_DIVR 		: {return "divr";}
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
		case IR_REG_XMM1_1 	: {return "XMM0_1";}
		case IR_REG_XMM1_2 	: {return "XMM0_2";}
		case IR_REG_XMM1_3 	: {return "XMM0_3";}
		case IR_REG_XMM1_4 	: {return "XMM0_4";}
		case IR_REG_XMM2_1 	: {return "XMM1_1";}
		case IR_REG_XMM2_2 	: {return "XMM1_2";}
		case IR_REG_XMM2_3 	: {return "XMM1_3";}
		case IR_REG_XMM2_4 	: {return "XMM1_4";}
		case IR_REG_XMM3_1 	: {return "XMM2_1";}
		case IR_REG_XMM3_2 	: {return "XMM2_2";}
		case IR_REG_XMM3_3 	: {return "XMM2_3";}
		case IR_REG_XMM3_4 	: {return "XMM2_4";}
		case IR_REG_XMM4_1 	: {return "XMM3_1";}
		case IR_REG_XMM4_2 	: {return "XMM3_2";}
		case IR_REG_XMM4_3 	: {return "XMM3_3";}
		case IR_REG_XMM4_4 	: {return "XMM3_4";}
		case IR_REG_XMM5_1 	: {return "XMM4_1";}
		case IR_REG_XMM5_2 	: {return "XMM4_2";}
		case IR_REG_XMM5_3 	: {return "XMM4_3";}
		case IR_REG_XMM5_4 	: {return "XMM4_4";}
		case IR_REG_XMM6_1 	: {return "XMM5_1";}
		case IR_REG_XMM6_2 	: {return "XMM5_2";}
		case IR_REG_XMM6_3 	: {return "XMM5_3";}
		case IR_REG_XMM6_4 	: {return "XMM5_4";}
		case IR_REG_XMM7_1 	: {return "XMM6_1";}
		case IR_REG_XMM7_2 	: {return "XMM6_2";}
		case IR_REG_XMM7_3 	: {return "XMM6_3";}
		case IR_REG_XMM7_4 	: {return "XMM6_4";}
		case IR_REG_XMM8_1 	: {return "XMM7_1";}
		case IR_REG_XMM8_2 	: {return "XMM7_2";}
		case IR_REG_XMM8_3 	: {return "XMM7_3";}
		case IR_REG_XMM8_4 	: {return "XMM7_4";}
		case IR_REG_XMM9_1 	: {return "XMM8_1";}
		case IR_REG_XMM9_2 	: {return "XMM8_2";}
		case IR_REG_XMM9_3 	: {return "XMM8_3";}
		case IR_REG_XMM9_4 	: {return "XMM8_4";}
		case IR_REG_XMM10_1 : {return "XMM9_1";}
		case IR_REG_XMM10_2 : {return "XMM9_2";}
		case IR_REG_XMM10_3 : {return "XMM9_3";}
		case IR_REG_XMM10_4 : {return "XMM9_4";}
		case IR_REG_XMM11_1 : {return "XMM10_1";}
		case IR_REG_XMM11_2 : {return "XMM10_2";}
		case IR_REG_XMM11_3 : {return "XMM10_3";}
		case IR_REG_XMM11_4 : {return "XMM10_4";}
		case IR_REG_XMM12_1 : {return "XMM11_1";}
		case IR_REG_XMM12_2 : {return "XMM11_2";}
		case IR_REG_XMM12_3 : {return "XMM11_3";}
		case IR_REG_XMM12_4 : {return "XMM11_4";}
		case IR_REG_XMM13_1 : {return "XMM12_1";}
		case IR_REG_XMM13_2 : {return "XMM12_2";}
		case IR_REG_XMM13_3 : {return "XMM12_3";}
		case IR_REG_XMM13_4 : {return "XMM12_4";}
		case IR_REG_XMM14_1 : {return "XMM13_1";}
		case IR_REG_XMM14_2 : {return "XMM13_2";}
		case IR_REG_XMM14_3 : {return "XMM13_3";}
		case IR_REG_XMM14_4 : {return "XMM13_4";}
		case IR_REG_XMM15_1 : {return "XMM14_1";}
		case IR_REG_XMM15_2 : {return "XMM14_2";}
		case IR_REG_XMM15_3 : {return "XMM14_3";}
		case IR_REG_XMM15_4 : {return "XMM14_4";}
		case IR_REG_XMM16_1 : {return "XMM15_1";}
		case IR_REG_XMM16_2 : {return "XMM15_2";}
		case IR_REG_XMM16_3 : {return "XMM15_3";}
		case IR_REG_XMM16_4 : {return "XMM15_4";}
		case IR_REG_MMX1_1 	: {return "MMX0_1";}
		case IR_REG_MMX1_2 	: {return "MMX0_2";}
		case IR_REG_MMX2_1 	: {return "MMX1_1";}
		case IR_REG_MMX2_2 	: {return "MMX1_2";}
		case IR_REG_MMX3_1 	: {return "MMX2_1";}
		case IR_REG_MMX3_2 	: {return "MMX2_2";}
		case IR_REG_MMX4_1 	: {return "MMX3_1";}
		case IR_REG_MMX4_2 	: {return "MMX3_2";}
		case IR_REG_MMX5_1 	: {return "MMX4_1";}
		case IR_REG_MMX5_2 	: {return "MMX4_2";}
		case IR_REG_MMX6_1 	: {return "MMX5_1";}
		case IR_REG_MMX6_2 	: {return "MMX5_2";}
		case IR_REG_MMX7_1 	: {return "MMX6_1";}
		case IR_REG_MMX7_2 	: {return "MMX6_2";}
		case IR_REG_MMX8_1 	: {return "MMX7_1";}
		case IR_REG_MMX8_2 	: {return "MMX7_2";}
		case IR_REG_TMP 	: {return "??";}
	}

	return NULL;
}