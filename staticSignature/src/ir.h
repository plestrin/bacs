#ifndef IR_H
#define IR_H

#include <stdint.h>

#include "assembly.h"
#include "graph.h"
#include "graphPrintDot.h"

enum irOpcode{
	IR_ADD 		= 0,
	IR_AND 		= 1,
	IR_DIV 		= 2,
	IR_IMUL 	= 3,
	IR_LEA 		= 4, 	/* importer */
	IR_MOV 		= 5, 	/* importer */
	IR_MOVZX 	= 6,
	IR_MUL 		= 7,
	IR_NOT 		= 8,
	IR_OR 		= 9,
	IR_PART1_8 	= 10, 	/* specific */
	IR_PART2_8 	= 11, 	/* specific */
	IR_PART1_16 = 12, 	/* specific */
	IR_ROL 		= 13,
	IR_ROR 		= 14,
	IR_SHL 		= 15,
	IR_SHR 		= 16,
	IR_SUB 		= 17,
	IR_XOR 		= 18,
	IR_LOAD 	= 19, 	/* signature */
	IR_STORE 	= 20, 	/* signature */
	IR_JOKER 	= 21, 	/* signature */
	IR_INVALID 	= 22 	/* specific */
};

#define NB_IR_OPCODE 23

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

enum irDependenceType{
	IR_DEPENDENCE_TYPE_DIRECT 		= 0x00000000,
	IR_DEPENDENCE_TYPE_ADDRESS 		= 0x00000001,
	IR_DEPENDENCE_TYPE_I1F1 		= 0x00000002,
	IR_DEPENDENCE_TYPE_I1F2 		= 0x00000003,
	IR_DEPENDENCE_TYPE_I1F3 		= 0x00000004,
	IR_DEPENDENCE_TYPE_I1F4 		= 0x00000005,
	IR_DEPENDENCE_TYPE_I2F1 		= 0x00000006,
	IR_DEPENDENCE_TYPE_I2F2 		= 0x00000007,
	IR_DEPENDENCE_TYPE_I2F3 		= 0x00000008,
	IR_DEPENDENCE_TYPE_I2F4 		= 0x00000009,
	IR_DEPENDENCE_TYPE_I3F1 		= 0x0000000a,
	IR_DEPENDENCE_TYPE_I3F2 		= 0x0000000b,
	IR_DEPENDENCE_TYPE_I3F3 		= 0x0000000c,
	IR_DEPENDENCE_TYPE_I3F4 		= 0x0000000d,
	IR_DEPENDENCE_TYPE_I4F1 		= 0x0000000e,
	IR_DEPENDENCE_TYPE_I4F2 		= 0x0000000f,
	IR_DEPENDENCE_TYPE_I4F3 		= 0x00000010,
	IR_DEPENDENCE_TYPE_I4F4 		= 0x00000011,
	IR_DEPENDENCE_TYPE_I5F1 		= 0x00000012,
	IR_DEPENDENCE_TYPE_I5F2 		= 0x00000013,
	IR_DEPENDENCE_TYPE_I5F3 		= 0x00000014,
	IR_DEPENDENCE_TYPE_I5F4 		= 0x00000015,
	IR_DEPENDENCE_TYPE_O1F1 		= 0x00000016,
	IR_DEPENDENCE_TYPE_O1F2 		= 0x00000017,
	IR_DEPENDENCE_TYPE_O1F3 		= 0x00000018,
	IR_DEPENDENCE_TYPE_O1F4 		= 0x00000019,
	IR_DEPENDENCE_TYPE_O2F1 		= 0x0000001a,
	IR_DEPENDENCE_TYPE_O2F2 		= 0x0000001b,
	IR_DEPENDENCE_TYPE_O2F3 		= 0x0000001c,
	IR_DEPENDENCE_TYPE_O2F4 		= 0x0000001d,
	IR_DEPENDENCE_TYPE_O3F1 		= 0x0000001e,
	IR_DEPENDENCE_TYPE_O3F2 		= 0x0000001f,
	IR_DEPENDENCE_TYPE_O3F3 		= 0x00000020,
	IR_DEPENDENCE_TYPE_O3F4 		= 0x00000021,
	IR_DEPENDENCE_TYPE_O4F1 		= 0x00000022,
	IR_DEPENDENCE_TYPE_O4F2 		= 0x00000023,
	IR_DEPENDENCE_TYPE_O4F3 		= 0x00000024,
	IR_DEPENDENCE_TYPE_O4F4 		= 0x00000025,
	IR_DEPENDENCE_TYPE_SHIFT_DISP 	= 0x00000026
};

#define irDependenceType_iocustom_get

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

struct irDependence{
	enum irDependenceType 		type;
} __attribute__((__may_alias__));

#define ir_edge_get_dependence(edge) 	((struct irDependence*)&((edge)->data))

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

void ir_remove_node(struct ir* ir, struct node* node);
void ir_remove_dependence(struct ir* ir, struct edge* edge);

#define ir_printDot(ir) graphPrintDot_print(&((ir)->graph), NULL, NULL)

void ir_dotPrint_node(void* data, FILE* file, void* arg);
void ir_dotPrint_edge(void* data, FILE* file, void* arg);

#define ir_clean(ir) 														\
	graph_clean(&(ir->graph));												\

#define ir_delete(ir) 														\
	ir_clean(ir); 															\
	free(ir);

#include "irNormalize.h"

#endif