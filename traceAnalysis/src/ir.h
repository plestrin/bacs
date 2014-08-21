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
	IR_BSWAP 	= 2,
	IR_DEC 		= 3, 	/* trace */
	IR_MOVZX 	= 4,
	IR_NOT 		= 5,
	IR_OR 		= 6,
	IR_PART1_8 	= 7, 	/* specific */
	IR_PART2_8 	= 8, 	/* specific */
	IR_PART1_16 = 9, 	/* specific */
	IR_ROL 		= 10,
	IR_ROR 		= 11,
	IR_SAR 		= 12,
	IR_SHL 		= 13,
	IR_SHR 		= 14,
	IR_SUB 		= 15,
	IR_XOR 		= 16,
	IR_INPUT 	= 17, 	/* signature */
	IR_JOKER 	= 18 	/* signature */
};

char* irOpcode_2_string(enum irOpcode opcode);

enum irOperationType{
	IR_OPERATION_TYPE_INPUT,
	IR_OPERATION_TYPE_OUTPUT,
	IR_OPERATION_TYPE_INNER,
	IR_OPERATION_TYPE_IMM
};

enum irDependenceType{
	IR_DEPENDENCE_TYPE_DIRECT,
	IR_DEPENDENCE_TYPE_BASE,
	IR_DEPENDENCE_TYPE_INDEX,
	IR_DEPENDENCE_TYPE_DISP
};

struct irOperation{
	enum irOperationType 		type;
	union {
		struct {
			struct operand* 	operand;
			struct node* 		next;
			struct node* 		prev;
		} 						input;
		struct {
			enum irOpcode 		opcode;
			struct operand* 	operand;
			struct node* 		next;
			struct node* 		prev;
		} 						output;
		struct {
			enum irOpcode 		opcode;
		} 						inner;
		struct {
			uint8_t 			signe;
			uint64_t 			value;
		} 						imm;
	} 							operation_type;
	uint8_t 					size;
	uint32_t 					data;
} __attribute__((__may_alias__));

#define ir_node_get_operation(node) 	((struct irOperation*)&((node)->data))

#define ir_imm_operation_get_signed_value(op) 		((int32_t)((op)->operation_type.imm.value & (0xffffffffffffffffULL >> (64 - (op)->size))))
#define ir_imm_operation_get_unsigned_value(op) 	((op)->operation_type.imm.value & (0xffffffffffffffffULL >> (64 - (op)->size)))

struct irDependence{
	enum irDependenceType 		type;
} __attribute__((__may_alias__));

#define ir_edge_get_dependence(edge) 	((struct irDependence*)&((edge)->data))

struct ir{
	struct trace* 				trace;
	struct graph 				graph;
	struct node* 				input_linkedList;
	struct node* 				output_linkedList;
};

enum irCreateMethod{
	IR_CREATE_TRACE,
	IR_CREATE_ASM
};

struct ir* ir_create(struct trace* trace, enum irCreateMethod create_method);
int32_t ir_init(struct ir* ir, struct trace* trace, enum irCreateMethod create_method);

struct node* ir_add_input(struct ir* ir, struct operand* operand, uint8_t size);
struct node* ir_add_output(struct ir* ir, enum irOpcode opcode, struct operand* operand, uint8_t size);
struct node* ir_add_immediate(struct ir* ir, uint8_t size, uint8_t signe, uint64_t value);
struct edge* ir_add_dependence(struct ir* ir, struct node* operation_src, struct node* operation_dst, enum irDependenceType type);

void ir_remove_node(struct ir* ir, struct node* node);

void ir_convert_output_to_inner(struct ir* ir, struct node* node);
void ir_convert_inner_to_output(struct ir* ir, struct node* node);
void ir_convert_input_to_inner(struct ir* ir, struct node* node, enum irOpcode opcode);

void ir_print_io(struct ir* ir);

#define ir_printDot(ir) graphPrintDot_print(&((ir)->graph), NULL, NULL)

#define ir_clean(ir) 													\
	graph_clean(&(ir->graph));											\

#define ir_delete(ir) 													\
	ir_clean(ir); 														\
	free(ir);

#include "irIOExtract.h"

#endif