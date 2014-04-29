#ifndef IR_H
#define IR_H

#include <stdint.h>

#include "instruction.h"
#include "trace.h"
#include "argSet.h"
#include "graph.h"
#include "graphPrintDot.h"

enum irOpcode{
	IR_ADD,
	IR_AND,
	IR_BSWAP,
	IR_MOVZX,
	IR_NOT,
	IR_OR,
	IR_PART,
	IR_SAR,
	IR_SHL,
	IR_SHR,
	IR_SUB,
	IR_ROR,
	IR_XOR
};

char* irOpcode_2_string(enum irOpcode opcode);

enum irOperationType{
	IR_OPERATION_TYPE_INPUT,
	IR_OPERATION_TYPE_OUTPUT,
	IR_OPERATION_TYPE_INNER
};

enum irDependenceType{
	IR_DEPENDENCE_TYPE_DIRECT,
	IR_DEPENDENCE_TYPE_BASE,
	IR_DEPENDENCE_TYPE_INDEX
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
	} 							operation_type;
	uint32_t 					data;
} __attribute__((__may_alias__));

#define ir_node_get_operation(node) 	((struct irOperation*)&((node)->data))

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

struct ir* ir_create(struct trace* trace);
int32_t ir_init(struct ir* ir, struct trace* trace);

struct node* ir_add_input(struct ir* ir, struct operand* operand);
struct node* ir_add_output(struct ir* ir, enum irOpcode opcode, struct operand* operand);
struct edge* ir_add_dependence(struct ir* ir, struct node* operation_src, struct node* operation_dst, enum irDependenceType type);

void ir_convert_output_to_inner(struct ir* ir, struct node* node);
void ir_convert_input_to_inner(struct ir* ir, struct node* node, enum irOpcode opcode);

void ir_print_io(struct ir* ir);

void ir_extract_arg(struct ir* ir, struct argSet* set);

#define ir_printDot(ir) graphPrintDot_print(&((ir)->graph), NULL)

#define ir_clean(ir) 													\
	graph_clean(&(ir->graph));											\

#define ir_delete(ir) 													\
	ir_clean(ir); 														\
	free(ir);

#endif