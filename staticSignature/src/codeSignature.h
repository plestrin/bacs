#ifndef CODESIGNATURE_H
#define CODESIGNATURE_H

#include <stdlib.h>
#include <stdint.h>

#include "array.h"
#include "graph.h"
#include "subGraphIsomorphism.h"
#include "ir.h"

#define CODESIGNATURE_NAME_MAX_SIZE 32

/* Status bitmap description:
	- bit 1 : has the symbol been resolved?
	- bit 2 : has the symbol been found ?
*/

struct signatureSymbol{
	uint8_t 	status;
	uint32_t 	id;
	char 		name[CODESIGNATURE_NAME_MAX_SIZE];
};

#define symbolTableEntry_set_resolved(status) 	(status) |= 0x01
#define symbolTableEntry_set_found(status) 		(status) |= 0x02
#define symbolTableEntry_set_not_found(status) 	(status) &= 0xfd
#define symbolTableEntry_is_resolved(status) 	((status) & 0x01)
#define symbolTableEntry_is_found(status) 		((status) & 0x02)

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

struct codeSignature{
	uint32_t 						id;
	char  							name[CODESIGNATURE_NAME_MAX_SIZE];
	char 							symbol[CODESIGNATURE_NAME_MAX_SIZE];
	struct graph					graph;
	struct subGraphIsoHandle* 		sub_graph_handle;
	struct signatureSymbolTable* 	symbol_table;
	uint32_t 						state;
} __attribute__((__may_alias__));

#define syntax_node_get_codeSignature(node) 	((struct codeSignature*)&((node)->data))
#define syntax_edge_get_index(edge) 			(*(uint32_t*)(edge)->data)

#define codeSignature_state_is_search(code_signature) ((code_signature)->state & 0x00000001)
#define codeSignature_state_is_found(code_signature) ((code_signature)->state & 0x00000002)
#define codeSignature_state_set_search(code_signature) (code_signature)->state |= 0x00000001
#define codeSignature_state_set_found(code_signature) (code_signature)->state |= 0x00000002

#define codeSignature_clean(code_signature) 																							\
	graphIso_delete_subGraph_handle((code_signature)->sub_graph_handle); 																\
	graph_clean(&((code_signature)->graph)); 																							\
	if ((code_signature)->symbol_table != NULL){ 																						\
		free((code_signature)->symbol_table); 																							\
	}

struct codeSignatureCollection{
	uint32_t 		code_signature_id_generator;
	struct graph 	syntax_graph;
};

struct codeSignatureCollection* codeSignature_create_collection();

#define codeSignature_init_collection(collection) 																						\
	graph_init(&((collection)->syntax_graph), sizeof(struct codeSignature), sizeof(uint32_t)); 											\
	(collection)->code_signature_id_generator = 0

int32_t codeSignature_add_signature_to_collection(struct codeSignatureCollection* collection, struct codeSignature* code_signature);

#define codeSignature_get_new_id(collection) ((collection)->code_signature_id_generator ++)

void codeSignature_search_collection(struct codeSignatureCollection* collection, struct ir** ir_buffer, uint32_t nb_ir);

void codeSignature_printDot_collection(struct codeSignatureCollection* collection);

void codeSignature_clean_collection(struct codeSignatureCollection* collection);

#define codeSignature_delete_collection(collection) 																					\
	codeSignature_clean_collection(collection); 																						\
	free(collection)


#endif