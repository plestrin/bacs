#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "traceMine.h"
#include "result.h"
#include "dijkstra.h"

uint32_t irEdge_get_distance(void* arg){
	struct irDependence* dependence = (struct irDependence*)arg;

	switch(dependence->type){
		case IR_DEPENDENCE_TYPE_DIRECT 		: {return 1;}
		case IR_DEPENDENCE_TYPE_ADDRESS 	: {return DIJKSTRA_INVALID_DST;}
		case IR_DEPENDENCE_TYPE_SHIFT_DISP 	: {return 1;}
		case IR_DEPENDENCE_TYPE_DIVISOR 	: {return 1;}
		case IR_DEPENDENCE_TYPE_ROUND_OFF 	: {return 1;}
		case IR_DEPENDENCE_TYPE_SUBSTITUTE 	: {return 1;}
		case IR_DEPENDENCE_TYPE_MACRO 		: {return DIJKSTRA_INVALID_DST;}
	}

	return 1;
}

enum synthesisNodeType{
	SYNTHESISNODETYPE_RESULT,
	SYNTHESISNODETYPE_IO_PATH,
	SYNTHESISNODETYPE_II_PATH,
	SYNTHESISNODETYPE_IR_NODE
};

struct signatureInstance{
	struct result* 	result;
	uint32_t		index;
};

struct signatureCluster{
	struct node* 				synthesis_graph_node;
	uint32_t 					nb_in_parameter;
	uint32_t 					nb_ou_parameter;
	struct parameterMapping* 	parameter_mapping;
	struct array 				instance_array;
};

static int32_t signatureCluster_init(struct signatureCluster* cluster, struct parameterMapping* mapping, struct result* result, uint32_t index){
	struct signatureInstance instance;

	cluster->synthesis_graph_node = NULL;
	cluster->nb_in_parameter = result->signature->nb_parameter_in;
	cluster->nb_ou_parameter = result->signature->nb_parameter_out;
	cluster->parameter_mapping = mapping;

	if (array_init(&(cluster->instance_array), sizeof(struct signatureInstance))){
		printf("ERROR: in %s, unable to init array\n", __func__);
		return -1;
	}

	instance.result = result;
	instance.index = index;

	if (array_add(&(cluster->instance_array), &instance) < 0){
		printf("ERROR: in %s, unable to add element to array\n", __func__);
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

static inline int32_t signatureCluster_append(struct signatureCluster* cluster, struct result* result, uint32_t index){
	struct signatureInstance instance;

	instance.result = result;
	instance.index = index;

	return array_add(&(cluster->instance_array), &instance);
}

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

void synthesisGraph_printDot_node(void* data, FILE* file, void* arg);
void synthesisGraph_printDot_edge(void* data, FILE* file, void* arg);

void synthesisGraph_clean_node(struct node* node);

static struct array* traceMine_cluster_results(struct array* result_array){
	struct result* 				result;
	uint32_t 					i;
	uint32_t 					j;
	uint32_t 					k;
	struct array* 				cluster_array;
	struct parameterMapping* 	mapping;
	struct signatureCluster* 	cluster_cursor;
	struct signatureCluster 	new_cluster;

	cluster_array = array_create(sizeof(struct signatureCluster));
	if (cluster_array == NULL){
		printf("ERROR: in %s, unable to create array\n", __func__);
		return NULL;
	}		

	for (i = 0, mapping = NULL; i < array_get_length(result_array); i++){
		result = (struct result*)array_get(result_array, i);
		if (result->state == RESULTSTATE_PUSH){
			for (j = 0; j < result->nb_occurrence; j++){
				if (mapping == NULL){
					mapping = parameterMapping_create(result->signature);
					if (mapping == NULL){
						printf("ERROR: in %s, unable to create parameterMapping\n", __func__);
						continue;
					}
				}

				if (parameterMapping_fill(mapping, result, j)){
					printf("ERROR: in %s, unable to fill mapping for occurrence %u\n", __func__, j);
					continue;
				}

				for (k = 0; k < array_get_length(cluster_array); k++){
					cluster_cursor = (struct signatureCluster*)array_get(cluster_array, k);
					if (!signatureCluster_may_append(cluster_cursor, mapping, result->signature->nb_parameter_in, result->signature->nb_parameter_out)){
						if (signatureCluster_append(cluster_cursor, result, j)){
							printf("ERROR: in %s, unable to add element to signatureCluster\n", __func__);
						}
						break;
					}
				}

				if (k == array_get_length(cluster_array)){
					if (signatureCluster_init(&new_cluster, mapping, result, j)){
						printf("ERROR: in %s, unable to init signatureCluster\n", __func__);
					}

					if (array_add(cluster_array, &new_cluster) < 0){
						printf("ERROR: in %s, unable to add element to array\n", __func__);
						signatureCluster_clean(&new_cluster);
					}
					else{
						mapping = NULL;
					}
				}
			}

			if (mapping != NULL){
				free(mapping);
				mapping = NULL;
			}		
		}
	}


	return cluster_array;
}

static void traceMine_search_IO_path(struct signatureCluster* cluster_in, uint32_t parameter_in, struct signatureCluster* cluster_ou, uint32_t parameter_ou, struct ir* ir, struct graph* synthesis_graph){
	int32_t 				return_code;
	struct array* 			path = NULL;
	struct synthesisNode 	synthesis_path;
	struct node* 			node_path;
	uint32_t 				edge_tag;


	return_code = dijkstra_min_path(&(ir->graph), signatureCluster_get_ou_parameter(cluster_ou, parameter_ou), signatureCluster_get_nb_frag_ou(cluster_ou, parameter_ou), signatureCluster_get_in_parameter(cluster_in, parameter_in), signatureCluster_get_nb_frag_in(cluster_in, parameter_in), &path, irEdge_get_distance);
	if (return_code < 0){
		printf("ERROR: in %s, unable to compute min path\n", __func__);
	}
	else if(return_code == 0){
		synthesis_path.type 			= SYNTHESISNODETYPE_IO_PATH;
		synthesis_path.node_type.path 	= path;

		if ((node_path = graph_add_node(synthesis_graph, &synthesis_path)) == NULL){
			printf("ERROR: in %s, unable to add node to graph\n", __func__);
		}
		else{
			edge_tag = synthesisGraph_get_edge_tag_output(parameter_ou);
			if (graph_add_edge(synthesis_graph, cluster_ou->synthesis_graph_node, node_path, &edge_tag) == NULL){
				printf("ERROR: in %s, unable to add edge to synthesisGraph\n", __func__);
			}
			edge_tag = synthesisGraph_get_edge_tag_input(parameter_in);
			if (graph_add_edge(synthesis_graph, node_path, cluster_in->synthesis_graph_node, &edge_tag) == NULL){
				printf("ERROR: in %s, unable to add edge to synthesisGraph\n", __func__);
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

	ancestor = dijkstra_lowest_common_ancestor(&(ir->graph), signatureCluster_get_in_parameter(cluster1, parameter1), signatureCluster_get_nb_frag_in(cluster1, parameter1), signatureCluster_get_in_parameter(cluster2, parameter2), signatureCluster_get_nb_frag_in(cluster2, parameter2), &path1, &path2, irEdge_get_distance);
	if (ancestor != NULL){
		synthesis_ancestor.type 				= SYNTHESISNODETYPE_IR_NODE;
		synthesis_ancestor.node_type.ir_node 	= ancestor;

		if ((node_ancestor = graph_add_node(synthesis_graph, &synthesis_ancestor)) == NULL){
			printf("ERROR: in %s, unable to add node to graph\n", __func__);
		}
		else{
			if (array_get_length(path1) > 0){
				synthesis_path.type 			= SYNTHESISNODETYPE_II_PATH;
				synthesis_path.node_type.path 	= path1;

				if ((node_path = graph_add_node(synthesis_graph, &synthesis_path)) == NULL){
					printf("ERROR: in %s, unable to add node to graph\n", __func__);
				}
				else{
					edge_tag = SYNTHESISGRAPH_EGDE_TAG_RAW;
					if (graph_add_edge(synthesis_graph, node_ancestor, node_path, &edge_tag) == NULL){
						printf("ERROR: in %s, unable to add edge to synthesisGraph\n", __func__);
					}
					edge_tag = synthesisGraph_get_edge_tag_input(parameter1);
					if (graph_add_edge(synthesis_graph, node_path, cluster1->synthesis_graph_node, &edge_tag) == NULL){
						printf("ERROR: in %s, unable to add edge to synthesisGraph\n", __func__);
					}

					path1 = NULL;
				}
			}
			else{
				edge_tag = synthesisGraph_get_edge_tag_input(parameter1);
				if (graph_add_edge(synthesis_graph, node_ancestor, cluster1->synthesis_graph_node, &edge_tag) == NULL){
					printf("ERROR: in %s, unable to add edge to synthesisGraph\n", __func__);
				}
			}

			if (array_get_length(path2) > 0){
				synthesis_path.type 			= SYNTHESISNODETYPE_II_PATH;
				synthesis_path.node_type.path 	= path2;

				if ((node_path = graph_add_node(synthesis_graph, &synthesis_path)) == NULL){
					printf("ERROR: in %s, unable to add node to graph\n", __func__);
				}
				else{
					edge_tag = SYNTHESISGRAPH_EGDE_TAG_RAW;
					if (graph_add_edge(synthesis_graph, node_ancestor, node_path, &edge_tag) == NULL){
						printf("ERROR: in %s, unable to add edge to synthesisGraph\n", __func__);
					}
					edge_tag = synthesisGraph_get_edge_tag_input(parameter2);
					if (graph_add_edge(synthesis_graph, node_path, cluster2->synthesis_graph_node, &edge_tag) == NULL){
						printf("ERROR: in %s, unable to add edge to synthesisGraph\n", __func__);
					}

					path2 = NULL;
				}
			}
			else{
				edge_tag = synthesisGraph_get_edge_tag_input(parameter2);
				if (graph_add_edge(synthesis_graph, node_ancestor, cluster2->synthesis_graph_node, &edge_tag) == NULL){
					printf("ERROR: in %s, unable to add edge to synthesisGraph\n", __func__);
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

static int32_t traceMine_find_cluster_relation(struct array* cluster_array, struct ir* ir, struct graph* synthesis_graph){
	struct signatureCluster* 	cluster_i;
	struct signatureCluster* 	cluster_j;
	uint32_t 					i;
	uint32_t 					j;
	uint32_t 					k;
	uint32_t 					l;
	struct synthesisNode 		synthesis_node;

	for (i = 0; i < array_get_length(cluster_array); i++){
		struct signatureCluster* cluster = (struct signatureCluster*)array_get(cluster_array, i);

		synthesis_node.type 				= SYNTHESISNODETYPE_RESULT;
		synthesis_node.node_type.cluster 	= cluster;

		if ((cluster->synthesis_graph_node = graph_add_node(synthesis_graph, &synthesis_node)) == NULL){
			printf("ERROR: in %s, unable to add node to graph\n", __func__);
			continue;
		}

		for (j = 0; j < cluster->nb_in_parameter; j++){
			for (k = j + 1; k < cluster->nb_ou_parameter; k++){
				traceMine_search_II_path(cluster, j, cluster, k, ir, synthesis_graph);
			}
		}
	}

	for (i = 0; i < array_get_length(cluster_array); i++){
		cluster_i = (struct signatureCluster*)array_get(cluster_array, i);
		for (j = 0; j < array_get_length(cluster_array); j++){
			if (j == i){
				continue;
			}

			cluster_j = (struct signatureCluster*)array_get(cluster_array, j);

			for (k = 0; k < cluster_i->nb_in_parameter; k++){
				for (l = 0; l < cluster_j->nb_ou_parameter; l++){
					traceMine_search_IO_path(cluster_i, k, cluster_j, l, ir, synthesis_graph);
				}
			}
		}

		for (j = i + 1; j < array_get_length(cluster_array); j++){
			cluster_j = (struct signatureCluster*)array_get(cluster_array, j);

			for (k = 0; k < cluster_i->nb_in_parameter; k++){
				for (l = 0; l < cluster_j->nb_in_parameter; l++){
					traceMine_search_II_path(cluster_i, k, cluster_j, l, ir, synthesis_graph);
				}
			}
		}
	}

	return 0;
}

void traceMine_mine(struct trace* trace){
	struct graph 	synthesis_graph;
	struct array* 	cluster_array;
	#define SYNTHESISGRAPHG_NAME_MAX_LENGTH 64
	char 			synthesis_graph_name[SYNTHESISGRAPHG_NAME_MAX_LENGTH] = {0};
	uint32_t 		i;

	if (trace->ir == NULL){
		printf("ERROR: in %s, the IR is NULL for fragment \"%s\"\n", __func__, trace->tag);
		return;
	}

	graph_init(&synthesis_graph, sizeof(struct synthesisNode), sizeof(uint32_t));
	graph_register_dotPrint_callback(&synthesis_graph, NULL, synthesisGraph_printDot_node, synthesisGraph_printDot_edge, NULL);
	graph_register_node_clean_call_back(&synthesis_graph, synthesisGraph_clean_node);

	if ((cluster_array = traceMine_cluster_results(&trace->result_array)) == NULL){
		printf("ERROR: in %s, unable to cluster results\n", __func__);
		return;
	}

	if (traceMine_find_cluster_relation(cluster_array, trace->ir, &synthesis_graph)){
		printf("ERROR: in %s, unable to find relation between clusters\n", __func__);
	}

	snprintf(synthesis_graph_name, SYNTHESISGRAPHG_NAME_MAX_LENGTH, "synthesis_%s.dot", trace->tag);
	for (i = 0; i < SYNTHESISGRAPHG_NAME_MAX_LENGTH; i++){
		switch(synthesis_graph_name[i]){
			case ' ' : {synthesis_graph_name[i] = '_'; break;}
			case ':' : {synthesis_graph_name[i] = '_'; break;}
			case '[' : {synthesis_graph_name[i] = '_'; break;}
			case ']' : {synthesis_graph_name[i] = '_'; break;}
			default  : {break;}
		}
	}

	printf("INFO: in %s, writing result(s) to: \"%s\"\n", __func__, synthesis_graph_name);

	graphPrintDot_print(&synthesis_graph, synthesis_graph_name, NULL, NULL);
	graph_clean(&synthesis_graph);

	for (i = 0; i < array_get_length(cluster_array); i++){
		signatureCluster_clean((struct signatureCluster*)array_get(cluster_array, i));
	}
	array_delete(cluster_array);

	#undef SYNTHESISGRAPHG_NAME_MAX_LENGTH
}

#pragma GCC diagnostic ignored "-Wunused-parameter"
void synthesisGraph_printDot_node(void* data, FILE* file, void* arg){
	struct synthesisNode*		synthesis_node;
	uint32_t 					i;
	struct signatureInstance* 	instance;
	struct edge* 				edge;
	struct irOperation* 		operation;

	synthesis_node = (struct synthesisNode*)data;
	switch(synthesis_node->type){
		case SYNTHESISNODETYPE_RESULT : {
			for (i = 0; i < array_get_length(&(synthesis_node->node_type.cluster->instance_array)); i++){
				instance = (struct signatureInstance*)array_get(&(synthesis_node->node_type.cluster->instance_array), i);

				if (i == 0){
					fprintf(file, "[shape=box,label=\"%s", instance->result->signature->name);
				}
				else if ((void*)instance != array_get(&(synthesis_node->node_type.cluster->instance_array), i - 1)){
					fprintf(file, "\\n%s", instance->result->signature->name);
				}
				if (i + 1 == array_get_length(&(synthesis_node->node_type.cluster->instance_array))){
					fprintf(file, "\"]");
				}
			}
			break;
		}
		case SYNTHESISNODETYPE_IO_PATH : {
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
							printf("ERROR: in %s, this case is not supposed to happen\n", __func__);
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
						
					switch(operation->type){
						case IR_OPERATION_TYPE_INST 	: {
							fprintf(file, "%s<BR ALIGN=\"LEFT\"/>", irOpcode_2_string(operation->operation_type.inst.opcode));
							break;
						}
						default 						: {
							printf("ERROR: in %s, this case is not supposed to happen: incorrect operation on path\n", __func__);
							break;
						}
					}
				}
				fprintf(file, ">]");
			}
			else{
				printf("ERROR: in %s, this case is not supposed to happen: empty II path\n", __func__);
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
void synthesisGraph_printDot_edge(void* data, FILE* file, void* arg){
	uint32_t tag = *(uint32_t*)data;

	if (synthesisGraph_edge_is_input(tag)){
		fprintf(file, "[label=\"I%u\"]", synthesisGraph_edge_get_parameter(tag));
	}
	else if (synthesisGraph_edge_is_output(tag)){
		fprintf(file, "[label=\"O%u\"]", synthesisGraph_edge_get_parameter(tag));
	}
}

void synthesisGraph_clean_node(struct node* node){
	struct synthesisNode* synthesis_node;

	synthesis_node = synthesisGraph_get_synthesisNode(node);
	if (synthesis_node->type == SYNTHESISNODETYPE_IO_PATH || synthesis_node->type == SYNTHESISNODETYPE_II_PATH){
		array_delete(synthesis_node->node_type.path);
	}
}
