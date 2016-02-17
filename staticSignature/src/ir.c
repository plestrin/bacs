#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "ir.h"
#include "irImporterAsm.h"
#include "irBuilder.h"
#include "codeSignature.h"
#include "base.h"

const uint32_t irRegister_simd_virt_base[4] = {0, 0, 8, 16};

struct ir* ir_create(const struct assembly* assembly, const struct memTrace* mem_trace){
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

int32_t ir_init(struct ir* ir, const struct assembly* assembly, const struct memTrace* mem_trace){
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

struct ir* ir_create_compound(const struct assembly* assembly, const struct memTrace* mem_trace, struct irComponent** ir_component_buffer, uint32_t nb_ir_component){
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

int32_t ir_init_compound(struct ir* ir, const struct assembly* assembly, const struct memTrace* mem_trace, struct irComponent** ir_component_buffer, uint32_t nb_ir_component){
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
		if (prev == NULL){
			operation->operation_type.mem.next 			= NULL;
			operation->operation_type.mem.order 		= 1;
		}
		else{
			operation->operation_type.mem.next 			= ir_node_get_operation(prev)->operation_type.mem.next;
			if (operation->operation_type.mem.next != NULL){
				ir_node_get_operation(operation->operation_type.mem.next)->operation_type.mem.prev = node;
			}
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
		if (prev == NULL){
			operation->operation_type.mem.next 			= NULL;
			operation->operation_type.mem.order 		= 1;
		}
		else{
			operation->operation_type.mem.next 			= ir_node_get_operation(prev)->operation_type.mem.next;
			if (operation->operation_type.mem.next != NULL){
				ir_node_get_operation(operation->operation_type.mem.next)->operation_type.mem.prev = node;
			}
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

struct edge* ir_add_dependence(struct ir* ir, struct node* node_src, struct node* node_dst, enum irDependenceType type){
	struct edge* 			edge;
	struct irDependence* 	dependence;

	edge = graph_add_edge_(&(ir->graph), node_src, node_dst);
	if (edge == NULL){
		log_err("unable to add edge to the graph");
	}
	else{
		dependence = ir_edge_get_dependence(edge);
		dependence->type 					= type;
	}

	return edge;
}

struct edge* ir_add_macro_dependence(struct ir* ir, struct node* node_src, struct node* node_dst, uint32_t desc){
	struct edge* 			edge;
	struct irDependence* 	dependence;

	edge = graph_add_edge_(&(ir->graph), node_src, node_dst);
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

void ir_merge_equivalent_node(struct ir* ir, struct node* node_dst, struct node* node_src){
	graph_transfert_src_edge(&(ir->graph), node_dst, node_src);
	if (ir_node_get_operation(node_src)->status_flag & IR_OPERATION_STATUS_FLAG_FINAL){
		irBuilder_chg_final_node(&(ir->builder), node_src, node_dst);
	}
	ir_remove_node(ir, node_src);
}

void ir_remove_node(struct ir* ir, struct node* node){
	struct edge* 	edge_curr;
	struct edge* 	edge_next;

	if (ir_node_get_operation(node)->type == IR_OPERATION_TYPE_IN_MEM || ir_node_get_operation(node)->type == IR_OPERATION_TYPE_OUT_MEM){
		ir_mem_remove(ir_node_get_operation(node));
	}

	if (ir_node_get_operation(node)->status_flag & IR_OPERATION_STATUS_FLAG_FINAL){
		irBuilder_del_final_node(&(ir->builder), node);
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
			irBuilder_del_final_node(&(ir->builder), node_buffer[i]);
		}

		graph_remove_node(&(ir->graph), node_buffer[i]);
	}
}

void ir_remove_dependence(struct ir* ir, struct edge* edge){
	struct node* node_src;

	node_src = edge_get_src(edge);
	graph_remove_edge(&(ir->graph), edge);
	if (!(ir_node_get_operation(node_src)->status_flag & IR_OPERATION_STATUS_FLAG_FINAL) && node_src->nb_edge_src == 0){
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
		case IR_OPERATION_INDEX_UNKOWN 			: {
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

const char* irOpcode_2_string(enum irOpcode opcode){
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

static const char irRegister_name_std[NB_IR_STD_REGISTER][4] = {
	"EAX",
	"AX",
	"AH",
	"AL",
	"EBX",
	"BX",
	"BH",
	"BL",
	"ECX",
	"CX",
	"CH",
	"CL",
	"EDX",
	"DX",
	"DH",
	"DL",
	"ESP",
	"SP",
	"EBP",
	"BP",
	"ESI",
	"SI",
	"EDI",
	"DI",
	"??"
};

static const char irRegister_name_mmx[NB_IR_MMX_REGISTER][3][8][10] = {
	{
		{"MMX0_8_0", "MMX0_8_1", "MMX0_8_2", "MMX0_8_3", "MMX0_8_4", "MMX0_8_5", "MMX0_8_6", "MMX0_8_7"},
		{"MMX0_16_0", "MMX0_16_1", "MMX0_16_2", "MMX0_16_3"},
		{"MMX0_32_0", "MMX0_32_1"}
	},
	{
		{"MMX1_8_0", "MMX1_8_1", "MMX1_8_2", "MMX1_8_3", "MMX1_8_4", "MMX1_8_5", "MMX1_8_6", "MMX1_8_7"},
		{"MMX1_16_0", "MMX1_16_1", "MMX1_16_2", "MMX1_16_3"},
		{"MMX1_32_0", "MMX1_32_1"}
	},
	{
		{"MMX2_8_0", "MMX2_8_1", "MMX2_8_2", "MMX2_8_3", "MMX2_8_4", "MMX2_8_5", "MMX2_8_6", "MMX2_8_7"},
		{"MMX2_16_0", "MMX2_16_1", "MMX2_16_2", "MMX2_16_3"},
		{"MMX2_32_0", "MMX2_32_1"}
	},
	{
		{"MMX3_8_0", "MMX3_8_1", "MMX3_8_2", "MMX3_8_3", "MMX3_8_4", "MMX3_8_5", "MMX3_8_6", "MMX3_8_7"},
		{"MMX3_16_0", "MMX3_16_1", "MMX3_16_2", "MMX3_16_3"},
		{"MMX3_32_0", "MMX3_32_1"}
	},
	{
		{"MMX4_8_0", "MMX4_8_1", "MMX4_8_2", "MMX4_8_3", "MMX4_8_4", "MMX4_8_5", "MMX4_8_6", "MMX4_8_7"},
		{"MMX4_16_0", "MMX4_16_1", "MMX4_16_2", "MMX4_16_3"},
		{"MMX4_32_0", "MMX4_32_1"}
	},
	{
		{"MMX5_8_0", "MMX5_8_1", "MMX5_8_2", "MMX5_8_3", "MMX5_8_4", "MMX5_8_5", "MMX5_8_6", "MMX5_8_7"},
		{"MMX5_16_0", "MMX5_16_1", "MMX5_16_2", "MMX5_16_3"},
		{"MMX5_32_0", "MMX5_32_1"}
	},
	{
		{"MMX6_8_0", "MMX6_8_1", "MMX6_8_2", "MMX6_8_3", "MMX6_8_4", "MMX6_8_5", "MMX6_8_6", "MMX6_8_7"},
		{"MMX6_16_0", "MMX6_16_1", "MMX6_16_2", "MMX6_16_3"},
		{"MMX6_32_0", "MMX6_32_1"}
	},
	{
		{"MMX7_8_0", "MMX7_8_1", "MMX7_8_2", "MMX7_8_3", "MMX7_8_4", "MMX7_8_5", "MMX7_8_6", "MMX7_8_7"},
		{"MMX7_16_0", "MMX7_16_1", "MMX7_16_2", "MMX7_16_3"},
		{"MMX7_32_0", "MMX7_32_1"}
	}
};

static const char irRegister_name_xmm[NB_IR_XMM_REGISTER][3][16][10] = {
	{
		{"XMM0_8_0", "XMM0_8_1", "XMM0_8_2", "XMM0_8_3", "XMM0_8_4", "XMM0_8_5", "XMM0_8_6", "XMM0_8_7", "XMM0_8_8", "XMM0_8_9", "XMM0_8_10", "XMM0_8_11", "XMM0_8_12", "XMM0_8_13", "XMM0_8_14", "XMM0_8_15"},
		{"XMM0_16_0", "XMM0_16_1", "XMM0_16_2", "XMM0_16_3", "XMM0_16_4", "XMM0_16_5", "XMM0_16_6", "XMM0_16_7"},
		{"XMM0_32_0", "XMM0_32_1", "XMM0_32_2", "XMM0_32_3"}
	},
	{
		{"XMM1_8_0", "XMM1_8_1", "XMM1_8_2", "XMM1_8_3", "XMM1_8_4", "XMM1_8_5", "XMM1_8_6", "XMM1_8_7", "XMM1_8_8", "XMM1_8_9", "XMM1_8_10", "XMM1_8_11", "XMM1_8_12", "XMM1_8_13", "XMM1_8_14", "XMM1_8_15"},
		{"XMM1_16_0", "XMM1_16_1", "XMM1_16_2", "XMM1_16_3", "XMM1_16_4", "XMM1_16_5", "XMM1_16_6", "XMM1_16_7"},
		{"XMM1_32_0", "XMM1_32_1", "XMM1_32_2", "XMM1_32_3"}
	},
	{
		{"XMM2_8_0", "XMM2_8_1", "XMM2_8_2", "XMM2_8_3", "XMM2_8_4", "XMM2_8_5", "XMM2_8_6", "XMM2_8_7", "XMM2_8_8", "XMM2_8_9", "XMM2_8_10", "XMM2_8_11", "XMM2_8_12", "XMM2_8_13", "XMM2_8_14", "XMM2_8_15"},
		{"XMM2_16_0", "XMM2_16_1", "XMM2_16_2", "XMM2_16_3", "XMM2_16_4", "XMM2_16_5", "XMM2_16_6", "XMM2_16_7"},
		{"XMM2_32_0", "XMM2_32_1", "XMM2_32_2", "XMM2_32_3"}
	},
	{
		{"XMM3_8_0", "XMM3_8_1", "XMM3_8_2", "XMM3_8_3", "XMM3_8_4", "XMM3_8_5", "XMM3_8_6", "XMM3_8_7", "XMM3_8_8", "XMM3_8_9", "XMM3_8_10", "XMM3_8_11", "XMM3_8_12", "XMM3_8_13", "XMM3_8_14", "XMM3_8_15"},
		{"XMM3_16_0", "XMM3_16_1", "XMM3_16_2", "XMM3_16_3", "XMM3_16_4", "XMM3_16_5", "XMM3_16_6", "XMM3_16_7"},
		{"XMM3_32_0", "XMM3_32_1", "XMM3_32_2", "XMM3_32_3"}
	},
	{
		{"XMM4_8_0", "XMM4_8_1", "XMM4_8_2", "XMM4_8_3", "XMM4_8_4", "XMM4_8_5", "XMM4_8_6", "XMM4_8_7", "XMM4_8_8", "XMM4_8_9", "XMM4_8_10", "XMM4_8_11", "XMM4_8_12", "XMM4_8_13", "XMM4_8_14", "XMM4_8_15"},
		{"XMM4_16_0", "XMM4_16_1", "XMM4_16_2", "XMM4_16_3", "XMM4_16_4", "XMM4_16_5", "XMM4_16_6", "XMM4_16_7"},
		{"XMM4_32_0", "XMM4_32_1", "XMM4_32_2", "XMM4_32_3"}
	},
	{
		{"XMM5_8_0", "XMM5_8_1", "XMM5_8_2", "XMM5_8_3", "XMM5_8_4", "XMM5_8_5", "XMM5_8_6", "XMM5_8_7", "XMM5_8_8", "XMM5_8_9", "XMM5_8_10", "XMM5_8_11", "XMM5_8_12", "XMM5_8_13", "XMM5_8_14", "XMM5_8_15"},
		{"XMM5_16_0", "XMM5_16_1", "XMM5_16_2", "XMM5_16_3", "XMM5_16_4", "XMM5_16_5", "XMM5_16_6", "XMM5_16_7"},
		{"XMM5_32_0", "XMM5_32_1", "XMM5_32_2", "XMM5_32_3"}
	},
	{
		{"XMM6_8_0", "XMM6_8_1", "XMM6_8_2", "XMM6_8_3", "XMM6_8_4", "XMM6_8_5", "XMM6_8_6", "XMM6_8_7", "XMM6_8_8", "XMM6_8_9", "XMM6_8_10", "XMM6_8_11", "XMM6_8_12", "XMM6_8_13", "XMM6_8_14", "XMM6_8_15"},
		{"XMM6_16_0", "XMM6_16_1", "XMM6_16_2", "XMM6_16_3", "XMM6_16_4", "XMM6_16_5", "XMM6_16_6", "XMM6_16_7"},
		{"XMM6_32_0", "XMM6_32_1", "XMM6_32_2", "XMM6_32_3"}
	},
	{
		{"XMM7_8_0", "XMM7_8_1", "XMM7_8_2", "XMM7_8_3", "XMM7_8_4", "XMM7_8_5", "XMM7_8_6", "XMM7_8_7", "XMM7_8_8", "XMM7_8_9", "XMM7_8_10", "XMM7_8_11", "XMM7_8_12", "XMM7_8_13", "XMM7_8_14", "XMM7_8_15"},
		{"XMM7_16_0", "XMM7_16_1", "XMM7_16_2", "XMM7_16_3", "XMM7_16_4", "XMM7_16_5", "XMM7_16_6", "XMM7_16_7"},
		{"XMM7_32_0", "XMM7_32_1", "XMM7_32_2", "XMM7_32_3"}
	}
};

static const char irRegister_name_ymm[NB_IR_YMM_REGISTER][3][16][11] = {
	{
		{"YMM0_8_16", "YMM0_8_17", "YMM0_8_18", "YMM0_8_19", "YMM0_8_20", "YMM0_8_21", "YMM0_8_22", "YMM0_8_23", "YMM0_8_24", "YMM0_8_25", "YMM0_8_26", "YMM0_8_27", "YMM0_8_28", "YMM0_8_29", "YMM0_8_30", "YMM0_8_31"},
		{"YMM0_16_8", "YMM0_16_9", "YMM0_16_10", "YMM0_16_11", "YMM0_16_12", "YMM0_16_13", "YMM0_16_14", "YMM0_16_15"},
		{"YMM0_32_4", "YMM0_32_5", "YMM0_32_6", "YMM0_32_7"}
	},
	{
		{"YMM1_8_16", "YMM1_8_17", "YMM1_8_18", "YMM1_8_19", "YMM1_8_20", "YMM1_8_21", "YMM1_8_22", "YMM1_8_23", "YMM1_8_24", "YMM1_8_25", "YMM1_8_26", "YMM1_8_27", "YMM1_8_28", "YMM1_8_29", "YMM1_8_30", "YMM1_8_31"},
		{"YMM1_16_8", "YMM1_16_9", "YMM1_16_10", "YMM1_16_11", "YMM1_16_12", "YMM1_16_13", "YMM1_16_14", "YMM1_16_15"},
		{"YMM1_32_4", "YMM1_32_5", "YMM1_32_6", "YMM1_32_7"}
	},
	{
		{"YMM2_8_16", "YMM2_8_17", "YMM2_8_18", "YMM2_8_19", "YMM2_8_20", "YMM2_8_21", "YMM2_8_22", "YMM2_8_23", "YMM2_8_24", "YMM2_8_25", "YMM2_8_26", "YMM2_8_27", "YMM2_8_28", "YMM2_8_29", "YMM2_8_30", "YMM2_8_31"},
		{"YMM2_16_8", "YMM2_16_9", "YMM2_16_10", "YMM2_16_11", "YMM2_16_12", "YMM2_16_13", "YMM2_16_14", "YMM2_16_15"},
		{"YMM2_32_4", "YMM2_32_5", "YMM2_32_6", "YMM2_32_7"}
	},
	{
		{"YMM3_8_16", "YMM3_8_17", "YMM3_8_18", "YMM3_8_19", "YMM3_8_20", "YMM3_8_21", "YMM3_8_22", "YMM3_8_23", "YMM3_8_24", "YMM3_8_25", "YMM3_8_26", "YMM3_8_27", "YMM3_8_28", "YMM3_8_29", "YMM3_8_30", "YMM3_8_31"},
		{"YMM3_16_8", "YMM3_16_9", "YMM3_16_10", "YMM3_16_11", "YMM3_16_12", "YMM3_16_13", "YMM3_16_14", "YMM3_16_15"},
		{"YMM3_32_4", "YMM3_32_5", "YMM3_32_6", "YMM3_32_7"}
	},
	{
		{"YMM4_8_16", "YMM4_8_17", "YMM4_8_18", "YMM4_8_19", "YMM4_8_20", "YMM4_8_21", "YMM4_8_22", "YMM4_8_23", "YMM4_8_24", "YMM4_8_25", "YMM4_8_26", "YMM4_8_27", "YMM4_8_28", "YMM4_8_29", "YMM4_8_30", "YMM4_8_31"},
		{"YMM4_16_8", "YMM4_16_9", "YMM4_16_10", "YMM4_16_11", "YMM4_16_12", "YMM4_16_13", "YMM4_16_14", "YMM4_16_15"},
		{"YMM4_32_4", "YMM4_32_5", "YMM4_32_6", "YMM4_32_7"}
	},
	{
		{"YMM5_8_16", "YMM5_8_17", "YMM5_8_18", "YMM5_8_19", "YMM5_8_20", "YMM5_8_21", "YMM5_8_22", "YMM5_8_23", "YMM5_8_24", "YMM5_8_25", "YMM5_8_26", "YMM5_8_27", "YMM5_8_28", "YMM5_8_29", "YMM5_8_30", "YMM5_8_31"},
		{"YMM5_16_8", "YMM5_16_9", "YMM5_16_10", "YMM5_16_11", "YMM5_16_12", "YMM5_16_13", "YMM5_16_14", "YMM5_16_15"},
		{"YMM5_32_4", "YMM5_32_5", "YMM5_32_6", "YMM5_32_7"}
	},
	{
		{"YMM6_8_16", "YMM6_8_17", "YMM6_8_18", "YMM6_8_19", "YMM6_8_20", "YMM6_8_21", "YMM6_8_22", "YMM6_8_23", "YMM6_8_24", "YMM6_8_25", "YMM6_8_26", "YMM6_8_27", "YMM6_8_28", "YMM6_8_29", "YMM6_8_30", "YMM6_8_31"},
		{"YMM6_16_8", "YMM6_16_9", "YMM6_16_10", "YMM6_16_11", "YMM6_16_12", "YMM6_16_13", "YMM6_16_14", "YMM6_16_15"},
		{"YMM6_32_4", "YMM6_32_5", "YMM6_32_6", "YMM6_32_7"}
	},
	{
		{"YMM7_8_16", "YMM7_8_17", "YMM7_8_18", "YMM7_8_19", "YMM7_8_20", "YMM7_8_21", "YMM7_8_22", "YMM7_8_23", "YMM7_8_24", "YMM7_8_25", "YMM7_8_26", "YMM7_8_27", "YMM7_8_28", "YMM7_8_29", "YMM7_8_30", "YMM7_8_31"},
		{"YMM7_16_8", "YMM7_16_9", "YMM7_16_10", "YMM7_16_11", "YMM7_16_12", "YMM7_16_13", "YMM7_16_14", "YMM7_16_15"},
		{"YMM7_32_4", "YMM7_32_5", "YMM7_32_6", "YMM7_32_7"}
	}
};

const char* irRegister_2_string(enum irRegister reg){
	if (irRegister_is_std(reg)){
		return irRegister_name_std[reg];
	}
	else if (irRegister_is_mmx(reg)){
		switch(irRegister_simd_get_size(reg)){
			case 8  : {return irRegister_name_mmx[irRegister_simd_get_index(reg)][0][irRegister_simd_get_frag(reg)];}
			case 16 : {return irRegister_name_mmx[irRegister_simd_get_index(reg)][1][irRegister_simd_get_frag(reg)];}
			case 32 : {return irRegister_name_mmx[irRegister_simd_get_index(reg)][2][irRegister_simd_get_frag(reg)];}
			default : {break;}
		}
	}
	else if (irRegister_is_xmm(reg)){
		switch(irRegister_simd_get_size(reg)){
			case 8  : {return irRegister_name_xmm[irRegister_simd_get_index(reg)][0][irRegister_simd_get_frag(reg)];}
			case 16 : {return irRegister_name_xmm[irRegister_simd_get_index(reg)][1][irRegister_simd_get_frag(reg)];}
			case 32 : {return irRegister_name_xmm[irRegister_simd_get_index(reg)][2][irRegister_simd_get_frag(reg)];}
			default : {break;}
		}
	}
	else if (irRegister_is_ymm(reg)){
		switch(irRegister_simd_get_size(reg)){
			case 8  : {return irRegister_name_ymm[irRegister_simd_get_index(reg)][0][irRegister_simd_get_frag(reg)];}
			case 16 : {return irRegister_name_ymm[irRegister_simd_get_index(reg)][1][irRegister_simd_get_frag(reg)];}
			case 32 : {return irRegister_name_ymm[irRegister_simd_get_index(reg)][2][irRegister_simd_get_frag(reg)];}
			default : {break;}
		}
	}

	log_err_m("this register 0x%08x is not handled yet", reg);

	return NULL;
}