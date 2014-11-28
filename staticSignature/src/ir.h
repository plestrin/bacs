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
	IR_IMUL 	= 4,
	IR_LEA 		= 5, 	/* importer */
	IR_MOV 		= 6, 	/* importer */
	IR_MOVZX 	= 7,
	IR_MUL 		= 8,
	IR_NOT 		= 9,
	IR_OR 		= 10,
	IR_PART1_8 	= 11, 	/* specific */
	IR_PART2_8 	= 12, 	/* specific */
	IR_PART1_16 = 13, 	/* specific */
	IR_ROL 		= 14,
	IR_ROR 		= 15,
	IR_SHL 		= 16,
	IR_SHR 		= 17,
	IR_SUB 		= 18,
	IR_XOR 		= 19,
	IR_LOAD 	= 20, 	/* signature */
	IR_STORE 	= 21, 	/* signature */
	IR_JOKER 	= 22, 	/* signature */
	IR_INVALID 	= 23 	/* specific */
};

#define NB_IR_OPCODE 24

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
	IR_REG_EBP 		= 17,
	IR_REG_ESI 		= 18,
	IR_REG_EDI 		= 19
};

#define NB_IR_REGISTER 20

char* irRegister_2_string(enum irRegister reg);
uint8_t irRegister_get_size(enum irRegister reg);

enum irOperationType{
	IR_OPERATION_TYPE_IN_REG,
	IR_OPERATION_TYPE_IN_MEM,
	IR_OPERATION_TYPE_OUT_MEM,
	IR_OPERATION_TYPE_IMM,
	IR_OPERATION_TYPE_INST,
	IR_OPERATION_TYPE_SYMBOL
};

#define IR_NODE_STATUS_FLAG_NONE 	0x00000000
#define IR_NODE_STATUS_FLAG_FINAL 	0x00000001

struct irOperation{
	enum irOperationType 		type;
	union {
		struct {
			enum irRegister 	reg;
		} 						in_reg;
		struct {
			uint32_t 			order;
		} 						in_mem;
		struct {
			uint32_t 			order;
		} 						out_mem;
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
	uint32_t 					status_flag;
} __attribute__((__may_alias__));

#define ir_node_get_operation(node) 	((struct irOperation*)&((node)->data))

#define ir_imm_operation_get_signed_value(op) 		((int64_t)((op)->operation_type.imm.value & (0xffffffffffffffffULL >> (64 - (op)->size))))
#define ir_imm_operation_get_unsigned_value(op) 	((op)->operation_type.imm.value & (0xffffffffffffffffULL >> (64 - (op)->size)))

int32_t irOperation_equal(const struct irOperation* op1, const  struct irOperation* op2);

enum irDependenceType{
	IR_DEPENDENCE_TYPE_DIRECT 		= 0x00000000,
	IR_DEPENDENCE_TYPE_ADDRESS 		= 0x00000001,
	IR_DEPENDENCE_TYPE_SHIFT_DISP 	= 0x00000002,
	IR_DEPENDENCE_TYPE_MACRO 		= 0x00000003
};

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

struct node* ir_add_in_reg(struct ir* ir, enum irRegister reg);
struct node* ir_add_in_mem(struct ir* ir, struct node* address, uint8_t size, uint32_t order);
struct node* ir_add_out_mem(struct ir* ir, struct node* address, uint8_t size, uint32_t order);
struct node* ir_add_immediate(struct ir* ir, uint8_t size, uint64_t value);
struct node* ir_add_inst(struct ir* ir, enum irOpcode opcode, uint8_t size);
struct node* ir_add_symbol(struct ir* ir, void* ptr);

#define ir_convert_node_to_inst(node, opcode_, size_)						\
	ir_node_get_operation(node)->type = IR_OPERATION_TYPE_INST; 			\
	ir_node_get_operation(node)->operation_type.inst.opcode = opcode_; 		\
	ir_node_get_operation(node)->size = size_; 								\
	ir_node_get_operation(node)->status_flag = IR_NODE_STATUS_FLAG_NONE;


struct edge* ir_add_dependence(struct ir* ir, struct node* operation_src, struct node* operation_dst, enum irDependenceType type);
struct edge* ir_add_macro_dependence(struct ir* ir, struct node* operation_src, struct node* operation_dst, uint32_t desc);

void ir_remove_node(struct ir* ir, struct node* node);
void ir_remove_dependence(struct ir* ir, struct edge* edge);

#define ir_printDot(ir, filters) graphPrintDot_print(&((ir)->graph), NULL, filters, NULL)

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

#endif