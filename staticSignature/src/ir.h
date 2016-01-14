#ifndef IR_H
#define IR_H

#include <stdint.h>

#include "assembly.h"
#include "memTrace.h"
#include "variableRange.h"
#include "graph.h"
#include "graphPrintDot.h"
#include "array.h"

#define IRLAYER

enum irOpcode{
	IR_ADC 		= 0,
	IR_ADD 		= 1,
	IR_AND 		= 2,
	IR_CMOV 	= 3, 	/* temp */
	IR_DIVQ 	= 4,
	IR_DIVR 	= 5,
	IR_IDIV 	= 6,
	IR_IMUL 	= 7,
	IR_LEA 		= 8, 	/* importer */
	IR_MOV 		= 9, 	/* importer */
	IR_MOVZX 	= 10,
	IR_MUL 		= 11,
	IR_NEG 		= 12,
	IR_NOT 		= 13,
	IR_OR 		= 14,
	IR_PART1_8 	= 15, 	/* specific */
	IR_PART2_8 	= 16, 	/* specific */
	IR_PART1_16 = 17, 	/* specific */
	IR_ROL 		= 18,
	IR_ROR 		= 19,
	IR_SHL 		= 20,
	IR_SHLD 	= 21,
	IR_SHR 		= 22,
	IR_SHRD 	= 23,
	IR_SUB 		= 24,
	IR_XOR 		= 25,
	IR_LOAD 	= 26, 	/* signature */
	IR_STORE 	= 27, 	/* signature */
	IR_JOKER 	= 28, 	/* signature */
	IR_INVALID 	= 29 	/* specific */
};

#define NB_IR_OPCODE 30 /* after updating this value, please grep in the code because a lot of static arrays depend on this value */

const char* irOpcode_2_string(enum irOpcode opcode);

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
	IR_REG_MMX1_1 	= 56,
	IR_REG_MMX1_2 	= 57,
	IR_REG_MMX2_1 	= 58,
	IR_REG_MMX2_2 	= 59,
	IR_REG_MMX3_1 	= 60,
	IR_REG_MMX3_2 	= 61,
	IR_REG_MMX4_1 	= 62,
	IR_REG_MMX4_2 	= 63,
	IR_REG_MMX5_1 	= 64,
	IR_REG_MMX5_2 	= 65,
	IR_REG_MMX6_1 	= 66,
	IR_REG_MMX6_2 	= 67,
	IR_REG_MMX7_1 	= 68,
	IR_REG_MMX7_2 	= 69,
	IR_REG_MMX8_1 	= 70,
	IR_REG_MMX8_2 	= 71,
	IR_REG_YMM1_5 	= 72,
	IR_REG_YMM1_6 	= 73,
	IR_REG_YMM1_7 	= 74,
	IR_REG_YMM1_8 	= 75,
	IR_REG_YMM2_5 	= 76,
	IR_REG_YMM2_6 	= 77,
	IR_REG_YMM2_7 	= 78,
	IR_REG_YMM2_8 	= 79,
	IR_REG_YMM3_5 	= 80,
	IR_REG_YMM3_6 	= 81,
	IR_REG_YMM3_7 	= 82,
	IR_REG_YMM3_8 	= 83,
	IR_REG_YMM4_5 	= 84,
	IR_REG_YMM4_6 	= 85,
	IR_REG_YMM4_7 	= 86,
	IR_REG_YMM4_8 	= 87,
	IR_REG_YMM5_5 	= 88,
	IR_REG_YMM5_6 	= 89,
	IR_REG_YMM5_7 	= 90,
	IR_REG_YMM5_8 	= 91,
	IR_REG_YMM6_5 	= 92,
	IR_REG_YMM6_6 	= 93,
	IR_REG_YMM6_7 	= 94,
	IR_REG_YMM6_8 	= 95,
	IR_REG_YMM7_5 	= 96,
	IR_REG_YMM7_6 	= 97,
	IR_REG_YMM7_7 	= 98,
	IR_REG_YMM7_8 	= 99,
	IR_REG_YMM8_5 	= 100,
	IR_REG_YMM8_6 	= 101,
	IR_REG_YMM8_7 	= 102,
	IR_REG_YMM8_8 	= 103,
	IR_REG_TMP 		= 104 	/* importer */
};

#define NB_IR_REGISTER 105 /* after updating this value, please grep in the code because a lot of static arrays depend on this value */

const char* irRegister_2_string(enum irRegister reg);

enum irOperationType{
	IR_OPERATION_TYPE_IN_REG,
	IR_OPERATION_TYPE_IN_MEM,
	IR_OPERATION_TYPE_OUT_MEM,
	IR_OPERATION_TYPE_IMM,
	IR_OPERATION_TYPE_INST,
	IR_OPERATION_TYPE_SYMBOL
};

#define IR_OPERATION_STATUS_FLAG_NONE 		0x00000000
#define IR_OPERATION_STATUS_FLAG_FINAL 		0x00000001
#define IR_OPERATION_STATUS_FLAG_ERROR 		0x80000000
#define IR_OPERATION_STATUS_FLAG_TEST 		0x40000000
#define IR_OPERATION_STATUS_FLAG_TESTING 	0x20000000

#define IR_OPERATION_INDEX_ADDRESS 			0xfffffffd
#define IR_OPERATION_INDEX_IMMEDIATE 		0xfffffffe
#define IR_OPERATION_INDEX_UNKOWN 			0xffffffff

#define IR_OPERATION_DST_UNKOWN 			0xffffffff

#define IR_IN_REG_IS_PRIMER 				0x00000001

struct irOperation{
	enum irOperationType 			type;
	union {
		struct {
			enum irRegister 		reg;
			uint32_t 				primer;
		} 							in_reg;
		struct {
			struct node* 			prev;
			struct node* 			next;
			uint32_t 				order;
			ADDRESS 				con_addr;
		} 							mem;
		struct {
			uint64_t 				value;
		} 							imm;
		struct {
			enum irOpcode 			opcode;
			uint32_t 				dst;
			struct variableRange	range;
			uint32_t 				seed;
		} 							inst;
		struct {
			void* 					code_signature;
			void* 					result;
			uint32_t 				index;
		} 							symbol;
	} 								operation_type;
	uint8_t 						size;
	uint32_t 						index;
	uint32_t 						status_flag;
};

#define ir_node_get_operation(node) 	((struct irOperation*)node_get_data(node))
#define irOperation_get_node(operation) (((struct node*)(operation)) - 1)

#define ir_imm_operation_get_signed_value(op) 		((int64_t)(((op)->operation_type.imm.value & (0xffffffffffffffffULL >> (64 - (op)->size))) | ((((op)->operation_type.imm.value >> ((op)->size - 1)) & 0x0000000000000001ULL) ? (0xffffffffffffffffULL << (op)->size) : 0)))
#define ir_imm_operation_get_unsigned_value(op) 	((op)->operation_type.imm.value & (0xffffffffffffffffULL >> (64 - (op)->size)))

#define ir_mem_get_next(op) ((op).operation_type.mem.next);
#define ir_mem_get_prev(op) ((op).operation_type.mem.prev);

static inline void ir_mem_remove(struct irOperation* operation){
	if (operation->operation_type.mem.next != NULL){
		ir_node_get_operation(operation->operation_type.mem.next)->operation_type.mem.prev = operation->operation_type.mem.prev;
	}
	if (operation->operation_type.mem.prev != NULL){
		ir_node_get_operation(operation->operation_type.mem.prev)->operation_type.mem.next = operation->operation_type.mem.next;
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
};

#define ir_edge_get_dependence(edge) 		((struct irDependence*)edge_get_data(edge))
#define irDependence_get_edge(dependence) 	(((struct edge*)(dependence)) - 1)

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

struct alias{
	struct node* 		ir_node;
	uint32_t 			order;
	uint8_t 			type;
};

#define IR_CALL_STACK_MAX_SIZE 	32
#define IR_CALL_STACK_PTR 		15 /* must be strictly smaller than IR_CALL_STACK_MAX_SIZE */

struct ir{
	struct graph 	graph;
	uint32_t 		range_seed;
	struct alias 	alias_buffer[NB_IR_REGISTER];
	uint32_t 		stack[IR_CALL_STACK_MAX_SIZE];
	uint32_t 		stack_ptr;
};

#define IR_INVALID_RANGE_SEED 0
#define ir_drop_range(ir) ((ir)->range_seed ++)

struct ir* ir_create(const struct assembly* assembly, struct memTrace* mem_trace);
int32_t ir_init(struct ir* ir, const struct assembly* assembly, struct memTrace* mem_trace);

struct irComponent{
	uint32_t 	instruction_start;
	uint32_t 	instruction_stop;
	struct ir*	ir;
};

struct ir* ir_create_compound(const struct assembly* assembly, struct memTrace* mem_trace, struct irComponent** ir_component_buffer, uint32_t nb_ir_component);
int32_t ir_init_compound(struct ir* ir, const struct assembly* assembly, struct memTrace* mem_trace, struct irComponent** ir_component_buffer, uint32_t nb_ir_component);

struct node* ir_add_in_reg(struct ir* ir, uint32_t index, enum irRegister reg, uint32_t primer);

struct node* ir_add_in_mem_(struct ir* ir, uint32_t index, uint8_t size, struct node* address, struct node* prev, ADDRESS concrete_address);
#define ir_add_in_mem(ir, index, size, address, prev) ir_add_in_mem_(ir, index, size, address, prev, MEMADDRESS_INVALID)

struct node* ir_add_out_mem_(struct ir* ir, uint32_t index, uint8_t size, struct node* address, struct node* prev, ADDRESS concrete_address);
#define ir_add_out_mem(ir, index, size, address, prev) ir_add_out_mem_(ir, index, size, address, prev, MEMADDRESS_INVALID)

struct node* ir_add_immediate(struct ir* ir, uint8_t size, uint64_t value);
struct node* ir_add_inst(struct ir* ir, uint32_t index, uint8_t size, enum irOpcode opcode, uint32_t dst);
struct node* ir_add_symbol(struct ir* ir, void* code_signature, void* result, uint32_t index);

struct node* ir_insert_immediate(struct ir* ir, struct node* root, uint8_t size, uint64_t value);
struct node* ir_insert_inst(struct ir* ir, struct node* root, uint32_t index, uint8_t size, enum irOpcode opcode, uint32_t dst);

static inline void ir_convert_node_to_inst(struct node* node, uint32_t index, uint8_t size, enum irOpcode opcode){
	struct irOperation* operation = ir_node_get_operation(node);

	if (operation->type == IR_OPERATION_TYPE_IN_MEM || operation->type == IR_OPERATION_TYPE_OUT_MEM){
		ir_mem_remove(operation);
	}

	operation->type 						= IR_OPERATION_TYPE_INST;
	operation->operation_type.inst.opcode 	= opcode;
	operation->operation_type.inst.dst 		= IR_OPERATION_DST_UNKOWN;
	operation->operation_type.inst.seed 	= IR_INVALID_RANGE_SEED;
	operation->size 						= size;
	operation->index 						= index;
	operation->status_flag 					= IR_OPERATION_STATUS_FLAG_NONE;
}

void ir_convert_node_to_imm(struct ir* ir, struct node* node, uint8_t size, uint64_t value);

struct edge* ir_add_dependence(struct ir* ir, struct node* operation_src, struct node* operation_dst, enum irDependenceType type);
struct edge* ir_add_macro_dependence(struct ir* ir, struct node* operation_src, struct node* operation_dst, uint32_t desc);

void ir_merge_equivalent_node(struct ir* ir, struct node* node_dst, struct node* node_src);

void ir_remove_node(struct ir* ir, struct node* node);
void ir_remove_dependence(struct ir* ir, struct edge* edge);

void ir_remove_footprint(struct ir* ir, struct node** node_buffer, uint32_t nb_node);

#define ir_printDot(ir) graphPrintDot_print(&((ir)->graph), NULL, NULL)

void ir_print_location_node(struct node* node, struct assembly* assembly);

void ir_print_node(struct irOperation* operation, FILE* file);
void ir_dotPrint_node(void* data, FILE* file, void* arg);
void ir_dotPrint_edge(void* data, FILE* file, void* arg);

#define ir_clean(ir) graph_clean(&((ir)->graph));

#define ir_delete(ir) 											\
	ir_clean(ir); 												\
	free(ir);

#include "irNormalize.h"
#include "irMemory.h"
#include "irCheck.h"
#include "irVariableSize.h"

#endif