#ifndef IR_H
#define IR_H

#include <stdint.h>

#include "instruction.h"
#include "trace.h"
#include "graph.h"
#include "graphPrintDot.h"

enum irOpcode{
	IR_ADD 		= 0,
	IR_AND 		= 1,
	IR_MOVZX 	= 2,
	IR_MUL 		= 3,
	IR_NOT 		= 4,
	IR_OR 		= 5,
	IR_PART1_8 	= 6, 	/* specific */
	IR_PART2_8 	= 7, 	/* specific */
	IR_PART1_16 = 8, 	/* specific */
	IR_ROL 		= 9,
	IR_ROR 		= 10,
	IR_SAR 		= 11,
	IR_SHL 		= 12,
	IR_SHR 		= 13,
	IR_SUB 		= 14,
	IR_XOR 		= 15,
	IR_INPUT 	= 16, 	/* signature */
	IR_JOKER 	= 17 	/* signature */
};

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
	IR_OPERATION_TYPE_INST
};

enum irDependenceType{ /* a reprendre */
	IR_DEPENDENCE_TYPE_DIRECT,
	IR_DEPENDENCE_TYPE_BASE,
	IR_DEPENDENCE_TYPE_INDEX,
	IR_DEPENDENCE_TYPE_DISP,
	IR_DEPENDENCE_TYPE_ADDRESS
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
			uint8_t 			signe;
			uint64_t 			value;
		} 						imm;
		struct {
			enum irOpcode 		opcode;
		} 						inst;
	} 							operation_type;
	uint8_t 					size;
	uint32_t 					data;
	uint32_t 					status_flag;
	/* maybe add stuff here */
} __attribute__((__may_alias__));

#define ir_node_get_operation(node) 	((struct irOperation*)&((node)->data))

#define ir_imm_operation_get_signed_value(op) 		((int32_t)((op)->operation_type.imm.value & (0xffffffffffffffffULL >> (64 - (op)->size))))
#define ir_imm_operation_get_unsigned_value(op) 	((op)->operation_type.imm.value & (0xffffffffffffffffULL >> (64 - (op)->size)))

int32_t irOperation_equal(const struct irOperation* op1, const  struct irOperation* op2);

struct irDependence{
	enum irDependenceType 		type;
} __attribute__((__may_alias__));

#define ir_edge_get_dependence(edge) 	((struct irDependence*)&((edge)->data))

struct ir{
	struct trace* 				trace;
	struct graph 				graph;
};

struct ir* ir_create(struct trace* trace);
int32_t ir_init(struct ir* ir, struct trace* trace);

struct node* ir_add_in_reg(struct ir* ir, enum irRegister reg);
struct node* ir_add_in_mem(struct ir* ir, struct node* address, uint8_t size, uint32_t order);
struct node* ir_add_out_mem(struct ir* ir, struct node* address, uint8_t size, uint32_t order);
struct node* ir_add_immediate(struct ir* ir, uint8_t size, uint8_t signe, uint64_t value);
struct node* ir_add_inst(struct ir* ir, enum irOpcode opcode, uint8_t size);

#define ir_convert_node_to_inst(node, opcode_, size_)						\
	ir_node_get_operation(node)->type = IR_OPERATION_TYPE_INST; 			\
	ir_node_get_operation(node)->operation_type.inst.opcode = opcode_; 		\
	ir_node_get_operation(node)->size = size_; 								\
	ir_node_get_operation(node)->status_flag = IR_NODE_STATUS_FLAG_NONE;


struct edge* ir_add_dependence(struct ir* ir, struct node* operation_src, struct node* operation_dst, enum irDependenceType type);

void ir_remove_node(struct ir* ir, struct node* node);

#define ir_printDot(ir) graphPrintDot_print(&((ir)->graph), NULL, NULL)

#define ir_clean(ir) 														\
	graph_clean(&(ir->graph));												\

#define ir_delete(ir) 														\
	ir_clean(ir); 															\
	free(ir);

#include "irNormalize.h"

#endif