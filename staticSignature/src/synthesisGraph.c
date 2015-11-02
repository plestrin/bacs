#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "synthesisGraph.h"
#include "dijkstra.h"
#include "arrayMinCoverage.h"
#include "base.h"

#define MIN_COVERAGE_STRATEGY 3 /* 0 is random, 1 is greedy, 2 is exact, 3 is split */

static const uint32_t irEdge_distance_array_OI[NB_DEPENDENCE_TYPE] = {
	0, 						/* DIRECT */
	DIJKSTRA_INVALID_DST, 	/* ADDRESS */
	0, 						/* SHIFT_DISP */
	0, 						/* DIVISOR */
	0, 						/* ROUND_OFF */
	0, 						/* SUBSTITUTE */
	DIJKSTRA_INVALID_DST 	/* MACRO */
};

static uint32_t irEdge_get_distance(void* arg){
	struct irDependence* dependence = (struct irDependence*)arg;
	struct edge* edge = irDependence_get_edge(arg);

	if (ir_node_get_operation(edge_get_src(edge))->type == IR_OPERATION_TYPE_IMM || ir_node_get_operation(edge_get_dst(edge))->type == IR_OPERATION_TYPE_IMM){
		return DIJKSTRA_INVALID_DST;
	}
	else{
		return irEdge_distance_array_OI[dependence->type];	
	}
}

static int32_t signatureCluster_init(struct signatureCluster* cluster, struct parameterMapping* mapping, const struct codeSignature* code_signature, struct node* node){
	cluster->synthesis_graph_node = NULL;
	cluster->nb_in_parameter = code_signature->nb_parameter_in;
	cluster->nb_ou_parameter = code_signature->nb_parameter_out;
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

static void synthesisGraph_printDot_node(void* data, FILE* file, void* arg);
static void synthesisGraph_printDot_edge(void* data, FILE* file, void* arg);

static void synthesisGraph_clean_node(struct node* node);

static void synthesisGraph_cluster_symbols(struct synthesisGraph* synthesis_graph, struct ir* ir){
	struct codeSignature* 		code_signature;
	uint32_t 					i;
	struct node*				node_cursor;
	struct irOperation* 		operation_cursor;
	struct parameterMapping* 	mapping;
	struct signatureCluster* 	cluster_cursor;
	struct signatureCluster 	new_cluster;

	for (node_cursor = graph_get_head_node(&(ir->graph)); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
		operation_cursor = ir_node_get_operation(node_cursor);

		if (operation_cursor->type == IR_OPERATION_TYPE_SYMBOL){
			code_signature = (struct codeSignature*)operation_cursor->operation_type.symbol.code_signature;

			mapping = parameterMapping_create(code_signature);
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
				if (!signatureCluster_may_append(cluster_cursor, mapping, code_signature->nb_parameter_in, code_signature->nb_parameter_out)){
					if (signatureCluster_add(cluster_cursor, node_cursor) < 0){
						log_err("unable to add element to signatureCluster");
					}
					free(mapping);
					break;
				}
			}

			if (i == array_get_length(&(synthesis_graph->cluster_array))){
				if (signatureCluster_init(&new_cluster, mapping, code_signature, node_cursor)){
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

	#if VERBOSE == 1
	log_info_m("symbols divided into %u clusters", array_get_length(&(synthesis_graph->cluster_array)));
	#endif
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
		return memcmp(edge_get_data(edge1), edge_get_data(edge2), sizeof(uint32_t));
	}
}

static struct node* synthesisGraph_add_ir_node(struct graph* synthesis_graph, struct node* ir_node){
	struct synthesisNode 	synthesis_node;
	struct node* 			result;

	synthesis_node.type 				= SYNTHESISNODETYPE_IR_NODE;
	synthesis_node.node_type.ir_node 	= ir_node;

	if ((result = graph_add_node(synthesis_graph, &synthesis_node)) == NULL){
		log_err("unable to add node to graph");
	}

	return result;
}

static struct node* synthesisGraph_add_path(struct graph* synthesis_graph, uint32_t nb_node){
	struct synthesisNode 	synthesis_node;
	struct node* 			result;

	synthesis_node.type 						= SYNTHESISNODETYPE_PATH;
	synthesis_node.node_type.path.nb_node 		= nb_node;
	synthesis_node.node_type.path.node_buffer 	= (struct node**)malloc(sizeof(struct node*) * nb_node);

	if (synthesis_node.node_type.path.node_buffer == NULL){
		log_err("unable to allocate memory");
		return NULL;
	}

	if ((result = graph_add_node(synthesis_graph, &synthesis_node)) == NULL){
		log_err("unable to add node to graph");
		free(synthesis_node.node_type.path.node_buffer);
	}

	return result;
}

static void synthesis_graph_collapse_node(struct graph* synthesis_graph, struct node* node_hi, struct node* node_lo){
	uint32_t 				nb_node;
	struct synthesisNode* 	syn_node_hi = synthesisGraph_get_synthesisNode(node_hi);
	struct synthesisNode* 	syn_node_lo = synthesisGraph_get_synthesisNode(node_lo);
	struct node* 			path;
	uint32_t 				i;
	struct synthesisNode* 	syn_path;

	if (syn_node_hi->type == SYNTHESISNODETYPE_IR_NODE){
		nb_node = 1;
	}
	else if (syn_node_hi->type == SYNTHESISNODETYPE_PATH){
		nb_node = syn_node_hi->node_type.path.nb_node;
	}
	else{
		log_err("incorrect node type");
		return;
	}

	if (syn_node_lo->type == SYNTHESISNODETYPE_IR_NODE){
		nb_node ++;
	}
	else if (syn_node_lo->type == SYNTHESISNODETYPE_PATH){
		nb_node += syn_node_lo->node_type.path.nb_node;
	}
	else{
		log_err("incorrect node type");
		return;
	}

	if ((path = synthesisGraph_add_path(synthesis_graph, nb_node)) == NULL){
		log_err("unable to add path to synthesisGraph");
		return;
	}

	syn_path = synthesisGraph_get_synthesisNode(path);

	if (syn_node_hi->type == SYNTHESISNODETYPE_IR_NODE){
		syn_path->node_type.path.node_buffer[0] = syn_node_hi->node_type.ir_node;
		i = 1;
	}
	else{
		memcpy(syn_path->node_type.path.node_buffer, syn_node_hi->node_type.path.node_buffer, sizeof(struct node*) * syn_node_hi->node_type.path.nb_node);
		i = syn_node_hi->node_type.path.nb_node;
	}
	if (syn_node_lo->type == SYNTHESISNODETYPE_IR_NODE){
		syn_path->node_type.path.node_buffer[i] = syn_node_lo->node_type.ir_node;
	}
	else{
		memcpy(syn_path->node_type.path.node_buffer + i, syn_node_lo->node_type.path.node_buffer, sizeof(struct node*) * syn_node_lo->node_type.path.nb_node);
	}

	graph_transfert_dst_edge(synthesis_graph, path, node_hi);
	graph_transfert_src_edge(synthesis_graph, path, node_lo);

	graph_remove_node(synthesis_graph, node_hi);
	graph_remove_node(synthesis_graph, node_lo);
}

static void synthesis_graph_convert_ir_node_to_path(struct node* node){
	struct node** 			node_buffer;
	struct synthesisNode* 	syn_node = synthesisGraph_get_synthesisNode(node);

	node_buffer = (struct node**)malloc(sizeof(struct node*));
	if (node_buffer != NULL){
		node_buffer[0] = syn_node->node_type.ir_node;
		syn_node->type = SYNTHESISNODETYPE_PATH;
		syn_node->node_type.path.nb_node = 1;
		syn_node->node_type.path.node_buffer = node_buffer;
	}
	else{
		log_err("unable to allocate memory");
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
			graph_transfert_dst_edge(graph, node_buffer[j], node_buffer[i]);
			graph_transfert_src_edge(graph, node_buffer[j], node_buffer[i]);
			graph_remove_node(graph, node_buffer[i]);
		}
		else{
			j = i;
		}
	}

	for (node_cursor = graph_get_head_node(graph); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
		synthesis_node_cursor = synthesisGraph_get_synthesisNode(node_cursor);

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

	for (node_cursor = graph_get_head_node(graph); node_cursor != NULL; ){
		synthesis_node_cursor = synthesisGraph_get_synthesisNode(node_cursor);

		if (synthesis_node_cursor->type == SYNTHESISNODETYPE_IR_NODE || synthesis_node_cursor->type == SYNTHESISNODETYPE_PATH){
			if (node_cursor->nb_edge_src == 1){
				struct node* node_dst = edge_get_dst(node_get_head_edge_src(node_cursor));
				
				if (node_dst->nb_edge_dst == 1 && (synthesisGraph_get_synthesisNode(node_dst)->type == SYNTHESISNODETYPE_IR_NODE || synthesisGraph_get_synthesisNode(node_dst)->type == SYNTHESISNODETYPE_PATH)){
					synthesis_graph_collapse_node(graph, node_cursor, node_dst);
					node_cursor = graph_get_head_node(graph);
					continue;
				}
			}
			if (node_cursor->nb_edge_dst == 1){
				struct node* node_src = edge_get_src(node_get_head_edge_dst(node_cursor));
				
				if (node_src->nb_edge_src == 1 && (synthesisGraph_get_synthesisNode(node_src)->type == SYNTHESISNODETYPE_IR_NODE || synthesisGraph_get_synthesisNode(node_src)->type == SYNTHESISNODETYPE_PATH)){
					synthesis_graph_collapse_node(graph, node_src, node_cursor);
					node_cursor = graph_get_head_node(graph);
					continue;
				}
			}

			if (synthesis_node_cursor->type == SYNTHESISNODETYPE_IR_NODE && node_cursor->nb_edge_src == 1 && node_cursor->nb_edge_dst == 1){
				synthesis_graph_convert_ir_node_to_path(node_cursor);
			}
		}

		node_cursor = node_get_next(node_cursor);
	}

	free(node_buffer);
}

static int32_t synthesisGraph_add_dijkstraPath(struct graph* synthesis_graph, struct node* node_src, uint32_t src_edge_tag, struct node* node_dst, uint32_t dst_edge_tag, struct dijkstraPath* path){
	uint32_t 					i;
	struct node* 				new_node;
	struct dijkstraPathStep* 	step;

	/* Special cases */
	if (array_get_length(path->step_array) == 0){
		if (synthesisGraph_edge_is_input(src_edge_tag) && synthesisGraph_edge_is_output(dst_edge_tag)){
			log_warn("get different tags for the same edge");
			if (graph_add_edge(synthesis_graph, node_dst, node_src, &src_edge_tag) == NULL){
				log_err("unable to add edge to synthesisGraph");
				return -1;
			}
			else{
				return 0;
			}
		}
		if (synthesisGraph_edge_is_input(dst_edge_tag) && synthesisGraph_edge_is_output(src_edge_tag)){
			log_warn("get different tags for the same edge");
			if (graph_add_edge(synthesis_graph, node_src, node_dst, &dst_edge_tag) == NULL){
				log_err("unable to add edge to synthesisGraph");
				return -1;
			}
			else{
				return 0;
			}
		}
	}

	/* Fetch IN */
	if (synthesisGraph_edge_is_input(src_edge_tag)){
		if (array_get_length(path->step_array) == 0){
			new_node = synthesisGraph_add_ir_node(synthesis_graph, path->reached_node);
		}
		else{
			step = (struct dijkstraPathStep*)array_get(path->step_array, array_get_length(path->step_array) - 1);
			if (step->dir == PATH_SRC_TO_DST){
				new_node = synthesisGraph_add_ir_node(synthesis_graph, edge_get_src(step->edge));
			}
			else{
				new_node = synthesisGraph_add_ir_node(synthesis_graph, edge_get_dst(step->edge));
			}
		}
		if (new_node == NULL){
			log_err("unable to add IR node to synthesisGraph");
			return -1;
		}

		if (graph_add_edge(synthesis_graph, new_node, node_src, &src_edge_tag) == NULL){
			log_err("unable to add edge to synthesisGraph");
			return -1;
		}

		node_src = new_node;
		src_edge_tag = SYNTHESISGRAPH_EGDE_TAG_RAW;
	}

	/* Fetch OUT */
	if (synthesisGraph_edge_is_input(dst_edge_tag)){
		new_node = synthesisGraph_add_ir_node(synthesis_graph, path->reached_node);
		if (new_node == NULL){
			log_err("unable to add IR node to synthesisGraph");
			return -1;
		}

		if (graph_add_edge(synthesis_graph, new_node, node_dst, &dst_edge_tag) == NULL){
			log_err("unable to add edge to synthesisGraph");
			return -1;
		}

		node_dst = new_node;
		dst_edge_tag = SYNTHESISGRAPH_EGDE_TAG_RAW;
	}

	/* Fill */
	for (i = array_get_length(path->step_array); i > 0; i --){
		step = (struct dijkstraPathStep*)array_get(path->step_array, i - 1);
		if (step->dir == PATH_SRC_TO_DST){
			if (i > 1){
				new_node = synthesisGraph_add_ir_node(synthesis_graph, edge_get_dst(step->edge));
				if (new_node == NULL){
					log_err("unable to add IR node to synthesisGraph");
					return -1;
				}
			}
			else{
				new_node = node_dst;
				if (dst_edge_tag != SYNTHESISGRAPH_EGDE_TAG_RAW && src_edge_tag != SYNTHESISGRAPH_EGDE_TAG_RAW){
					log_warn("get different tags for the same edge");
				}
				src_edge_tag = max(dst_edge_tag, src_edge_tag);
			}

			if (graph_add_edge(synthesis_graph, node_src, new_node, &src_edge_tag) == NULL){
				log_err("unable to add edge to synthesisGraph");
				return -1;
			}
		}
		else{
			if (i > 1){
				new_node = synthesisGraph_add_ir_node(synthesis_graph, edge_get_src(step->edge));
				if (new_node == NULL){
					log_err("unable to add IR node to synthesisGraph");
					return -1;
				}
			}
			else{
				new_node = node_dst;
				if (dst_edge_tag != SYNTHESISGRAPH_EGDE_TAG_RAW && src_edge_tag != SYNTHESISGRAPH_EGDE_TAG_RAW){
					log_warn("get different tags for the same edge");
				}
				src_edge_tag = max(dst_edge_tag, src_edge_tag);
			}

			if (graph_add_edge(synthesis_graph, new_node, node_src, &src_edge_tag) == NULL){
				log_err("unable to add edge to synthesisGraph");
				return -1;
			}
		}

		node_src = new_node;
		src_edge_tag = SYNTHESISGRAPH_EGDE_TAG_RAW;
	}

	return 0;
}

static int32_t dijkstraPathStep_compare(void* arg1, void* arg2){
	struct dijkstraPathStep* step1 = (struct dijkstraPathStep*)arg1;
	struct dijkstraPathStep* step2 = (struct dijkstraPathStep*)arg2;

	if (step1->edge < step2->edge){
		return -1;
	}
	else if (step1->edge > step2->edge){
		return 1;
	}
	else{
		return 0;
	}
}

static void synthesisGraph_find_cluster_relation(struct synthesisGraph* synthesis_graph, struct ir* ir){
	struct relationResultDescriptor{
		uint32_t 		offset;
		uint32_t 		nb_element;
		struct node* 	node_src;
		uint32_t 		tag_src;
		struct node* 	node_dst;
		uint32_t 		tag_dst;
	};

	struct signatureCluster* 	cluster_i;
	struct signatureCluster* 	cluster_j;
	uint32_t 					i;
	uint32_t 					j;
	uint32_t 					k;
	uint32_t 					l;
	struct synthesisNode 		synthesis_node;
	struct array 				path_array;
	struct array 				rrd_array;
	uint32_t 					nb_category;

	if (array_init(&path_array, sizeof(struct dijkstraPath))){
		log_err("unable to init array");
		return;
	}

	if (array_init(&rrd_array, sizeof(struct relationResultDescriptor))){
		log_err("unable to init array");
		array_clean(&path_array);
		return;
	}

	#define traceMine_search_path(buffer_src, nb_src, node_src_, tag_src_, buffer_dst, nb_dst, node_dst_, tag_dst_) 												\
	{ 																																								\
		struct relationResultDescriptor rrd; 																														\
																																									\
		rrd.offset 		= array_get_length(&path_array); 																											\
		rrd.node_src 	= node_src_; 																																\
		rrd.tag_src 	= tag_src_; 																																\
		rrd.node_dst 	= node_dst_; 																																\
		rrd.tag_dst 	= tag_dst_; 																																\
																																									\
		if (dijkstra_min_path(&(ir->graph), buffer_src, nb_src, buffer_dst, nb_dst, &path_array, irEdge_get_distance)){ 											\
			log_err("Dijkstra min path returned an error code"); 																									\
		} 																																							\
		else{ 																																						\
			rrd.nb_element = array_get_length(&path_array) - rrd.offset; 																							\
			if (array_add(&rrd_array, &rrd) < 0){ 																														\
				log_err("unable to add element to array"); 																											\
			} 																																						\
		} 																																							\
	}

	for (i = 0; i < array_get_length(&(synthesis_graph->cluster_array)); i++){
		struct signatureCluster* cluster = (struct signatureCluster*)array_get(&(synthesis_graph->cluster_array), i);

		synthesis_node.type 				= SYNTHESISNODETYPE_RESULT;
		synthesis_node.node_type.cluster 	= cluster;

		if ((cluster->synthesis_graph_node = graph_add_node(&(synthesis_graph->graph), &synthesis_node)) == NULL){
			log_err("unable to add node to graph");
			continue;
		}

		for (j = 0; j < cluster->nb_in_parameter; j++){
			for (k = j + 1; k < cluster->nb_in_parameter; k++){
				traceMine_search_path( 	signatureCluster_get_in_parameter(cluster, j), 		/* buffer_src 	*/
										signatureCluster_get_nb_frag_in(cluster, j), 		/* nb_src 		*/
										cluster->synthesis_graph_node, 						/* node_src 	*/
										synthesisGraph_get_edge_tag_input(j), 				/* tag_src 		*/
										signatureCluster_get_in_parameter(cluster, k), 		/* buffer_dst 	*/
										signatureCluster_get_nb_frag_in(cluster, k), 		/* nb_dst 		*/
										cluster->synthesis_graph_node, 						/* node_dst 	*/
										synthesisGraph_get_edge_tag_input(k)) 				/* tag_dst 		*/
			}
		}

		for (j = 0; j < cluster->nb_ou_parameter; j++){
			for (k = j + 1; k < cluster->nb_ou_parameter; k++){
				traceMine_search_path( 	signatureCluster_get_ou_parameter(cluster, j), 		/* buffer_src 	*/
										signatureCluster_get_nb_frag_ou(cluster, j), 		/* nb_src 		*/
										cluster->synthesis_graph_node, 						/* node_src 	*/
										synthesisGraph_get_edge_tag_output(j), 				/* tag_src 		*/
										signatureCluster_get_ou_parameter(cluster, k), 		/* buffer_dst 	*/
										signatureCluster_get_nb_frag_ou(cluster, k), 		/* nb_dst 		*/
										cluster->synthesis_graph_node, 						/* node_dst 	*/
										synthesisGraph_get_edge_tag_output(k)) 				/* tag_dst 		*/
			}
		}

		for (j = 0; j < cluster->nb_in_parameter; j++){
			for (k = 0; k < cluster->nb_ou_parameter; k++){
				traceMine_search_path( 	signatureCluster_get_in_parameter(cluster, j), 		/* buffer_src 	*/
										signatureCluster_get_nb_frag_in(cluster, j), 		/* nb_src 		*/
										cluster->synthesis_graph_node, 						/* node_src 	*/
										synthesisGraph_get_edge_tag_input(j), 				/* tag_src 		*/
										signatureCluster_get_ou_parameter(cluster, k), 		/* buffer_dst 	*/
										signatureCluster_get_nb_frag_ou(cluster, k), 		/* nb_dst 		*/
										cluster->synthesis_graph_node, 						/* node_dst 	*/
										synthesisGraph_get_edge_tag_output(k)) 				/* tag_dst 		*/
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
					traceMine_search_path( 	signatureCluster_get_in_parameter(cluster_i, k), 	/* buffer_src 	*/
											signatureCluster_get_nb_frag_in(cluster_i, k), 		/* nb_src 		*/
											cluster_i->synthesis_graph_node, 					/* node_src 	*/
											synthesisGraph_get_edge_tag_input(k), 				/* tag_src 		*/
											signatureCluster_get_ou_parameter(cluster_j, l), 	/* buffer_dst 	*/
											signatureCluster_get_nb_frag_ou(cluster_j, l), 		/* nb_dst 		*/
											cluster_j->synthesis_graph_node, 					/* node_dst 	*/
											synthesisGraph_get_edge_tag_output(l)) 				/* tag_dst 		*/
				}
			}
		}
		for (j = i + 1; j < array_get_length(&(synthesis_graph->cluster_array)); j++){
			cluster_j = (struct signatureCluster*)array_get(&(synthesis_graph->cluster_array), j);

			for (k = 0; k < cluster_i->nb_in_parameter; k++){
				for (l = 0; l < cluster_j->nb_in_parameter; l++){
					traceMine_search_path( 	signatureCluster_get_in_parameter(cluster_i, k), 	/* buffer_src 	*/
											signatureCluster_get_nb_frag_in(cluster_i, k), 		/* nb_src 		*/
											cluster_i->synthesis_graph_node, 					/* node_src 	*/
											synthesisGraph_get_edge_tag_input(k), 				/* tag_src 		*/
											signatureCluster_get_in_parameter(cluster_j, l), 	/* buffer_dst 	*/
											signatureCluster_get_nb_frag_in(cluster_j, l), 		/* nb_dst 		*/
											cluster_j->synthesis_graph_node, 					/* node_dst 	*/
											synthesisGraph_get_edge_tag_input(l)) 				/* tag_dst 		*/
				}
			}

			for (k = 0; k < cluster_i->nb_ou_parameter; k++){
				for (l = 0; l < cluster_j->nb_ou_parameter; l++){
					traceMine_search_path( 	signatureCluster_get_ou_parameter(cluster_i, k), 	/* buffer_src 	*/
											signatureCluster_get_nb_frag_ou(cluster_i, k), 		/* nb_src 		*/
											cluster_i->synthesis_graph_node, 					/* node_src 	*/
											synthesisGraph_get_edge_tag_output(k), 				/* tag_src 		*/
											signatureCluster_get_ou_parameter(cluster_j, l), 	/* buffer_dst 	*/
											signatureCluster_get_nb_frag_ou(cluster_j, l), 		/* nb_dst 		*/
											cluster_j->synthesis_graph_node, 					/* node_dst 	*/
											synthesisGraph_get_edge_tag_output(l)) 				/* tag_dst 		*/
				}
			}
		}
	}

	for (i = 0, nb_category = 0; i < array_get_length(&rrd_array); i++){
		if (((struct relationResultDescriptor*)array_get(&rrd_array, i))->nb_element > 1){
			nb_category ++;
		}
	}

	if (nb_category > 0){
		struct categoryDesc* 				desc_buffer;
		struct relationResultDescriptor* 	rrd_ptr;
		int32_t 							result;
		struct dijkstraPath* 				path;

		desc_buffer = (struct categoryDesc*)malloc(sizeof(struct categoryDesc) * nb_category);
		if (desc_buffer == NULL){
			log_err("unable to allocate memory");
		}
		else{
			for (i = 0, j = 0; i < array_get_length(&rrd_array); i++){
				rrd_ptr = (struct relationResultDescriptor*)array_get(&rrd_array, i);
				if (rrd_ptr->nb_element > 1){
					desc_buffer[j].offset 		= rrd_ptr->offset;
					desc_buffer[j].nb_element 	= rrd_ptr->nb_element;
					j ++;
				}
			}

			#if MIN_COVERAGE_STRATEGY == 0
			result = arrayMinCoverage_rand(&path_array, nb_category, desc_buffer, dijkstraPathStep_compare, NULL);
			#elif MIN_COVERAGE_STRATEGY == 1
			result = arrayMinCoverage_greedy(&path_array, nb_category, desc_buffer, dijkstraPathStep_compare, NULL);
			#elif MIN_COVERAGE_STRATEGY == 2
			result = arrayMinCoverage_exact(&path_array, nb_category, desc_buffer, dijkstraPathStep_compare, NULL);
			#elif MIN_COVERAGE_STRATEGY == 3
			result = arrayMinCoverage_split(&path_array, nb_category, desc_buffer, dijkstraPathStep_compare);
			#else
			#error Incorrect strategy number. Valid values are: 0, 1, 2
			#endif

			if (result){
				log_err("minCoverage function returned an error code");
			}
			else{
				for (i = 0, j = 0; i < array_get_length(&rrd_array); i++){
					rrd_ptr = (struct relationResultDescriptor*)array_get(&rrd_array, i);

					if (rrd_ptr->nb_element == 0){
						continue;
					}
					else if (rrd_ptr->nb_element == 1){
						path = (struct dijkstraPath*)array_get(&path_array, rrd_ptr->offset);
					}
					else{
						path = (struct dijkstraPath*)array_get(&path_array, rrd_ptr->offset + desc_buffer[j].choice);
						j++;
						
					}
					if (synthesisGraph_add_dijkstraPath(&(synthesis_graph->graph), rrd_ptr->node_src, rrd_ptr->tag_src, rrd_ptr->node_dst, rrd_ptr->tag_dst, path)){
						log_err("unable add path to synthesisGraph");
					}
				}
			}
		}

		free(desc_buffer);
	}

	for (i = 0; i < array_get_length(&path_array); i++){
		array_delete(((struct dijkstraPath*)array_get(&path_array, i))->step_array);
	}

	array_clean(&path_array);
	array_clean(&rrd_array);
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

void synthesisGraph_delete_edge(struct graph* graph, struct edge* edge){
	struct node* 			node;
	struct synthesisNode* 	synthesis_node;
	struct node* 			to_be_delete[2] = {NULL, NULL};

	node = edge_get_src(edge);
	synthesis_node = synthesisGraph_get_synthesisNode(node);

	if (synthesis_node->type == SYNTHESISNODETYPE_PATH){
		if (node->nb_edge_src == 1){
			to_be_delete[0] = node;
		}
	}
	else if (synthesis_node->type == SYNTHESISNODETYPE_IR_NODE){
		if (node->nb_edge_src == 1 && node->nb_edge_dst == 0){
			to_be_delete[0] = node;
		}
	}

	node = edge_get_dst(edge);
	synthesis_node = synthesisGraph_get_synthesisNode(node);

	if (synthesis_node->type == SYNTHESISNODETYPE_PATH){
		if (node->nb_edge_dst == 1){
			to_be_delete[1] = node;
		}
	}
	else if (synthesis_node->type == SYNTHESISNODETYPE_IR_NODE){
		if (node->nb_edge_dst == 1 && node->nb_edge_src == 0){
			to_be_delete[1] = node;
		}
	}

	graph_remove_edge(graph, edge);

	if (to_be_delete[0] != NULL){
		graph_remove_node(graph, to_be_delete[0]);
	}
	if (to_be_delete[1] != NULL){
		graph_remove_node(graph, to_be_delete[1]);
	}
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
	struct synthesisNode*	synthesis_node;
	uint32_t 				i;
	struct node* 			symbol;
	struct irOperation* 	operation;
	uint32_t 				nb_element;

	synthesis_node = (struct synthesisNode*)data;
	switch(synthesis_node->type){
		case SYNTHESISNODETYPE_RESULT : {
			for (i = 0; i < array_get_length(&(synthesis_node->node_type.cluster->instance_array)); i++){
				symbol = *(struct node**)array_get(&(synthesis_node->node_type.cluster->instance_array), i);

				if (i == 0){
					fprintf(file, "[shape=box,label=\"%s", ((struct codeSignature*)(ir_node_get_operation(symbol)->operation_type.symbol.code_signature))->signature.name);
				}
				else if (ir_node_get_operation(symbol)->operation_type.symbol.code_signature != ir_node_get_operation(*(struct node**)array_get(&(synthesis_node->node_type.cluster->instance_array), i - 1))->operation_type.symbol.code_signature){
					fprintf(file, "\\n%s", ((struct codeSignature*)(ir_node_get_operation(symbol)->operation_type.symbol.code_signature))->signature.name);
				}
				if (i + 1 == array_get_length(&(synthesis_node->node_type.cluster->instance_array))){
					fprintf(file, "\"]");
				}
			}
			break;
		}
		case SYNTHESISNODETYPE_PATH 	: {
			fputs("[shape=plaintext,label=<", file);
			for (i = 0, nb_element = 0; i < synthesis_node->node_type.path.nb_node; i++){
				operation = ir_node_get_operation(synthesis_node->node_type.path.node_buffer[i]);

				switch(operation->type){
					case IR_OPERATION_TYPE_INST 	: {
						fprintf(file, "%s<BR ALIGN=\"LEFT\"/>", irOpcode_2_string(operation->operation_type.inst.opcode));
						nb_element ++;
						break;
					}
					default 						: {
						log_err("this case is not supposed to happen: incorrect operation on path");
						break;
					}
				}
			}
			if (nb_element){
				fputs(">]", file);
			}
			else{
				fputs("->]", file);
			}
			break;
		}
		case SYNTHESISNODETYPE_IR_NODE 	: {
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
	if(synthesis_node->type == SYNTHESISNODETYPE_PATH){
		free(synthesis_node->node_type.path.node_buffer);
	}
}
