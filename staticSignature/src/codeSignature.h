#ifndef CODESIGNATURE_H
#define CODESIGNATURE_H

#include <stdint.h>

#include "ir.h"
#include "signatureCollection.h"

enum codeSignatureNodeType{
	CODESIGNATURE_NODE_TYPE_OPCODE,
	CODESIGNATURE_NODE_TYPE_SYMBOL,
	CODESIGNATURE_NODE_TYPE_INVALID
};

struct codeSignatureNode{
	enum codeSignatureNodeType 	type;
	union{
		enum irOpcode 			opcode;
		struct signatureSymbol* symbol;
	} 							node_type;
	uint16_t 					input_number;
	uint16_t 					input_frag_order;
	uint16_t 					output_number;
	uint16_t 					output_frag_order;
};

struct codeSignatureEdge{
	enum irDependenceType 		type;
	uint32_t 					macro_desc;
};

uint32_t codeSignatureNode_get_label(struct node* node);
uint32_t codeSignatureEdge_get_label(struct edge* edge);

uint32_t irNode_get_label(struct node* node);
uint32_t irEdge_get_label(struct edge* edge);

struct codeSignature{
	struct signature 	signature;
	uint32_t 			nb_parameter_in;
	uint32_t 			nb_parameter_out;
	uint32_t 			nb_frag_tot_in;
	uint32_t 			nb_frag_tot_out;
};

void codeSignature_init(struct codeSignature* code_signature);

#define signatureCollection_node_get_codeSignature(node) ((struct codeSignature*)node_get_data(node))

#endif