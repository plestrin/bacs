#include <stdlib.h>
#include <stdio.h>
#include <search.h>

#include "modeSignature.h"
#include "synthesisGraph.h"
#include "result.h"
#include "ir.h"
#include "base.h"

#define MODESIGNATURE_NAMEENGINE_MAX_ENTRY 128

struct nameEngine{
	struct hsearch_data hashtable;
	uint32_t 			nb_entry;
	uint32_t 			ref_count;
};

struct nameEngine engine = {
	.nb_entry = 0,
	.ref_count = 0,
};

void nameEngine_get(void){
	if (engine.ref_count == 0){
		if (hcreate_r(MODESIGNATURE_NAMEENGINE_MAX_ENTRY, &(engine.hashtable)) == 0){
			log_err("unable to create hashTable");
			return;
		}
	}
	engine.ref_count ++;
}

uint32_t nameEngine_search(char* name){
	ENTRY 	item;
	ENTRY* 	result;

	if (engine.nb_entry == MODESIGNATURE_NAMEENGINE_MAX_ENTRY){
		log_err("the max number of entry has been reached");
		return 0;
	}

	item.key = name;
	item.data = (void*)engine.nb_entry;

	if (hsearch_r(item, ENTER, &result, &(engine.hashtable)) == 0){
		log_err("something went wrong with hsearch_r");
	}
	else if (result->data == item.data){
		engine.nb_entry ++;
	}

	return (uint32_t)result->data;
}

void nameEngine_release(void){
	if (engine.ref_count == 0){
		log_err("reference counter is already equal to 0");
		return;
	}

	engine.ref_count --;
	if (engine.ref_count == 0){
		hdestroy_r(&(engine.hashtable));
	}
}

#pragma GCC diagnostic ignored "-Wunused-parameter"
static void modeSignature_printDot_node(void* data, FILE* file, void* arg){
	struct modeSignatureNode* mode_signature_node = (struct modeSignatureNode*)data;

	switch(mode_signature_node->type){
		case MODESIGNATURE_NODE_TYPE_PATH 		: {
			fprintf(file, "[label=\"path\"]");
			break;
		}
		case MODESIGNATURE_NODE_TYPE_SYMBOL 	: {
			fprintf(file, "[label=\"%s\"]", mode_signature_node->node_type.name);
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

#pragma GCC diagnostic ignored "-Wunused-parameter"
static void modeSignature_printDot_edge(void* data, FILE* file, void* arg){
	struct modeSignatureEdge* mode_signature_edge = (struct modeSignatureEdge*)data;

	if (synthesisGraph_edge_is_input(mode_signature_edge->tag)){
		fprintf(file, "[label=\"I%u\"]", synthesisGraph_edge_get_parameter(mode_signature_edge->tag));
	}
	else if (synthesisGraph_edge_is_output(mode_signature_edge->tag)){
		fprintf(file, "[label=\"O%u\"]", synthesisGraph_edge_get_parameter(mode_signature_edge->tag));
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
			label = nameEngine_search(mode_signature_node->node_type.name) << 2;
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

uint32_t modeSignatureEdge_get_label(struct edge* edge){
	return ((struct modeSignatureEdge*)edge_get_data(edge))->tag;
}

uint32_t synthesisGraphNode_get_label(struct node* node){
	struct synthesisNode* 	synthesis_node = synthesisGraph_get_synthesisNode(node);
	uint32_t 				label = 0;
	struct node*			symbol;

	switch(synthesis_node->type){
		case SYNTHESISNODETYPE_RESULT 			: {
			symbol = *(struct node**)array_get(&(synthesis_node->node_type.cluster->instance_array), 0);
			nameEngine_get();
			label = nameEngine_search(((struct result*)(ir_node_get_operation(symbol)->operation_type.symbol.result_ptr))->code_signature->signature.symbol) << 2;
			nameEngine_release();
			break;
		}
		case SYNTHESISNODETYPE_FORWARD_PATH 	: {
			label = 0x00000001;
			break;
		}
		case SYNTHESISNODETYPE_BACKWARD_PATH 	: {
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
	return *(uint32_t*)edge_get_data(edge);
}

void modeSignature_init(struct modeSignature* mode_signature){
	nameEngine_get();
	graph_register_dotPrint_callback(&(mode_signature->signature.graph), NULL, modeSignature_printDot_node, modeSignature_printDot_edge, NULL);
}