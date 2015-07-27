#ifndef IR_H
#define IR_H

#include <stdint.h>

#include "assembly.h"
#include "memTrace.h"
#include "graph.h"
#include "graphPrintDot.h"
#include "array.h"

#define IRLAYER

enum irOpcode{
	IR_ADD 		= 0,
	IR_AND 		= 1,
	IR_CMOV 	= 2, 	/* temp */
	IR_DIVQ 	= 3,
	IR_DIVR 	= 4,
	IR_IDIV 	= 5,
	IR_IMUL 	= 6,
	IR_LEA 		= 7, 	/* importer */
	IR_MOV 		= 8, 	/* importer */
	IR_MOVZX 	= 9,
	IR_MUL 		= 10,
	IR_NEG 		= 11,
	IR_NOT 		= 12,
	IR_OR 		= 13,
	IR_PART1_8 	= 14, 	/* specific */
	IR_PART2_8 	= 15, 	/* specific */
	IR_PART1_16 = 16, 	/* specific */
	IR_ROL 		= 17,
	IR_ROR 		= 18,
	IR_SHL 		= 19,
	IR_SHLD 	= 20,
	IR_SHR 		= 21,
	IR_SHRD 	= 22,
	IR_SUB 		= 23,
	IR_XOR 		= 24,
	IR_LOAD 	= 25, 	/* signature */
	IR_STORE 	= 26, 	/* signature */
	IR_JOKER 	= 27, 	/* signature */
	IR_INVALID 	= 28 	/* specific */
};

#define NB_IR_OPCODE 29 /* after updating this value, please grep in the code because a lot of static arrays depend on this value */

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
	IR_REG_DI 		= 23,
	IR_REG_XMM1_1 	= 24,
	IR_REG_XMM1_2 	= 25,
	IR_REG_XMM1_3 	= 26,
	IR_REG_XMM1_4 	= 27,
	IR_REG_XMM2_1 	= 28,
	IR_REG_XMM2_2 	= 29,
	IR_REG_XMM2_3 	= 30,
	IR_REG_XMM2_4 	= 31,
	IR_REG_XMM3_1 	= 32,
	IR_REG_XMM3_2 	= 33,
	IR_REG_XMM3_3 	= 34,
	IR_REG_XMM3_4 	= 35,
	IR_REG_XMM4_1 	= 36,
	IR_REG_XMM4_2 	= 37,
	IR_REG_XMM4_3 	= 38,
	IR_REG_XMM4_4 	= 39,
	IR_REG_XMM5_1 	= 40,
	IR_REG_XMM5_2 	= 41,
	IR_REG_XMM5_3 	= 42,
	IR_REG_XMM5_4 	= 43,
	IR_REG_XMM6_1 	= 44,
	IR_REG_XMM6_2 	= 45,
	IR_REG_XMM6_3 	= 46,
	IR_REG_XMM6_4 	= 47,
	IR_REG_XMM7_1 	= 48,
	IR_REG_XMM7_2 	= 49,
	IR_REG_XMM7_3 	= 50,
	IR_REG_XMM7_4 	= 51,
	IR_REG_XMM8_1 	= 52,
	IR_REG_XMM8_2 	= 53,
	IR_REG_XMM8_3 	= 54,
	IR_REG_XMM8_4 	= 55,
	IR_REG_XMM9_1 	= 56,
	IR_REG_XMM9_2 	= 57,
	IR_REG_XMM9_3 	= 58,
	IR_REG_XMM9_4 	= 59,
	IR_REG_XMM10_1 	= 60,
	IR_REG_XMM10_2 	= 61,
	IR_REG_XMM10_3 	= 62,
	IR_REG_XMM10_4 	= 63,
	IR_REG_XMM11_1 	= 64,
	IR_REG_XMM11_2 	= 65,
	IR_REG_XMM11_3 	= 66,
	IR_REG_XMM11_4 	= 67,
	IR_REG_XMM12_1 	= 68,
	IR_REG_XMM12_2 	= 69,
	IR_REG_XMM12_3 	= 70,
	IR_REG_XMM12_4 	= 71,
	IR_REG_XMM13_1 	= 72,
	IR_REG_XMM13_2 	= 73,
	IR_REG_XMM13_3 	= 74,
	IR_REG_XMM13_4 	= 75,
	IR_REG_XMM14_1 	= 76,
	IR_REG_XMM14_2 	= 77,
	IR_REG_XMM14_3 	= 78,
	IR_REG_XMM14_4 	= 79,
	IR_REG_XMM15_1 	= 80,
	IR_REG_XMM15_2 	= 81,
	IR_REG_XMM15_3 	= 82,
	IR_REG_XMM15_4 	= 83,
	IR_REG_XMM16_1 	= 84,
	IR_REG_XMM16_2 	= 85,
	IR_REG_XMM16_3 	= 86,
	IR_REG_XMM16_4 	= 87,
	IR_REG_MMX1_1 	= 88,
	IR_REG_MMX1_2 	= 89,
	IR_REG_MMX2_1 	= 90,
	IR_REG_MMX2_2 	= 91,
	IR_REG_MMX3_1 	= 92,
	IR_REG_MMX3_2 	= 93,
	IR_REG_MMX4_1 	= 94,
	IR_REG_MMX4_2 	= 95,
	IR_REG_MMX5_1 	= 96,
	IR_REG_MMX5_2 	= 97,
	IR_REG_MMX6_1 	= 98,
	IR_REG_MMX6_2 	= 99,
	IR_REG_MMX7_1 	= 100,
	IR_REG_MMX7_2 	= 101,
	IR_REG_MMX8_1 	= 102,
	IR_REG_MMX8_2 	= 103,
	IR_REG_TMP 		= 104 	/* importer */
};

#define NB_IR_REGISTER 105 /* after updating this value, please grep in the code because a lot of static arrays depend on this value */

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
	ADDRESS 		con_addr;
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
			void* 				result_ptr;
			int32_t 			index;
		} 						symbol;
	} 							operation_type;
	uint8_t 					size;
	uint32_t 					index;
	uint32_t 					status_flag;
} __attribute__((__may_alias__));

#define ir_node_get_operation(node) 	((struct irOperation*)&((node)->data))

#define ir_imm_operation_get_signed_value(op) 		((int64_t)(((op)->operation_type.imm.value & (0xffffffffffffffffULL >> (64 - (op)->size))) | ((((op)->operation_type.imm.value >> ((op)->size - 1)) & 0x0000000000000001ULL) ? (0xffffffffffffffffULL << (op)->size) : 0)))
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

#define NB_DEPENDENCE_TYPE 7

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
	struct graph graph;
};

struct ir* ir_create(struct assembly* assembly, struct memTrace* mem_trace);
int32_t ir_init(struct ir* ir, struct assembly* assembly, struct memTrace* mem_trace);

struct node* ir_add_in_reg(struct ir* ir, uint32_t index, enum irRegister reg);

struct node* ir_add_in_mem_(struct ir* ir, uint32_t index, uint8_t size, struct node* address, struct node* prev, ADDRESS concrete_address);
#define ir_add_in_mem(ir, index, size, address, prev) ir_add_in_mem_(ir, index, size, address, prev, MEMADDRESS_INVALID)

struct node* ir_add_out_mem_(struct ir* ir, uint32_t index, uint8_t size, struct node* address, struct node* prev, ADDRESS concrete_address);
#define ir_add_out_mem(ir, index, size, address, prev) ir_add_out_mem_(ir, index, size, address, prev, MEMADDRESS_INVALID)

struct node* ir_add_immediate(struct ir* ir, uint8_t size, uint64_t value);
struct node* ir_add_inst(struct ir* ir, uint32_t index, uint8_t size, enum irOpcode opcode);
struct node* ir_add_symbol(struct ir* ir, void* result_ptr, uint32_t index);

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

void ir_remove_footprint(struct ir* ir, struct node** node_buffer, uint32_t nb_node);

#define ir_printDot(ir) graphPrintDot_print(&((ir)->graph), NULL, NULL)

void ir_print_location_node(struct node* node, struct assembly* assembly);

int32_t ir_printDot_filter_macro_node(struct node* node, void* arg);
int32_t ir_printDot_filter_macro_edge(struct edge* edge, void* arg);

void ir_dotPrint_node(void* data, FILE* file, void* arg);
void ir_dotPrint_edge(void* data, FILE* file, void* arg);

#define ir_clean(ir) graph_clean(&(ir->graph));

#define ir_delete(ir) 											\
	ir_clean(ir); 												\
	free(ir);

#include "irNormalize.h"
#include "irMemory.h"
#include "irCheck.h"
#include "irVariableSize.h"

#endif