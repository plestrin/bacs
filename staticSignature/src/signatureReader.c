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
	enum signatureNodeType 	src_type;
	enum irOpcode 			src_opcode;
	int32_t 				src_symbol;
	uint8_t 				src_label_set;
	uint16_t 				src_input_number;
	uint16_t 				src_input_frag_order;

	uint32_t 				dst_id;
	enum signatureNodeType 	dst_type;
	enum irOpcode 			dst_opcode;
	int32_t 				dst_symbol;
	uint8_t 				dst_label_set;
	uint16_t 				dst_output_number;
	uint16_t 				dst_output_frag_order;

	enum irDependenceType 	edge_type;
	uint8_t 				edge_type_set;
};

static void readerCursor_get_next(struct readerCursor* reader_cursor);

static void signatureReader_get_graph_name(struct readerCursor* reader_cursor, char* buffer, uint32_t buffer_length);

static enum signatureNodeType codeSignatureReader_get_node_label(struct readerCursor* reader_cursor, enum irOpcode* opcode, struct array* symbol_array);
static enum irDependenceType codeSignatureReader_get_irDependenceType(struct readerCursor* reader_cursor);
#define codeSignatureReader_get_IO_in(reader_cursor) codeSignatureReader_get_IO(reader_cursor, 'I')
#define codeSignatureReader_get_IO_out(reader_cursor) codeSignatureReader_get_IO(reader_cursor, 'O')
static uint32_t codeSignatureReader_get_IO(struct readerCursor* reader_cursor, char first_char);

static void codeSignatureReader_push_signature(struct codeSignatureCollection* collection, struct array* local_edge_array, struct array* symbol_array, const char* graph_name);


void codeSignatureReader_parse(struct codeSignatureCollection* collection, const char* file_name){
	void* 						buffer;
	uint64_t 					buffer_size;
	struct readerCursor 		reader_cursor;
	enum readerState 			reader_state;
	struct localCodeEdge 		local_edge;
	struct array 				local_edge_array;
	char 						graph_name[CODESIGNATURE_NAME_MAX_SIZE];
	struct array 				symbol_array;

	buffer = mapFile_map(file_name, &buffer_size);
	if (buffer == NULL){
		printf("ERROR: in %s, unable to map file: %s\n", __func__, file_name);
		return;
	}

	if (array_init(&local_edge_array, sizeof(struct localCodeEdge)) != 0 || array_init(&symbol_array, CODESIGNATURE_NAME_MAX_SIZE) != 0){
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
						local_edge.src_label_set = 0;
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
						local_edge.src_type = codeSignatureReader_get_node_label(&reader_cursor, &(local_edge.src_opcode), &symbol_array);
						if (local_edge.src_type == SIGNATURE_NODE_TYPE_SYMBOL){
							local_edge.src_symbol = array_get_length(&symbol_array) - 1;
						}
						local_edge.src_label_set = 1;

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
						local_edge.dst_label_set = 0;
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
						local_edge.dst_label_set = 0;
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
						codeSignatureReader_push_signature(collection, &local_edge_array, &symbol_array, graph_name);
						array_empty(&local_edge_array);
						array_empty(&symbol_array);
						signatureReader_get_graph_name(&reader_cursor, graph_name, CODESIGNATURE_NAME_MAX_SIZE);

						reader_state = READER_STATE_GRAPH;
						break;
					}
					case READER_TOKEN_NODE_ID 			: {
						if (array_add(&local_edge_array, &local_edge) < 0){
							printf("ERROR: in %s, unable to add element to array\n", __func__);
						}
						local_edge.src_id = atoi(reader_cursor.cursor);
						local_edge.src_label_set = 0;
						local_edge.src_input_number = 0;
						local_edge.src_input_frag_order = 0;

						reader_state = READER_STATE_SRC;
						break;
					}
					case READER_TOKEN_LABEL 		: {
						local_edge.dst_type = codeSignatureReader_get_node_label(&reader_cursor, &(local_edge.dst_opcode), &symbol_array);
						if (local_edge.dst_type == SIGNATURE_NODE_TYPE_SYMBOL){
							local_edge.dst_symbol = array_get_length(&symbol_array) - 1;
						}
						local_edge.dst_label_set = 1;

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
						codeSignatureReader_push_signature(collection, &local_edge_array, &symbol_array, graph_name);
						array_empty(&local_edge_array);
						array_empty(&symbol_array);
						signatureReader_get_graph_name(&reader_cursor, graph_name, CODESIGNATURE_NAME_MAX_SIZE);

						reader_state = READER_STATE_GRAPH;
						break;
					}
					case READER_TOKEN_NODE_ID 			: {
						if (array_add(&local_edge_array, &local_edge) < 0){
							printf("ERROR: in %s, unable to add element to array\n", __func__);
						}
						local_edge.src_id = atoi(reader_cursor.cursor);
						local_edge.src_label_set = 0;
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

	codeSignatureReader_push_signature(collection, &local_edge_array, &symbol_array, graph_name);

	array_clean(&local_edge_array);
	array_clean(&symbol_array);

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

static enum signatureNodeType codeSignatureReader_get_node_label(struct readerCursor* reader_cursor, enum irOpcode* opcode, struct array* symbol_array){
	uint32_t 	length;
	char 		symbol[CODESIGNATURE_NAME_MAX_SIZE];

	for (length = 0; length + 1 < reader_cursor->remaining_size; length++){
		if (reader_cursor->cursor[length + 1] == ')'){
			break;
		}
	}

	if (!strncmp(reader_cursor->cursor + 1, "ADD", length)){
		*opcode = IR_ADD;
		return SIGNATURE_NODE_TYPE_OPCODE;
	}
	else if (!strncmp(reader_cursor->cursor + 1, "AND", length)){
		*opcode = IR_AND;
		return SIGNATURE_NODE_TYPE_OPCODE;
	}
	else if (!strncmp(reader_cursor->cursor + 1, "NOT", length)){
		*opcode = IR_NOT;
		return SIGNATURE_NODE_TYPE_OPCODE;
	}
	else if (!strncmp(reader_cursor->cursor + 1, "OR", length)){
		*opcode = IR_OR;
		return SIGNATURE_NODE_TYPE_OPCODE;
	}
	else if (!strncmp(reader_cursor->cursor + 1, "ROR", length)){
		*opcode = IR_ROR;
		return SIGNATURE_NODE_TYPE_OPCODE;
	}
	else if (!strncmp(reader_cursor->cursor + 1, "SHL", length)){
		*opcode = IR_SHL;
		return SIGNATURE_NODE_TYPE_OPCODE;
	}
	else if (!strncmp(reader_cursor->cursor + 1, "SHR", length)){
		*opcode = IR_SHR;
		return SIGNATURE_NODE_TYPE_OPCODE;
	}
	else if (!strncmp(reader_cursor->cursor + 1, "SUB", length)){
		*opcode = IR_SUB;
		return SIGNATURE_NODE_TYPE_OPCODE;
	}
	else if (!strncmp(reader_cursor->cursor + 1, "XOR", length)){
		*opcode = IR_XOR;
		return SIGNATURE_NODE_TYPE_OPCODE;
	}
	else if (!strncmp(reader_cursor->cursor + 1, "INPUT", length)){
		*opcode = IR_INPUT;
		return SIGNATURE_NODE_TYPE_OPCODE;
	}
	else if (!strncmp(reader_cursor->cursor + 1, "*", length)){
		*opcode = IR_JOKER;
		return SIGNATURE_NODE_TYPE_OPCODE;
	}
	
	memset(symbol, '\0', CODESIGNATURE_NAME_MAX_SIZE);
	memcpy(symbol, reader_cursor->cursor + 1, (length > CODESIGNATURE_NAME_MAX_SIZE - 1) ? (CODESIGNATURE_NAME_MAX_SIZE - 1) : length);
	if (array_add(symbol_array, &symbol) < 0){
		printf("ERROR: in %s, unable to add element to array\n", __func__);
	}

	return SIGNATURE_NODE_TYPE_SYMBOL;
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
	else if (!strncmp(reader_cursor->cursor + 1, "I1F1", length)){
		return IR_DEPENDENCE_TYPE_I1F1;
	}
	else if (!strncmp(reader_cursor->cursor + 1, "I1F2", length)){
		return IR_DEPENDENCE_TYPE_I1F2;
	}
	else if (!strncmp(reader_cursor->cursor + 1, "I1F3", length)){
		return IR_DEPENDENCE_TYPE_I1F3;
	}
	else if (!strncmp(reader_cursor->cursor + 1, "I1F4", length)){
		return IR_DEPENDENCE_TYPE_I1F4;
	}
	else if (!strncmp(reader_cursor->cursor + 1, "I2F1", length)){
		return IR_DEPENDENCE_TYPE_I2F1;
	}
	else if (!strncmp(reader_cursor->cursor + 1, "I2F2", length)){
		return IR_DEPENDENCE_TYPE_I2F2;
	}
	else if (!strncmp(reader_cursor->cursor + 1, "I2F3", length)){
		return IR_DEPENDENCE_TYPE_I2F3;
	}
	else if (!strncmp(reader_cursor->cursor + 1, "I2F4", length)){
		return IR_DEPENDENCE_TYPE_I2F4;
	}
	else if (!strncmp(reader_cursor->cursor + 1, "I3F1", length)){
		return IR_DEPENDENCE_TYPE_I3F1;
	}
	else if (!strncmp(reader_cursor->cursor + 1, "I3F2", length)){
		return IR_DEPENDENCE_TYPE_I3F2;
	}
	else if (!strncmp(reader_cursor->cursor + 1, "I3F3", length)){
		return IR_DEPENDENCE_TYPE_I3F3;
	}
	else if (!strncmp(reader_cursor->cursor + 1, "I3F4", length)){
		return IR_DEPENDENCE_TYPE_I3F4;
	}
	else if (!strncmp(reader_cursor->cursor + 1, "I4F1", length)){
		return IR_DEPENDENCE_TYPE_I4F1;
	}
	else if (!strncmp(reader_cursor->cursor + 1, "I4F2", length)){
		return IR_DEPENDENCE_TYPE_I4F2;
	}
	else if (!strncmp(reader_cursor->cursor + 1, "I4F3", length)){
		return IR_DEPENDENCE_TYPE_I4F3;
	}
	else if (!strncmp(reader_cursor->cursor + 1, "I4F4", length)){
		return IR_DEPENDENCE_TYPE_I4F4;
	}
	else if (!strncmp(reader_cursor->cursor + 1, "I5F1", length)){
		return IR_DEPENDENCE_TYPE_I5F1;
	}
	else if (!strncmp(reader_cursor->cursor + 1, "I5F2", length)){
		return IR_DEPENDENCE_TYPE_I5F2;
	}
	else if (!strncmp(reader_cursor->cursor + 1, "I5F3", length)){
		return IR_DEPENDENCE_TYPE_I5F3;
	}
	else if (!strncmp(reader_cursor->cursor + 1, "I5F4", length)){
		return IR_DEPENDENCE_TYPE_I5F4;
	}
	else if (!strncmp(reader_cursor->cursor + 1, "O1F1", length)){
		return IR_DEPENDENCE_TYPE_O1F1;
	}
	else if (!strncmp(reader_cursor->cursor + 1, "O1F2", length)){
		return IR_DEPENDENCE_TYPE_O1F2;
	}
	else if (!strncmp(reader_cursor->cursor + 1, "O1F3", length)){
		return IR_DEPENDENCE_TYPE_O1F3;
	}
	else if (!strncmp(reader_cursor->cursor + 1, "O1F4", length)){
		return IR_DEPENDENCE_TYPE_O1F4;
	}
	else if (!strncmp(reader_cursor->cursor + 1, "O2F1", length)){
		return IR_DEPENDENCE_TYPE_O2F1;
	}
	else if (!strncmp(reader_cursor->cursor + 1, "O2F2", length)){
		return IR_DEPENDENCE_TYPE_O2F2;
	}
	else if (!strncmp(reader_cursor->cursor + 1, "O2F3", length)){
		return IR_DEPENDENCE_TYPE_O2F3;
	}
	else if (!strncmp(reader_cursor->cursor + 1, "O2F4", length)){
		return IR_DEPENDENCE_TYPE_O2F4;
	}
	else if (!strncmp(reader_cursor->cursor + 1, "O3F1", length)){
		return IR_DEPENDENCE_TYPE_O3F1;
	}
	else if (!strncmp(reader_cursor->cursor + 1, "O3F2", length)){
		return IR_DEPENDENCE_TYPE_O3F2;
	}
	else if (!strncmp(reader_cursor->cursor + 1, "O3F3", length)){
		return IR_DEPENDENCE_TYPE_O3F3;
	}
	else if (!strncmp(reader_cursor->cursor + 1, "O3F4", length)){
		return IR_DEPENDENCE_TYPE_O3F4;
	}
	else if (!strncmp(reader_cursor->cursor + 1, "O4F1", length)){
		return IR_DEPENDENCE_TYPE_O4F1;
	}
	else if (!strncmp(reader_cursor->cursor + 1, "O4F2", length)){
		return IR_DEPENDENCE_TYPE_O4F2;
	}
	else if (!strncmp(reader_cursor->cursor + 1, "O4F3", length)){
		return IR_DEPENDENCE_TYPE_O4F3;
	}
	else if (!strncmp(reader_cursor->cursor + 1, "O4F4", length)){
		return IR_DEPENDENCE_TYPE_O4F4;
	}

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

	if (reader_cursor->remaining_size < 6){
		printf("ERROR: in %s, incomplete IO tag\n", __func__);
	}
	else{
		if (reader_cursor->cursor[1] == first_char && reader_cursor->cursor[2] == ':'){
			for (i = 3; i < reader_cursor->remaining_size; i++){
				if (reader_cursor->cursor[i] == ':'){
					result = atoi(reader_cursor->cursor + 3) | (atoi(reader_cursor->cursor + i + 1) << 16);
					break;
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
	uint32_t 				id;
	uint8_t 				label_set;
	struct signatureNode 	signature_node;
	struct node* 			graph_node;
};

int32_t compare_localCodeNode(const void* arg1, const void* arg2);
int32_t compare_signatureSymbol(const void* arg1, const void* arg2);

static void codeSignatureReader_push_signature(struct codeSignatureCollection* collection, struct array* local_edge_array, struct array* symbol_array, const char* graph_name){
	uint32_t 						i;
	struct localCodeEdge* 			edge;
	struct localCodeNode* 			node_buffer;
	struct localCodeNode* 			node_buffer_realloc;
	uint32_t 						nb_node;
	uint32_t 						node_offset;
	struct codeSignature 			code_signature;
	struct signatureSymbolTable* 	symbol_table;
	uint8_t 						nb_raw_symbol;
	struct signatureSymbol** 		raw_symbol_index;

	
	if (array_get_length(local_edge_array) > 0){
		if (array_get_length(symbol_array) > 0){
			struct signatureSymbolTable* 	realloc_symbol_table;
			uint32_t 						j;

			nb_raw_symbol = array_get_length(symbol_array);
			symbol_table = (struct signatureSymbolTable*)malloc(signatureSymbolTable_get_size(nb_raw_symbol));
			if (symbol_table == NULL){
				printf("ERROR: in %s, unable to allocate memory\n", __func__);
				return;
			}

			for (i = 0; i < nb_raw_symbol; i++){
				symbol_table->symbols[i].id = 0;
				memcpy(symbol_table->symbols[i].name, (char*)array_get(symbol_array, i), CODESIGNATURE_NAME_MAX_SIZE);	
			}

			qsort(symbol_table->symbols, nb_raw_symbol, sizeof(struct signatureSymbol), compare_signatureSymbol);

			symbol_table->nb_symbol = 1;
			for (i = 1; i < nb_raw_symbol; i++){
				if (strncmp(symbol_table->symbols[i].name, symbol_table->symbols[i - 1].name, CODESIGNATURE_NAME_MAX_SIZE)){
					if (symbol_table->nb_symbol != i){
						memcpy(symbol_table->symbols + symbol_table->nb_symbol, symbol_table->symbols + i, sizeof(struct signatureSymbol));
					}
					symbol_table->nb_symbol ++;
				}
			}

			realloc_symbol_table = (struct signatureSymbolTable*)realloc(symbol_table, signatureSymbolTable_get_size(symbol_table->nb_symbol));
			if (realloc_symbol_table == NULL){
				printf("ERROR: in %s, unable to realloc memory buffer\n", __func__);
			}
			else{
				symbol_table = realloc_symbol_table;
			}

			raw_symbol_index = (struct signatureSymbol**)malloc(sizeof(struct signatureSymbol*) * nb_raw_symbol);
			if (raw_symbol_index == NULL){
				printf("ERROR: in %s, unable to allocate memory\n", __func__);
				free(symbol_table);
				return;
			}

			for (i = 0; i < nb_raw_symbol; i++){
				for (j = 0; j < symbol_table->nb_symbol; j++){
					if (!strncmp(symbol_table->symbols[j].name, (char*)array_get(symbol_array, i), CODESIGNATURE_NAME_MAX_SIZE)){
						raw_symbol_index[i] = symbol_table->symbols + j;
						break;
					}
				}
			}
		}
		else{
			raw_symbol_index = NULL;
			symbol_table = NULL;
		}

	
		nb_node = 2 * array_get_length(local_edge_array);
		node_buffer = (struct localCodeNode*)malloc(sizeof(struct localCodeNode) * nb_node);
		if (node_buffer == NULL){
			printf("ERROR: in %s, unable to allocate memory\n", __func__);
			if (raw_symbol_index != NULL){
				free(raw_symbol_index);
			}
			if (symbol_table != NULL){
				free(symbol_table);
			}
			return;
		}

		for (i = 0; i < array_get_length(local_edge_array); i++){
			edge = (struct localCodeEdge*)array_get(local_edge_array, i);

			node_buffer[2 * i].id 												= edge->src_id;
			node_buffer[2 * i].label_set 										= edge->src_label_set;
			if (node_buffer[2 * i].label_set){
				node_buffer[2 * i].signature_node.type 							= edge->src_type;
				switch(node_buffer[2 * i].signature_node.type){
					case SIGNATURE_NODE_TYPE_OPCODE : {
						node_buffer[2 * i].signature_node.node_type.opcode 		= edge->src_opcode;
						break;
					}
					case SIGNATURE_NODE_TYPE_SYMBOL : {
						node_buffer[2 * i].signature_node.node_type.symbol 		= raw_symbol_index[edge->src_symbol];
						break;
					}
				}
			}

			node_buffer[2 * i].signature_node.input_number 						= edge->src_input_number;
			node_buffer[2 * i].signature_node.input_frag_order 					= edge->src_input_frag_order;
			node_buffer[2 * i].signature_node.output_number 					= 0;
			node_buffer[2 * i].signature_node.output_frag_order 				= 0;
			node_buffer[2 * i].graph_node 										= NULL;

			node_buffer[2 * i + 1].id 											= edge->dst_id;
			node_buffer[2 * i + 1].label_set 									= edge->dst_label_set;
			if (node_buffer[2 * i + 1].label_set){
				node_buffer[2 * i + 1].signature_node.type 						= edge->dst_type;
				switch(node_buffer[2 * i + 1].signature_node.type){
					case SIGNATURE_NODE_TYPE_OPCODE : {
						node_buffer[2 * i + 1].signature_node.node_type.opcode 	= edge->dst_opcode;
						break;
					}
					case SIGNATURE_NODE_TYPE_SYMBOL : {
						node_buffer[2 * i + 1].signature_node.node_type.symbol 	= raw_symbol_index[edge->dst_symbol];
						break;
					}
				}
			}
			node_buffer[2 * i + 1].signature_node.input_number 					= 0;
			node_buffer[2 * i + 1].signature_node.input_frag_order 				= 0;
			node_buffer[2 * i + 1].signature_node.output_number 				= edge->dst_output_number;
			node_buffer[2 * i + 1].signature_node.output_frag_order 			= edge->dst_output_frag_order;
			node_buffer[2 * i + 1].graph_node 									= NULL;

		}

		qsort(node_buffer, nb_node, sizeof(struct localCodeNode), compare_localCodeNode);

		for (i = 1, node_offset = 0; i < nb_node; i++){
			if (node_buffer[i].id != node_buffer[node_offset].id){
				node_offset ++;
				if (node_offset != i){
					memcpy(node_buffer + node_offset, node_buffer + i, sizeof(struct localCodeNode));
				}
			}
			else if (node_buffer[i].label_set){
				if (!node_buffer[node_offset].label_set){
					node_buffer[node_offset].label_set 									= 1;
					node_buffer[node_offset].signature_node.type 						= node_buffer[i].signature_node.type;
					switch(node_buffer[node_offset].signature_node.type){
						case SIGNATURE_NODE_TYPE_OPCODE : {
							node_buffer[node_offset].signature_node.node_type.opcode 	= node_buffer[i].signature_node.node_type.opcode;
							break;
						}
						case SIGNATURE_NODE_TYPE_SYMBOL : {
							node_buffer[node_offset].signature_node.node_type.symbol 	= node_buffer[i].signature_node.node_type.symbol;
							break;
						}
					}
					
					node_buffer[node_offset].signature_node.input_number 				= node_buffer[i].signature_node.input_number;
					node_buffer[node_offset].signature_node.input_frag_order 			= node_buffer[i].signature_node.input_frag_order;
					node_buffer[node_offset].signature_node.output_number 				= node_buffer[i].signature_node.output_number;
					node_buffer[node_offset].signature_node.output_frag_order 			= node_buffer[i].signature_node.output_frag_order;
				}
				else{
					if (node_buffer[node_offset].signature_node.type != node_buffer[i].signature_node.type){
						printf("ERROR: in %s, multiple opcode defined for node %u\n", __func__, node_buffer[node_offset].id);
					}
					else{
						switch (node_buffer[node_offset].signature_node.type){
							case SIGNATURE_NODE_TYPE_OPCODE  :{
								if (node_buffer[node_offset].signature_node.node_type.opcode != node_buffer[i].signature_node.node_type.opcode){
									printf("ERROR: in %s, multiple opcode defined for node %u\n", __func__, node_buffer[node_offset].id);
								}
								break;
							}
							case SIGNATURE_NODE_TYPE_SYMBOL : {
								if (node_buffer[node_offset].signature_node.node_type.symbol != node_buffer[i].signature_node.node_type.symbol){
									printf("ERROR: in %s, multiple opcode defined for node %u\n", __func__, node_buffer[node_offset].id);
								}
								break;
							}
						}
					}
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
			if (!node_buffer[i].label_set){
				printf("ERROR: in %s, opcode has not been set for node %u\n", __func__, node_buffer[i].id);
			}
			else{
				node_buffer[i].graph_node = graph_add_node(&(code_signature.graph), &(node_buffer[i].signature_node));
			}
		}

		for (i = 0; i < array_get_length(local_edge_array); i++){
			struct localCodeNode* 	src_node;
			struct localCodeNode* 	dst_node;
			struct localCodeNode 	cmp_node;
			struct signatureEdge 	signature_edge;

			edge = (struct localCodeEdge*)array_get(local_edge_array, i);
				
			cmp_node.id = edge->src_id;
			src_node = (struct localCodeNode*)bsearch (&cmp_node, node_buffer, nb_node, sizeof(struct localCodeNode), compare_localCodeNode);
			if (src_node == NULL){
				printf("ERROR: in %s, edge %u unable to fetch src node\n", __func__, i);
				continue;
			}
			if (src_node->graph_node == NULL){
				printf("ERROR: in %s, src node is NULL\n", __func__);
				continue;
			}

			cmp_node.id = edge->dst_id;
			dst_node = (struct localCodeNode*)bsearch(&cmp_node, node_buffer, nb_node, sizeof(struct localCodeNode), compare_localCodeNode);
			if (dst_node == NULL){
				printf("ERROR: in %s, edge %u unable to fetch dst node\n", __func__, i);
				continue;
			}
			if (dst_node->graph_node == NULL){
				printf("ERROR: in %s, dst node is NULL\n", __func__);
				continue;
			}

			if (edge->edge_type_set){
				signature_edge.type = edge->edge_type;
			}
			else{
				signature_edge.type = IR_DEPENDENCE_TYPE_DIRECT;
			}
			if (graph_add_edge(&(code_signature.graph), src_node->graph_node, dst_node->graph_node, &signature_edge) == NULL){
				printf("ERROR: in %s, unable to add edge to the graph\n", __func__);
			}
		}

		strncpy(code_signature.name, graph_name, CODESIGNATURE_NAME_MAX_SIZE);
		code_signature.sub_graph_handle 	= NULL;
		code_signature.symbol_table 		= symbol_table;

		if (codeSignature_add_signature_to_collection(collection, &code_signature)){
			printf("ERROR: in %s, unable to add signature to collection\n", __func__);
		}

		free(node_buffer);

		if (raw_symbol_index != NULL){
			free(raw_symbol_index);
		}
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

int32_t compare_signatureSymbol(const void* arg1, const void* arg2){
	struct signatureSymbol* sym1 = (struct signatureSymbol*)arg1;
	struct signatureSymbol* sym2 = (struct signatureSymbol*)arg2;

	return strncmp(sym1->name, sym2->name, CODESIGNATURE_NAME_MAX_SIZE);
}