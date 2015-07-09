#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <search.h>

#include "graphReader.h"
#include "base.h"

enum graphReaderToken{
	READER_TOKEN_NONE,
	READER_TOKEN_COMMENT,
	READER_TOKEN_GRAPH_NAME,
	READER_TOKEN_EDGE,
	READER_TOKEN_NODE_ID,
	READER_TOKEN_LABEL,
	READER_TOKEN_NODE_IO
};

enum graphReaderState{
	READER_STATE_START,
	READER_STATE_GRAPH,
	READER_STATE_SRC,
	READER_STATE_SRC_DESC,
	READER_STATE_EDGE,
	READER_STATE_EDGE_DESC,
	READER_STATE_DST,
	READER_STATE_DST_DESC
};

struct graphReaderCursor{
	const char* 			cursor;
	size_t 					remaining_size;
	enum graphReaderToken 	token;
};

static void graphReaderCursor_get_next(struct graphReaderCursor* reader_cursor);

static void graphReaderCursor_get_next(struct graphReaderCursor* reader_cursor){
	uint32_t fetch_new = 0;
	size_t 	i;
	size_t 	j;

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
				log_err_m("incorrect character: %c", *reader_cursor->cursor);
				break;
			}
		}
	}
}

struct graphReaderNode{
	uint32_t 		id;
	void* 			ptr;
};

static int32_t graphReaderNode_compare_id(const void* data1, const void* data2){
	const struct graphReaderNode* node1 = (const struct graphReaderNode*)data1;
	const struct graphReaderNode* node2 = (const struct graphReaderNode*)data2;

	if (node1->id < node2->id){
		return -1;
	}
	else if (node1->id > node2->id){
		return 1;
	}
	else{
		return 0;
	}
}

struct graphReaderEdge{
	struct graphReaderNode* src;
	struct graphReaderNode* dst;
	const char* 			label_ptr;
	size_t 					label_size;
};

static void graphReader_get_graph_name(struct graphReaderCursor* reader_cursor, struct graphReaderCallback* callback);
static void graphReader_get_graph_label(struct graphReaderCursor* reader_cursor, struct graphReaderCallback* callback);
static struct graphReaderNode* graphReader_get_node(struct graphReaderCursor* reader_cursor, void** binary_tree_root, struct graphReaderCallback* callback);
static void graphReader_get_node_label(struct graphReaderCursor* reader_cursor, struct graphReaderNode* node, struct graphReaderCallback* callback);
static void graphReader_get_node_io(struct graphReaderCursor* reader_cursor, struct graphReaderNode* node, struct graphReaderCallback* callback);
static void graphReader_get_edge_label(struct graphReaderCursor* reader_cursor, struct graphReaderEdge* edge);
static void graphReader_set_edge(struct graphReaderEdge* edge, struct graphReaderCallback* callback);

void graphReader_parse(const void* buffer, size_t buffer_size, struct graphReaderCallback* callback){
	struct graphReaderCursor 	reader_cursor;
	enum graphReaderState 		reader_state;
	struct graphReaderNode*	 	node 				= NULL;
	struct graphReaderEdge		edge;
	void* 						binary_tree_root 	= NULL;

	reader_cursor.cursor = (const char*)buffer;
	reader_cursor.remaining_size = buffer_size;
	reader_cursor.token = READER_TOKEN_NONE;
	reader_state = READER_STATE_START;

	graphReaderCursor_get_next(&reader_cursor);
	while(reader_cursor.token != READER_TOKEN_NONE){
		switch(reader_state){
			case READER_STATE_START 		: {
				switch(reader_cursor.token){
					case READER_TOKEN_GRAPH_NAME 	: {
						graphReader_get_graph_name(&reader_cursor, callback);

						reader_state = READER_STATE_GRAPH;
						break;
					}
					case READER_TOKEN_COMMENT 		: {
						break;
					}
					default 						: {
						log_err("syntaxe error: state is START");
					}
				}
				break;
			}
			case READER_STATE_GRAPH 		: {
				switch(reader_cursor.token){
					case READER_TOKEN_GRAPH_NAME 	: {
						log_warn("empty signature -> skip");
						graphReader_get_graph_name(&reader_cursor, callback);

						reader_state = READER_STATE_GRAPH;
						break;
					}
					case READER_TOKEN_LABEL 		: {
						graphReader_get_graph_label(&reader_cursor, callback);
						break;
					}
					case READER_TOKEN_NODE_ID 		: {
						if ((node = graphReader_get_node(&reader_cursor, &binary_tree_root, callback)) == NULL){
							log_err("unable to get graphReaderNode");
						}

						reader_state = READER_STATE_SRC;
						break;
					}
					case READER_TOKEN_COMMENT 		: {
						break;
					}
					default 						: {
						log_err("syntaxe error: state is GRAPH");
					}
				}
				break;
			}
			case READER_STATE_SRC 			: {
				switch(reader_cursor.token){
					case READER_TOKEN_LABEL 		: {
						graphReader_get_node_label(&reader_cursor, node, callback);

						reader_state = READER_STATE_SRC_DESC;
						break;
					}
					case READER_TOKEN_EDGE 			: {
						edge.src = node;
						edge.label_ptr = NULL;

						reader_state = READER_STATE_EDGE;
						break;
					}
					case READER_TOKEN_COMMENT 		: {
						break;
					}
					default 						: {
						log_err("syntaxe error: state is SRC");
					}
				}
				break;
			}
			case READER_STATE_SRC_DESC 		: {
				switch(reader_cursor.token){
					case READER_TOKEN_EDGE 			: {
						edge.src = node;
						edge.label_ptr = NULL;

						reader_state = READER_STATE_EDGE;
						break;
					}
					case READER_TOKEN_NODE_IO 		: {
						graphReader_get_node_io(&reader_cursor, node, callback);
						break;
					}
					case READER_TOKEN_COMMENT 		: {
						break;
					}
					default 						: {
						log_err("syntaxe error: state is SRC_DESC");
					}
				}
				break;
			}
			case READER_STATE_EDGE 			: {
				switch(reader_cursor.token){
					case READER_TOKEN_NODE_ID 		: {
						if ((node = graphReader_get_node(&reader_cursor, &binary_tree_root, callback)) == NULL){
							log_err("unable to get graphReaderNode");
						}

						reader_state = READER_STATE_DST;
						break;
					}
					case READER_TOKEN_LABEL 		: {
						graphReader_get_edge_label(&reader_cursor, &edge);

						reader_state = READER_STATE_EDGE_DESC;
						break;
					}
					case READER_TOKEN_COMMENT 		: {
						break;
					}
					default 						: {
						log_err("syntaxe error: state is EDGE");
					}
				}
				break;
			}
			case READER_STATE_EDGE_DESC 	: {
				switch(reader_cursor.token){
					case READER_TOKEN_NODE_ID 		: {
						if ((node = graphReader_get_node(&reader_cursor, &binary_tree_root, callback)) == NULL){
							log_err("unable to get graphReaderNode");
						}

						reader_state = READER_STATE_DST;
						break;
					}
					case READER_TOKEN_COMMENT 		: {
						break;
					}
					default 						: {
						log_err("syntaxe error: state is EDGE");
					}
				}
				break;
			}
			case READER_STATE_DST 			: {
				switch(reader_cursor.token){
					case READER_TOKEN_GRAPH_NAME 		: {
						edge.dst = node;
						graphReader_set_edge(&edge, callback);

						if (callback->callback_graph_end != NULL){
							callback->callback_graph_end(callback->arg);
						}

						tdestroy(binary_tree_root, free);
						binary_tree_root = NULL;

						graphReader_get_graph_name(&reader_cursor, callback);

						reader_state = READER_STATE_GRAPH;
						break;
					}
					case READER_TOKEN_NODE_ID 			: {
						edge.dst = node;
						graphReader_set_edge(&edge, callback);

						if ((node = graphReader_get_node(&reader_cursor, &binary_tree_root, callback)) == NULL){
							log_err("unable to get graphReaderNode");
						}

						reader_state = READER_STATE_SRC;
						break;
					}
					case READER_TOKEN_LABEL 		: {
						graphReader_get_node_label(&reader_cursor, node, callback);

						reader_state = READER_STATE_DST_DESC;
						break;
					}
					case READER_TOKEN_COMMENT 		: {
						break;
					}
					default 						: {
						log_err("syntaxe error: state is DST");
					}
				}
				break;
			}
			case READER_STATE_DST_DESC 		: {
				switch(reader_cursor.token){
					case READER_TOKEN_GRAPH_NAME 		: {
						edge.dst = node;
						graphReader_set_edge(&edge, callback);

						if (callback->callback_graph_end != NULL){
							callback->callback_graph_end(callback->arg);
						}

						tdestroy(binary_tree_root, free);
						binary_tree_root = NULL;

						graphReader_get_graph_name(&reader_cursor, callback);

						reader_state = READER_STATE_GRAPH;
						break;
					}
					case READER_TOKEN_NODE_ID 			: {
						edge.dst = node;
						graphReader_set_edge(&edge, callback);

						if ((node = graphReader_get_node(&reader_cursor, &binary_tree_root, callback)) == NULL){
							log_err("unable to get graphReaderNode");
						}

						reader_state = READER_STATE_SRC;
						break;
					}
					case READER_TOKEN_NODE_IO 		: {
						graphReader_get_node_io(&reader_cursor, node, callback);
						break;
					}
					case READER_TOKEN_COMMENT 		: {
						break;
					}
					default 						: {
						log_err("syntaxe error: state is DST_DESC");
					}
				}
				break;
			}
		}
		graphReaderCursor_get_next(&reader_cursor);
	}

	if (reader_state == READER_STATE_DST || reader_state == READER_STATE_DST_DESC){
		edge.dst = node;
		graphReader_set_edge(&edge, callback);
	}
	else{
		log_err("syntaxe error: incorrect state at the end of the file");
	}

	if (callback->callback_graph_end != NULL){
		callback->callback_graph_end(callback->arg);
	}

	tdestroy(binary_tree_root, free);
}

static void graphReader_get_graph_name(struct graphReaderCursor* reader_cursor, struct graphReaderCallback* callback){
	char* end;

	if (reader_cursor->remaining_size <= 1){
		return;
	}

	if (callback->callback_graph_name != NULL){
		if ((end = (char*)memchr(reader_cursor->cursor + 1, '"', reader_cursor->remaining_size - 1)) == NULL){
			callback->callback_graph_name(reader_cursor->cursor + 1, reader_cursor->remaining_size - 1, callback->arg);
		}
		else{
			callback->callback_graph_name(reader_cursor->cursor + 1, (size_t)(end - (reader_cursor->cursor + 1)), callback->arg);
		}
	}
}

static void graphReader_get_graph_label(struct graphReaderCursor* reader_cursor, struct graphReaderCallback* callback){
	char* end;

	if (reader_cursor->remaining_size <= 1){
		return;
	}

	if (callback->callback_graph_label != NULL){
		if ((end = (char*)memchr(reader_cursor->cursor + 1, ')', reader_cursor->remaining_size - 1)) == NULL){
			callback->callback_graph_label(reader_cursor->cursor + 1, reader_cursor->remaining_size - 1, callback->arg);
		}
		else{
			callback->callback_graph_label(reader_cursor->cursor + 1, (size_t)(end - (reader_cursor->cursor + 1)), callback->arg);
		}
	}
}

static struct graphReaderNode* graphReader_get_node(struct graphReaderCursor* reader_cursor, void** binary_tree_root, struct graphReaderCallback* callback){
	struct graphReaderNode*		new_node;
	struct graphReaderNode** 	existing_node;

	if ((new_node = (struct graphReaderNode*)malloc(sizeof(struct graphReaderNode))) == NULL){
		log_err("unable to allocate memory");
	}
	else{
		new_node->id = (uint32_t)atoi(reader_cursor->cursor);

		existing_node = (struct graphReaderNode**)tsearch((void*)new_node, binary_tree_root, graphReaderNode_compare_id);
		if (existing_node == NULL){
			log_err("tsearch failed");
			free(new_node);
		}
		else if (*existing_node != new_node){
			free(new_node);
			new_node = *existing_node;
		}
		else{
			new_node->ptr = NULL;
			if (callback->callback_create_node != NULL){
				new_node->ptr = callback->callback_create_node(callback->arg);
			}
		}
	}

	return new_node;
}

static void graphReader_get_node_label(struct graphReaderCursor* reader_cursor, struct graphReaderNode* node, struct graphReaderCallback* callback){
	char* end;

	if (reader_cursor->remaining_size <= 1){
		return;
	}

	if (callback->callback_node_label != NULL && node != NULL){
		if ((end = (char*)memchr(reader_cursor->cursor + 1, ')', reader_cursor->remaining_size - 1)) == NULL){
			callback->callback_node_label(reader_cursor->cursor + 1, reader_cursor->remaining_size - 1, node->ptr, callback->arg);
		}
		else{
			callback->callback_node_label(reader_cursor->cursor + 1, (size_t)(end - (reader_cursor->cursor + 1)), node->ptr, callback->arg);
		}
	}
}

static void graphReader_get_node_io(struct graphReaderCursor* reader_cursor, struct graphReaderNode* node, struct graphReaderCallback* callback){
	char* end;

	if (reader_cursor->remaining_size <= 1){
		return;
	}

	if (callback->callback_node_io != NULL && node != NULL){
		if ((end = (char*)memchr(reader_cursor->cursor + 1, ']', reader_cursor->remaining_size - 1)) == NULL){
			callback->callback_node_io(reader_cursor->cursor + 1, reader_cursor->remaining_size - 1, node->ptr, callback->arg);
		}
		else{
			callback->callback_node_io(reader_cursor->cursor + 1, (size_t)(end - (reader_cursor->cursor + 1)), node->ptr, callback->arg);
		}
	}
}

static void graphReader_get_edge_label(struct graphReaderCursor* reader_cursor, struct graphReaderEdge* edge){
	char* end;

	if (reader_cursor->remaining_size <= 1){
		return;
	}

	edge->label_ptr = reader_cursor->cursor + 1;
	if ((end = (char*)memchr(reader_cursor->cursor + 1, ')', reader_cursor->remaining_size - 1)) == NULL){
		edge->label_size = reader_cursor->remaining_size - 1;
	}
	else{
		edge->label_size = (size_t)(end - (reader_cursor->cursor + 1));
	}
}

static void graphReader_set_edge(struct graphReaderEdge* edge, struct graphReaderCallback* callback){
	void* edge_ptr;

	if (callback->callback_create_edge != NULL && edge->src != NULL && edge->dst != NULL){
		edge_ptr = callback->callback_create_edge(edge->src->ptr, edge->dst->ptr, callback->arg);
		if (callback->callback_edge_label != NULL && edge_ptr != NULL && edge->label_ptr != NULL){
			callback->callback_edge_label(edge->label_ptr, edge->label_size, edge_ptr, callback->arg);
		}
	}
}