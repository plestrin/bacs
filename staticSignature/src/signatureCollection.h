#ifndef SIGNATURECOLLECTION_H
#define SIGNATURECOLLECTION_H

#include <stdint.h>
#include <string.h>

#include "graph.h"
#include "subGraphIsomorphism.h"
#include "array.h"

#define SIGNATURE_NAME_MAX_SIZE 32

/* Status bitmap description:
	- bit 1 : has the symbol been resolved
	- bit 2 : has the symbol been found
*/

struct signatureSymbol{
	uint32_t 	id;
	char 		name[SIGNATURE_NAME_MAX_SIZE];
};

#define SIGNATURESYMBOL_RAW_ID 0
#define signatureSymbol_is_resolved(symbol) ((symbol)->id != SIGNATURESYMBOL_RAW_ID)

struct signatureSymbolTable{
	uint32_t 				nb_symbol;
	struct signatureSymbol 	symbols[1];
};

#define signatureSymbolTable_get_size(nb_symbol) (sizeof(struct signatureSymbolTable) + ((nb_symbol) - 1) * sizeof(struct signatureSymbol))

struct signature{
	struct signatureSymbol 			symbol;
	struct graph					graph;
	struct subGraphIsoHandle* 		sub_graph_handle;
	struct signatureSymbolTable* 	symbol_table;
	int32_t 						result_index;
	uint32_t 						state;
};

#define signature_state_is_search(signature) 	((signature)->state & 0x00000001)
#define signature_state_is_found(signature) 	((signature)->state & 0x00000002)
#define signature_state_is_pushed(signature) 	((signature)->state & 0x00000004)
#define signature_state_set_search(signature) 	(signature)->state |= 0x00000001
#define signature_state_set_found(signature) 	(signature)->state |= 0x00000002
#define signature_state_set_pushed(signature) 	(signature)->state |= 0x00000004
#define signature_state_set_poped(signature) 	(signature)->state &= 0xfffffffb

#define signature_clean(signature) 																					\
	if ((signature)->sub_graph_handle != NULL){ 																	\
		graphIso_delete_subGraph_handle((signature)->sub_graph_handle); 											\
	} 																												\
	graph_clean(&((signature)->graph)); 																			\
	if ((signature)->symbol_table != NULL){ 																		\
		free((signature)->symbol_table); 																			\
	}

#define signatureCollection_node_get_signature(node) ((struct signature*)node_get_data(node))

struct signatureCallback{
	uint32_t(*signatureNode_get_label)(struct node*);
	uint32_t(*signatureEdge_get_label)(struct edge*);
};

struct signatureCollection{
	struct graph 				syntax_graph;
	struct signatureCallback 	callback;
};

struct signatureCollection* signatureCollection_create(size_t custom_signature_size, struct signatureCallback* callback);
void signatureCollection_init(struct signatureCollection* collection, size_t custom_signature_size, struct signatureCallback* callback);

#define signatureCollection_get_nb_signature(collection) ((collection)->syntax_graph.nb_node)

void signatureCollection_printDot(struct signatureCollection* collection);
int32_t signatureCollection_add(struct signatureCollection* collection, void* custom_signature, char* export_name);

struct graphSearcher{
	struct graph* 	graph;
	int32_t(*result_register)(void*,struct array*,void*);
	void(*result_push)(int32_t,void*);
	void(*result_pop)(int32_t,void*);
	void* 			arg;
};

void signatureCollection_search(struct signatureCollection* collection, struct graphSearcher* graph_searcher_buffer, uint32_t nb_graph_searcher, uint32_t(*graphNode_get_label)(struct node*), uint32_t(*graphEdge_get_label)(struct edge*));

void signatureCollection_clean(struct signatureCollection* collection);

#define signatureCollection_delete(collection) 																		\
	signatureCollection_clean(collection); 																			\
	free(collection);

void signatureSymbol_register(struct signatureSymbol* symbol, char* export_name, struct signatureCollection* collection);
void signatureSymbol_fetch(struct signatureSymbol* symbol, struct signatureCollection* collection);

#endif