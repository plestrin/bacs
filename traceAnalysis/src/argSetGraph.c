#include <stdlib.h>
#include <stdio.h>

#include "argSetGraph.h"

static void argSetGraph_node_print_dot(void* node_data, FILE* file, void* arg);
static void argSetGraph_edge_print_dot(void* edge_data, FILE* file, void* arg);

static int32_t argSetGraph_add_argSet(struct argSetGraph* arg_set_graph, uint32_t index);


struct argSetGraph* argSetGraph_create(struct array* arg_array){
	struct argSetGraph* result;

	result = (struct argSetGraph*)malloc(sizeof(struct argSetGraph));
	if (result != NULL){
		if (argSetGraph_init(result, arg_array)){
			printf("ERROR: in %s, unable to init argSetGraph\n", __func__);
			free(result);
			result = NULL;
		}
	}

	return result;
}

int32_t argSetGraph_init(struct argSetGraph* arg_set_graph, struct array* arg_array){
	struct graphCallback 	callback;
	uint32_t				i;

	callback.node_print_dot = argSetGraph_node_print_dot;
	callback.edge_print_dot = argSetGraph_edge_print_dot;
	callback.node_clean 	= NULL;
	callback.edge_clean 	= (void(*)(void*))argBuffer_delete;

	if(graph_init(&(arg_set_graph->graph), &callback)){
		printf("ERROR: in %s, unable to init graph\n", __func__);
		return -1;
	}

	arg_set_graph->arg_array = arg_array;

	for (i = 0; i < array_get_length(arg_array); i++){
		if (argSetGraph_add_argSet(arg_set_graph, i)){
			printf("ERROR: in %s, unable to add argSet %u to argSetGraph\n", __func__, i);
			break;
		}
	}

	return 0;
}

static int32_t argSetGraph_add_argSet(struct argSetGraph* arg_set_graph, uint32_t index){
	uint32_t 			i;
	uint32_t 			j;
	uint32_t 			k;
	struct argSet* 		argSet;
	struct argSet* 		argSet_node;
	int32_t 			new_node;
	struct argBuffer* 	argBuffer;
	struct argBuffer* 	argBuffer_node;
	struct argBuffer* 	new_argBuffer;

	argSet = (struct argSet*)array_get(arg_set_graph->arg_array, index);
	new_node = graph_add_node(&(arg_set_graph->graph), (void*)index);
	if (new_node < 0){
		printf("ERROR: in %s, unable to add node to the graph\n", __func__);
		return -1;
	}

	for (i = 0; i < graph_get_nb_node(&(arg_set_graph->graph)); i++){
		if (i != (uint32_t)new_node){
			argSet_node = (struct argSet*)array_get(arg_set_graph->arg_array, (uint32_t)node_get_data(&(arg_set_graph->graph), i));

			for (j = 0; j < array_get_length(argSet_node->input); j++){
				argBuffer_node = (struct argBuffer*)array_get(argSet_node->input, j);

				for(k = 0; k < array_get_length(argSet->output); k++){
					argBuffer = (struct argBuffer*)array_get(argSet->output, k);

					new_argBuffer = argBuffer_compare(argBuffer_node, argBuffer);
					if (new_argBuffer != NULL){
						if (graph_add_edge(&(arg_set_graph->graph), new_node, i, (void*)new_argBuffer) < 0){
							printf("ERROR: in %s, unable to add an edge to the graph\n", __func__);
							argBuffer_delete(new_argBuffer);
						}
					}
				}
			}

			for (j = 0; j < array_get_length(argSet_node->output); j++){
				argBuffer_node = (struct argBuffer*)array_get(argSet_node->output, j);

				for(k = 0; k < array_get_length(argSet->input); k++){
					argBuffer = (struct argBuffer*)array_get(argSet->input, k);

					new_argBuffer = argBuffer_compare(argBuffer_node, argBuffer);
					if (new_argBuffer != NULL){
						if (graph_add_edge(&(arg_set_graph->graph), i, new_node, (void*)new_argBuffer) < 0){
							printf("ERROR: in %s, unable to add an edge to the graph\n", __func__);
							argBuffer_delete(new_argBuffer);
						}
					}
				}
			}
		}
	}

	return 0;
}

int32_t argSetGraph_pack_simple(struct argSetGraph* arg_set_graph, uint32_t dst_node){
	uint32_t 			i;
	int32_t 			src_node;
	int32_t 			src_node_prev;
	struct argSet* 		set_dst;
	struct argSet* 		set_src;
	struct argSet 		result_set;

	if (dst_node >= graph_get_nb_node(&(arg_set_graph->graph))){
		printf("ERROR: in %s, requestrf argSet (index: %u) does not belong to the argumentGraph\n", __func__, dst_node);
		return -1;
	}

	for (i = 0, src_node_prev = -1; i < node_get_nb_dst(&(arg_set_graph->graph), dst_node); i++){
		src_node = edge_get_src(&(arg_set_graph->graph), node_get_dst_edge(&(arg_set_graph->graph), dst_node, i));
		if (src_node != src_node_prev){
			set_dst = (struct argSet*)array_get(arg_set_graph->arg_array, (uint32_t)node_get_data(&(arg_set_graph->graph), dst_node));
			set_src = (struct argSet*)array_get(arg_set_graph->arg_array, (uint32_t)node_get_data(&(arg_set_graph->graph), src_node));

			if (argSet_combine(set_dst, &set_src, 1, &result_set)){
				printf("ERROR: in %s, unable to combine argSet %u with argSet %u\n", __func__, dst_node, src_node);
			}
			else{
				if (array_add(arg_set_graph->arg_array, &result_set) < 0){
					printf("ERROR: in %s unable to add argSet to array\n", __func__);
				}
			}
			src_node_prev = src_node;
		}
	}

	return 0;
}

static void argSetGraph_node_print_dot(void* node_data, FILE* file, void* arg){
	struct array* 	arg_array = (struct array*)arg;
	struct argSet* 	arg_set = (struct argSet*)array_get(arg_array, (uint32_t)node_data);

	fprintf(file, "[label=<Tag: %s<BR/>Nb I: %u<BR/>Nb O: %u>]", arg_set->tag, array_get_length(arg_set->input), array_get_length(arg_set->output));
}

#pragma GCC diagnostic ignored "-Wunused-parameter"
static void argSetGraph_edge_print_dot(void* edge_data, FILE* file, void* arg){
	struct argBuffer* 	argBuffer = (struct argBuffer*)edge_data;

	if (argBuffer->location_type == ARG_LOCATION_MEMORY){
		#if defined ARCH_32
		fprintf(file, "[label=<Mem: 0x%08x<BR/>Size: %u>]", argBuffer->location.address, argBuffer->size);
		#elif defined ARCH_64
		#pragma GCC diagnostic ignored "-Wformat" /* ISO C90 does not support the ‘ll’ gnu_printf length modifier */
		fprintf(file, "[label=<Mem: 0x%llx<BR/>Size: %u>]", argBuffer->location.address, argBuffer->size);
		#else
		#error Please specify an architecture {ARCH_32 or ARCH_64}
		#endif
	}
	else if (argBuffer->location_type == ARG_LOCATION_REGISTER){
		/* a completer */
	}
	else{
		printf("ERROR: in %s, incorrect location type in argBuffer\n", __func__);
	}
}

void argSetGraph_clean(struct argSetGraph* arg_set_graph){
	graph_clean(&(arg_set_graph->graph));
}

void argSetGraph_delete(struct argSetGraph* arg_set_graph){
	if (arg_set_graph != NULL){
		argSetGraph_clean(arg_set_graph);
		free(arg_set_graph);
	}
}