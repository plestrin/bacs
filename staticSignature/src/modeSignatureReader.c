#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>

#include "modeSignatureReader.h"
#include "modeSignature.h"
#include "synthesisGraph.h"
#include "mapFile.h"
#include "graphReader.h"
#include "set.h"
#include "base.h"

struct modeSignatureBuilder{
	struct modeSignature 		mode_signature;
	struct set* 				symbol_set;
	uint32_t 					is_name_set;
	struct signatureCollection* collection;
};

static int32_t modeSignatureBuilder_init(struct modeSignatureBuilder* builder, struct signatureCollection* collection){
	graph_init(&(builder->mode_signature.signature.graph), sizeof(struct modeSignatureNode), sizeof(struct modeSignatureEdge));
	
	builder->is_name_set = 0;
	builder->collection = collection;

	if ((builder->symbol_set = set_create(SIGNATURE_NAME_MAX_SIZE, 4)) == NULL){
		log_err("unable to create set");
		return -1;
	}

	return 0;
}

#define modeSignatureBuilder_reset(builder) 																						\
	(builder)->is_name_set = 0; 																									\
	set_empty((builder)->symbol_set); 																								\
	graph_init(&((builder)->mode_signature.signature.graph), sizeof(struct modeSignatureNode), sizeof(struct modeSignatureEdge));

#define modeSignatureBuidler_clean(builder) set_delete((builder)->symbol_set)

static void modeSignatureReader_handle_graph_name(const char* str, size_t str_len, void* arg){
	struct modeSignatureBuilder* builder = (struct modeSignatureBuilder*)arg;

	memset(builder->mode_signature.signature.name, 0, SIGNATURE_NAME_MAX_SIZE);
	memcpy(builder->mode_signature.signature.name, str, min(SIGNATURE_NAME_MAX_SIZE - 1, str_len));

	memset(builder->mode_signature.signature.symbol, 0, SIGNATURE_NAME_MAX_SIZE);
	memcpy(builder->mode_signature.signature.symbol, str, min(SIGNATURE_NAME_MAX_SIZE - 1, str_len));

	builder->is_name_set = 1;
}

static void* modeSignatureReader_handle_new_node(void* arg){
	struct node* 					node;
	struct modeSignatureNode* 		mode_signature_node;
	struct modeSignatureBuilder* 	builder = (struct modeSignatureBuilder*)arg;

	if ((node = graph_add_node_(&(builder->mode_signature.signature.graph))) == NULL){
		log_err("unable to add node to graph");
	}
	else{
		mode_signature_node = (struct modeSignatureNode*)&(node->data);
		mode_signature_node->type = MODESIGNATURE_NODE_TYPE_INVALID;
	}

	return node;
}

#pragma GCC diagnostic ignored "-Wunused-parameter"
static void modeSignatureReader_handle_node_label(const char* str, size_t str_len, void* ptr, void* arg){
	struct node* 				node = (struct node*)ptr;
	struct modeSignatureNode* 	mode_signature_node = (struct modeSignatureNode*)&(node->data);

	if (str_len == 1 && str[0] == '*'){
		mode_signature_node->type = MODESIGNATURE_NODE_TYPE_RAW;
	}
	else{
		if (str_len >= SIGNATURE_NAME_MAX_SIZE){
			log_warn("node label is too long");
		}
		memset(mode_signature_node->node_type.name, 0, SIGNATURE_NAME_MAX_SIZE);
		memcpy(mode_signature_node->node_type.name, str, min(SIGNATURE_NAME_MAX_SIZE - 1, str_len));
		mode_signature_node->type = MODESIGNATURE_NODE_TYPE_SYMBOL;
	}
}

static void* modeSignatureReader_handle_new_edge(void* src, void* dst, void* arg){
	struct edge* 					edge;
	struct modeSignatureEdge* 		mode_signature_edge;
	struct modeSignatureBuilder* 	builder = (struct modeSignatureBuilder*)arg;

	if ((edge = graph_add_edge_(&(builder->mode_signature.signature.graph), (struct node*)src, (struct node*)dst)) == NULL){
		log_err("unable to add edge to graph");
	}
	else{
		mode_signature_edge = (struct modeSignatureEdge*)&(edge->data);
		mode_signature_edge->tag = SYNTHESISGRAPH_EGDE_TAG_RAW;
	}

	return edge;
}

#pragma GCC diagnostic ignored "-Wunused-parameter"
static void modeSignatureReader_handle_edge_label(const char* str, size_t str_len, void* ptr, void* arg){
	struct edge* 				edge = (struct edge*)ptr;
	struct modeSignatureEdge* 	mode_signature_edge = (struct modeSignatureEdge*)&(edge->data);
	
	if (str_len > 1 && (str[0] == 'I' || str[0] == 'O')){
		if (str[1] >= 48 && str[1] <= 57){
			if (str[0] == 'I'){
				mode_signature_edge->tag = synthesisGraph_get_edge_tag_input(atoi(str + 1));
			}
			else{
				mode_signature_edge->tag = synthesisGraph_get_edge_tag_output(atoi(str + 1));
			}
			return;
		}
	}

	log_err("unable to convert string to edge tag");
}

static void modeSignatureReader_handle_flush_graph(void* arg){
	struct modeSignatureBuilder* builder = (struct modeSignatureBuilder*)arg;

	if (!builder->is_name_set){
		log_warn("modeSignature name has not been set");
	}

	builder->mode_signature.signature.sub_graph_handle 	= NULL;
	builder->mode_signature.signature.symbol_table 		= NULL;

	modeSignature_init(&(builder->mode_signature));
	if (signatureCollection_add(builder->collection, &(builder->mode_signature))){
		log_err("unable to add signature to collection");
	}

	modeSignatureBuilder_reset(builder);
}

void modeSignatureReader_parse(struct signatureCollection* collection, const char* file_name){
	void* 						buffer;
	size_t 						buffer_size;
	struct modeSignatureBuilder builder;
	struct graphReaderCallback 	callback = {
		.callback_graph_name 	= modeSignatureReader_handle_graph_name,
		.callback_graph_label 	= NULL,
		.callback_graph_end 	= modeSignatureReader_handle_flush_graph,
		.callback_create_node 	= modeSignatureReader_handle_new_node,
		.callback_node_label 	= modeSignatureReader_handle_node_label,
		.callback_node_io 		= NULL,
		.callback_create_edge 	= modeSignatureReader_handle_new_edge,
		.callback_edge_label 	= modeSignatureReader_handle_edge_label,
		.arg 					= (void*)&builder
	};

	buffer = mapFile_map(file_name, &buffer_size);
	if (buffer == NULL){
		log_err_m("unable to map file: %s", file_name);
		return;
	}

	if (modeSignatureBuilder_init(&builder, collection)){
		log_err("unable to init modeSignatureBuilder");
		goto exit;
	}

	graphReader_parse(buffer, buffer_size, &callback);
	modeSignatureBuidler_clean(&builder);

	exit:
	munmap(buffer, buffer_size);
}