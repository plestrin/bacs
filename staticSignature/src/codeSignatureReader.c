#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>

#include "codeSignatureReader.h"
#include "codeSignature.h"
#include "mapFile.h"
#include "graphReader.h"
#include "set.h"
#include "base.h"

struct codeSignatureBuilder{
	struct codeSignature 		code_signature;
	struct set* 				symbol_set;
	uint32_t 					is_name_set;
	struct signatureCollection* collection;
};

static int32_t codeSignatureBuilder_init(struct codeSignatureBuilder* builder, struct signatureCollection* collection){
	graph_init(&(builder->code_signature.signature.graph), sizeof(struct codeSignatureNode), sizeof(struct codeSignatureEdge));
	
	builder->is_name_set = 0;
	builder->collection = collection;

	if ((builder->symbol_set = set_create(SIGNATURE_NAME_MAX_SIZE, 4)) == NULL){
		log_err("unable to create set");
		return -1;
	}

	return 0;
}

#define codeSignatureBuilder_reset(builder) 																						\
	(builder)->is_name_set = 0; 																									\
	set_empty((builder)->symbol_set); 																								\
	graph_init(&((builder)->code_signature.signature.graph), sizeof(struct codeSignatureNode), sizeof(struct codeSignatureEdge));

#define codeSignatureBuidler_clean(builder) set_delete((builder)->symbol_set)

static void codeSignatureReader_handle_graph_name(const char* str, size_t str_len, void* arg){
	struct codeSignatureBuilder* builder = (struct codeSignatureBuilder*)arg;

	memset(builder->code_signature.signature.name, 0, SIGNATURE_NAME_MAX_SIZE);
	memcpy(builder->code_signature.signature.name, str, min(SIGNATURE_NAME_MAX_SIZE - 1, str_len));

	memset(builder->code_signature.signature.symbol, 0, SIGNATURE_NAME_MAX_SIZE);
	memcpy(builder->code_signature.signature.symbol, str, min(SIGNATURE_NAME_MAX_SIZE - 1, str_len));

	builder->is_name_set = 1;
}

static void codeSignatureReader_handle_graph_label(const char* str, size_t str_len, void* arg){
	struct codeSignatureBuilder* builder = (struct codeSignatureBuilder*)arg;

	memset(builder->code_signature.signature.symbol, 0, SIGNATURE_NAME_MAX_SIZE);
	memcpy(builder->code_signature.signature.symbol, str, min(SIGNATURE_NAME_MAX_SIZE - 1, str_len));
}

static void* codeSignatureReader_handle_new_node(void* arg){
	struct node* 					node;
	struct codeSignatureNode* 		code_signature_node;
	struct codeSignatureBuilder* 	builder = (struct codeSignatureBuilder*)arg;

	if ((node = graph_add_node_(&(builder->code_signature.signature.graph))) == NULL){
		log_err("unable to add node to graph");
	}
	else{
		code_signature_node = (struct codeSignatureNode*)&(node->data);
		code_signature_node->type 				= SIGNATURE_NODE_TYPE_INVALID;
		code_signature_node->input_number 		= 0;
		code_signature_node->input_frag_order 	= 0;
		code_signature_node->output_number 		= 0;
		code_signature_node->output_frag_order 	= 0;
	}

	return node;
}

static void codeSignatureReader_handle_node_label(const char* str, size_t str_len, void* ptr, void* arg){
	struct node* 					node = (struct node*)ptr;
	struct codeSignatureNode* 		code_signature_node = (struct codeSignatureNode*)&(node->data);
	struct codeSignatureBuilder* 	builder = (struct codeSignatureBuilder*)arg;
	char 							symbol[SIGNATURE_NAME_MAX_SIZE];

	if (code_signature_node->type != SIGNATURE_NODE_TYPE_INVALID){
		log_warn_m("the label has already been set for this node %s (signature: %s)", (code_signature_node->type == SIGNATURE_NODE_TYPE_OPCODE) ? irOpcode_2_string(code_signature_node->node_type.opcode): "", builder->is_name_set ? builder->code_signature.signature.name : "?");
	}

	if (!strncmp(str, "ADD", str_len)){
		code_signature_node->type = SIGNATURE_NODE_TYPE_OPCODE;
		code_signature_node->node_type.opcode = IR_ADD;
		return;
	}
	else if (!strncmp(str, "AND", str_len)){
		code_signature_node->type = SIGNATURE_NODE_TYPE_OPCODE;
		code_signature_node->node_type.opcode = IR_AND;
		return;
	}
	else if (!strncmp(str, "MUL", str_len)){
		code_signature_node->type = SIGNATURE_NODE_TYPE_OPCODE;
		code_signature_node->node_type.opcode = IR_MUL;
		return;
	}
	else if (!strncmp(str, "MOVZX", str_len)){
		code_signature_node->type = SIGNATURE_NODE_TYPE_OPCODE;
		code_signature_node->node_type.opcode = IR_MOVZX;
		return;
	}
	else if (!strncmp(str, "NOT", str_len)){
		code_signature_node->type = SIGNATURE_NODE_TYPE_OPCODE;
		code_signature_node->node_type.opcode = IR_NOT;
		return;
	}
	else if (!strncmp(str, "OR", str_len)){
		code_signature_node->type = SIGNATURE_NODE_TYPE_OPCODE;
		code_signature_node->node_type.opcode = IR_OR;
		return;
	}
	else if (!strncmp(str, "ROR", str_len)){
		code_signature_node->type = SIGNATURE_NODE_TYPE_OPCODE;
		code_signature_node->node_type.opcode = IR_ROR;
		return;
	}
	else if (!strncmp(str, "SHL", str_len)){
		code_signature_node->type = SIGNATURE_NODE_TYPE_OPCODE;
		code_signature_node->node_type.opcode = IR_SHL;
		return;
	}
	else if (!strncmp(str, "SHR", str_len)){
		code_signature_node->type = SIGNATURE_NODE_TYPE_OPCODE;
		code_signature_node->node_type.opcode = IR_SHR;
		return;
	}
	else if (!strncmp(str, "SUB", str_len)){
		code_signature_node->type = SIGNATURE_NODE_TYPE_OPCODE;
		code_signature_node->node_type.opcode = IR_SUB;
		return;
	}
	else if (!strncmp(str, "XOR", str_len)){
		code_signature_node->type = SIGNATURE_NODE_TYPE_OPCODE;
		code_signature_node->node_type.opcode = IR_XOR;
		return;
	}
	else if (!strncmp(str, "LOAD", str_len)){
		code_signature_node->type = SIGNATURE_NODE_TYPE_OPCODE;
		code_signature_node->node_type.opcode = IR_LOAD;
		return;
	}
	else if (!strncmp(str, "STORE", str_len)){
		code_signature_node->type = SIGNATURE_NODE_TYPE_OPCODE;
		code_signature_node->node_type.opcode = IR_STORE;
		return;
	}
	else if (!strncmp(str, "*", str_len)){
		code_signature_node->type = SIGNATURE_NODE_TYPE_OPCODE;
		code_signature_node->node_type.opcode = IR_JOKER;
		return;
	}

	memset(symbol, 0, SIGNATURE_NAME_MAX_SIZE);
	memcpy(symbol, str, min(SIGNATURE_NAME_MAX_SIZE - 1, str_len));
	
	if ((uint32_t)(code_signature_node->node_type.symbol = (void*)set_add_unique(builder->symbol_set, symbol)) & 0x80000000){
		log_err("unable to add element to array");
	}
	else{
		code_signature_node->type = SIGNATURE_NODE_TYPE_SYMBOL;	
	}
}

#pragma GCC diagnostic ignored "-Wunused-parameter"
static void codeSignatureReader_handle_node_io(const char* str, size_t str_len, void* ptr, void* arg){
	struct node* 				node = (struct node*)ptr;
	struct codeSignatureNode* 	code_signature_node = (struct codeSignatureNode*)&(node->data);
	size_t 					i;

	if (str_len < 5){
		log_err("incorrect IO tag");
		return;
	}

	if (!memcmp(str, "I:", 2)){
		if (code_signature_node->input_number | code_signature_node->input_frag_order){
			log_warn("the IO tag has already been set for this node");
		}

		for (i = 2; i < str_len - 1; i++){
			if (str[i] == ':'){
				code_signature_node->input_number = atoi(str + 2);
				code_signature_node->input_frag_order = atoi(str + i + 1);
				return;
			}
		}
		log_err("incorrect IO tag");
	}
	else if (!memcmp(str, "O:", 2)){
		if (code_signature_node->output_number | code_signature_node->output_frag_order){
			log_warn("the IO tag has already been set for this node");
		}

		for (i = 2; i < str_len - 1; i++){
			if (str[i] == ':'){
				code_signature_node->output_number = atoi(str + 2);
				code_signature_node->output_frag_order = atoi(str + i + 1);
				return;
			}
		}
		log_err("incorrect IO tag");
	}
	else{
		log_err("incorrect IO tag");
	}
}

static void* codeSignatureReader_handle_new_edge(void* src, void* dst, void* arg){
	struct edge* 					edge;
	struct codeSignatureEdge* 		code_signature_edge;
	struct codeSignatureBuilder* 	builder = (struct codeSignatureBuilder*)arg;

	if ((edge = graph_add_edge_(&(builder->code_signature.signature.graph), (struct node*)src, (struct node*)dst)) == NULL){
		log_err("unable to add edge to graph");
	}
	else{
		code_signature_edge = (struct codeSignatureEdge*)&(edge->data);
		code_signature_edge->type = IR_DEPENDENCE_TYPE_DIRECT;
		code_signature_edge->macro_desc = 0;
	}

	return edge;
}

static void codeSignatureReader_handle_edge_label(const char* str, size_t str_len, void* ptr, void* arg){
	struct edge* 				edge = (struct edge*)ptr;
	struct codeSignatureEdge* 	code_signature_edge = (struct codeSignatureEdge*)&(edge->data);

	if (str_len == 1 && str[0] == '@'){
		code_signature_edge->type = IR_DEPENDENCE_TYPE_ADDRESS;
		return;
	}
	
	if (str_len >= 4 && (str[0] == 'I' || str[0] == 'O')){
		uint32_t nb_digit1;
		uint32_t nb_digit2;

		for (nb_digit1 = 0; nb_digit1 < str_len - 1; nb_digit1 ++){
			if (str[1 + nb_digit1] < 48 || str[1 + nb_digit1] > 57){
				break;
			}
		}

		if (nb_digit1 > 0 && str_len > 2 + nb_digit1 && nb_digit1 > 0 && str[1 + nb_digit1] == 'F'){
			for (nb_digit2 = 0; nb_digit2 < str_len - 2 - nb_digit1; nb_digit2 ++){
				if (str[2 + nb_digit1 + nb_digit2] < 48 || str[2 + nb_digit1 + nb_digit2] > 57){
					break;
				}
			}

			if (nb_digit2 > 0){
				if (str[0] == 'I'){
					code_signature_edge->type = IR_DEPENDENCE_TYPE_MACRO;
					code_signature_edge->macro_desc = IR_DEPENDENCE_MACRO_DESC_SET_INPUT(atoi(str + 2 + nb_digit1), atoi(str + 1));
				}
				else{
					code_signature_edge->type = IR_DEPENDENCE_TYPE_MACRO;
					code_signature_edge->macro_desc = IR_DEPENDENCE_MACRO_DESC_SET_OUTPUT(atoi(str + 2 + nb_digit1), atoi(str + 1));
				}
				return;
			}
		}
	}

	if (str_len >= 3){
		log_err_m("unable to convert string to irDependenceType %.3s, by default return DIRECT", str);
	}
	else{
		log_err("unable to convert string to irDependenceType, by default return DIRECT");
	}
}

static void codeSignatureReader_handle_flush_graph(void* arg){
	struct codeSignatureBuilder* 	builder = (struct codeSignatureBuilder*)arg;
	struct signatureSymbolTable* 	symbol_table;
	struct setIterator 				iterator;
	char* 							symbol_ptr;
	struct node* 					node_cursor;
	uint32_t 						i;
	struct codeSignatureNode* 		code_signature_node;

	if ((symbol_table = (struct signatureSymbolTable*)malloc(signatureSymbolTable_get_size(set_get_length(builder->symbol_set)))) == NULL){
		log_err("unable to allocate memory");
		graph_clean(&(builder->code_signature.signature.graph));
		goto exit;
	}

	symbol_table->nb_symbol = set_get_length(builder->symbol_set);
	for (symbol_ptr = (char*)setIterator_get_first(builder->symbol_set, &iterator), i = 0; symbol_ptr != NULL; symbol_ptr = setIterator_get_next(&iterator), i++){
		symbol_table->symbols[i].status = 0;
		memcpy(symbol_table->symbols[i].name, symbol_ptr, SIGNATURE_NAME_MAX_SIZE);	
	}

	for (node_cursor = graph_get_head_node(&(builder->code_signature.signature.graph)); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
		code_signature_node = (struct codeSignatureNode*)&(node_cursor->data);

		if (code_signature_node->type == SIGNATURE_NODE_TYPE_INVALID){
			log_warn("a node label has not been set");
		}
		else if (code_signature_node->type == SIGNATURE_NODE_TYPE_SYMBOL){
			code_signature_node->node_type.symbol = symbol_table->symbols + (uint32_t)code_signature_node->node_type.symbol;
		}
	}

	if (!builder->is_name_set){
		log_warn("codeSignature name has not been set");
	}

	builder->code_signature.signature.sub_graph_handle 	= NULL;
	builder->code_signature.signature.symbol_table 		= symbol_table;

	if (signatureCollection_add(builder->collection, &(builder->code_signature))){
		log_err("unable to add signature to collection");
	}

	exit:
	codeSignatureBuilder_reset(builder);
}

void codeSignatureReader_parse(struct signatureCollection* collection, const char* file_name){
	void* 						buffer;
	size_t 						buffer_size;
	struct codeSignatureBuilder builder;
	struct graphReaderCallback 	callback = {
		.callback_graph_name 	= codeSignatureReader_handle_graph_name,
		.callback_graph_label 	= codeSignatureReader_handle_graph_label,
		.callback_graph_end 	= codeSignatureReader_handle_flush_graph,
		.callback_create_node 	= codeSignatureReader_handle_new_node,
		.callback_node_label 	= codeSignatureReader_handle_node_label,
		.callback_node_io 		= codeSignatureReader_handle_node_io,
		.callback_create_edge 	= codeSignatureReader_handle_new_edge,
		.callback_edge_label 	= codeSignatureReader_handle_edge_label,
		.arg 					= (void*)&builder
	};

	buffer = mapFile_map(file_name, &buffer_size);
	if (buffer == NULL){
		log_err_m("unable to map file: %s", file_name);
		return;
	}

	if (codeSignatureBuilder_init(&builder, collection)){
		log_err("unable to init codeSignatureBuilder");
		goto exit;
	}

	graphReader_parse(buffer, buffer_size, &callback);
	codeSignatureBuidler_clean(&builder);

	exit:
	munmap(buffer, buffer_size);
}