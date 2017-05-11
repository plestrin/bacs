#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#include "synthesisGraph.h"
#include "dijkstra.h"
#include "arrayMinCoverage.h"
#include "base.h"

#define MIN_COVERAGE_STRATEGY 4 /* 0 is random, 1 is greedy, 2 reshape, 3 is exact, 4 is split, 5 is super */

static const uint32_t irEdge_distance_array[NB_DEPENDENCE_TYPE] = {
	0, 						/* DIRECT 		*/
	DIJKSTRA_INVALID_DST, 	/* ADDRESS 		*/
	0, 						/* SHIFT_DISP 	*/
	0, 						/* DIVISOR 		*/
	0, 						/* ROUND_OFF 	*/
	0, 						/* SUBSTITUTE 	*/
	DIJKSTRA_INVALID_DST 	/* MACRO 		*/
};

static const uint32_t irNode_distance_array[NB_OPERATION_TYPE] = {
	0, 						/* IN_REG 	*/
	0, 						/* IN_MEM 	*/
	DIJKSTRA_INVALID_DST, 	/* OUT_MEM 	*/
	DIJKSTRA_INVALID_DST, 	/* IMM 		*/
	0, 						/* INST 	*/
	0, 						/* SYMBOL 	*/
	DIJKSTRA_INVALID_DST 	/* NULL 	*/
};

static inline uint64_t irInstruction_and_get_mask(struct node* node_dst, uint64_t mask){
	uint64_t 			result = 0xffffffffffffffff;
	struct edge* 		edge_cursor;
	struct irOperation* operand;

	for (edge_cursor = node_get_head_edge_dst(node_dst); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
		operand = ir_node_get_operation(edge_get_src(edge_cursor));
		if (operand->type == IR_OPERATION_TYPE_IMM){
			result &= ir_imm_operation_get_unsigned_value(operand);
		}
	}

	return result & mask;
}

static inline uint64_t irInstruction_movzx_get_mask(struct node* node_src, uint64_t mask, enum dijkstraPathDirection dir){
	if (dir == PATH_SRC_TO_DST){
		return mask;
	}
	else{
		return mask & bitmask64(ir_node_get_operation(node_src)->size);
	}
}

static inline uint64_t irInstruction_or_get_mask(struct node* node_dst, uint64_t mask){
	uint64_t 			result = 0;
	struct edge* 		edge_cursor;
	struct irOperation* operand;

	for (edge_cursor = node_get_head_edge_dst(node_dst); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
		operand = ir_node_get_operation(edge_get_src(edge_cursor));
		if (operand->type == IR_OPERATION_TYPE_IMM){
			result |= ir_imm_operation_get_unsigned_value(operand);
		}
	}

	return (~result) & mask;
}

static inline uint64_t irInstruction_part1_x_get_mask(struct node* node_dst, uint64_t mask, enum dijkstraPathDirection dir){
	if (dir == PATH_SRC_TO_DST){
		return mask & bitmask64(ir_node_get_operation(node_dst)->size);
	}
	else{
		return mask;
	}
}

static inline uint64_t irInstruction_part2_8_get_mask(struct node* node_dst, uint64_t mask, enum dijkstraPathDirection dir){
	if (dir == PATH_SRC_TO_DST){
		return (mask >> 8) & bitmask64(ir_node_get_operation(node_dst)->size);
	}
	else{
		return mask << 8;
	}
}

static inline uint64_t irInstruction_shl_get_mask(struct node* node_dst, uint64_t mask, enum dijkstraPathDirection dir){
	uint32_t 			disp = 0;
	struct edge* 		edge_cursor;
	struct irOperation* operand;

	for (edge_cursor = node_get_head_edge_dst(node_dst); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
		if (ir_edge_get_dependence(edge_cursor)->type == IR_DEPENDENCE_TYPE_SHIFT_DISP){
			operand = ir_node_get_operation(edge_get_src(edge_cursor));
			if (operand->type == IR_OPERATION_TYPE_IMM){
				disp = (uint32_t)ir_imm_operation_get_unsigned_value(operand);
			}
			break;
		}
	}

	if (dir == PATH_SRC_TO_DST){
		return (mask << disp) & bitmask64(ir_node_get_operation(node_dst)->size);
	}
	else{
		return mask >> disp;
	}
}

static inline uint64_t irInstruction_rol_get_mask(struct node* node_dst, uint64_t mask, enum dijkstraPathDirection dir){
	uint32_t 			disp = 0;
	struct edge* 		edge_cursor;
	struct irOperation* operand;

	for (edge_cursor = node_get_head_edge_dst(node_dst); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
		if (ir_edge_get_dependence(edge_cursor)->type == IR_DEPENDENCE_TYPE_SHIFT_DISP){
			operand = ir_node_get_operation(edge_get_src(edge_cursor));
			if (operand->type == IR_OPERATION_TYPE_IMM){
				disp = (uint32_t)ir_imm_operation_get_unsigned_value(operand);
			}
			break;
		}
	}

	if (dir == PATH_SRC_TO_DST){
		return ((mask << disp) & bitmask64(ir_node_get_operation(node_dst)->size)) | (mask >> (ir_node_get_operation(node_dst)->size - disp));
	}
	else{
		return (mask >> disp) | ((mask << (ir_node_get_operation(node_dst)->size - disp)) & bitmask64(ir_node_get_operation(node_dst)->size));
	}
}

static uint64_t irOperation_get_mask(uint64_t mask, struct node* node, struct edge* edge, enum dijkstraPathDirection dir){
	struct node* 			node_src;
	struct node* 			node_dst;
	struct irOperation* 	operation;
	struct irDependence* 	dependence;

	dependence = ir_edge_get_dependence(edge);
	if (irEdge_distance_array[dependence->type] == DIJKSTRA_INVALID_DST){
		return 0;
	}

	switch(dir){
		case PATH_SRC_TO_DST : {
			if (irNode_distance_array[ir_node_get_operation(edge_get_dst(edge))->type] == DIJKSTRA_INVALID_DST){
				return 0;
			}
			node_src = node;
			node_dst = edge_get_dst(edge);
			break;
		}
		case PATH_DST_TO_SRC : {
			if (irNode_distance_array[ir_node_get_operation(edge_get_src(edge))->type] == DIJKSTRA_INVALID_DST){
				return 0;
			}
			node_src = edge_get_src(edge);
			node_dst = node;
			break;
		}
		default : {
			log_err("this case is not supposed to happen");
			return 0;
		}
	}

	operation = ir_node_get_operation(node_dst);
	switch (operation->type){
		case IR_OPERATION_TYPE_INST : {
			switch (operation->operation_type.inst.opcode){
				case IR_ADC 		: {break;}
				case IR_ADD 		: {break;}
				case IR_AND 		: {
					return irInstruction_and_get_mask(node_dst, mask);
				}
				case IR_CMOV 		: {
					return mask;
				}
				case IR_DIVQ 		: {break;}
				case IR_DIVR 		: {break;}
				case IR_IDIVQ 		: {break;}
				case IR_IDIVR 		: {break;}
				case IR_IMUL 		: {break;}
				case IR_MOVZX 		: {
					return irInstruction_movzx_get_mask(node_src, mask, dir);
				}
				case IR_MUL 		: {break;}
				case IR_NEG 		: {break;}
				case IR_NOT 		: {
					return mask;
				}
				case IR_OR 			: {
					return irInstruction_or_get_mask(node_dst, mask);
				}
				case IR_PART1_8 	: {
					return irInstruction_part1_x_get_mask(node_dst, mask, dir);
				}
				case IR_PART2_8 	: {
					return irInstruction_part2_8_get_mask(node_dst, mask, dir);
				}
				case IR_PART1_16 	: {
					return irInstruction_part1_x_get_mask(node_dst, mask, dir);
				}
				case IR_ROL 		: {
					return irInstruction_rol_get_mask(node_dst, mask, dir);
				}
				case IR_ROR 		: {
					return irInstruction_rol_get_mask(node_dst, mask, dijkstraPathDirection_invert(dir));
				}
				case IR_SHL 		: {
					return irInstruction_shl_get_mask(node_dst, mask, dir);
				}
				case IR_SHLD 		: {break;}
				case IR_SHR 		: {
					return irInstruction_shl_get_mask(node_dst, mask, dijkstraPathDirection_invert(dir));
				}
				case IR_SHRD 		: {break;}
				case IR_SUB 		: {break;}
				case IR_XOR 		: {
					return mask;
				}
				default 			: {
					break;
				}
			}
			break;
		}
		default : {
			break;
		}
	}

	return bitmask64(operation->size);
}

static int32_t signatureCluster_init(struct signatureCluster* cluster, struct symbolMapping* mapping, struct node* node){
	cluster->synthesis_graph_node 	= NULL;
	cluster->symbol_mapping 		= mapping;

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

#define signatureCluster_get_nb_para_in(cluster) ((cluster)->symbol_mapping[0].nb_parameter)
#define signatureCluster_get_nb_para_ou(cluster) ((cluster)->symbol_mapping[1].nb_parameter)

#define signatureCluster_get_nb_frag_in(cluster, parameter) ((cluster)->symbol_mapping[0].mapping_buffer[parameter].nb_fragment)
#define signatureCluster_get_nb_frag_ou(cluster, parameter) ((cluster)->symbol_mapping[1].mapping_buffer[parameter].nb_fragment)

#define signatureCluster_get_in_parameter(cluster, parameter) ((cluster)->symbol_mapping[0].mapping_buffer[parameter].ptr_buffer)
#define signatureCluster_get_ou_parameter(cluster, parameter) ((cluster)->symbol_mapping[1].mapping_buffer[parameter].ptr_buffer)

#define signatureCluster_add(cluster, node) array_add(&((cluster)->instance_array), &(node))

static inline void signatureCluster_clean(struct signatureCluster* cluster){
	if (cluster->symbol_mapping != NULL){
		free(cluster->symbol_mapping);
	}
	array_clean(&(cluster->instance_array));
}

static void synthesisGraph_clean_node(struct node* node);

static void synthesisGraph_cluster_symbols(struct synthesisGraph* synthesis_graph, struct ir* ir){
	uint32_t 					i;
	struct node*				node_cursor;
	struct irOperation* 		operation_cursor;
	struct symbolMapping* 		mapping;
	struct signatureCluster* 	cluster_cursor;
	struct signatureCluster 	new_cluster;

	for (node_cursor = graph_get_head_node(&(ir->graph)); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
		operation_cursor = ir_node_get_operation(node_cursor);

		if (operation_cursor->type == IR_OPERATION_TYPE_SYMBOL){
			if ((mapping = symbolMapping_create_from_ir(node_cursor)) == NULL){
				log_err("unable to create parameterMapping");
				continue;
			}

			for (i = 0; i < array_get_length(&(synthesis_graph->cluster_array)); i++){
				cluster_cursor = (struct signatureCluster*)array_get(&(synthesis_graph->cluster_array), i);
				if (symbolMapping_may_append(cluster_cursor->symbol_mapping, mapping) == 0){
					if (signatureCluster_add(cluster_cursor, node_cursor) < 0){
						log_err("unable to add element to signatureCluster");
					}
					free(mapping);
					break;
				}
			}

			if (i == array_get_length(&(synthesis_graph->cluster_array))){
				if (signatureCluster_init(&new_cluster, mapping, node_cursor)){
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
	struct synthesisNode* synthesis_node1 = synthesisGraph_get_synthesisNode(*(struct node* const*)data1);
	struct synthesisNode* synthesis_node2 = synthesisGraph_get_synthesisNode(*(struct node* const*)data2);

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
	struct edge* edge1 = *(struct edge* const*)data1;
	struct edge* edge2 = *(struct edge* const*)data2;

	if (edge_get_src(edge1) < edge_get_src(edge2)){
		return -1;
	}
	else if (edge_get_src(edge1) > edge_get_src(edge2)){
		return 1;
	}
	else{
		return 0;
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
	struct synthesisEdge* 	synthesis_edge_cursor;
	uint32_t 				i;
	uint32_t 				j;
	uint32_t 				k;

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

	if (nb_node){
		qsort(node_buffer, nb_node, sizeof(struct node*), synthesisGraph_compare_ir_node);

		for (i = 1, j = 0; i < nb_node; i++){
			if (synthesisGraph_compare_ir_node(node_buffer + j, node_buffer + i) == 0){
				graph_transfert_dst_edge(graph, node_buffer[j], node_buffer[i]);
				graph_transfert_src_edge(graph, node_buffer[j], node_buffer[i]);
				graph_remove_node(graph, node_buffer[i]);
			}
			else{
				node_buffer[++ j] = node_buffer[i];
			}
		}

		for (nb_node = j + 1, node_cursor = graph_get_head_node(graph); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
			synthesis_node_cursor = synthesisGraph_get_synthesisNode(node_cursor);

			if (synthesis_node_cursor->type == SYNTHESISNODETYPE_RESULT){
				for (i = 0; i < synthesis_node_cursor->node_type.cluster->symbol_mapping[1].nb_parameter; i++){
					for (j = 0; j < synthesis_node_cursor->node_type.cluster->symbol_mapping[1].mapping_buffer[i].nb_fragment; j++){
						for (k = 0; k < nb_node; k++){
							if (synthesisGraph_get_synthesisNode(node_buffer[k])->node_type.ir_node == synthesis_node_cursor->node_type.cluster->symbol_mapping[1].mapping_buffer[i].ptr_buffer[j]){
								graph_transfert_dst_edge(graph, synthesis_node_cursor->node_type.cluster->synthesis_graph_node, node_buffer[k]);
								graph_transfert_src_edge(graph, synthesis_node_cursor->node_type.cluster->synthesis_graph_node, node_buffer[k]);
								graph_remove_node(graph, node_buffer[k]);

								node_buffer[k] = node_buffer[-- nb_node];
								break;
							}
						}
					}
				}
			}
		}
	}

	for (node_cursor = graph_get_head_node(graph); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
		synthesis_node_cursor = synthesisGraph_get_synthesisNode(node_cursor);

		for (edge_cursor = node_get_head_edge_dst(node_cursor), nb_edge = 0; edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
			edge_buffer[nb_edge ++] = edge_cursor;
		}

		qsort(edge_buffer, nb_edge, sizeof(struct edge*), synthesisGraph_compare_edge);

		for (i = 1, j = 0; i < nb_edge; i++){
			synthesis_edge_cursor = synthesisGraph_get_synthesisEdge(edge_buffer[j]);
			if (synthesisGraph_compare_edge(edge_buffer + j, edge_buffer + i) == 0){
				synthesis_edge_cursor->tag = max(synthesis_edge_cursor->tag, synthesisGraph_get_synthesisEdge(edge_buffer[i])->tag);
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
	struct synthesisEdge 		synthesis_edge;

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
				if (dst_edge_tag != SYNTHESISEGDE_TAG_RAW && src_edge_tag != SYNTHESISEGDE_TAG_RAW){
					log_warn("get different tags for the same edge");
				}
				src_edge_tag = max(dst_edge_tag, src_edge_tag);
			}

			synthesis_edge.tag = src_edge_tag;
			if (graph_add_edge(synthesis_graph, node_src, new_node, &synthesis_edge) == NULL){
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
				if (dst_edge_tag != SYNTHESISEGDE_TAG_RAW && src_edge_tag != SYNTHESISEGDE_TAG_RAW){
					log_warn("get different tags for the same edge");
				}
				src_edge_tag = max(dst_edge_tag, src_edge_tag);
			}

			synthesis_edge.tag = src_edge_tag;
			if (graph_add_edge(synthesis_graph, new_node, node_src, &synthesis_edge) == NULL){
				log_err("unable to add edge to synthesisGraph");
				return -1;
			}
		}

		node_src = new_node;
		src_edge_tag = SYNTHESISEGDE_TAG_RAW;
	}

	return 0;
}

#define FALSE_POSITIVE_FILTER_HEURISTIC

#ifdef FALSE_POSITIVE_FILTER_HEURISTIC
#define MAX_DIRECTION_CHANGE 5
#define synthesisGraph_false_positive_filter_heuristic(path) (dijkstraPath_get_nb_dir_change(path) > MAX_DIRECTION_CHANGE)
#endif

static int32_t dijkstraPathStep_compare(const void* arg1, const void* arg2){
	const struct dijkstraPathStep* step1 = (const struct dijkstraPathStep*)cmp_get_element(arg1);
	const struct dijkstraPathStep* step2 = (const struct dijkstraPathStep*)cmp_get_element(arg2);

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

static void dijkstraPath_add_input_start(struct array* path_array, uint32_t offset, uint32_t length, struct signatureCluster* cluster, uint32_t parameter){
	uint32_t 					i;
	uint32_t 					j;
	struct dijkstraPath* 		path;
	struct edge* 				edge_cursor;
	struct node* 				symbol;
	struct irDependence* 		dependence;
	struct dijkstraPathStep 	step;
	struct dijkstraPathStep* 	first_step;
	struct node* 				starting_node;

	for (i = offset; i < offset + length; i++){
		path = (struct dijkstraPath*)array_get(path_array, i);

		if (array_get_length(path->step_array) == 0){
			starting_node = path->reached_node;
		}
		else{
			first_step = (struct dijkstraPathStep*)array_get(path->step_array, array_get_length(path->step_array) - 1);
			if (first_step->dir == PATH_SRC_TO_DST){
				starting_node = edge_get_src(first_step->edge);
			}
			else{
				starting_node = edge_get_dst(first_step->edge);
			}
		}

		for (j = 0; j < array_get_length(&(cluster->instance_array)); j++){
			symbol = *(struct node**)array_get(&(cluster->instance_array), j);
			for (edge_cursor = node_get_head_edge_dst(symbol); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
				dependence = ir_edge_get_dependence(edge_cursor);
				if (dependence->type != IR_DEPENDENCE_TYPE_MACRO || IR_DEPENDENCE_MACRO_DESC_IS_OUTPUT(dependence->dependence_type.macro) || IR_DEPENDENCE_MACRO_DESC_GET_PARA(dependence->dependence_type.macro) != parameter + 1){
					continue;
				}
				if (edge_get_src(edge_cursor) == starting_node){
					goto found;
				}
			}
		}

		log_err("unable to find the first edge between the symbol and the path");
		continue;

		found:
		step.edge = edge_cursor;
		step.dir = PATH_DST_TO_SRC;

		if (array_add(path->step_array, &step) < 0){
			log_err("unable to add element to array");
		}

		#ifdef EXTRA_CHECK
		if (dijkstraPath_check(path)){
			log_err("incorrect path");
		}
		#endif
	}
}

static void dijkstraPath_add_input_stop(struct array* path_array, uint32_t offset, uint32_t length, struct signatureCluster* cluster, uint32_t parameter){
	uint32_t 					i;
	uint32_t 					j;
	struct dijkstraPath* 		path;
	struct array* 				step_array;
	struct edge* 				edge_cursor;
	struct node* 				symbol;
	struct irDependence* 		dependence;
	struct dijkstraPathStep 	step;

	for (i = offset; i < offset + length; i++){
		path = (struct dijkstraPath*)array_get(path_array, i);
		for (j = 0; j < array_get_length(&(cluster->instance_array)); j++){
			symbol = *(struct node**)array_get(&(cluster->instance_array), j);
			for (edge_cursor = node_get_head_edge_dst(symbol); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
				dependence = ir_edge_get_dependence(edge_cursor);
				if (dependence->type != IR_DEPENDENCE_TYPE_MACRO || IR_DEPENDENCE_MACRO_DESC_IS_OUTPUT(dependence->dependence_type.macro) || IR_DEPENDENCE_MACRO_DESC_GET_PARA(dependence->dependence_type.macro) != parameter + 1){
					continue;
				}
				if (edge_get_src(edge_cursor) == path->reached_node){
					goto found;
				}
			}
		}

		log_err("unable to find the last edge between the reached node and the symbol");
		continue;

		found:

		if ((step_array = array_create(sizeof(struct dijkstraPathStep))) == NULL){
			log_err("unable to create array");
			continue;
		}

		step.edge = edge_cursor;
		step.dir = PATH_SRC_TO_DST;

		if (array_add(step_array, &step) < 0){
			log_err("unable to add element to array");
			array_delete(step_array);
			continue;
		}

		if (array_copy(path->step_array, step_array, 0, array_get_length(path->step_array)) < 0){
			log_err("unable to copy array");
			array_delete(step_array);
			continue;
		}

		array_delete(path->step_array);
		path->step_array = step_array;

		#ifdef EXTRA_CHECK
		if (dijkstraPath_check(path)){
			log_err("incorrect path");
		}
		#endif
	}
}

static void synthesisGraph_find_cluster_relation(struct synthesisGraph* synthesis_graph, struct ir* ir){
	struct relationResultDescriptor{
		uint32_t 					offset;
		uint32_t 					length;
		struct signatureCluster* 	cluster_src;
		uint32_t 					tag_src;
		struct signatureCluster* 	cluster_dst;
		uint32_t 					tag_dst;
	};

	struct signatureCluster* 			cluster_i;
	struct signatureCluster* 			cluster_j;
	uint32_t 							i;
	uint32_t 							j;
	uint32_t 							k;
	uint32_t 							l;
	struct synthesisNode 				synthesis_node;
	struct array 						path_array;
	struct array 						rrd_array;
	struct relationResultDescriptor* 	rrd_ptr;
	uint32_t 							nb_category;

	if (array_init(&path_array, sizeof(struct dijkstraPath))){
		log_err("unable to init array");
		return;
	}

	if (array_init(&rrd_array, sizeof(struct relationResultDescriptor))){
		log_err("unable to init array");
		array_clean(&path_array);
		return;
	}

	#define traceMine_search_path(buffer_src, nb_src, cluster_src_, tag_src_, buffer_dst, nb_dst, cluster_dst_, tag_dst_) 						\
	{ 																																			\
		struct relationResultDescriptor rrd; 																									\
																																				\
		rrd.offset 		= array_get_length(&path_array); 																						\
		rrd.cluster_src = cluster_src_; 																										\
		rrd.tag_src 	= tag_src_; 																											\
		rrd.cluster_dst = cluster_dst_; 																										\
		rrd.tag_dst 	= tag_dst_; 																											\
																																				\
		if (dijkstra_min_path(&(ir->graph), buffer_src, nb_src, buffer_dst, nb_dst, &path_array, irOperation_get_mask)){ 						\
			log_err("Dijkstra min path returned an error code"); 																				\
		} 																																		\
		else if ((rrd.length = array_get_length(&path_array) - rrd.offset) > 0){ 																\
			if (array_add(&rrd_array, &rrd) < 0){ 																								\
				log_err("unable to add element to array"); 																						\
			} 																																	\
			if (synthesisEdge_is_input(tag_src_)){ 																								\
				dijkstraPath_add_input_start(&path_array, rrd.offset, rrd.length, cluster_src_, synthesisEdge_get_parameter(tag_src_)); 		\
			} 																																	\
			if (synthesisEdge_is_input(tag_dst_)){ 																								\
				dijkstraPath_add_input_stop(&path_array, rrd.offset, rrd.length, cluster_dst_, synthesisEdge_get_parameter(tag_dst_)); 			\
			} 																																	\
		} 																																		\
	}

	for (i = 0; i < array_get_length(&(synthesis_graph->cluster_array)); i++){
		struct signatureCluster* cluster = (struct signatureCluster*)array_get(&(synthesis_graph->cluster_array), i);

		synthesis_node.type 				= SYNTHESISNODETYPE_RESULT;
		synthesis_node.node_type.cluster 	= cluster;

		if ((cluster->synthesis_graph_node = graph_add_node(&(synthesis_graph->graph), &synthesis_node)) == NULL){
			log_err("unable to add node to graph");
			continue;
		}

		for (j = 0; j < signatureCluster_get_nb_para_in(cluster); j++){
			for (k = j + 1; k < signatureCluster_get_nb_para_in(cluster); k++){
				traceMine_search_path( 	signatureCluster_get_in_parameter(cluster, j), 		/* buffer_src 	*/
										signatureCluster_get_nb_frag_in(cluster, j), 		/* nb_src 		*/
										cluster, 											/* cluster_src 	*/
										synthesisEdge_get_tag_input(j), 					/* tag_src 		*/
										signatureCluster_get_in_parameter(cluster, k), 		/* buffer_dst 	*/
										signatureCluster_get_nb_frag_in(cluster, k), 		/* nb_dst 		*/
										cluster, 											/* cluster_dst 	*/
										synthesisEdge_get_tag_input(k)) 					/* tag_dst 		*/
			}
		}

		for (j = 0; j < signatureCluster_get_nb_para_ou(cluster); j++){
			for (k = j + 1; k < signatureCluster_get_nb_para_ou(cluster); k++){
				traceMine_search_path( 	signatureCluster_get_ou_parameter(cluster, j), 		/* buffer_src 	*/
										signatureCluster_get_nb_frag_ou(cluster, j), 		/* nb_src 		*/
										cluster, 											/* cluster_src 	*/
										synthesisEdge_get_tag_output(j), 					/* tag_src 		*/
										signatureCluster_get_ou_parameter(cluster, k), 		/* buffer_dst 	*/
										signatureCluster_get_nb_frag_ou(cluster, k), 		/* nb_dst 		*/
										cluster, 											/* cluster_dst 	*/
										synthesisEdge_get_tag_output(k)) 					/* tag_dst 		*/
			}
		}

		for (j = 0; j < signatureCluster_get_nb_para_in(cluster); j++){
			for (k = 0; k < signatureCluster_get_nb_para_ou(cluster); k++){
				traceMine_search_path( 	signatureCluster_get_in_parameter(cluster, j), 		/* buffer_src 	*/
										signatureCluster_get_nb_frag_in(cluster, j), 		/* nb_src 		*/
										cluster, 											/* cluster_src 	*/
										synthesisEdge_get_tag_input(j), 					/* tag_src 		*/
										signatureCluster_get_ou_parameter(cluster, k), 		/* buffer_dst 	*/
										signatureCluster_get_nb_frag_ou(cluster, k), 		/* nb_dst 		*/
										cluster, 											/* cluster_dst 	*/
										synthesisEdge_get_tag_output(k)) 					/* tag_dst 		*/
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

			for (k = 0; k < signatureCluster_get_nb_para_in(cluster_i); k++){
				for (l = 0; l < signatureCluster_get_nb_para_ou(cluster_j); l++){
					traceMine_search_path( 	signatureCluster_get_in_parameter(cluster_i, k), 	/* buffer_src 	*/
											signatureCluster_get_nb_frag_in(cluster_i, k), 		/* nb_src 		*/
											cluster_i, 											/* cluster_src 	*/
											synthesisEdge_get_tag_input(k), 					/* tag_src 		*/
											signatureCluster_get_ou_parameter(cluster_j, l), 	/* buffer_dst 	*/
											signatureCluster_get_nb_frag_ou(cluster_j, l), 		/* nb_dst 		*/
											cluster_j, 											/* cluster_dst 	*/
											synthesisEdge_get_tag_output(l)) 					/* tag_dst 		*/
				}
			}
		}
		for (j = i + 1; j < array_get_length(&(synthesis_graph->cluster_array)); j++){
			cluster_j = (struct signatureCluster*)array_get(&(synthesis_graph->cluster_array), j);

			for (k = 0; k < signatureCluster_get_nb_para_in(cluster_i); k++){
				for (l = 0; l < signatureCluster_get_nb_para_in(cluster_j); l++){
					traceMine_search_path( 	signatureCluster_get_in_parameter(cluster_i, k), 	/* buffer_src 	*/
											signatureCluster_get_nb_frag_in(cluster_i, k), 		/* nb_src 		*/
											cluster_i, 											/* cluster_src 	*/
											synthesisEdge_get_tag_input(k), 					/* tag_src 		*/
											signatureCluster_get_in_parameter(cluster_j, l), 	/* buffer_dst 	*/
											signatureCluster_get_nb_frag_in(cluster_j, l), 		/* nb_dst 		*/
											cluster_j, 											/* cluster_dst 	*/
											synthesisEdge_get_tag_input(l)) 					/* tag_dst 		*/
				}
			}

			for (k = 0; k < signatureCluster_get_nb_para_ou(cluster_i); k++){
				for (l = 0; l < signatureCluster_get_nb_para_ou(cluster_j); l++){
					traceMine_search_path( 	signatureCluster_get_ou_parameter(cluster_i, k), 	/* buffer_src 	*/
											signatureCluster_get_nb_frag_ou(cluster_i, k), 		/* nb_src 		*/
											cluster_i, 											/* cluster_src 	*/
											synthesisEdge_get_tag_output(k), 					/* tag_src 		*/
											signatureCluster_get_ou_parameter(cluster_j, l), 	/* buffer_dst 	*/
											signatureCluster_get_nb_frag_ou(cluster_j, l), 		/* nb_dst 		*/
											cluster_j, 											/* cluster_dst 	*/
											synthesisEdge_get_tag_output(l)) 					/* tag_dst 		*/
				}
			}
		}
	}

	for (i = 0, nb_category = 0; i < array_get_length(&rrd_array); i++){
		rrd_ptr = (struct relationResultDescriptor*)array_get(&rrd_array, i);

		#ifdef FALSE_POSITIVE_FILTER_HEURISTIC
		{
			uint32_t 			length = rrd_ptr->length;
			struct dijkstraPath tmp_path;

			for (k = 0; k < length; ){
				if (!synthesisGraph_false_positive_filter_heuristic((struct dijkstraPath*)array_get(&path_array, rrd_ptr->offset + k))){
					k ++;
				}
				else{
					if (k < length - 1){
						memcpy(&tmp_path, array_get(&path_array, rrd_ptr->offset + length - 1), sizeof(struct dijkstraPath));
						memcpy(array_get(&path_array, rrd_ptr->offset + length - 1), array_get(&path_array, rrd_ptr->offset + k), sizeof(struct dijkstraPath));
						memcpy(array_get(&path_array, rrd_ptr->offset + k), &tmp_path, sizeof(struct dijkstraPath));
					}

					length --;
				}
			}

			if (length < rrd_ptr->length){
				log_debug_m("filtering %u false positive path(s)", rrd_ptr->length - length);
				rrd_ptr->length = length;
			}
		}
		#endif

		if (rrd_ptr->length == 1){
			if (synthesisGraph_add_dijkstraPath(&(synthesis_graph->graph), rrd_ptr->cluster_src->synthesis_graph_node, rrd_ptr->tag_src, rrd_ptr->cluster_dst->synthesis_graph_node, rrd_ptr->tag_dst, array_get(&path_array, rrd_ptr->offset))){
				log_err("unable add path to synthesisGraph");
			}
		}
		else if (rrd_ptr->length > 1){
			nb_category ++;
		}
	}

	if (nb_category){
		struct categoryDesc* 	desc_buffer;
		int32_t 				result;
		#ifdef VERBOSE
		uint32_t 				score;
		#endif

		if ((desc_buffer = (struct categoryDesc*)malloc(sizeof(struct categoryDesc) * nb_category)) == NULL){
			log_err("unable to allocate memory");
		}
		else{
			for (i = 0, j = 0; i < array_get_length(&rrd_array); i++){
				rrd_ptr = (struct relationResultDescriptor*)array_get(&rrd_array, i);
				if (rrd_ptr->length > 1){
					desc_buffer[j].offset 		= rrd_ptr->offset;
					desc_buffer[j].nb_element 	= rrd_ptr->length;
					j ++;
				}
			}

			#ifdef VERBOSE
			#if MIN_COVERAGE_STRATEGY == 0
			result = arrayMinCoverage_rand_wrapper(&path_array, nb_category, desc_buffer, dijkstraPathStep_compare, &score);
			#elif MIN_COVERAGE_STRATEGY == 1
			result = arrayMinCoverage_greedy_wrapper(&path_array, nb_category, desc_buffer, dijkstraPathStep_compare, &score);
			#elif MIN_COVERAGE_STRATEGY == 2
			result = arrayMinCoverage_reshape_wrapper(&path_array, nb_category, desc_buffer, dijkstraPathStep_compare, &score);
			#elif MIN_COVERAGE_STRATEGY == 3
			result = arrayMinCoverage_exact_wrapper(&path_array, nb_category, desc_buffer, dijkstraPathStep_compare, &score);
			#elif MIN_COVERAGE_STRATEGY == 4
			result = arrayMinCoverage_split_wrapper(&path_array, nb_category, desc_buffer, dijkstraPathStep_compare, &score);
			#elif MIN_COVERAGE_STRATEGY == 5
			result = arrayMinCoverage_super_wrapper(&path_array, nb_category, desc_buffer, dijkstraPathStep_compare, &score);
			#else
			#error Incorrect strategy number. Valid values are: 0, 1, 2, 3, 4, 5
			#endif
			#else
			#if MIN_COVERAGE_STRATEGY == 0
			result = arrayMinCoverage_rand_wrapper(&path_array, nb_category, desc_buffer, dijkstraPathStep_compare, NULL);
			#elif MIN_COVERAGE_STRATEGY == 1
			result = arrayMinCoverage_greedy_wrapper(&path_array, nb_category, desc_buffer, dijkstraPathStep_compare, NULL);
			#elif MIN_COVERAGE_STRATEGY == 2
			result = arrayMinCoverage_reshape_wrapper(&path_array, nb_category, desc_buffer, dijkstraPathStep_compare, NULL);
			#elif MIN_COVERAGE_STRATEGY == 3
			result = arrayMinCoverage_exact_wrapper(&path_array, nb_category, desc_buffer, dijkstraPathStep_compare, NULL);
			#elif MIN_COVERAGE_STRATEGY == 4
			result = arrayMinCoverage_split_wrapper(&path_array, nb_category, desc_buffer, dijkstraPathStep_compare, NULL);
			#elif MIN_COVERAGE_STRATEGY == 5
			result = arrayMinCoverage_super_wrapper(&path_array, nb_category, desc_buffer, dijkstraPathStep_compare, NULL);
			#else
			#error Incorrect strategy number. Valid values are: 0, 1, 2, 3, 4, 5
			#endif
			#endif

			if (result){
				log_err("minCoverage function returned an error code");
			}
			else{
				#ifdef VERBOSE
				log_info_m("min coverage score is %u", score);
				#endif

				for (i = 0, j = 0; i < array_get_length(&rrd_array); i++){
					rrd_ptr = (struct relationResultDescriptor*)array_get(&rrd_array, i);
					if (rrd_ptr->length > 1){
						if (synthesisGraph_add_dijkstraPath(&(synthesis_graph->graph), rrd_ptr->cluster_src->synthesis_graph_node, rrd_ptr->tag_src, rrd_ptr->cluster_dst->synthesis_graph_node, rrd_ptr->tag_dst, array_get(&path_array, rrd_ptr->offset + desc_buffer[j].choice))){
							log_err("unable add path to synthesisGraph");
						}
						j++;
					}
				}
			}

			free(desc_buffer);
		}
	}

	for (i = 0; i < array_get_length(&path_array); i++){
		dijkstraPath_clean((struct dijkstraPath*)array_get(&path_array, i));
	}

	array_clean(&path_array);
	array_clean(&rrd_array);
}

struct synthesisGraph* synthesisGraph_create(struct ir* ir){
	struct synthesisGraph* 	synthesis_graph;
	struct timespec 		timer_start;
	struct timespec 		timer_stop;

	synthesis_graph = (struct synthesisGraph*)malloc(sizeof(struct synthesisGraph));
	if (synthesis_graph != NULL){
		if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &timer_start)){
			log_err("clock_gettime fails");
		}

		if (synthesisGraph_init(synthesis_graph, ir)){
			log_err("unable to init synthesisGraph");
			free(synthesis_graph);
			synthesis_graph = NULL;
		}
		else{
			if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &timer_stop)){
				log_err("clock_gettime fails");
			}

			log_info_m("total elapsed time: %f for %u node(s) and %u edge(s)", (timer_stop.tv_sec - timer_start.tv_sec) + (timer_stop.tv_nsec - timer_start.tv_nsec) / 1000000000., ir->graph.nb_node, ir->graph.nb_edge);
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

	graph_init(&(synthesis_graph->graph), sizeof(struct synthesisNode), sizeof(struct synthesisEdge));
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
void synthesisGraph_printDot_node(void* data, FILE* file, void* arg){
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
					fprintf(file, "[shape=box,label=\"%s", ir_node_get_operation(symbol)->operation_type.symbol.sym_sig->name);
				}
				else if (ir_node_get_operation(symbol)->operation_type.symbol.sym_sig->id != ir_node_get_operation(*(struct node**)array_get(&(synthesis_node->node_type.cluster->instance_array), i - 1))->operation_type.symbol.sym_sig->id){
					fprintf(file, "\\n%s", ir_node_get_operation(symbol)->operation_type.symbol.sym_sig->name);
				}
				if (i + 1 == array_get_length(&(synthesis_node->node_type.cluster->instance_array))){
					fputs("\"]", file);
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
void synthesisGraph_printDot_edge(void* data, FILE* file, void* arg){
	struct synthesisEdge* synthesis_edge = (struct synthesisEdge*)data;

	if (synthesisEdge_is_input(synthesis_edge->tag)){
		fprintf(file, "[label=\"I%u\"]", synthesisEdge_get_parameter(synthesis_edge->tag));
	}
	else if (synthesisEdge_is_output(synthesis_edge->tag)){
		fprintf(file, "[label=\"O%u\"]", synthesisEdge_get_parameter(synthesis_edge->tag));
	}
}

static void synthesisGraph_clean_node(struct node* node){
	struct synthesisNode* synthesis_node;

	synthesis_node = synthesisGraph_get_synthesisNode(node);
	if(synthesis_node->type == SYNTHESISNODETYPE_PATH){
		free(synthesis_node->node_type.path.node_buffer);
	}
}
