#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>

#include "signatureReader.h"
#include "mapFile.h"
#include "array.h"

enum readerToken{
	READER_TOKEN_NONE,
	READER_TOKEN_COMMENT,
	READER_TOKEN_GRAPH_NAME,
	READER_TOKEN_EDGE,
	READER_TOKEN_NODE_ID,
	READER_TOKEN_LABEL,
	READER_TOKEN_NODE_IO
};

enum readerState{
	READER_STATE_START,
	READER_STATE_GRAPH,
	READER_STATE_SRC,
	READER_STATE_SRC_DESC,
	READER_STATE_EDGE,
	READER_STATE_EDGE_DESC,
	READER_STATE_DST,
	READER_STATE_DST_DESC
};

struct readerCursor{
	char* 					cursor;
	uint64_t 				remaining_size;
	enum readerToken 		token;
};

struct localCodeEdge{
	uint32_t 				src_id;
	enum irOpcode 			src_opcode;
	uint8_t 				src_opcode_set;
	uint16_t 				src_input_number;
	uint16_t 				src_input_frag_order;

	uint32_t 				dst_id;
	enum irOpcode 			dst_opcode;
	uint8_t 				dst_opcode_set;
	uint16_t 				dst_output_number;
	uint16_t 				dst_output_frag_order;

	enum irDependenceType 	edge_type;
	uint8_t 				edge_type_set;
};

static void readerCursor_get_next(struct readerCursor* reader_cursor);

static void signatureReader_get_graph_name(struct readerCursor* reader_cursor, char* buffer, uint32_t buffer_length);

static enum irOpcode codeSignatureReader_get_opcode(struct readerCursor* reader_cursor);
static enum irDependenceType codeSignatureReader_get_irDependenceType(struct readerCursor* reader_cursor);
#define codeSignatureReader_get_IO_in(reader_cursor) codeSignatureReader_get_IO(reader_cursor, 'I')
#define codeSignatureReader_get_IO_out(reader_cursor) codeSignatureReader_get_IO(reader_cursor, 'O')
static uint32_t codeSignatureReader_get_IO(struct readerCursor* reader_cursor, char first_char);

static void codeSignatureReader_push_signature(struct codeSignatureCollection* collection, struct array* array, const char* graph_name);


void codeSignatureReader_parse(struct codeSignatureCollection* collection, const char* file_name){
	void* 						buffer;
	uint64_t 					buffer_size;
	struct readerCursor 		reader_cursor;
	enum readerState 			reader_state;
	struct localCodeEdge 		local_edge;
	struct array 				local_edge_array;
	char 						graph_name[CODESIGNATURE_NAME_MAX_SIZE];

	buffer = mapFile_map(file_name, &buffer_size);
	if (buffer == NULL){
		printf("ERROR: in %s, unable to map file: %s\n", __func__, file_name);
		return;
	}

	if (array_init(&local_edge_array, sizeof(struct localCodeEdge))){
		printf("ERROR: in %s, unable to init array\n", __func__);
		munmap(buffer, buffer_size);
		return;
	}

	reader_cursor.cursor = (char*)buffer;
	reader_cursor.remaining_size = buffer_size;
	reader_cursor.token = READER_TOKEN_NONE;
	reader_state = READER_STATE_START;

	readerCursor_get_next(&reader_cursor);
	while(reader_cursor.token != READER_TOKEN_NONE){
		switch(reader_state){
			case READER_STATE_START 		: {
				switch(reader_cursor.token){
					case READER_TOKEN_GRAPH_NAME 	: {
						signatureReader_get_graph_name(&reader_cursor, graph_name, CODESIGNATURE_NAME_MAX_SIZE);

						reader_state = READER_STATE_GRAPH;
						break;
					}
					case READER_TOKEN_COMMENT 		: {
						break;
					}
					default 						: {
						printf("ERROR: in %s, syntaxe error: state is START\n", __func__);
					}
				}
				break;
			}
			case READER_STATE_GRAPH 		: {
				switch(reader_cursor.token){
					case READER_TOKEN_GRAPH_NAME 	: {
						printf("WARNING: in %s, empty signature -> skip\n", __func__);
						signatureReader_get_graph_name(&reader_cursor, graph_name, CODESIGNATURE_NAME_MAX_SIZE);

						reader_state = READER_STATE_GRAPH;
						break;
					}
					case READER_TOKEN_NODE_ID 		: {
						local_edge.src_id = atoi(reader_cursor.cursor);
						local_edge.src_opcode_set = 0;
						local_edge.src_input_number = 0;
						local_edge.src_input_frag_order = 0;

						reader_state = READER_STATE_SRC;
						break;
					}
					case READER_TOKEN_COMMENT 		: {
						break;
					}
					default 						: {
						printf("ERROR: in %s, syntaxe error: state is GRAPH\n", __func__);
					}
				}
				break;
			}
			case READER_STATE_SRC 			: {
				switch(reader_cursor.token){
					case READER_TOKEN_LABEL 		: {
						local_edge.src_opcode = codeSignatureReader_get_opcode(&reader_cursor);
						local_edge.src_opcode_set = 1;

						reader_state = READER_STATE_SRC_DESC;
						break;
					}
					case READER_TOKEN_EDGE 			: {
						local_edge.edge_type_set = 0;

						reader_state = READER_STATE_EDGE;
						break;
					}
					case READER_TOKEN_COMMENT 		: {
						break;
					}
					default 						: {
						printf("ERROR: in %s, syntaxe error: state is SRC\n", __func__);
					}
				}
				break;
			}
			case READER_STATE_SRC_DESC 		: {
				switch(reader_cursor.token){
					case READER_TOKEN_EDGE 			: {
						local_edge.edge_type_set = 0;

						reader_state = READER_STATE_EDGE;
						break;
					}
					case READER_TOKEN_NODE_IO 		: {
						uint32_t io;

						io = codeSignatureReader_get_IO_in(&reader_cursor);
						local_edge.src_input_number = io & 0x0000ffff;
						local_edge.src_input_frag_order = io >> 16;
						break;
					}
					case READER_TOKEN_COMMENT 		: {
						break;
					}
					default 						: {
						printf("ERROR: in %s, syntaxe error: state is SRC_DESC\n", __func__);
					}
				}
				break;
			}
			case READER_STATE_EDGE 			: {
				switch(reader_cursor.token){
					case READER_TOKEN_NODE_ID 	: {
						local_edge.dst_id = atoi(reader_cursor.cursor);
						local_edge.dst_opcode_set = 0;
						local_edge.dst_output_number = 0;
						local_edge.dst_output_frag_order = 0;

						reader_state = READER_STATE_DST;
						break;
					}
					case READER_TOKEN_LABEL 		: {
						local_edge.edge_type = codeSignatureReader_get_irDependenceType(&reader_cursor);
						local_edge.edge_type_set = 1;

						reader_state = READER_STATE_EDGE_DESC;
						break;
					}
					case READER_TOKEN_COMMENT 		: {
						break;
					}
					default 						: {
						printf("ERROR: in %s, syntaxe error: state is EDGE\n", __func__);
					}
				}
				break;
			}
			case READER_STATE_EDGE_DESC 	: {
				switch(reader_cursor.token){
					case READER_TOKEN_NODE_ID 	: {
						local_edge.dst_id = atoi(reader_cursor.cursor);
						local_edge.dst_opcode_set = 0;
						local_edge.dst_output_number = 0;
						local_edge.dst_output_frag_order = 0;

						reader_state = READER_STATE_DST;
						break;
					}
					case READER_TOKEN_COMMENT 		: {
						break;
					}
					default 						: {
						printf("ERROR: in %s, syntaxe error: state is EDGE\n", __func__);
					}
				}
				break;
			}
			case READER_STATE_DST 			: {
				switch(reader_cursor.token){
					case READER_TOKEN_GRAPH_NAME 		: {
						if (array_add(&local_edge_array, &local_edge) < 0){
							printf("ERROR: in %s, unable to add element to array\n", __func__);
						}
						codeSignatureReader_push_signature(collection, &local_edge_array, graph_name);
						array_empty(&local_edge_array);
						signatureReader_get_graph_name(&reader_cursor, graph_name, CODESIGNATURE_NAME_MAX_SIZE);

						reader_state = READER_STATE_GRAPH;
						break;
					}
					case READER_TOKEN_NODE_ID 			: {
						if (array_add(&local_edge_array, &local_edge) < 0){
							printf("ERROR: in %s, unable to add element to array\n", __func__);
						}
						local_edge.src_id = atoi(reader_cursor.cursor);
						local_edge.src_opcode_set = 0;
						local_edge.src_input_number = 0;
						local_edge.src_input_frag_order = 0;

						reader_state = READER_STATE_SRC;
						break;
					}
					case READER_TOKEN_LABEL 		: {
						local_edge.dst_opcode = codeSignatureReader_get_opcode(&reader_cursor);
						local_edge.dst_opcode_set = 1;

						reader_state = READER_STATE_DST_DESC;
						break;
					}
					case READER_TOKEN_COMMENT 		: {
						break;
					}
					default 						: {
						printf("ERROR: in %s, syntaxe error: state is DST\n", __func__);
					}
				}
				break;
			}
			case READER_STATE_DST_DESC 		: {
				switch(reader_cursor.token){
					case READER_TOKEN_GRAPH_NAME 		: {
						if (array_add(&local_edge_array, &local_edge) < 0){
							printf("ERROR: in %s, unable to add element to array\n", __func__);
						}
						codeSignatureReader_push_signature(collection, &local_edge_array, graph_name);
						array_empty(&local_edge_array);
						signatureReader_get_graph_name(&reader_cursor, graph_name, CODESIGNATURE_NAME_MAX_SIZE);

						reader_state = READER_STATE_GRAPH;
						break;
					}
					case READER_TOKEN_NODE_ID 			: {
						if (array_add(&local_edge_array, &local_edge) < 0){
							printf("ERROR: in %s, unable to add element to array\n", __func__);
						}
						local_edge.src_id = atoi(reader_cursor.cursor);
						local_edge.src_opcode_set = 0;
						local_edge.src_input_number = 0;
						local_edge.src_input_frag_order = 0;

						reader_state = READER_STATE_SRC;
						break;
					}
					case READER_TOKEN_NODE_IO 		: {
						uint32_t io;

						io = codeSignatureReader_get_IO_out(&reader_cursor);
						local_edge.dst_output_number = io & 0x0000ffff;
						local_edge.dst_output_frag_order = io >> 16;
						break;
					}
					case READER_TOKEN_COMMENT 		: {
						break;
					}
					default 						: {
						printf("ERROR: in %s, syntaxe error: state is DST_DESC\n", __func__);
					}
				}
				break;
			}
		}
		readerCursor_get_next(&reader_cursor);
	}

	if (reader_state == READER_STATE_DST || reader_state == READER_STATE_DST_DESC){
		if (array_add(&local_edge_array, &local_edge) < 0){
			printf("ERROR: in %s, unable to add element to array\n", __func__);
		}
	}
	else{
		printf("ERROR: in %s, syntaxe error: incorrect state at the end of the file\n", __func__);
	}

	codeSignatureReader_push_signature(collection, &local_edge_array, graph_name);

	array_clean(&local_edge_array);

	munmap(buffer, buffer_size);
}

static void readerCursor_get_next(struct readerCursor* reader_cursor){
	uint8_t 	fetch_new = 0;
	uint64_t 	i;
	uint64_t 	j;

	switch(reader_cursor->token){
		case READER_TOKEN_NONE 			: {
			for (i = 0; i < reader_cursor->remaining_size; i++){
				if (reader_cursor->cursor[i] != ' ' && reader_cursor->cursor[i] != '\n' && reader_cursor->cursor[i] != '\a' && reader_cursor->cursor[i] != '\t'){
					reader_cursor->cursor += i;
					reader_cursor->remaining_size -= i;
					fetch_new = 1;
					break;
				}
			}
			break;
		}
		case READER_TOKEN_COMMENT 		: {
			reader_cursor->token = READER_TOKEN_NONE;

			for (i = 0; i < reader_cursor->remaining_size; i++){
				if (reader_cursor->cursor[i] == '\n' || reader_cursor->cursor[i] == '\a'){
					for (j = i + 1; j < reader_cursor->remaining_size; j++){
						if (reader_cursor->cursor[j] != ' ' && reader_cursor->cursor[j] != '\n' && reader_cursor->cursor[j] != '\a' && reader_cursor->cursor[j] != '\t'){
							reader_cursor->cursor += j;
							reader_cursor->remaining_size -= j;
							fetch_new = 1;
							break;
						}
					}
					break;
				}
			}
			break;
		}
		case READER_TOKEN_GRAPH_NAME 	: {
			reader_cursor->token = READER_TOKEN_NONE;

			for (i = 1; i < reader_cursor->remaining_size; i++){
				if (reader_cursor->cursor[i] == '"'){
					for (j = i + 1; j < reader_cursor->remaining_size; j++){
						if (reader_cursor->cursor[j] != ' ' && reader_cursor->cursor[j] != '\n' && reader_cursor->cursor[j] != '\a' && reader_cursor->cursor[j] != '\t'){
							reader_cursor->cursor += j;
							reader_cursor->remaining_size -= j;
							fetch_new = 1;
							break;
						}
					}
					break;
				}
			}
			break;
		}
		case READER_TOKEN_EDGE 			: {
			reader_cursor->token = READER_TOKEN_NONE;

			if (reader_cursor->remaining_size > 1 && reader_cursor->cursor[1] == '>'){
				for (j = 2; j < reader_cursor->remaining_size; j++){
					if (reader_cursor->cursor[j] != ' ' && reader_cursor->cursor[j] != '\n' && reader_cursor->cursor[j] != '\a' && reader_cursor->cursor[j] != '\t'){
						reader_cursor->cursor += j;
						reader_cursor->remaining_size -= j;
						fetch_new = 1;
						break;
					}
				}
			}
			break;
		}
		case READER_TOKEN_NODE_ID 		: {
			reader_cursor->token = READER_TOKEN_NONE;

			for (i = 0; i < reader_cursor->remaining_size; i++){
				if (reader_cursor->cursor[i] != '0' && reader_cursor->cursor[i] != '1' && reader_cursor->cursor[i] != '2' && reader_cursor->cursor[i] != '3' && reader_cursor->cursor[i] != '4' && reader_cursor->cursor[i] != '5' && reader_cursor->cursor[i] != '6' && reader_cursor->cursor[i] != '7' && reader_cursor->cursor[i] != '8' && reader_cursor->cursor[i] != '9'){
					for (j = i; j < reader_cursor->remaining_size; j++){
						if (reader_cursor->cursor[j] != ' ' && reader_cursor->cursor[j] != '\n' && reader_cursor->cursor[j] != '\a' && reader_cursor->cursor[j] != '\t'){
							reader_cursor->cursor += j;
							reader_cursor->remaining_size -= j;
							fetch_new = 1;
							break;
						}
					}
					break;
				}
			}
			break;
		}
		case READER_TOKEN_LABEL 		: {
			reader_cursor->token = READER_TOKEN_NONE;

			for (i = 1; i < reader_cursor->remaining_size; i++){
				if (reader_cursor->cursor[i] == ')'){
					for (j = i + 1; j < reader_cursor->remaining_size; j++){
						if (reader_cursor->cursor[j] != ' ' && reader_cursor->cursor[j] != '\n' && reader_cursor->cursor[j] != '\a' && reader_cursor->cursor[j] != '\t'){
							reader_cursor->cursor += j;
							reader_cursor->remaining_size -= j;
							fetch_new = 1;
							break;
						}
					}
					break;
				}
			}
			break;
		}
		case READER_TOKEN_NODE_IO 		: {
			reader_cursor->token = READER_TOKEN_NONE;

			for (i = 1; i < reader_cursor->remaining_size; i++){
				if (reader_cursor->cursor[i] == ']'){
					for (j = i + 1; j < reader_cursor->remaining_size; j++){
						if (reader_cursor->cursor[j] != ' ' && reader_cursor->cursor[j] != '\n' && reader_cursor->cursor[j] != '\a' && reader_cursor->cursor[j] != '\t'){
							reader_cursor->cursor += j;
							reader_cursor->remaining_size -= j;
							fetch_new = 1;
							break;
						}
					}
					break;
				}
			}
			break;
		}
	}

	if (fetch_new){
		switch(*reader_cursor->cursor){
			case '"' : {
				reader_cursor->token = READER_TOKEN_GRAPH_NAME;
				break;
			}
			case '#' : {
				reader_cursor->token = READER_TOKEN_COMMENT;
				break;
			}
			case '-' : {
				reader_cursor->token = READER_TOKEN_EDGE;
				break;
			}
			case '(' : {
				reader_cursor->token = READER_TOKEN_LABEL;
				break;
			}
			case '[' : {
				reader_cursor->token = READER_TOKEN_NODE_IO;
				break;
			}
			case '0' :
			case '1' :
			case '2' :
			case '3' :
			case '4' :
			case '5' :
			case '6' :
			case '7' :
			case '8' :
			case '9' : {
				reader_cursor->token = READER_TOKEN_NODE_ID;
				break;
			}
			default  : {
				printf("ERROR: in %s, incorrect character: %c\n", __func__, *reader_cursor->cursor);
				break;
			}
		}
	}
}

static void signatureReader_get_graph_name(struct readerCursor* reader_cursor, char* buffer, uint32_t buffer_length){
	uint32_t i;

	for (i = 1; i < reader_cursor->remaining_size && i < buffer_length - 1; i++){
		if (reader_cursor->cursor[i] == '"'){
			break;
		}
	}

	memcpy(buffer, reader_cursor->cursor + 1, i - 1);
	buffer[i - 1] = '\0';
}

static enum irOpcode codeSignatureReader_get_opcode(struct readerCursor* reader_cursor){
	uint32_t length;

	for (length = 0; length + 1 < reader_cursor->remaining_size; length++){
		if (reader_cursor->cursor[length + 1] == ')'){
			break;
		}
	}

	if (!strncmp(reader_cursor->cursor + 1, "ADD", length)){
		return IR_ADD;
	}
	else if (!strncmp(reader_cursor->cursor + 1, "AND", length)){
		return IR_AND;
	}
	else if (!strncmp(reader_cursor->cursor + 1, "NOT", length)){
		return IR_NOT;
	}
	else if (!strncmp(reader_cursor->cursor + 1, "OR", length)){
		return IR_OR;
	}
	else if (!strncmp(reader_cursor->cursor + 1, "ROR", length)){
		return IR_ROR;
	}
	else if (!strncmp(reader_cursor->cursor + 1, "SHL", length)){
		return IR_SHL;
	}
	else if (!strncmp(reader_cursor->cursor + 1, "SHR", length)){
		return IR_SHR;
	}
	else if (!strncmp(reader_cursor->cursor + 1, "SUB", length)){
		return IR_SUB;
	}
	else if (!strncmp(reader_cursor->cursor + 1, "XOR", length)){
		return IR_XOR;
	}
	else if (!strncmp(reader_cursor->cursor + 1, "INPUT", length)){
		return IR_INPUT;
	}
	else if (!strncmp(reader_cursor->cursor + 1, "*", length)){
		return IR_JOKER;
	}
	
	if (length >= 3){
		printf("ERROR: in %s, unable to convert string to ir Opcode %.3s, by default return ADD\n", __func__, reader_cursor->cursor + 1);
	}
	else{
		printf("ERROR: in %s, unable to convert string to ir Opcode, by default return ADD\n", __func__);
	}
	return IR_JOKER;
}

static enum irDependenceType codeSignatureReader_get_irDependenceType(struct readerCursor* reader_cursor){
	uint32_t length;

	for (length = 0; length + 1 < reader_cursor->remaining_size; length++){
		if (reader_cursor->cursor[length + 1] == ')'){
			break;
		}
	}

	if (!strncmp(reader_cursor->cursor + 1, "@", length)){
		return IR_DEPENDENCE_TYPE_ADDRESS;
	}
	/* a completer */

	if (length >= 3){
		printf("ERROR: in %s, unable to convert string to irDependenceType %.3s, by default return DIRECT\n", __func__, reader_cursor->cursor + 1);
	}
	else{
		printf("ERROR: in %s, unable to convert string to irDependenceType, by default return DIRECT\n", __func__);
	}
	return IR_DEPENDENCE_TYPE_DIRECT;
}

static uint32_t codeSignatureReader_get_IO(struct readerCursor* reader_cursor, char first_char){
	uint32_t 	result = 0;
	uint64_t 	i;
	uint64_t 	j;

	if (reader_cursor->remaining_size < 6){
		printf("ERROR: in %s, incomplete IO tag\n", __func__);
	}
	else{
		if (reader_cursor->cursor[1] == first_char && reader_cursor->cursor[2] == ':'){
			for (i = 0; i + 3 < reader_cursor->remaining_size; i++){
				if (reader_cursor->cursor[i + 3] == ':'){
					result = atoi(reader_cursor->cursor + 3) & 0x0000ffff;
					for (j = i + 1; j + 3 < reader_cursor->remaining_size; j++){
						if (reader_cursor->cursor[i + 3] == ']'){
							result |= atoi(reader_cursor->cursor + i + 4) << 16;
						}
					}
				}
			}
		}
		else{
			printf("ERROR: in %s, incorrect IO tag\n", __func__);
		}
	}

	return result;
}

struct localCodeNode{
	uint32_t 		id;
	enum irOpcode 	opcode;
	uint8_t 		opcode_set;
	uint16_t 		input_number;
	uint16_t 		input_frag_order;
	uint16_t 		output_number;
	uint16_t 		output_frag_order;
	struct node* 	node;
};

int32_t compare_localCodeNode(const void* arg1, const void* arg2);

static void codeSignatureReader_push_signature(struct codeSignatureCollection* collection, struct array* array, const char* graph_name){
	uint32_t 				i;
	struct localCodeEdge* 	edge;
	struct localCodeNode* 	node_buffer;
	struct localCodeNode* 	node_buffer_realloc;
	uint32_t 				nb_node;
	uint32_t 				node_offset;
	struct signatureNode 	signature_node;
	struct codeSignature 	code_signature;

	if (array_get_length(array) > 0){
		nb_node = 2 * array_get_length(array);
		node_buffer = (struct localCodeNode*)malloc(sizeof(struct localCodeNode) * nb_node);
		if (node_buffer == NULL){
			printf("ERROR: in %s, unable to allocate memory\n", __func__);
			return;
		}

		for (i = 0; i < array_get_length(array); i++){
			edge = (struct localCodeEdge*)array_get(array, i);

			node_buffer[2 * i].id 						= edge->src_id;
			node_buffer[2 * i].opcode 					= edge->src_opcode;
			node_buffer[2 * i].opcode_set 				= edge->src_opcode_set;
			node_buffer[2 * i].input_number 			= edge->src_input_number;
			node_buffer[2 * i].input_frag_order 		= edge->src_input_frag_order;
			node_buffer[2 * i].output_number 			= 0;
			node_buffer[2 * i].output_frag_order 		= 0;
			node_buffer[2 * i].node 					= NULL;

			node_buffer[2 * i + 1].id 					= edge->dst_id;
			node_buffer[2 * i + 1].opcode 				= edge->dst_opcode;
			node_buffer[2 * i + 1].opcode_set 			= edge->dst_opcode_set;
			node_buffer[2 * i + 1].input_number 		= 0;
			node_buffer[2 * i + 1].input_frag_order 	= 0;
			node_buffer[2 * i + 1].output_number 		= edge->dst_output_number;
			node_buffer[2 * i + 1].output_frag_order 	= edge->dst_output_frag_order;
			node_buffer[2 * i + 1].node 				= NULL;

		}

		qsort(node_buffer, nb_node, sizeof(struct localCodeNode), compare_localCodeNode);

		for (i = 1, node_offset = 0; i < nb_node; i++){
			if (node_buffer[i].id != node_buffer[node_offset].id){
				node_offset ++;
				if (node_offset != i){
					memcpy(node_buffer + node_offset, node_buffer + i, sizeof(struct localCodeNode));
				}
			}
			else if (node_buffer[i].opcode_set){
				if (!node_buffer[node_offset].opcode_set){
					node_buffer[node_offset].opcode 			= node_buffer[i].opcode;
					node_buffer[node_offset].opcode_set 		= 1;
					node_buffer[node_offset].input_number 		= node_buffer[i].input_number;
					node_buffer[node_offset].input_frag_order 	= node_buffer[i].input_frag_order;
					node_buffer[node_offset].output_number 		= node_buffer[i].output_number;
					node_buffer[node_offset].output_frag_order 	= node_buffer[i].output_frag_order;
				}
				else if (node_buffer[node_offset].opcode != node_buffer[i].opcode){
					printf("ERROR: in %s, multiple opcode defined for node %u\n", __func__, node_buffer[node_offset].id);
				}
			}
		}

		nb_node = node_offset + 1;
		node_buffer_realloc = (struct localCodeNode*)realloc(node_buffer, nb_node * sizeof(struct localCodeNode));
		if (node_buffer_realloc != NULL){
			node_buffer = node_buffer_realloc;
		}
		else{
			printf("ERROR: in %s, unable to realloc memory buffer\n", __func__);
		}

		graph_init(&(code_signature.graph), sizeof(struct signatureNode), sizeof(struct signatureEdge));

		for (i = 0; i < nb_node; i++){
			if (!node_buffer[i].opcode_set){
				printf("ERROR: in %s, opcode has not been set for node %u\n", __func__, node_buffer[i].id);
			}
			signature_node.opcode 				= node_buffer[i].opcode;
			signature_node.input_number 		= node_buffer[i].input_number;
			signature_node.input_frag_order 	= node_buffer[i].input_frag_order;
			signature_node.output_number 		= node_buffer[i].output_number;
			signature_node.output_frag_order 	= node_buffer[i].output_frag_order;
			node_buffer[i].node = graph_add_node(&(code_signature.graph), &signature_node);
		}

		for (i = 0; i < array_get_length(array); i++){
			struct localCodeNode* 	src_node;
			struct localCodeNode* 	dst_node;
			struct localCodeNode 	cmp_node;
			struct signatureEdge 	signature_edge;

			edge = (struct localCodeEdge*)array_get(array, i);
				
			cmp_node.id = edge->src_id;
			src_node = (struct localCodeNode*)bsearch (&cmp_node, node_buffer, nb_node, sizeof(struct localCodeNode), compare_localCodeNode);
			if (src_node == NULL){
				printf("ERROR: in %s, edge %u unable to fetch src node\n", __func__, i);
				continue;
			}
			if (src_node->node == NULL){
				printf("ERROR: in %s, rsc node is NULL\n", __func__);
			}

			cmp_node.id = edge->dst_id;
			dst_node = (struct localCodeNode*)bsearch (&cmp_node, node_buffer, nb_node, sizeof(struct localCodeNode), compare_localCodeNode);
			if (dst_node == NULL){
				printf("ERROR: in %s, edge %u unable to fetch dst node\n", __func__, i);
				continue;
			}
			if (dst_node->node == NULL){
				printf("ERROR: in %s, rsc node is NULL\n", __func__);
			}

			if (edge->edge_type_set){
				signature_edge.type = edge->edge_type;
			}
			else{
				signature_edge.type = IR_DEPENDENCE_TYPE_DIRECT;
			}
			if (graph_add_edge(&(code_signature.graph), src_node->node, dst_node->node, &signature_edge) == NULL){
				printf("ERROR: in %s, unable to add edge to the graph\n", __func__);
			}
		}

		strncpy(code_signature.name, graph_name, CODESIGNATURE_NAME_MAX_SIZE);
		code_signature.sub_graph_handle = NULL;

		if (codeSignature_add_signature_to_collection(collection, &code_signature)){
			printf("ERROR: in %s, unable to add signature to collection\n", __func__);
		}

		free(node_buffer);
	}
}

int32_t compare_localCodeNode(const void* arg1, const void* arg2){
	struct localCodeNode* node1 = (struct localCodeNode*)arg1;
	struct localCodeNode* node2 = (struct localCodeNode*)arg2;

	if (node1->id < node2->id){
		return -1;
	}
	else if (node1->id > node2->id){
		return 1;
	}
	return 0;
}