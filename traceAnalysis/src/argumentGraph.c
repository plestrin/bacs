#include <stdlib.h>
#include <stdio.h>

#include "argumentGraph.h"

int32_t argumentGraph_add_argument(struct graph* graph, struct argument* argument){
	uint32_t 			i;
	uint32_t 			j;
	uint32_t 			k;
	struct argument* 	argument_node;
	int32_t 			new_node;
	struct argBuffer* 	argBuffer;
	struct argBuffer* 	argBuffer_node;
	struct argBuffer* 	new_argBuffer;

	new_node = graph_add_node(graph, (void*)argument);
	if (new_node < 0){
		printf("ERROR: in %s, unable to add node to the graph\n", __func__);
		return -1;
	}

	for (i = 0; i < graph_get_nb_node(graph); i++){
		if (i != (uint32_t)new_node){
			argument_node = (struct argument*)node_get_data(graph, i);

			for (j = 0; j < array_get_length(argument_node->input); j++){
				argBuffer_node = (struct argBuffer*)array_get(argument_node->input, j);

				for(k = 0; k < array_get_length(argument->output); k++){
					argBuffer = (struct argBuffer*)array_get(argument->output, k);

					new_argBuffer = argBuffer_compare(argBuffer_node, argBuffer);
					if (new_argBuffer != NULL){
						if (graph_add_edge(graph, new_node, i, (void*)new_argBuffer) < 0){
							printf("ERROR: in %s, unable to add an edge to the graph\n", __func__);
							argBuffer_delete(new_argBuffer);
						}
					}
				}
			}

			for (j = 0; j < array_get_length(argument_node->output); j++){
				argBuffer_node = (struct argBuffer*)array_get(argument_node->output, j);

				for(k = 0; k < array_get_length(argument->input); k++){
					argBuffer = (struct argBuffer*)array_get(argument->input, k);

					new_argBuffer = argBuffer_compare(argBuffer_node, argBuffer);
					if (new_argBuffer != NULL){
						if (graph_add_edge(graph, i, new_node, (void*)new_argBuffer) < 0){
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

void argumentGraph_node_print_dot(void* node_data, FILE* file){
	struct argument* argument = (struct argument*)node_data;
	fprintf(file, "[label=<Tag: %s<BR/>Nb I: %u<BR/>Nb O: %u>]", argument->tag, array_get_length(argument->input), array_get_length(argument->output));
}

void argumentGraph_edge_print_dot(void* edge_data, FILE* file){
	struct argBuffer* argBuffer = (struct argBuffer*)edge_data;

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