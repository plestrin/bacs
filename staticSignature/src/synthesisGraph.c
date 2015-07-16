#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "synthesisGraph.h"
#include "result.h"
#include "dijkstra.h"
#include "base.h"

/* 	DIRECT, 				ADDRESS, 				SHIFT_DISP, 			DIVISOR, 				ROUND_OFF, 				SUBSTITUTE, 			MACRO*/
static const uint32_t irEdge_distance_array_OI[NB_DEPENDENCE_TYPE] = {
	0, 						DIJKSTRA_INVALID_DST, 	0, 						0, 						0, 						0, 						DIJKSTRA_INVALID_DST
};

static const uint32_t irEdge_distance_array_II[2][NB_DEPENDENCE_TYPE] = {
	{0, 					1, 						0, 						0, 						0, 						0, 						DIJKSTRA_INVALID_DST},
	{DIJKSTRA_INVALID_DST, 	DIJKSTRA_INVALID_DST, 	DIJKSTRA_INVALID_DST, 	DIJKSTRA_INVALID_DST, 	DIJKSTRA_INVALID_DST, 	DIJKSTRA_INVALID_DST, 	DIJKSTRA_INVALID_DST}
};

uint32_t irEdge_get_distance_OI(void* arg){
	return irEdge_distance_array_OI[((struct irDependence*)arg)->type];
}

uint32_t irEdge_get_distance_II(void* arg, uint32_t dst){
	return irEdge_distance_array_II[dst][((struct irDependence*)arg)->type];
}

enum synthesisNodeType{
	SYNTHESISNODETYPE_RESULT,
	SYNTHESISNODETYPE_OI_PATH,
	SYNTHESISNODETYPE_II_PATH,
	SYNTHESISNODETYPE_IR_NODE
};

struct signatureCluster{
	struct node* 				synthesis_graph_node;
	uint32_t 					nb_in_parameter;
	uint32_t 					nb_ou_parameter;
	struct parameterMapping* 	parameter_mapping;
	struct array 				instance_array;
};

static int32_t signatureCluster_init(struct signatureCluster* cluster, struct parameterMapping* mapping, struct result* result, struct node* node){
	cluster->synthesis_graph_node = NULL;
	cluster->nb_in_parameter = result->code_signature->nb_parameter_in;
	cluster->nb_ou_parameter = result->code_signature->nb_parameter_out;
	cluster->parameter_mapping = mapping;

	if (array_init(&(cluster->instance_array), sizeof(struct node*))){
		log_err("unable to init array");
		return -1;
	}

	if (array_add(&(cluster->instance_array), &node) < 0){
		log_err("unable to add element to array");
		array_clean(&(cluster->instance_array));
		return -1;
	}

	return 0;
}

#define signatureCluster_get_nb_frag_in(cluster, parameter) ((cluster)->parameter_mapping[parameter].nb_fragment)
#define signatureCluster_get_nb_frag_ou(cluster, parameter) ((cluster)->parameter_mapping[(cluster)->nb_in_parameter + (parameter)].nb_fragment)

#define signatureCluster_get_in_parameter(cluster, parameter) parameterMapping_get_node_buffer((cluster)->parameter_mapping + (parameter))
#define signatureCluster_get_ou_parameter(cluster, parameter) parameterMapping_get_node_buffer((cluster)->parameter_mapping + (cluster)->nb_in_parameter + (parameter))

static int32_t signatureCluster_may_append(struct signatureCluster* cluster, struct parameterMapping* mapping, uint32_t nb_in, uint32_t nb_ou){
	uint32_t 					i;
	enum parameterSimilarity 	similarity;

	if (cluster->nb_in_parameter != nb_in || cluster->nb_ou_parameter != nb_ou){
		return -1;
	}

	for (i = 0; i < nb_in; i++){
		similarity = parameterSimilarity_get(signatureCluster_get_in_parameter(cluster, i), signatureCluster_get_nb_frag_in(cluster, i), parameterMapping_get_node_buffer(mapping + i), mapping[i].nb_fragment);
		if (similarity == PARAMETER_OVERLAP || similarity == PARAMETER_DISJOINT){
			return -1;
		}
	}

	for (i = 0; i < nb_ou; i++){
		similarity = parameterSimilarity_get(signatureCluster_get_ou_parameter(cluster, i), signatureCluster_get_nb_frag_ou(cluster, i), parameterMapping_get_node_buffer(mapping + nb_in + i), mapping[nb_in + i].nb_fragment);
		if (similarity == PARAMETER_OVERLAP || similarity == PARAMETER_DISJOINT){
			return -1;
		}
	}

	return 0;
}

#define signatureCluster_add(cluster, node) array_add(&((cluster)->instance_array), &(node))

static inline void signatureCluster_clean(struct signatureCluster* cluster){
	if (cluster->parameter_mapping != NULL){
		free(cluster->parameter_mapping);
	}
	array_clean(&(cluster->instance_array));
}

struct synthesisNode{
	enum synthesisNodeType 			type;
	union{
		struct signatureCluster* 	cluster;
		struct array* 				path;
		struct node*				ir_node;
	}								node_type;
} __attribute__((__may_alias__));

#define synthesisGraph_get_synthesisNode(node) ((struct synthesisNode*)&((node)->data))

#define SYNTHESISGRAPH_EGDE_TAG_RAW 0x00000000

#define synthesisGraph_get_edge_tag_input(index) 	(((index) & 0x3fffffff) | 0x80000000)
#define synthesisGraph_get_edge_tag_output(index) 	(((index) & 0x3fffffff) | 0xc0000000)
#define synthesisGraph_edge_is_input(tag)			(((tag) & 0xc0000000) == 0x80000000)
#define synthesisGraph_edge_is_output(tag) 			(((tag) & 0xc0000000) == 0xc0000000)
#define synthesisGraph_edge_get_parameter(tag) 		((tag) & 0x3fffffff)

static void synthesisGraph_printDot_node(void* data, FILE* file, void* arg);
static void synthesisGraph_printDot_edge(void* data, FILE* file, void* arg);

static void synthesisGraph_clean_node(struct node* node);

static void synthesisGraph_cluster_symbols(struct synthesisGraph* synthesis_graph, struct ir* ir){
	struct result* 				result;
	uint32_t 					i;
	struct node*				node_cursor;
	struct irOperation* 		operation_cursor;
	struct parameterMapping* 	mapping;
	struct signatureCluster* 	cluster_cursor;
	struct signatureCluster 	new_cluster;

	for (node_cursor = graph_get_head_node(&(ir->graph)); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
		operation_cursor = ir_node_get_operation(node_cursor);

		if (operation_cursor->type == IR_OPERATION_TYPE_SYMBOL){
			result = (struct result*)operation_cursor->operation_type.symbol.result_ptr;

			mapping = parameterMapping_create(result->code_signature);
			if (mapping == NULL){
				log_err("unable to create parameterMapping");
				continue;
			}

			if (parameterMapping_fill_from_ir(mapping, node_cursor)){
				log_err("unable to fill mapping");
				free(mapping);
				continue;
			}

			for (i = 0; i < array_get_length(&(synthesis_graph->cluster_array)); i++){
				cluster_cursor = (struct signatureCluster*)array_get(&(synthesis_graph->cluster_array), i);
				if (!signatureCluster_may_append(cluster_cursor, mapping, result->code_signature->nb_parameter_in, result->code_signature->nb_parameter_out)){
					if (signatureCluster_add(cluster_cursor, node_cursor) < 0){
						log_err("unable to add element to signatureCluster");
					}
					break;
				}
			}

			if (i == array_get_length(&(synthesis_graph->cluster_array))){
				if (signatureCluster_init(&new_cluster, mapping, result, node_cursor)){
					log_err("unable to init signatureCluster");
					free(mapping);
					continue;
				}

				if (array_add(&(synthesis_graph->cluster_array), &new_cluster) < 0){
					log_err("unable to add element to array");
					signatureCluster_clean(&new_cluster);
				}
			}
		}
	}
}

static int32_t synthesisGraph_compare_ir_node(const void* data1, const void* data2){
	struct synthesisNode* synthesis_node1  = synthesisGraph_get_synthesisNode(*(struct node**)data1);
	struct synthesisNode* synthesis_node2  = synthesisGraph_get_synthesisNode(*(struct node**)data2);

	if (synthesis_node1->node_type.ir_node < synthesis_node2->node_type.ir_node){
		return -1;
	}
	else if (synthesis_node1->node_type.ir_node > synthesis_node2->node_type.ir_node){
		return 1;
	}
	else{
		return 0;
	}
}

static int32_t synthesisGraph_compare_II_path(const void* data1, const void* data2){
	struct node* 			node1 = *(struct node**)data1;
	struct node* 			node2 = *(struct node**)data2;
	struct synthesisNode* 	synthesis_node1  = synthesisGraph_get_synthesisNode(node1);
	struct synthesisNode* 	synthesis_node2  = synthesisGraph_get_synthesisNode(node2);
	uint32_t 				i;
	struct edge* 			edge_cursor1;
	struct edge* 			edge_cursor2;

	for (edge_cursor1 = node_get_head_edge_dst(node1), edge_cursor2 = node_get_head_edge_dst(node2); edge_cursor1 != NULL && edge_cursor2 != NULL; edge_cursor1 = edge_get_next_dst(edge_cursor1), edge_cursor2 = edge_get_next_dst(edge_cursor2)){
		int32_t r = memcmp(edge_cursor1->data, edge_cursor2->data, sizeof(uint32_t));
		if (r != 0){
			return r;
		}
		else if (edge_get_src(edge_cursor1) < edge_get_src(edge_cursor2)){
			return -1;
		}
		else if (edge_get_src(edge_cursor1) > edge_get_src(edge_cursor2)){
			return 1;
		}
	}

	if (edge_cursor1 == NULL && edge_cursor2 != NULL){
		return -1;
	}
	else if (edge_cursor1 != NULL && edge_cursor2 == NULL){
		return 1;
	}

	for (i = 0; i < array_get_length(synthesis_node1->node_type.path) && i < array_get_length(synthesis_node2->node_type.path); i++){
		edge_cursor1 = *(struct edge**)array_get(synthesis_node1->node_type.path, i);
		edge_cursor2 = *(struct edge**)array_get(synthesis_node2->node_type.path, i);

		if (edge_get_dst(edge_cursor1) < edge_get_dst(edge_cursor2)){
			return -1;
		}
		else if (edge_get_dst(edge_cursor1) > edge_get_dst(edge_cursor2)){
			return 1;
		}
	}

	if (i < array_get_length(synthesis_node2->node_type.path)){
		return -1;
	}
	else if(i < array_get_length(synthesis_node1->node_type.path)){
		return 1;
	}
	else{
		return 0;
	}
}

static int32_t synthesisGraph_compare_edge(const void* data1, const void* data2){
	struct edge* edge1 = *(struct edge**)data1;
	struct edge* edge2 = *(struct edge**)data2;

	if (edge_get_src(edge1) < edge_get_src(edge2)){
		return -1;
	}
	else if (edge_get_src(edge1) > edge_get_src(edge2)){
		return 1;
	}
	else{
		return memcmp(edge1->data, edge2->data, sizeof(uint32_t));
	}
}

static void synthesisGraph_pack(struct graph* graph){
	struct node* 			node_cursor;
	struct edge* 			edge_cursor;
	struct node** 			node_buffer;
	struct edge** 			edge_buffer;
	uint32_t 				nb_node;
	uint32_t 				nb_edge;
	struct synthesisNode* 	synthesis_node_cursor;
	uint32_t 				i;
	uint32_t 				j;

	node_buffer = (struct node**)malloc(sizeof(struct node*) * graph->nb_node);
	edge_buffer = (struct edge**)node_buffer;
	if (node_buffer == NULL){
		log_err("unable to allocate memory");
		return;
	}

	for (node_cursor = graph_get_head_node(graph), nb_node = 0; node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
		synthesis_node_cursor = synthesisGraph_get_synthesisNode(node_cursor);

		if (synthesis_node_cursor->type == SYNTHESISNODETYPE_IR_NODE){
			node_buffer[nb_node ++] = node_cursor;
		}
	}

	qsort(node_buffer, nb_node, sizeof(struct node*), synthesisGraph_compare_ir_node);

	for (i = 1, j = 0; i < nb_node; i++){
		if (synthesisGraph_compare_ir_node(node_buffer + j, node_buffer + i) == 0){
			graph_transfert_src_edge(graph, node_buffer[j], node_buffer[i]);
			graph_remove_node(graph, node_buffer[i]);
		}
		else{
			j = i;
		}
	}

	for (node_cursor = graph_get_head_node(graph), nb_node = 0; node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
		synthesis_node_cursor = synthesisGraph_get_synthesisNode(node_cursor);

		if (synthesis_node_cursor->type == SYNTHESISNODETYPE_II_PATH){
			node_buffer[nb_node ++] = node_cursor;
		}
	}

	qsort(node_buffer, nb_node, sizeof(struct node*), synthesisGraph_compare_II_path);

	for (i = 1, j = 0; i < nb_node; i++){
		if (synthesisGraph_compare_II_path(node_buffer + j, node_buffer + i) == 0){
			graph_transfert_src_edge(graph, node_buffer[j], node_buffer[i]);
			graph_remove_node(graph, node_buffer[i]);
		}
		else{
			j = i;
		}
	}

	for (node_cursor = graph_get_head_node(graph); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
		synthesis_node_cursor = synthesisGraph_get_synthesisNode(node_cursor);

		if (synthesis_node_cursor->type == SYNTHESISNODETYPE_RESULT){
			for (edge_cursor = node_get_head_edge_dst(node_cursor), nb_edge = 0; edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
				edge_buffer[nb_edge ++] = edge_cursor;
			}

			qsort(edge_buffer, nb_edge, sizeof(struct edge*), synthesisGraph_compare_edge);

			for (i = 1, j = 0; i < nb_edge; i++){
				if (synthesisGraph_compare_edge(edge_buffer + j, edge_buffer + i) == 0){
					graph_remove_edge(graph, edge_buffer[i]);
				}
				else{
					j = i;
				}
			}
		}
	}

	free(node_buffer);
}

static void traceMine_search_OI_path(struct signatureCluster* cluster_in, uint32_t parameter_in, struct signatureCluster* cluster_ou, uint32_t parameter_ou, struct ir* ir, struct graph* synthesis_graph){
	int32_t 				return_code;
	struct array* 			path = NULL;
	struct synthesisNode 	synthesis_path;
	struct node* 			node_path;
	uint32_t 				edge_tag;


	return_code = dijkstra_min_path(&(ir->graph), signatureCluster_get_ou_parameter(cluster_ou, parameter_ou), signatureCluster_get_nb_frag_ou(cluster_ou, parameter_ou), signatureCluster_get_in_parameter(cluster_in, parameter_in), signatureCluster_get_nb_frag_in(cluster_in, parameter_in), &path, irEdge_get_distance_OI);
	if (return_code < 0){
		log_err("unable to compute min path");
	}
	else if(return_code == 0){
		synthesis_path.type 			= SYNTHESISNODETYPE_OI_PATH;
		synthesis_path.node_type.path 	= path;

		if ((node_path = graph_add_node(synthesis_graph, &synthesis_path)) == NULL){
			log_err("unable to add node to graph");
		}
		else{
			edge_tag = synthesisGraph_get_edge_tag_output(parameter_ou);
			if (graph_add_edge(synthesis_graph, cluster_ou->synthesis_graph_node, node_path, &edge_tag) == NULL){
				log_err("unable to add edge to synthesisGraph");
			}
			edge_tag = synthesisGraph_get_edge_tag_input(parameter_in);
			if (graph_add_edge(synthesis_graph, node_path, cluster_in->synthesis_graph_node, &edge_tag) == NULL){
				log_err("unable to add edge to synthesisGraph");
			}

			path = NULL;
		}
	}

	if (path != NULL){
		array_delete(path);
	}
}

static void traceMine_search_II_path(struct signatureCluster* cluster1, uint32_t parameter1, struct signatureCluster* cluster2, uint32_t parameter2, struct ir* ir, struct graph* synthesis_graph){
	struct node* 			ancestor;
	struct array* 			path1 = NULL;
	struct array* 			path2 = NULL;
	struct synthesisNode 	synthesis_ancestor;
	struct synthesisNode 	synthesis_path;
	struct node* 			node_ancestor;
	struct node* 			node_path;
	uint32_t 				edge_tag;

	ancestor = dijkstra_lowest_common_ancestor(&(ir->graph), signatureCluster_get_in_parameter(cluster1, parameter1), signatureCluster_get_nb_frag_in(cluster1, parameter1), signatureCluster_get_in_parameter(cluster2, parameter2), signatureCluster_get_nb_frag_in(cluster2, parameter2), &path1, &path2, irEdge_get_distance_II);
	if (ancestor != NULL){
		synthesis_ancestor.type 				= SYNTHESISNODETYPE_IR_NODE;
		synthesis_ancestor.node_type.ir_node 	= ancestor;

		if ((node_ancestor = graph_add_node(synthesis_graph, &synthesis_ancestor)) == NULL){
			log_err("unable to add node to graph");
		}
		else{
			if (array_get_length(path1) > 0){
				synthesis_path.type 			= SYNTHESISNODETYPE_II_PATH;
				synthesis_path.node_type.path 	= path1;

				if ((node_path = graph_add_node(synthesis_graph, &synthesis_path)) == NULL){
					log_err("unable to add node to graph");
				}
				else{
					edge_tag = SYNTHESISGRAPH_EGDE_TAG_RAW;
					if (graph_add_edge(synthesis_graph, node_ancestor, node_path, &edge_tag) == NULL){
						log_err("unable to add edge to synthesisGraph");
					}
					edge_tag = synthesisGraph_get_edge_tag_input(parameter1);
					if (graph_add_edge(synthesis_graph, node_path, cluster1->synthesis_graph_node, &edge_tag) == NULL){
						log_err("unable to add edge to synthesisGraph");
					}

					path1 = NULL;
				}
			}
			else{
				edge_tag = synthesisGraph_get_edge_tag_input(parameter1);
				if (graph_add_edge(synthesis_graph, node_ancestor, cluster1->synthesis_graph_node, &edge_tag) == NULL){
					log_err("unable to add edge to synthesisGraph");
				}
			}

			if (array_get_length(path2) > 0){
				synthesis_path.type 			= SYNTHESISNODETYPE_II_PATH;
				synthesis_path.node_type.path 	= path2;

				if ((node_path = graph_add_node(synthesis_graph, &synthesis_path)) == NULL){
					log_err("unable to add node to graph");
				}
				else{
					edge_tag = SYNTHESISGRAPH_EGDE_TAG_RAW;
					if (graph_add_edge(synthesis_graph, node_ancestor, node_path, &edge_tag) == NULL){
						log_err("unable to add edge to synthesisGraph");
					}
					edge_tag = synthesisGraph_get_edge_tag_input(parameter2);
					if (graph_add_edge(synthesis_graph, node_path, cluster2->synthesis_graph_node, &edge_tag) == NULL){
						log_err("unable to add edge to synthesisGraph");
					}

					path2 = NULL;
				}
			}
			else{
				edge_tag = synthesisGraph_get_edge_tag_input(parameter2);
				if (graph_add_edge(synthesis_graph, node_ancestor, cluster2->synthesis_graph_node, &edge_tag) == NULL){
					log_err("unable to add edge to synthesisGraph");
				}
			}
		}
	}

	if (path1 != NULL){
		array_delete(path1);
	}
	if (path2 != NULL){
		array_delete(path2);
	}
}

static void synthesisGraph_find_cluster_relation(struct synthesisGraph* synthesis_graph, struct ir* ir){
	struct signatureCluster* 	cluster_i;
	struct signatureCluster* 	cluster_j;
	uint32_t 					i;
	uint32_t 					j;
	uint32_t 					k;
	uint32_t 					l;
	struct synthesisNode 		synthesis_node;

	for (i = 0; i < array_get_length(&(synthesis_graph->cluster_array)); i++){
		struct signatureCluster* cluster = (struct signatureCluster*)array_get(&(synthesis_graph->cluster_array), i);

		synthesis_node.type 				= SYNTHESISNODETYPE_RESULT;
		synthesis_node.node_type.cluster 	= cluster;

		if ((cluster->synthesis_graph_node = graph_add_node(&(synthesis_graph->graph), &synthesis_node)) == NULL){
			log_err("unable to add node to graph");
			continue;
		}

		for (j = 0; j < cluster->nb_in_parameter; j++){
			for (k = j + 1; k < cluster->nb_ou_parameter; k++){
				traceMine_search_II_path(cluster, j, cluster, k, ir, &(synthesis_graph->graph));
			}
		}
	}

	for (i = 0; i < array_get_length(&(synthesis_graph->cluster_array)); i++){
		cluster_i = (struct signatureCluster*)array_get(&(synthesis_graph->cluster_array), i);
		for (j = 0; j < array_get_length(&(synthesis_graph->cluster_array)); j++){
			if (j == i){
				continue;
			}

			cluster_j = (struct signatureCluster*)array_get(&(synthesis_graph->cluster_array), j);

			for (k = 0; k < cluster_i->nb_in_parameter; k++){
				for (l = 0; l < cluster_j->nb_ou_parameter; l++){
					traceMine_search_OI_path(cluster_i, k, cluster_j, l, ir, &(synthesis_graph->graph));
				}
			}
		}

		for (j = i + 1; j < array_get_length(&(synthesis_graph->cluster_array)); j++){
			cluster_j = (struct signatureCluster*)array_get(&(synthesis_graph->cluster_array), j);

			for (k = 0; k < cluster_i->nb_in_parameter; k++){
				for (l = 0; l < cluster_j->nb_in_parameter; l++){
					traceMine_search_II_path(cluster_i, k, cluster_j, l, ir, &(synthesis_graph->graph));
				}
			}
		}
	}
}

struct synthesisGraph* synthesisGraph_create(struct ir* ir){
	struct synthesisGraph* synthesis_graph;

	synthesis_graph = (struct synthesisGraph*)malloc(sizeof(struct synthesisGraph));
	if (synthesis_graph != NULL){
		if (synthesisGraph_init(synthesis_graph, ir)){
			log_err("unable to init synthesisGraph");
			free(synthesis_graph);
			synthesis_graph = NULL;
		}
	}
	else{
		log_err("unable to allocate memory");
	}

	return synthesis_graph;
}

int32_t synthesisGraph_init(struct synthesisGraph* synthesis_graph, struct ir* ir){
	if (array_init(&(synthesis_graph->cluster_array), sizeof(struct signatureCluster))){
		log_err("unable to init array");
		return -1;
	}

	graph_init(&(synthesis_graph->graph), sizeof(struct synthesisNode), sizeof(uint32_t));
	graph_register_dotPrint_callback(&(synthesis_graph->graph), NULL, synthesisGraph_printDot_node, synthesisGraph_printDot_edge, NULL);
	graph_register_node_clean_call_back(&(synthesis_graph->graph), synthesisGraph_clean_node);

	synthesisGraph_cluster_symbols(synthesis_graph, ir);
	synthesisGraph_find_cluster_relation(synthesis_graph, ir);
	synthesisGraph_pack(&(synthesis_graph->graph));

	return 0;
}

void synthesisGraph_clean(struct synthesisGraph* synthesis_graph){
	uint32_t i;

	graph_clean(&(synthesis_graph->graph));

	for (i = 0; i < array_get_length(&(synthesis_graph->cluster_array)); i++){
		signatureCluster_clean((struct signatureCluster*)array_get(&(synthesis_graph->cluster_array), i));
	}
	array_clean(&(synthesis_graph->cluster_array));
}

#pragma GCC diagnostic ignored "-Wunused-parameter"
static void synthesisGraph_printDot_node(void* data, FILE* file, void* arg){
	struct synthesisNode*		synthesis_node;
	uint32_t 					i;
	struct node* 				symbol;
	struct edge* 				edge;
	struct irOperation* 		operation;
	struct irDependence* 		dependence;

	synthesis_node = (struct synthesisNode*)data;
	switch(synthesis_node->type){
		case SYNTHESISNODETYPE_RESULT : {
			for (i = 0; i < array_get_length(&(synthesis_node->node_type.cluster->instance_array)); i++){
				symbol = *(struct node**)array_get(&(synthesis_node->node_type.cluster->instance_array), i);

				if (i == 0){
					fprintf(file, "[shape=box,label=\"%s", ((struct result*)(ir_node_get_operation(symbol)->operation_type.symbol.result_ptr))->code_signature->signature.name);
				}
				else if (((struct result*)(ir_node_get_operation(symbol)->operation_type.symbol.result_ptr))->code_signature != ((struct result*)(ir_node_get_operation(*(struct node**)array_get(&(synthesis_node->node_type.cluster->instance_array), i - 1))->operation_type.symbol.result_ptr))->code_signature){
					fprintf(file, "\\n%s", ((struct result*)(ir_node_get_operation(symbol)->operation_type.symbol.result_ptr))->code_signature->signature.name);
				}
				if (i + 1 == array_get_length(&(synthesis_node->node_type.cluster->instance_array))){
					fprintf(file, "\"]");
				}
			}
			break;
		}
		case SYNTHESISNODETYPE_OI_PATH : {
			if (array_get_length(synthesis_node->node_type.path) == 0){
				fprintf(file, "[shape=plaintext,label=<->");
			}
			else{
				fprintf(file, "[shape=plaintext,label=<");
				for (i = array_get_length(synthesis_node->node_type.path); i > 0; i --){
					edge = *(struct edge**)array_get(synthesis_node->node_type.path, i - 1);
					operation = ir_node_get_operation(edge_get_dst(edge));

					switch(operation->type){
						case IR_OPERATION_TYPE_INST 	: {
							fprintf(file, "%s<BR ALIGN=\"LEFT\"/>", irOpcode_2_string(operation->operation_type.inst.opcode));
							break;
						}
						default 						: {
							log_err("this case is not supposed to happen");
						}
					}
				}
				fprintf(file, ">]");
			}
			break;
		}
		case SYNTHESISNODETYPE_II_PATH : {
			if (array_get_length(synthesis_node->node_type.path) > 0){
				fprintf(file, "[shape=plaintext,label=<");
				for (i = 0; i < array_get_length(synthesis_node->node_type.path); i++){
					edge = *(struct edge**)array_get(synthesis_node->node_type.path, i);
					operation = ir_node_get_operation(edge_get_dst(edge));
					dependence = ir_edge_get_dependence(edge);

					switch(operation->type){
						case IR_OPERATION_TYPE_IN_MEM 	: {
							if (dependence->type == IR_DEPENDENCE_TYPE_ADDRESS){
								fprintf(file, "@ LOAD<BR ALIGN=\"LEFT\"/>");
							}
							else{
								fprintf(file, "LOAD<BR ALIGN=\"LEFT\"/>");
							}
							break;
						}
						case IR_OPERATION_TYPE_INST 	: {
							fprintf(file, "%s<BR ALIGN=\"LEFT\"/>", irOpcode_2_string(operation->operation_type.inst.opcode));
							break;
						}
						default 						: {
							log_err("this case is not supposed to happen: incorrect operation on path");
							break;
						}
					}
				}
				fprintf(file, ">]");
			}
			else{
				log_err("this case is not supposed to happen: empty II path");
			}
			break;
		}
		case SYNTHESISNODETYPE_IR_NODE : {
			ir_dotPrint_node(ir_node_get_operation(synthesis_node->node_type.ir_node), file, NULL);
			break;
		}
	}
}

#pragma GCC diagnostic ignored "-Wunused-parameter"
static void synthesisGraph_printDot_edge(void* data, FILE* file, void* arg){
	uint32_t tag = *(uint32_t*)data;

	if (synthesisGraph_edge_is_input(tag)){
		fprintf(file, "[label=\"I%u\"]", synthesisGraph_edge_get_parameter(tag));
	}
	else if (synthesisGraph_edge_is_output(tag)){
		fprintf(file, "[label=\"O%u\"]", synthesisGraph_edge_get_parameter(tag));
	}
}

static void synthesisGraph_clean_node(struct node* node){
	struct synthesisNode* synthesis_node;

	synthesis_node = synthesisGraph_get_synthesisNode(node);
	if (synthesis_node->type == SYNTHESISNODETYPE_OI_PATH || synthesis_node->type == SYNTHESISNODETYPE_II_PATH){
		array_delete(synthesis_node->node_type.path);
	}
}
