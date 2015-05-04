#ifndef IR_H
#define IR_H

#include <stdint.h>

#include "assembly.h"
#include "graph.h"
#include "graphPrintDot.h"

enum irOpcode{
	IR_ADD 		= 0,
	IR_AND 		= 1,
	IR_CMOV 	= 2, 	/* temp */
	IR_DIV 		= 3,
	IR_IDIV 	= 4,
	IR_IMUL 	= 5,
	IR_LEA 		= 6, 	/* importer */
	IR_MOV 		= 7, 	/* importer */
	IR_MOVZX 	= 8,
	IR_MUL 		= 9,
	IR_NOT 		= 10,
	IR_OR 		= 11,
	IR_PART1_8 	= 12, 	/* specific */
	IR_PART2_8 	= 13, 	/* specific */
	IR_PART1_16 = 14, 	/* specific */
	IR_ROL 		= 15,
	IR_ROR 		= 16,
	IR_SHL 		= 17,
	IR_SHLD 	= 18,
	IR_SHR 		= 19,
	IR_SHRD 	= 20,
	IR_SUB 		= 21,
	IR_XOR 		= 22,
	IR_LOAD 	= 23, 	/* signature */
	IR_STORE 	= 24, 	/* signature */
	IR_JOKER 	= 25, 	/* signature */
	IR_INVALID 	= 26 	/* specific */
};

#define NB_IR_OPCODE 27 /* after updating this value, please grep in the code on NB_IR_OPCODE because a lot of static arrays depend on this value */

char* irOpcode_2_string(enum irOpcode opcode);

enum irRegister{
	IR_REG_EAX 		= 0,
	IR_REG_AX 		= 1,
	IR_REG_AH 		= 2,
	IR_REG_AL 		= 3,
	IR_REG_EBX 		= 4,
	IR_REG_BX 		= 5,
	IR_REG_BH 		= 6,
	IR_REG_BL 		= 7,
	IR_REG_ECX 		= 8,
	IR_REG_CX 		= 9,
	IR_REG_CH 		= 10,
	IR_REG_CL 		= 11,
	IR_REG_EDX 		= 12,
	IR_REG_DX 		= 13,
	IR_REG_DH 		= 14,
	IR_REG_DL 		= 15,
	IR_REG_ESP 		= 16,
	IR_REG_SP 		= 17,
	IR_REG_EBP 		= 18,
	IR_REG_BP 		= 19,
	IR_REG_ESI 		= 20,
	IR_REG_SI 		= 21,
	IR_REG_EDI 		= 22,
	IR_REG_DI 		= 23
};

#define NB_IR_REGISTER 24

char* irRegister_2_string(enum irRegister reg);

enum irOperationType{
	IR_OPERATION_TYPE_IN_REG,
	IR_OPERATION_TYPE_IN_MEM,
	IR_OPERATION_TYPE_OUT_MEM,
	IR_OPERATION_TYPE_IMM,
	IR_OPERATION_TYPE_INST,
	IR_OPERATION_TYPE_SYMBOL
};

struct irMemAccess{
	struct node* 	prev;
	struct node* 	next;
	uint32_t 		order;
};

#define IR_NODE_STATUS_FLAG_NONE 	0x00000000
#define IR_NODE_STATUS_FLAG_FINAL 	0x00000001
#define IR_NODE_STATUS_FLAG_ERROR 	0x40000000
#define IR_NODE_STATUS_FLAG_TEST 	0x80000000

#define IR_INSTRUCTION_INDEX_ADDRESS 	0xfffffffd
#define IR_INSTRUCTION_INDEX_IMMEDIATE 	0xfffffffe
#define IR_INSTRUCTION_INDEX_UNKOWN 	0xffffffff

struct irOperation{
	enum irOperationType 		type;
	union {
		struct {
			enum irRegister 	reg;
		} 						in_reg;
		struct {
			struct irMemAccess 	access;
		} 						mem;
		struct {
			uint64_t 			value;
		} 						imm;
		struct {
			enum irOpcode 		opcode;
		} 						inst;
		struct {
			void* 				ptr;
		} 						symbol;
	} 							operation_type;
	uint8_t 					size;
	uint32_t 					index;
	uint32_t 					status_flag;
} __attribute__((__may_alias__));

#define ir_node_get_operation(node) 	((struct irOperation*)&((node)->data))

#define ir_imm_operation_get_signed_value(op) 		((int64_t)((op)->operation_type.imm.value & (0xffffffffffffffffULL >> (64 - (op)->size))))
#define ir_imm_operation_get_unsigned_value(op) 	((op)->operation_type.imm.value & (0xffffffffffffffffULL >> (64 - (op)->size)))

#define ir_mem_get_next(op) ((op).operation_type.mem.access.next);
#define ir_mem_get_prev(op) ((op).operation_type.mem.access.prev);

static inline void ir_mem_remove(struct irOperation* operation){
	if (operation->operation_type.mem.access.next != NULL){
		ir_node_get_operation(operation->operation_type.mem.access.next)->operation_type.mem.access.prev = operation->operation_type.mem.access.prev;
	}
	if (operation->operation_type.mem.access.prev != NULL){
		ir_node_get_operation(operation->operation_type.mem.access.prev)->operation_type.mem.access.next = operation->operation_type.mem.access.next;
	}
}

enum irDependenceType{
	IR_DEPENDENCE_TYPE_DIRECT 		= 0x00000000, 	/* 1  Default dependence type 											*/
	IR_DEPENDENCE_TYPE_ADDRESS 		= 0x00000001, 	/* 2  Address 															*/
	IR_DEPENDENCE_TYPE_SHIFT_DISP 	= 0x00000002, 	/* 3  Used for the last operand of {ROL, ROR, SHL, SHLD, SHR, SHLD} 	*/
	IR_DEPENDENCE_TYPE_DIVISOR 		= 0x00000003, 	/* 4  Used for the last operand of {DIV, IDIV} 							*/
	IR_DEPENDENCE_TYPE_ROUND_OFF 	= 0x00000004, 	/* 5  Used for the second operand of {SHLD, SHRD} 						*/
	IR_DEPENDENCE_TYPE_SUBSTITUTE 	= 0x00000005, 	/* 6  Used for the last operand of {SUB} 								*/
	IR_DEPENDENCE_TYPE_MACRO 		= 0x00000006 	/* 7  Signature input and output parameters 							*/
};

#define NB_DEPENDENCE_TYPE 6

struct irDependence{
	enum irDependenceType 		type;
	union{
		uint32_t 				macro;
	} 							dependence_type;
} __attribute__((__may_alias__));

#define ir_edge_get_dependence(edge) 	((struct irDependence*)&((edge)->data))

/* Bit map description of the macro parameter (read the edge labeling prior to modify this mapping)
	- [0 :6 ] 	reserved
	- [7] 		0 for input and 1 for output
	- [8 :15] 	fragment index 0 to 255
	- [16:23] 	argument index 0 to 255
	- [24:31] 	reserved
*/

#define IR_DEPENDENCE_MACRO_DESC_SET_INPUT(nf, na) 		((((nf) & 0x000000ff) << 8) | (((na) & 0x000000ff) << 16))
#define IR_DEPENDENCE_MACRO_DESC_SET_OUTPUT(nf, na) 	((((nf) & 0x000000ff) << 8) | (((na) & 0x000000ff) << 16) | 0x00000080)
#define IR_DEPENDENCE_MACRO_DESC_IS_INPUT(desc) 		(((desc) & 0x00000080) == 0x00000000)
#define IR_DEPENDENCE_MACRO_DESC_IS_OUTPUT(desc) 		(((desc) & 0x00000080) == 0x00000080)
#define IR_DEPENDENCE_MACRO_DESC_GET_FRAG(desc) 		(((desc) >> 8) & 0x000000ff)
#define IR_DEPENDENCE_MACRO_DESC_GET_ARG(desc) 			(((desc) >> 16) & 0x00000ff)

struct ir{
	struct graph 				graph;
};

struct ir* ir_create(struct assembly* assembly);
int32_t ir_init(struct ir* ir, struct assembly* assembly);

struct node* ir_add_in_reg(struct ir* ir, uint32_t index, enum irRegister reg);
struct node* ir_add_in_mem(struct ir* ir, uint32_t index, uint8_t size, struct node* address, struct node* prev);
struct node* ir_add_out_mem(struct ir* ir, uint32_t index,  uint8_t size, struct node* address, struct node* prev);
struct node* ir_add_immediate(struct ir* ir, uint8_t size, uint64_t value);
struct node* ir_add_inst(struct ir* ir, uint32_t index, uint8_t size, enum irOpcode opcode);
struct node* ir_add_symbol(struct ir* ir, void* ptr);

struct node* ir_insert_immediate(struct ir* ir, struct node* root, uint8_t size, uint64_t value);
struct node* ir_insert_inst(struct ir* ir, struct node* root, uint32_t index, uint8_t size, enum irOpcode opcode);

static inline void ir_convert_node_to_inst(struct node* node, uint32_t index, uint8_t size, enum irOpcode opcode){
	struct irOperation* operation = ir_node_get_operation(node);

	if (operation->type == IR_OPERATION_TYPE_IN_MEM || operation->type == IR_OPERATION_TYPE_OUT_MEM){
		ir_mem_remove(operation);
	}

	operation->type = IR_OPERATION_TYPE_INST;
	operation->operation_type.inst.opcode = opcode;
	operation->size = size;
	operation->index = index;
	operation->status_flag = IR_NODE_STATUS_FLAG_NONE;
}

static inline void ir_convert_node_to_imm(struct node* node, uint8_t size, uint64_t value){
	struct irOperation* operation = ir_node_get_operation(node);

	if (operation->type == IR_OPERATION_TYPE_IN_MEM || operation->type == IR_OPERATION_TYPE_OUT_MEM){
		ir_mem_remove(operation);
	}

	operation->type = IR_OPERATION_TYPE_IMM;
	operation->operation_type.imm.value = value;
	operation->size = size;
	operation->index = IR_INSTRUCTION_INDEX_IMMEDIATE;
	operation->status_flag = IR_NODE_STATUS_FLAG_NONE;
}

struct edge* ir_add_dependence(struct ir* ir, struct node* operation_src, struct node* operation_dst, enum irDependenceType type);
struct edge* ir_add_macro_dependence(struct ir* ir, struct node* operation_src, struct node* operation_dst, uint32_t desc);

void ir_remove_node(struct ir* ir, struct node* node);
void ir_remove_dependence(struct ir* ir, struct edge* edge);

#define ir_printDot(ir, filters) graphPrintDot_print(&((ir)->graph), NULL, filters, NULL)

void ir_print_location_node(struct node* node, struct assembly* assembly);

int32_t ir_printDot_filter_macro_node(struct node* node, void* arg);
int32_t ir_printDot_filter_macro_edge(struct edge* edge, void* arg);

void ir_dotPrint_node(void* data, FILE* file, void* arg);
void ir_dotPrint_edge(void* data, FILE* file, void* arg);

#define ir_clean(ir) 														\
	graph_clean(&(ir->graph));												\

#define ir_delete(ir) 														\
	ir_clean(ir); 															\
	free(ir);

#include "irNormalize.h"
#include "irCheck.h"
#include "irVariableSize.h"

#endif