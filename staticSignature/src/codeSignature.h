#ifndef CODESIGNATURE_H
#define CODESIGNATURE_H

#include <stdint.h>

#include "array.h"
#include "graph.h"
#include "subGraphIsomorphism.h"
#include "ir.h"

#define CODESIGNATURE_NAME_MAX_SIZE 32

struct signatureNode{
	enum irOpcode 				opcode;
	uint16_t 					input_number;
	uint16_t 					input_frag_order;
	uint16_t 					output_number;
	uint16_t 					output_frag_order;
};

struct codeSignature{
	char  						name[CODESIGNATURE_NAME_MAX_SIZE];
	struct graph				graph;
	struct subGraphIsoHandle* 	sub_graph_handle;
};

#define codeSignature_clean(code_signature) 																							\
	graphIso_delete_subGraph_handle((code_signature)->sub_graph_handle); 																\
	graph_clean(&((code_signature)->graph));

struct codeSignatureCollection{
	struct array 				signature_array;
};

struct codeSignatureCollection* codeSignature_create_collection();

#define codeSignature_init_collection(collection) array_init(&((collection)->signature_array), sizeof(struct codeSignature))

int32_t codeSignature_add_signature_to_collection(struct codeSignatureCollection* collection, struct codeSignature* code_signature);

void codeSignature_search(struct codeSignatureCollection* collection, struct ir* ir);

void codeSignature_printDot_collection(struct codeSignatureCollection* collection);

void codeSignature_empty_collection(struct codeSignatureCollection* collection);

void codeSignature_clean_collection(struct codeSignatureCollection* collection);

#define codeSignature_delete_collection(collection) 																					\
	codeSignature_clean_collection(collection); 																						\
	free(collection)


#endif