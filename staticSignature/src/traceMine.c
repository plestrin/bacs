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
	SYNTHESISNODETYPE_PATH,
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
	}								node_type;
} __attribute__((__may_alias__));

#define synthesisGraph_get_synthesisNode(node) ((struct synthesisNode*)&((node)->data))

void synthesisGraph_printDot_node(void* data, FILE* file, void* arg);
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

static int32_t traceMine_find_cluster_relation(struct array* cluster_array, struct ir* ir, struct graph* synthesis_graph){
	struct signatureCluster* 	cluster_i;
	struct signatureCluster* 	cluster_j;
	uint32_t 					i;
	uint32_t 					j;
	uint32_t 					k;
	uint32_t 					l;
	struct synthesisNode 		synthesis_node;
	struct array* 				path 			= NULL;
	int32_t 					return_code;
	struct node* 				node;

	for (i = 0; i < array_get_length(cluster_array); i++){
		struct signatureCluster* cluster = (struct signatureCluster*)array_get(cluster_array, i);

		synthesis_node.type 				= SYNTHESISNODETYPE_RESULT;
		synthesis_node.node_type.cluster 	= cluster;

		if ((cluster->synthesis_graph_node = graph_add_node(synthesis_graph, &synthesis_node)) == NULL){
			printf("ERROR: in %s, unable to add node to graph\n", __func__);
			continue;
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
					return_code = dijkstra_min_path(&(ir->graph), signatureCluster_get_ou_parameter(cluster_j, l), signatureCluster_get_nb_frag_ou(cluster_j, l), signatureCluster_get_in_parameter(cluster_i, k), signatureCluster_get_nb_frag_in(cluster_i, k), &path, irEdge_get_distance);
					if (return_code < 0){
						printf("ERROR: in %s, unable to compute min path\n", __func__);
					}
					else if(return_code == 0){
						synthesis_node.type 			= SYNTHESISNODETYPE_PATH;
						synthesis_node.node_type.path 	= path;

						if ((node = graph_add_node(synthesis_graph, &synthesis_node)) == NULL){
							printf("ERROR: in %s, unable to add node to graph\n", __func__);
						}
						else{
							if (graph_add_edge_(synthesis_graph, cluster_j->synthesis_graph_node, node) == NULL){
								printf("ERROR: in %s, unable to add edge to synthesisGraph\n", __func__);
							}
							if (graph_add_edge_(synthesis_graph, node, cluster_i->synthesis_graph_node) == NULL){
								printf("ERROR: in %s, unable to add edge to synthesisGraph\n", __func__);
							}

							path = NULL;
						}
					}
				}
			}
		}
	}

	if (path != NULL){
		array_delete(path);
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

	graph_init(&synthesis_graph, sizeof(struct synthesisNode), 0);
	graph_register_dotPrint_callback(&synthesis_graph, NULL, synthesisGraph_printDot_node, NULL, NULL);
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
		case SYNTHESISNODETYPE_PATH : {
			if (array_get_length(synthesis_node->node_type.path) == 1){
				operation = ir_node_get_operation(*(struct node**)array_get(synthesis_node->node_type.path, 0));
				switch(operation->type){
					case IR_OPERATION_TYPE_INST 	: {
						fprintf(file, "[shape=plaintext,label=<%s<BR ALIGN=\"LEFT\"/>>]", irOpcode_2_string(operation->operation_type.inst.opcode));
						break;
					}
					default 						: {
						printf("ERROR: in %s, this case is not supposed to happen\n", __func__);
					}
				}
			}
			else{
				for (i = 0; i < array_get_length(synthesis_node->node_type.path); i++){
					if (i == 0){
						operation = ir_node_get_operation(*(struct node**)array_get(synthesis_node->node_type.path, 0));
						fprintf(file, "[shape=plaintext,label=<");
					}
					else{
						edge = *(struct edge**)array_get(synthesis_node->node_type.path, i);
						operation = ir_node_get_operation(edge_get_src(edge));
					}

					if (i + 1 == array_get_length(synthesis_node->node_type.path)){
						fprintf(file, ">]");
					}
					else{
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
				}
			}
			break;
		}
	}
}

void synthesisGraph_clean_node(struct node* node){
	struct synthesisNode* synthesis_node;

	synthesis_node = synthesisGraph_get_synthesisNode(node);
	if (synthesis_node->type == SYNTHESISNODETYPE_PATH){
		array_delete(synthesis_node->node_type.path);
	}
}
