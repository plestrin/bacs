#ifndef MODESIGNATURE_H
#define MODESIGNATURE_H

#include <stdint.h>

#include "signatureCollection.h"
#include "synthesisGraph.h"

enum modeSignatureNodeType{
	MODESIGNATURE_NODE_TYPE_PATH,
	MODESIGNATURE_NODE_TYPE_SYMBOL,
	MODESIGNATURE_NODE_TYPE_RAW,
	MODESIGNATURE_NODE_TYPE_INVALID
};

struct modeSignatureNode{
	enum modeSignatureNodeType 	type;
	union{
		struct signatureSymbol 	symbol;
	} 							node_type;
};

#define modeSignatureEdge synthesisEdge

void modeSignature_printDot_node(void* data, FILE* file, void* arg);
#define modeSignature_printDot_edge synthesisGraph_printDot_edge

uint32_t modeSignatureNode_get_label(struct node* node);
#define modeSignatureEdge_get_label synthesisGraphEdge_get_label

uint32_t synthesisGraphNode_get_label(struct node* node);
uint32_t synthesisGraphEdge_get_label(struct edge* edge);

struct modeSignature{
	struct signature signature;
};

#define signatureCollection_node_get_modeSignature(node) ((struct modeSignature*)node_get_data(node))

void modeSignature_init(struct modeSignature* mode_signature);

#endif