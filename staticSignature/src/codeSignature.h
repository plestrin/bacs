#ifndef CODESIGNATURE_H
#define CODESIGNATURE_H

#include <stdlib.h>
#include <stdint.h>

#include "array.h"
#include "graph.h"

struct codeSignatureCollection{
	uint32_t 		code_signature_id_generator;
	struct graph 	syntax_graph;
};

struct codeSignatureCollection* codeSignatureCollection_create();

#define codeSignatureCollection_init(collection) 																					\
	graph_init(&((collection)->syntax_graph), sizeof(struct codeSignature), sizeof(uint32_t)); 										\
	(collection)->code_signature_id_generator = 0;

#define codeSignatureCollection_get_new_id(collection) ((collection)->code_signature_id_generator ++)
#define codeSignaturecollection_get_nb_signature(collection) ((collection)->syntax_graph.nb_node)

void codeSignatureCollection_printDot(struct codeSignatureCollection* collection);

void codeSignatureCollection_clean(struct codeSignatureCollection* collection);

#define codeSignatureCollection_delete(collection) 																					\
	codeSignatureCollection_clean(collection); 																						\
	free(collection);

#include "subGraphIsomorphism.h"
#include "ir.h"

#define CODESIGNATURE_NAME_MAX_SIZE 32

/* Status bitmap description:
	- bit 1 : has the symbol been resolved
	- bit 2 : has the symbol been found
*/

struct signatureSymbol{
	uint8_t 	status;
	uint32_t 	id;
	char 		name[CODESIGNATURE_NAME_MAX_SIZE];
};

#define symbolTableEntry_set_resolved(symbol) 	((symbol)->status) |= 0x01
#define symbolTableEntry_is_resolved(symbol) 	((symbol)->status & 0x01)

#define signatureSymbol_set_id(sym, index) (sym)->id = 0x0000ffff | ((index) << 16)

struct signatureSymbolTable{
	uint32_t 				nb_symbol;
	struct signatureSymbol 	symbols[1];
};

#define signatureSymbolTable_get_size(nb_symbol) (sizeof(struct signatureSymbolTable) + ((nb_symbol) - 1) * sizeof(struct signatureSymbol))

enum signatureNodeType{
	SIGNATURE_NODE_TYPE_OPCODE,
	SIGNATURE_NODE_TYPE_SYMBOL
};

struct signatureNode{
	enum signatureNodeType 		type;
	union{
		enum irOpcode 			opcode;
		struct signatureSymbol* symbol;
	} 							node_type;
	uint16_t 					input_number;
	uint16_t 					input_frag_order;
	uint16_t 					output_number;
	uint16_t 					output_frag_order;
};

struct signatureEdge{
	enum irDependenceType 		type;
	uint32_t 					macro_desc;
};

uint32_t irNode_get_label(struct node* node);
uint32_t signatureNode_get_label(struct node* node);
uint32_t irEdge_get_label(struct edge* edge);
uint32_t signatureEdge_get_label(struct edge* edge);

struct codeSignature{
	uint32_t 						id;
	char  							name[CODESIGNATURE_NAME_MAX_SIZE];
	char 							symbol[CODESIGNATURE_NAME_MAX_SIZE];
	struct graph					graph;
	struct subGraphIsoHandle* 		sub_graph_handle;
	struct signatureSymbolTable* 	symbol_table;
	int32_t 						result_index;
	uint32_t 						state;
	uint32_t 						nb_parameter_in;
	uint32_t 						nb_parameter_out;
	uint32_t 						nb_frag_tot_in;
	uint32_t 						nb_frag_tot_out;
} __attribute__((__may_alias__));

#define syntax_node_get_codeSignature(node) 	((struct codeSignature*)&((node)->data))
#define syntax_edge_get_index(edge) 			(*(uint32_t*)(edge)->data)

#define codeSignature_state_is_search(code_signature) 	((code_signature)->state & 0x00000001)
#define codeSignature_state_is_found(code_signature) 	((code_signature)->state & 0x00000002)
#define codeSignature_state_is_pushed(code_signature) 	((code_signature)->state & 0x00000004)
#define codeSignature_state_set_search(code_signature) 	(code_signature)->state |= 0x00000001
#define codeSignature_state_set_found(code_signature) 	(code_signature)->state |= 0x00000002
#define codeSignature_state_set_pushed(code_signature) 	(code_signature)->state |= 0x00000004
#define codeSignature_state_set_poped(code_signature) 	(code_signature)->state &= 0xfffffffb

#define codeSignature_clean(code_signature) 																							\
	if ((code_signature)->sub_graph_handle != NULL){ 																					\
		graphIso_delete_subGraph_handle((code_signature)->sub_graph_handle); 															\
	} 																																	\
	graph_clean(&((code_signature)->graph)); 																							\
	if ((code_signature)->symbol_table != NULL){ 																						\
		free((code_signature)->symbol_table); 																							\
	}

int32_t codeSignature_add_signature_to_collection(struct codeSignatureCollection* collection, struct codeSignature* code_signature);

#include "trace.h"

void codeSignature_search_collection(struct codeSignatureCollection* collection, struct trace** trace_buffer, uint32_t nb_trace);

#endif