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

#define codeSignatureEdge irDependence

uint32_t codeSignatureNode_get_label(struct node* node);
#define codeSignatureEdge_get_label irEdge_get_label

uint32_t irNode_get_label(struct node* node);
uint32_t irEdge_get_label(struct edge* edge);

#define CODESIGNATURE_NB_PARA_MAX 32

struct codeSignature{
	struct signature 	signature;
	uint32_t 			nb_para_in;
	uint32_t 			nb_para_ou;
	uint32_t 			nb_frag_in[CODESIGNATURE_NB_PARA_MAX];
	uint32_t 			nb_frag_ou[CODESIGNATURE_NB_PARA_MAX];
};

#define signatureCollection_node_get_codeSignature(node) ((struct codeSignature*)node_get_data(node))

void codeSignature_init(struct codeSignature* code_signature);

static inline uint32_t codeSignature_get_nb_frag_in(const struct codeSignature* code_signature){
	uint32_t i;
	uint32_t result;

	for (i = 0, result = 0; i < code_signature->nb_para_in; i++){
		result += code_signature->nb_frag_in[i];
	}

	return result;
}

static inline uint32_t codeSignature_get_nb_frag_ou(const struct codeSignature* code_signature){
	uint32_t i;
	uint32_t result;

	for (i = 0, result = 0; i < code_signature->nb_para_ou; i++){
		result += code_signature->nb_frag_ou[i];
	}

	return result;
}

#define codeSignature_check_macro_dependence_in(code_signature, macro) (IR_DEPENDENCE_MACRO_DESC_IS_INPUT(macro) && IR_DEPENDENCE_MACRO_DESC_GET_ARG(macro) - 1 < (code_signature)->nb_para_in && IR_DEPENDENCE_MACRO_DESC_GET_FRAG(macro) - 1 < (code_signature)->nb_frag_in[IR_DEPENDENCE_MACRO_DESC_GET_ARG(macro) - 1])
#define codeSignature_check_macro_dependence_ou(code_signature, macro) (IR_DEPENDENCE_MACRO_DESC_IS_OUTPUT(macro) && IR_DEPENDENCE_MACRO_DESC_GET_ARG(macro) - 1 < (code_signature)->nb_para_ou && IR_DEPENDENCE_MACRO_DESC_GET_FRAG(macro) - 1 < (code_signature)->nb_frag_ou[IR_DEPENDENCE_MACRO_DESC_GET_ARG(macro) - 1])

#endif
