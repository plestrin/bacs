#include <stdlib.h>
#include <stdio.h>

#include "modeSignature.h"
#include "ir.h"
#include "base.h"

#pragma GCC diagnostic ignored "-Wunused-parameter"
void modeSignature_printDot_node(void* data, FILE* file, void* arg){
	struct modeSignatureNode* mode_signature_node = (struct modeSignatureNode*)data;

	switch(mode_signature_node->type){
		case MODESIGNATURE_NODE_TYPE_PATH 		: {
			fprintf(file, "[label=\"path\"]");
			break;
		}
		case MODESIGNATURE_NODE_TYPE_SYMBOL 	: {
			fprintf(file, "[label=\"%s\"]", mode_signature_node->node_type.symbol.name);
			break;

		}
		case MODESIGNATURE_NODE_TYPE_RAW 		: {
			fprintf(file, "[label=\"*\"]");
			break;
		}
		case MODESIGNATURE_NODE_TYPE_INVALID 	: {
			fprintf(file, "[label=\"inv\"]");
			break;
		}
	}
}

uint32_t modeSignatureNode_get_label(struct node* node){
	struct modeSignatureNode* 	mode_signature_node = (struct modeSignatureNode*)node_get_data(node);
	uint32_t 					label = SUBGRAPHISOMORPHISM_JOKER_LABEL;

	switch(mode_signature_node->type){
		case MODESIGNATURE_NODE_TYPE_PATH 		: {
			label = 0x00000001;
			break;
		}
		case MODESIGNATURE_NODE_TYPE_SYMBOL 	: {
			label = mode_signature_node->node_type.symbol.id << 2;
			break;

		}
		case MODESIGNATURE_NODE_TYPE_RAW 		: {
			break;
		}
		case MODESIGNATURE_NODE_TYPE_INVALID 	: {
			break;
		}
	}

	return label;
}

uint32_t synthesisGraphNode_get_label(struct node* node){
	struct synthesisNode* 	synthesis_node = synthesisGraph_get_synthesisNode(node);
	uint32_t 				label = 0;
	struct node*			symbol;

	switch(synthesis_node->type){
		case SYNTHESISNODETYPE_RESULT 			: {
			symbol = *(struct node**)array_get(&(synthesis_node->node_type.cluster->instance_array), 0);
			label = ir_node_get_operation(symbol)->operation_type.symbol.sym_sig->id << 2;
			break;
		}
		case SYNTHESISNODETYPE_PATH 			: {
			label = 0x00000001;
			break;
		}
		case SYNTHESISNODETYPE_IR_NODE 			: {
			label = 0x00000003;
			break;
		}
	}

	return label;
}

uint32_t synthesisGraphEdge_get_label(struct edge* edge){
	return synthesisGraph_get_synthesisEdge(edge)->tag;
}

void modeSignature_init(struct modeSignature* mode_signature){
	struct node* 				node_cursor;
	struct modeSignatureNode* 	mode_signature_node;

	graph_register_dotPrint_callback(&((mode_signature)->signature.graph), NULL, modeSignature_printDot_node, modeSignature_printDot_edge, NULL);

	for (node_cursor = graph_get_head_node(&(mode_signature->signature.graph)); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
		mode_signature_node = (struct modeSignatureNode*)node_get_data(node_cursor);
		if (mode_signature_node->type == MODESIGNATURE_NODE_TYPE_SYMBOL){
			signatureSymbol_fetch(&(mode_signature_node->node_type.symbol), NULL);
		}
	}
} 