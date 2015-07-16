#ifndef MODESIGNATURE_H
#define MODESIGNATURE_H

#include <stdint.h>

#include "signatureCollection.h"

void nameEngine_get(void);
uint32_t nameEngine_search(char* name);
void nameEngine_release(void);

enum modeSignatureNodeType{
	MODESIGNATURE_NODE_TYPE_PATH,
	MODESIGNATURE_NODE_TYPE_SYMBOL,
	MODESIGNATURE_NODE_TYPE_RAW,
	MODESIGNATURE_NODE_TYPE_INVALID
};

struct modeSignatureNode{
	enum modeSignatureNodeType 	type;
	union{
		char 					name[SIGNATURE_NAME_MAX_SIZE];
	} 							node_type;
};

struct modeSignatureEdge{
	uint32_t tag;
};

uint32_t modeSignatureNode_get_label(struct node* node);
uint32_t modeSignatureEdge_get_label(struct edge* edge);

uint32_t synthesisGraphNode_get_label(struct node* node);
uint32_t synthesisGraphEdge_get_label(struct edge* edge);

struct modeSignature{
	struct signature signature;
};

#define signatureCollection_node_get_modeSignature(node) ((struct modeSignature*)&((node)->data))

void modeSignature_init(struct modeSignature* mode_signature);
#define modeSignature_clean (void(*)(struct node*))nameEngine_release

#endif