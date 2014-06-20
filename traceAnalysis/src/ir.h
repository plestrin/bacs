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
	IR_DEC 		= 3, 	/* tmp */
	IR_MOVZX 	= 4,
	IR_NOT 		= 5,
	IR_OR 		= 6,
	IR_PART 	= 7,
	IR_ROL 		= 8,
	IR_ROR 		= 9,
	IR_SAR 		= 10,
	IR_SHL 		= 11,
	IR_SHR 		= 12,
	IR_SUB 		= 13,
	IR_XOR 		= 14
};

#define IR_NB_OPCODE 15

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

enum irCreateMethod{
	IR_CREATE_TRACE,
	IR_CREATE_ASM
};

struct ir* ir_create(struct trace* trace, enum irCreateMethod create_method);
int32_t ir_init(struct ir* ir, struct trace* trace, enum irCreateMethod create_method);

struct node* ir_add_input(struct ir* ir, struct operand* operand);
struct node* ir_add_output(struct ir* ir, enum irOpcode opcode, struct operand* operand);
struct edge* ir_add_dependence(struct ir* ir, struct node* operation_src, struct node* operation_dst, enum irDependenceType type);

void ir_convert_output_to_inner(struct ir* ir, struct node* node);
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