#include <stdlib.h>
#include <stdio.h>

#include "codeSignature.h"
#include "result.h"
#include "base.h"

static void codeSignature_dotPrint_node(void* data, FILE* file, void* arg);
static void codeSignature_dotPrint_edge(void* data, FILE* file, void* arg);

void codeSignature_init(struct codeSignature* code_signature){
	struct node* 				node_cursor;
	struct codeSignatureNode* 	sig_node_cursor;

	graph_register_dotPrint_callback(&(code_signature->signature.graph), NULL, codeSignature_dotPrint_node, codeSignature_dotPrint_edge, NULL);

	code_signature->nb_parameter_in 	= 0;
	code_signature->nb_parameter_out 	= 0;
	code_signature->nb_frag_tot_in 		= 0;
	code_signature->nb_frag_tot_out 	= 0;

	for (node_cursor = graph_get_head_node(&(code_signature->signature.graph)); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
		sig_node_cursor = (struct codeSignatureNode*)node_get_data(node_cursor);
		if (sig_node_cursor->input_number > 0){
			code_signature->nb_frag_tot_in ++;
			if (sig_node_cursor->input_number > code_signature->nb_parameter_in){
				code_signature->nb_parameter_in = sig_node_cursor->input_number;
			}
		}
		if (sig_node_cursor->output_number > 0){
			code_signature->nb_frag_tot_out ++;
			if (sig_node_cursor->output_number > code_signature->nb_parameter_out){
				code_signature->nb_parameter_out = sig_node_cursor->output_number;
			}
		}
	}

	if (code_signature->nb_parameter_in == 0 || code_signature->nb_parameter_out == 0){
		log_warn_m("signature \"%s\" has an incorrect number of parameter", code_signature->signature.name);
	}
}

#pragma GCC diagnostic ignored "-Wunused-parameter"
static void codeSignature_dotPrint_node(void* data, FILE* file, void* arg){
	struct codeSignatureNode* node = (struct codeSignatureNode*)data;

	switch (node->type){
		case CODESIGNATURE_NODE_TYPE_OPCODE : {
			if (node->input_number > 0){
				fprintf(file, "[label=\"%s %u:%u\",shape=\"box\"]", irOpcode_2_string(node->node_type.opcode), node->input_number, node->input_frag_order);
			}
			else if (node->output_number > 0){
				fprintf(file, "[label=\"%s %u:%u\",shape=\"invhouse\"]", irOpcode_2_string(node->node_type.opcode), node->output_number, node->output_frag_order);
			}
			else{
				fprintf(file, "[label=\"%s\"]", irOpcode_2_string(node->node_type.opcode));
			}
			break;
		}
		case CODESIGNATURE_NODE_TYPE_SYMBOL : {
			if (node->input_number > 0){
				fprintf(file, "[label=\"%s %u:%u\",shape=\"box\"]", node->node_type.symbol->name, node->input_number, node->input_frag_order);
			}
			else if (node->output_number > 0){
				fprintf(file, "[label=\"%s %u:%u\",shape=\"invhouse\"]", node->node_type.symbol->name, node->output_number, node->output_frag_order);
			}
			else{
				fprintf(file, "[label=\"%s\"]", node->node_type.symbol->name);
			}
			break;
		}
		case CODESIGNATURE_NODE_TYPE_INVALID : {
			log_err("this case is not supposed to happen");
			break;
		}
	}
}

#pragma GCC diagnostic ignored "-Wunused-parameter"
static void codeSignature_dotPrint_edge(void* data, FILE* file, void* arg){
	struct codeSignatureEdge* edge = (struct codeSignatureEdge*)data;

	switch(edge->type){
		case IR_DEPENDENCE_TYPE_DIRECT 		: {
			break;
		}
		case IR_DEPENDENCE_TYPE_ADDRESS 	: {
			fprintf(file, "[label=\"@\"]");
			break;
		}
		case IR_DEPENDENCE_TYPE_SHIFT_DISP 	: {
			fprintf(file, "[label=\"disp\"]");
			break;
		}
		case IR_DEPENDENCE_TYPE_DIVISOR 	: {
			fprintf(file, "[label=\"/\"]");
			break;
		}
		case IR_DEPENDENCE_TYPE_ROUND_OFF 	:
		case IR_DEPENDENCE_TYPE_SUBSTITUTE 	: {
			fprintf(file, "[label=\"s\"]"); 		/* the s is used to tag special operands */
			break;
		}
		case IR_DEPENDENCE_TYPE_MACRO 		: {
			if (IR_DEPENDENCE_MACRO_DESC_IS_INPUT(edge->macro_desc)){
				fprintf(file, "[label=\"I%uF%u\"]", IR_DEPENDENCE_MACRO_DESC_GET_ARG(edge->macro_desc), IR_DEPENDENCE_MACRO_DESC_GET_FRAG(edge->macro_desc));
			}
			else{
				fprintf(file, "[label=\"O%uF%u\"]", IR_DEPENDENCE_MACRO_DESC_GET_ARG(edge->macro_desc), IR_DEPENDENCE_MACRO_DESC_GET_FRAG(edge->macro_desc));
			}
			break;
		}
	}
}

uint32_t irNode_get_label(struct node* node){
	struct irOperation* operation = ir_node_get_operation(node);

	switch (operation->type){
		case IR_OPERATION_TYPE_IN_REG 	: {
			return 0xfffffffd;
		}
		case IR_OPERATION_TYPE_IN_MEM 	: {
			return IR_LOAD;
		}
		case IR_OPERATION_TYPE_OUT_MEM 	: {
			return IR_STORE;
		}
		case IR_OPERATION_TYPE_INST 	: {
			return operation->operation_type.inst.opcode;
		}
		case IR_OPERATION_TYPE_IMM 		: {
			return 0xfffffffe;
		}
		case IR_OPERATION_TYPE_SYMBOL 	: {
			return 0x0000ffff | (((struct result*)operation->operation_type.symbol.result_ptr)->code_signature->signature.id << 16);
		}
	}

	log_err("this case is not supposed to happen");
	return 0;
}

uint32_t codeSignatureNode_get_label(struct node* node){
	struct codeSignatureNode* signature_node = (struct codeSignatureNode*)node_get_data(node);

	switch(signature_node->type){
		case CODESIGNATURE_NODE_TYPE_OPCODE : {
			if (signature_node->node_type.opcode == IR_JOKER){
				return SUBGRAPHISOMORPHISM_JOKER_LABEL;
			}
			else{
				return (uint32_t)signature_node->node_type.opcode;
			}
		}
		case CODESIGNATURE_NODE_TYPE_SYMBOL : {
			if (!signatureSymbol_is_resolved(signature_node->node_type.symbol)){
				log_warn_m("the current node is an unresolved symbol (%s) setting its label to joker", signature_node->node_type.symbol->name);
				return SUBGRAPHISOMORPHISM_JOKER_LABEL;
			}
			else{
				return (signatureSymbol_get_id(signature_node->node_type.symbol) << 16 ) | 0x0000ffff;
			}
		}
		case CODESIGNATURE_NODE_TYPE_INVALID : {
			log_err("this case is not supposed to happen");
			return SUBGRAPHISOMORPHISM_JOKER_LABEL;
		}
	}

	log_err("this case is not supposed to happen (incorrect signature node type)");
	return SUBGRAPHISOMORPHISM_JOKER_LABEL;
}

uint32_t irEdge_get_label(struct edge* edge){
	struct irDependence* dependence = ir_edge_get_dependence(edge);

	switch(dependence->type){
		case IR_DEPENDENCE_TYPE_DIRECT 		: {
			return IR_DEPENDENCE_TYPE_DIRECT;
		}
		case IR_DEPENDENCE_TYPE_ADDRESS 	: {
			return IR_DEPENDENCE_TYPE_ADDRESS;
		}
		case IR_DEPENDENCE_TYPE_SHIFT_DISP 	: {
			return IR_DEPENDENCE_TYPE_DIRECT;
		}
		case IR_DEPENDENCE_TYPE_DIVISOR 	: {
			return IR_DEPENDENCE_TYPE_DIRECT;
		}
		case IR_DEPENDENCE_TYPE_ROUND_OFF 	: {
			return IR_DEPENDENCE_TYPE_DIRECT;
		}
		case IR_DEPENDENCE_TYPE_SUBSTITUTE 	: {
			return IR_DEPENDENCE_TYPE_DIRECT;
		}
		case IR_DEPENDENCE_TYPE_MACRO 		: {
			return IR_DEPENDENCE_TYPE_MACRO | dependence->dependence_type.macro;
		}
	}

	return IR_DEPENDENCE_TYPE_DIRECT;
}

uint32_t codeSignatureEdge_get_label(struct edge* edge){
	struct codeSignatureEdge* signature_edge = (struct codeSignatureEdge*)edge_get_data(edge);

	return signature_edge->type | signature_edge->macro_desc;
}