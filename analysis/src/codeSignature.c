#include <stdlib.h>
#include <stdio.h>

#include "codeSignature.h"
#include "base.h"

static void codeSignature_dotPrint_node(void* data, FILE* file, void* arg);
#define codeSignature_dotPrint_edge ir_dotPrint_edge

void codeSignature_init(struct codeSignature* code_signature){
	struct node* 				node_cursor;
	struct codeSignatureNode* 	sig_node_cursor;

	graph_register_dotPrint_callback(&(code_signature->signature.graph), NULL, codeSignature_dotPrint_node, codeSignature_dotPrint_edge, NULL);

	code_signature->nb_para_in = 0;
	code_signature->nb_para_ou = 0;

	memset(code_signature->nb_frag_in, 0, sizeof(code_signature->nb_frag_in));
	memset(code_signature->nb_frag_ou, 0, sizeof(code_signature->nb_frag_ou));

	for (node_cursor = graph_get_head_node(&(code_signature->signature.graph)); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
		sig_node_cursor = (struct codeSignatureNode*)node_get_data(node_cursor);

		if (sig_node_cursor->input_number > CODESIGNATURE_NB_PARA_MAX){
			log_warn_m("signature \"%s\" has too many input parameter: %u -> limiting to %u", code_signature->signature.symbol.name, sig_node_cursor->input_number, CODESIGNATURE_NB_PARA_MAX);
			sig_node_cursor->input_number = CODESIGNATURE_NB_PARA_MAX;
		}

		code_signature->nb_para_in = max(sig_node_cursor->input_number, code_signature->nb_para_in);
		if (sig_node_cursor->input_number > 0){
			code_signature->nb_frag_in[sig_node_cursor->input_number - 1] ++;
		}

		if (sig_node_cursor->output_number > CODESIGNATURE_NB_PARA_MAX){
			log_warn_m("signature \"%s\" has too many output parameter: %u -> limiting to %u", code_signature->signature.symbol.name, sig_node_cursor->output_number, CODESIGNATURE_NB_PARA_MAX);
			sig_node_cursor->output_number = CODESIGNATURE_NB_PARA_MAX;
		}

		code_signature->nb_para_ou = max(sig_node_cursor->output_number, code_signature->nb_para_ou);
		if (sig_node_cursor->output_number > 0){
			code_signature->nb_frag_ou[sig_node_cursor->output_number - 1] ++;
		}
	}

	#ifdef EXTRA_CHECK
	{
		uint32_t i;

		if (code_signature->nb_para_in == 0){
			log_warn_m("signature \"%s\" has no input parameter", code_signature->signature.symbol.name);
		}
		else{
			for (i = 0; i < code_signature->nb_para_in; i++){
				if (code_signature->nb_frag_in[i] == 0){
					log_warn_m("signature \"%s\", input parameter %u has no fragment", code_signature->signature.symbol.name, i);
				}
			}
		}
		if (code_signature->nb_para_ou == 0){
			log_warn_m("signature \"%s\" has no output parameter", code_signature->signature.symbol.name);
		}
		else{
			for (i = 0; i < code_signature->nb_para_ou; i++){
				if (code_signature->nb_frag_ou[i] == 0){
					log_warn_m("signature \"%s\", output parameter %u has no fragment", code_signature->signature.symbol.name, i);
				}
			}
		}

		for (node_cursor = graph_get_head_node(&(code_signature->signature.graph)); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
			sig_node_cursor = (struct codeSignatureNode*)node_get_data(node_cursor);

			code_signature->nb_para_in = max(sig_node_cursor->input_number, code_signature->nb_para_in);
			if (sig_node_cursor->input_number > 0){
				if ((uint32_t)sig_node_cursor->input_frag_order - 1 >= code_signature->nb_frag_in[sig_node_cursor->input_number - 1]){
					log_warn_m("signature \"%s\", input parameter %u, fragment out of bound: %u", code_signature->signature.symbol.name, sig_node_cursor->input_number - 1, sig_node_cursor->input_frag_order - 1);
				}
			}

			code_signature->nb_para_ou = max(sig_node_cursor->output_number, code_signature->nb_para_ou);
			if (sig_node_cursor->output_number > 0){
				if ((uint32_t)sig_node_cursor->output_frag_order - 1 >= code_signature->nb_frag_ou[sig_node_cursor->output_number - 1]){
					log_warn_m("signature \"%s\", input parameter %u, fragment out of bound: %u", code_signature->signature.symbol.name, sig_node_cursor->output_number - 1, sig_node_cursor->output_frag_order - 1);
				}
			}
		}
	}
	#endif
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
			return 0x0000ffff | (operation->operation_type.symbol.sym_sig->id << 16);
		}
		case IR_OPERATION_TYPE_NULL 	: {
			return 0xfffffffc;
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
				return (signature_node->node_type.symbol->id << 16 ) | 0x0000ffff;
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

	if (dependence->type == IR_DEPENDENCE_TYPE_MACRO){
		return dependence->type | dependence->dependence_type.macro;
	}
	else{
		return dependence->type;
	}
}
