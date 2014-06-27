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
	READER_TOKEN_NODE_LABEL
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
	uint32_t 				dst_id;
	enum irOpcode 			dst_opcode;
	uint8_t 				dst_opcode_set;
};

static void readerCursor_get_next(struct readerCursor* reader_cursor);
static enum irOpcode codeSignatureReader_str_2_irOpcode(const char* str, uint64_t max_size);
static void codeSignatureReader_push_signature(struct codeSignatureCollection* collection, struct array* array, const char* graph_name);

void codeSignatureReader_parse(struct codeSignatureCollection* collection, const char* file_name){
	void* 						buffer;
	uint64_t 					buffer_size;
	struct readerCursor 		reader_cursor;
	struct localCodeEdge 		local_edge;
	struct array 				local_edge_array;
	char 						graph_name[CODESIGNATURE_NAME_MAX_SIZE];

	uint8_t 					src_set_id 			= 0;
	uint8_t 					src_set_opcode 		= 0;
	uint8_t 					dst_set_id 			= 0;
	uint8_t 					dst_set_opcode 		= 0;

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

	readerCursor_get_next(&reader_cursor);
	while(reader_cursor.token != READER_TOKEN_NONE){
		switch(reader_cursor.token){
			case READER_TOKEN_GRAPH_NAME 	: {
				uint32_t i;

				if (src_set_id && dst_set_id){
					local_edge.src_opcode_set = src_set_opcode;
					local_edge.dst_opcode_set = dst_set_opcode;

					if (array_add(&local_edge_array, &local_edge) < 0){
						printf("ERROR: in %s, unable to add element to array\n", __func__);
					}
				}

				codeSignatureReader_push_signature(collection, &local_edge_array, graph_name);

				array_empty(&local_edge_array);

				src_set_id 		= 0;
				src_set_opcode 	= 0;
				dst_set_id 		= 0;
				dst_set_opcode 	= 0;

				for (i = 1; i < reader_cursor.remaining_size && i < CODESIGNATURE_NAME_MAX_SIZE - 1; i++){
					if (reader_cursor.cursor[i] == '"'){
						break;
					}
				}

				memcpy(graph_name, reader_cursor.cursor + 1, i - 1);
				graph_name[i - 1] = '\0';

				break;
			}
			case READER_TOKEN_NODE_ID 		: {
				if (src_set_id){
					if (dst_set_id){
						local_edge.src_opcode_set = src_set_opcode;
						local_edge.dst_opcode_set = dst_set_opcode;

						if (array_add(&local_edge_array, &local_edge) < 0){
							printf("ERROR: in %s, unable to add element to array\n", __func__);
						}

						src_set_id 		= 0;
						src_set_opcode 	= 0;
						dst_set_id 		= 0;
						dst_set_opcode 	= 0;

						local_edge.src_id = atoi(reader_cursor.cursor);
						src_set_id = 1;
					}
					else{
						local_edge.dst_id = atoi(reader_cursor.cursor);
						dst_set_id = 1;
					}
				}
				else{
					local_edge.src_id = atoi(reader_cursor.cursor);
					src_set_id = 1;
				}
				break;
			}
			case READER_TOKEN_NODE_LABEL 	: {
				if (!dst_set_id){
					local_edge.src_opcode = codeSignatureReader_str_2_irOpcode(reader_cursor.cursor, reader_cursor.remaining_size);
					src_set_opcode = 1;
				}
				else{
					local_edge.dst_opcode = codeSignatureReader_str_2_irOpcode(reader_cursor.cursor, reader_cursor.remaining_size);
					dst_set_opcode = 1;
				}
				break;
			}
			default 						: {
				break;
			}
		}
		readerCursor_get_next(&reader_cursor);
	}

	if (src_set_id && dst_set_id){
		local_edge.src_opcode_set = src_set_opcode;
		local_edge.dst_opcode_set = dst_set_opcode;

		if (array_add(&local_edge_array, &local_edge) < 0){
			printf("ERROR: in %s, unable to add element to array\n", __func__);
		}
	}

	codeSignatureReader_push_signature(collection, &local_edge_array, graph_name);

	array_clean(&local_edge_array);

	munmap(buffer, buffer_size);
}

static void readerCursor_get_next(struct readerCursor* reader_cursor){
	switch(reader_cursor->token){
		case READER_TOKEN_NONE 			: {
			uint64_t i;

			for (i = 0; i < reader_cursor->remaining_size; i++){
				if (reader_cursor->cursor[i] != ' ' && reader_cursor->cursor[i] != '\n' && reader_cursor->cursor[i] != '\a' && reader_cursor->cursor[i] != '\t'){
					reader_cursor->cursor += i;
					reader_cursor->remaining_size -= i;

					switch(*reader_cursor->cursor){
						case '#' : {
							reader_cursor->token = READER_TOKEN_COMMENT;
							break;
						}
						case '"' : {
							reader_cursor->token = READER_TOKEN_GRAPH_NAME;
							break;
						}
						default  : {
							printf("ERROR: in %s, incorrect character: %c (case NONE)\n", __func__, *reader_cursor->cursor);
							break;
						}
					}
					break;
				}
			}

			break;
		}
		case READER_TOKEN_COMMENT 		: {
			uint64_t i;
			uint64_t j;

			reader_cursor->token = READER_TOKEN_NONE;

			for (i = 0; i < reader_cursor->remaining_size; i++){
				if (reader_cursor->cursor[i] == '\n' || reader_cursor->cursor[i] == '\a'){
					for (j = i + 1; j < reader_cursor->remaining_size; j++){
						if (reader_cursor->cursor[j] != ' ' && reader_cursor->cursor[j] != '\n' && reader_cursor->cursor[j] != '\a' && reader_cursor->cursor[j] != '\t'){
							reader_cursor->cursor += j;
							reader_cursor->remaining_size -= j;

							switch(*reader_cursor->cursor){
								case '#' : {
									reader_cursor->token = READER_TOKEN_COMMENT;
									break;
								}
								case '"' : {
									reader_cursor->token = READER_TOKEN_GRAPH_NAME;
									break;
								}
								case '(' : {
									reader_cursor->token = READER_TOKEN_NODE_LABEL;
									break;
								}
								case '-' : {
									reader_cursor->token = READER_TOKEN_EDGE;
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
									printf("ERROR: in %s, incorrect character: %c (case COMMENT)\n", __func__, *reader_cursor->cursor);
									break;
								}
							}
							break;
						}
					}
					break;
				}
			}

			break;
		}
		case READER_TOKEN_GRAPH_NAME 	: {
			uint64_t i;
			uint64_t j;

			reader_cursor->token = READER_TOKEN_NONE;

			for (i = 1; i < reader_cursor->remaining_size; i++){
				if (reader_cursor->cursor[i] == '"'){
					for (j = i + 1; j < reader_cursor->remaining_size; j++){
						if (reader_cursor->cursor[j] != ' ' && reader_cursor->cursor[j] != '\n' && reader_cursor->cursor[j] != '\a' && reader_cursor->cursor[j] != '\t'){
							reader_cursor->cursor += j;
							reader_cursor->remaining_size -= j;

							switch(*reader_cursor->cursor){
								case '#' : {
									reader_cursor->token = READER_TOKEN_COMMENT;
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
									printf("ERROR: in %s, incorrect character: %c (case NAME)\n", __func__, *reader_cursor->cursor);
									break;
								}
							}
							break;
						}
					}
					break;
				}
			}

			break;
		}
		case READER_TOKEN_EDGE 			: {
			uint64_t j;

			reader_cursor->token = READER_TOKEN_NONE;

			if (reader_cursor->remaining_size > 1 && reader_cursor->cursor[1] == '>'){
				for (j = 2; j < reader_cursor->remaining_size; j++){
					if (reader_cursor->cursor[j] != ' ' && reader_cursor->cursor[j] != '\n' && reader_cursor->cursor[j] != '\a' && reader_cursor->cursor[j] != '\t'){
						reader_cursor->cursor += j;
						reader_cursor->remaining_size -= j;

						switch(*reader_cursor->cursor){
							case '#' : {
								reader_cursor->token = READER_TOKEN_COMMENT;
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
								printf("ERROR: in %s, incorrect character: %c (case EDGE)\n", __func__, *reader_cursor->cursor);
								break;
							}
						}
						break;
					}
				}
			}

			break;
		}
		case READER_TOKEN_NODE_ID 		: {
			uint64_t i;
			uint64_t j;

			reader_cursor->token = READER_TOKEN_NONE;

			for (i = 0; i < reader_cursor->remaining_size; i++){
				if (reader_cursor->cursor[i] != '0' && reader_cursor->cursor[i] != '1' && reader_cursor->cursor[i] != '2' && reader_cursor->cursor[i] != '3' && reader_cursor->cursor[i] != '4' && reader_cursor->cursor[i] != '5' && reader_cursor->cursor[i] != '6' && reader_cursor->cursor[i] != '7' && reader_cursor->cursor[i] != '8' && reader_cursor->cursor[i] != '9'){
					for (j = i; j < reader_cursor->remaining_size; j++){
						if (reader_cursor->cursor[j] != ' ' && reader_cursor->cursor[j] != '\n' && reader_cursor->cursor[j] != '\a' && reader_cursor->cursor[j] != '\t'){
							reader_cursor->cursor += j;
							reader_cursor->remaining_size -= j;

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
									reader_cursor->token = READER_TOKEN_NODE_LABEL;
									break;
								}
								default  : {
									printf("ERROR: in %s, incorrect character: %c (case ID)\n", __func__, *reader_cursor->cursor);
									break;
								}
							}
							break;
						}
					}
					break;
				}
			}


			break;
		}
		case READER_TOKEN_NODE_LABEL 	: {
			uint64_t i;
			uint64_t j;

			reader_cursor->token = READER_TOKEN_NONE;

			for (i = 1; i < reader_cursor->remaining_size; i++){
				if (reader_cursor->cursor[i] == ')'){
					for (j = i + 1; j < reader_cursor->remaining_size; j++){
						if (reader_cursor->cursor[j] != ' ' && reader_cursor->cursor[j] != '\n' && reader_cursor->cursor[j] != '\a' && reader_cursor->cursor[j] != '\t'){
							reader_cursor->cursor += j;
							reader_cursor->remaining_size -= j;

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
									printf("ERROR: in %s, incorrect character: %c (case LABEL)\n", __func__, *reader_cursor->cursor);
									break;
								}
							}
							break;
						}
					}
					break;
				}
			}

			break;
		}
	}
}

static enum irOpcode codeSignatureReader_str_2_irOpcode(const char* str, uint64_t max_size){
	if (max_size > 3 && !memcmp (str + 1, "ADD", 3)){
		return IR_ADD;
	}
	else if (max_size > 3 && !memcmp(str + 1, "AND", 3)){
		return IR_AND;
	}
	else if (max_size > 3 && !memcmp(str + 1, "NOT", 3)){
		return IR_NOT;
	}
	else if (max_size > 2 && !memcmp(str + 1, "OR", 2)){
		return IR_OR;
	}
	else if (max_size > 3 && !memcmp(str + 1, "ROR", 3)){
		return IR_ROR;
	}
	else if (max_size > 3 && !memcmp(str + 1, "SHL", 3)){
		return IR_SHL;
	}
	else if (max_size > 3 && !memcmp(str + 1, "SHR", 3)){
		return IR_SHR;
	}
	else if (max_size > 3 && !memcmp(str + 1, "XOR", 3)){
		return IR_XOR;
	}
	
	if (max_size > 3){
		printf("ERROR: in %s, unable to convert string to ir Opcode (%c%c%c), by default return ADD\n", __func__, str[1], str[2], str[3]);
	}
	else{
		printf("ERROR: in %s, unable to convert string to ir Opcode, by default return ADD\n", __func__);
	}
	return IR_ADD;
}

struct localCodeNode{
	uint32_t 		id;
	enum irOpcode 	opcode;
	uint8_t 		opcode_set;
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

			node_buffer[2 * i].id 				= edge->src_id;
			node_buffer[2 * i].opcode 			= edge->src_opcode;
			node_buffer[2 * i].opcode_set 		= edge->src_opcode_set;
			node_buffer[2 * i].node 			= NULL;


			node_buffer[2 * i + 1].id 			= edge->dst_id;
			node_buffer[2 * i + 1].opcode 		= edge->dst_opcode;
			node_buffer[2 * i + 1].opcode_set 	= edge->dst_opcode_set;
			node_buffer[2 * i + 1].node 		= NULL;

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
					node_buffer[node_offset].opcode = node_buffer[i].opcode;
					node_buffer[node_offset].opcode_set = 1;
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

		graph_init(&(code_signature.graph), sizeof(struct signatureNode), 0);
			
		signature_node.input_number 		= 0;
		signature_node.input_frag_order 	= 0;
		signature_node.output_number 		= 0;
		signature_node.output_frag_order 	= 0;
		signature_node.is_input 			= 0;

		for (i = 0; i < nb_node; i++){
			if (!node_buffer[i].opcode_set){
				printf("ERROR: in %s, opcode has not been set for node %u\n", __func__, node_buffer[i].id);
			}
			signature_node.opcode = node_buffer[i].opcode;
			node_buffer[i].node = graph_add_node(&(code_signature.graph), &signature_node);
		}

		for (i = 0; i < array_get_length(array); i++){
			struct localCodeNode* 	src_node;
			struct localCodeNode* 	dst_node;
			struct localCodeNode 	cmp_node;

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

			if (graph_add_edge_(&(code_signature.graph), src_node->node, dst_node->node) == NULL){
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