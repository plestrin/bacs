#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "ir.h"
#include "irImporterDynTrace.h"
#include "array.h"
#include "permutation.h"
#include "multiColumn.h"

void ir_dotPrint_node(void* data, FILE* file);
void ir_dotPrint_edge(void* data, FILE* file);

struct ir* ir_create(struct trace* trace){
	struct ir* ir;

	ir =(struct ir*)malloc(sizeof(struct ir));
	if (ir != NULL){
		if(ir_init(ir, trace)){
			printf("ERROR: in %s, unable to init ir\n", __func__);
			free(ir);
			ir = NULL;
		}
	}
	else{
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
	}

	return ir;
}

int32_t ir_init(struct ir* ir, struct trace* trace){
	ir->trace 				= trace;
	graph_init(&(ir->graph), sizeof(struct irOperation), sizeof(struct irDependence))
	graph_register_dotPrint_callback(&(ir->graph), ir_dotPrint_node, ir_dotPrint_edge)
	ir->input_linkedList 	= NULL;
	ir->output_linkedList 	= NULL;

	if (irImporterDynTrace_import(ir)){
		printf("ERROR: in %s, trace import has failed\n", __func__);
		return -1;
	}
	
	return 0;
}

struct node* ir_add_input(struct ir* ir, struct operand* operand){
	struct node* 			node;
	struct irOperation* 	operation;

	node = graph_add_node_(&(ir->graph));
	if (node == NULL){
		printf("ERROR: in %s, unable to add node to the graph\n", __func__);
	}
	else{
		operation = ir_node_get_operation(node);
		operation->type 							= IR_OPERATION_TYPE_INPUT;
		operation->operation_type.input.operand 	= operand;
		operation->operation_type.input.prev 		= NULL;
		operation->operation_type.input.next 		= ir->input_linkedList;

		if (ir->input_linkedList != NULL){
			ir_node_get_operation(ir->input_linkedList)->operation_type.input.prev = node;
		}

		ir->input_linkedList = node;
	}

	return node;
}

struct node* ir_add_output(struct ir* ir, enum irOpcode opcode, struct operand* operand){
	struct node* 			node;
	struct irOperation* 	operation;

	node = graph_add_node_(&(ir->graph));
	if (node == NULL){
		printf("ERROR: in %s, unable to add node to the graph\n", __func__);
	}
	else{
		operation = ir_node_get_operation(node);
		operation->type 							= IR_OPERATION_TYPE_OUTPUT;
		operation->operation_type.output.opcode 	= opcode;
		operation->operation_type.output.operand 	= operand;
		operation->operation_type.output.prev 		= NULL;
		operation->operation_type.output.next 		= ir->output_linkedList;

		if (ir->output_linkedList != NULL){
			ir_node_get_operation(ir->output_linkedList)->operation_type.output.prev = node;
		}

		ir->output_linkedList = node;
	}

	return node;
}

struct edge* ir_add_dependence(struct ir* ir, struct node* operation_src, struct node* operation_dst, enum irDependenceType type){
	struct edge* 			edge;
	struct irDependence* 	dependence;

	edge = graph_add_edge_(&(ir->graph), operation_src, operation_dst);
	if (edge == NULL){
		printf("ERROR: in %s, unable to add edge to the graph\n", __func__);
	}
	else{
		dependence = ir_edge_get_dependence(edge);
		dependence->type = type;
	}

	return edge;
}

void ir_convert_output_to_inner(struct ir* ir, struct node* node){
	enum irOpcode 			opcode;
	struct irOperation* 	operation;

	operation = ir_node_get_operation(node);
	if (operation->type == IR_OPERATION_TYPE_OUTPUT){
		opcode = operation->operation_type.output.opcode;
		operation->type = IR_OPERATION_TYPE_INNER;
		operation->operation_type.inner.opcode = opcode;

		if (operation->operation_type.output.prev == NULL){
			ir->output_linkedList = operation->operation_type.output.next;
		}
		else{
			ir_node_get_operation(operation->operation_type.output.prev)->operation_type.output.next = operation->operation_type.output.next;
		}

		if (operation->operation_type.output.next != NULL){
			ir_node_get_operation(operation->operation_type.output.next)->operation_type.output.prev = operation->operation_type.output.prev;
		}
	}
}

void ir_convert_input_to_inner(struct ir* ir, struct node* node, enum irOpcode opcode){
	struct irOperation* operation;

	operation = ir_node_get_operation(node);
	if (operation->type == IR_OPERATION_TYPE_INPUT){
		operation->type = IR_OPERATION_TYPE_INNER;
		operation->operation_type.inner.opcode = opcode;

		if (operation->operation_type.input.prev == NULL){
			ir->output_linkedList = operation->operation_type.input.next;
		}
		else{
			ir_node_get_operation(operation->operation_type.input.prev)->operation_type.input.next = operation->operation_type.input.next;
		}

		if (operation->operation_type.input.next != NULL){
			ir_node_get_operation(operation->operation_type.input.next)->operation_type.input.prev = operation->operation_type.input.prev;
		}
	}
}

/* ===================================================================== */
/* Argument functions						                         	 */
/* ===================================================================== */

#define ARGCLUSTER_MAX_OPCODE_SEQUENCE 	32
#define ARGCLUSTER_MAX_SIZE_BRUTE_FORCE 4

enum argLocationType{
	ARG_LOCATION_MEMORY 		= 0x00000001,
	ARG_LOCATION_REGISTER 		= 0x00000002,
	ARG_LOCATION_MIX 			= 0x00000003
};

struct argCluster{
	enum argLocationType 	location_type;

	enum irOpcode 			opcode_seq[ARGCLUSTER_MAX_OPCODE_SEQUENCE];
	uint8_t 				nb_opcode;

	struct node* 			mem_base;
	uint8_t  				mem_base_set;

	struct array 			node_array;
};

uint8_t argCluster_create_opcode_seq(struct node* node, enum irOpcode* seq);
int32_t argCluster_compare_opcode_seq(enum irOpcode* seq1, uint8_t nb_opcode1, enum irOpcode* seq2, uint8_t nb_opcode2);
void argCluster_split_mem_base(struct array* cluster_array, uint32_t index);
void argCluster_create_adjacent_arg_strict(struct trace* trace, struct argCluster* cluster, struct argSet* set);
void argCluster_create_adjacent_arg_max(struct trace* trace, struct argCluster* cluster, struct argSet* set);
void argCluster_brute_force_small(struct trace* trace, struct argCluster* cluster, struct argSet* set);

int32_t argCluster_compare_address(struct node** node1, struct node** node2);

#define argCluster_get_size(cluster) 		(array_get_length(&((cluster)->node_array)))
#define argCluster_add_node(cluster, node) 	(array_add(&((cluster)->node_array), (node)))
#define argCluster_get_node(cluster, index) (array_get(&((cluster)->node_array), (index)))
#define argCluster_clean(cluster) 			(array_clean(&((cluster)->node_array)))

uint8_t argCluster_create_opcode_seq(struct node* node, enum irOpcode* seq){
	uint8_t 				nb_opcode = 0;
	struct edge* 			edge;
	struct irDependence* 	dependence;
	struct irOperation* 	operation;

	edge = node_get_head_edge_src(node);
	while(edge != NULL){
		dependence = ir_edge_get_dependence(edge);
		if (dependence->type == IR_DEPENDENCE_TYPE_DIRECT){
			if (nb_opcode < ARGCLUSTER_MAX_OPCODE_SEQUENCE){
				operation = ir_node_get_operation(edge->dst_node);
				if (operation->type == IR_OPERATION_TYPE_OUTPUT){
					seq[nb_opcode ++] = operation->operation_type.output.opcode;
				}
				else if (operation->type == IR_OPERATION_TYPE_INNER){
					seq[nb_opcode ++] = operation->operation_type.inner.opcode;
				}
				else{
					printf("ERROR: in %s, incorrect operation type\n", __func__);
				}
			}
			else{
				printf("ERROR: in %s, the ARGCLUSTER_MAX_OPCODE_SEQUENCE has been reached\n", __func__);
				break;
			}
		}

		edge = edge_get_next_src(edge);
	}

	return nb_opcode;
}

int32_t argCluster_compare_opcode_seq(enum irOpcode* seq1, uint8_t nb_opcode1, enum irOpcode* seq2, uint8_t nb_opcode2){
	uint8_t i;
	uint8_t j;

	if (nb_opcode1 != nb_opcode2){
		return -1;
	}

	for (i = 0; i < nb_opcode1; i++){
		for (j = 0; j < nb_opcode2; j++){
			if (seq1[i] == seq2[j]){
				break;
			}
			else if (j == nb_opcode2 - 1){
				return -1;
			}
		}
	}

	return 0;
}

void argCluster_split_mem_base(struct array* cluster_array, uint32_t index){
	uint32_t 				i;
	uint32_t 				j;
	uint32_t 				new_cluster_offset;
	struct node* 			node_ptr;
	struct argCluster* 		cluster;
	struct argCluster* 		cluster_ptr;
	struct argCluster 		new_cluster;
	struct edge* 			edge;
	struct node* 			base_node;
	struct irDependence* 	dependence;

	cluster = (struct argCluster*)array_get(cluster_array, index);
	new_cluster_offset = array_get_length(cluster_array);

	for (i = 0; i < argCluster_get_size(cluster); i++){
		node_ptr = *(struct node**)argCluster_get_node(cluster, i);
		base_node = NULL;

		edge = node_get_head_edge_dst(node_ptr);
		while(edge != NULL){
			dependence = ir_edge_get_dependence(edge);
			if (dependence->type == IR_DEPENDENCE_TYPE_BASE){
				if (base_node == NULL){
					base_node = edge->src_node;
				}
				else{
					printf("WARNING: in %s, multiple base edges\n", __func__);
				}
			}

			edge = edge_get_next_dst(edge);
		}

		for (j = new_cluster_offset; j < array_get_length(cluster_array); j++){
			cluster_ptr = (struct argCluster*)array_get(cluster_array, j);

			if (cluster_ptr->mem_base == base_node){
				if (argCluster_add_node(cluster_ptr, &node_ptr) < 0){
					printf("ERROR: in %s, unable to add node to node_array\n", __func__);
				}
				goto next;
			}
		}

		new_cluster.location_type 	= ARG_LOCATION_MEMORY;
		new_cluster.nb_opcode 		= cluster->nb_opcode;
		new_cluster.mem_base 		= base_node;
		new_cluster.mem_base_set 	= 1;
		memcpy(new_cluster.opcode_seq, cluster->opcode_seq, cluster->nb_opcode * sizeof(enum irOpcode));

		if (array_init(&(new_cluster.node_array), sizeof(struct node*))){
			printf("ERROR: in %s, unable to init array\n", __func__);
			goto next;
		}
		if (argCluster_add_node(&new_cluster, &node_ptr) < 0){
			printf("ERROR: in %s, unable to add node to node_array\n", __func__);
			argCluster_clean(&new_cluster);
			goto next;
		}
		if (array_add(cluster_array, &new_cluster) < 0){
			printf("ERROR: in %s, unable to add argCluster to cluster_array\n", __func__);
			argCluster_clean(&new_cluster);
		}
		else{
			cluster = (struct argCluster*)array_get(cluster_array, index);
		}

		next:;
	}
}

int32_t argCluster_compare_address(struct node** node1, struct node** node2){
	struct irOperation* operation1;
	struct irOperation* operation2;

	operation1 = ir_node_get_operation(*node1);
	operation2 = ir_node_get_operation(*node2);

	if (operation1->operation_type.input.operand->location.address < operation2->operation_type.input.operand->location.address){
		return -1;
	}
	else if (operation1->operation_type.input.operand->location.address > operation2->operation_type.input.operand->location.address){
		return 1;
	}
	else{
		return 0;
	}
}

void argCluster_create_adjacent_arg_strict(struct trace* trace, struct argCluster* cluster, struct argSet* set){
	uint32_t* 				mapping;
	uint32_t 				i;
	struct irOperation* 	operation;
	ADDRESS 				start_address 	= 0;
	uint32_t 				size 			= 0;
	uint32_t 				access_size 	= 0;
	struct inputArgument 	arg;

	mapping = array_create_mapping(&(cluster->node_array), (int32_t(*)(void*,void*))argCluster_compare_address);
	if (mapping == NULL){
		printf("ERROR: in %s, unable to create array mapping\n", __func__);
		return;
	}

	for (i = 0; i < argCluster_get_size(cluster); i++){
		operation = ir_node_get_operation(*(struct node**)argCluster_get_node(cluster, mapping[i]));
		if (i == 0){
			start_address = operation->operation_type.input.operand->location.address;
			size = operation->operation_type.input.operand->size;
			access_size = operation->operation_type.input.operand->size;
		}
		else{
			if (operation->operation_type.input.operand->size != access_size){
				printf("WARNING: in %s, operand access size varies across the cluster -> drop\n", __func__);
				goto quit;
			}

			if (start_address + size < operation->operation_type.input.operand->location.address){
				printf("WARNING: in %s, missing value in the cluster -> drop\n", __func__);
				goto quit;
			}

			if (start_address + (size - access_size) < operation->operation_type.input.operand->location.address && start_address + size != operation->operation_type.input.operand->location.address){
				printf("WARNING: in %s, wrong access alignment in cluster -> drop\n", __func__);
				goto quit;
			}

			if (start_address + size == operation->operation_type.input.operand->location.address){
				size += access_size;
			}
		}
	}

	if (inputArgument_init(&arg, size, 1, access_size)){
		printf("ERROR: in %s, unable to init input argument\n", __func__);
		goto quit;
	}

	arg.desc[0].type 				= ARGFRAG_MEM;
	arg.desc[0].location.address 	= start_address;
	arg.desc[0].size 				= size;

	for (i = 0, size = 0; i < argCluster_get_size(cluster); i++){
		operation = ir_node_get_operation(*(struct node**)argCluster_get_node(cluster, mapping[i]));
		
		if (start_address + size == operation->operation_type.input.operand->location.address){
			memcpy(arg.data + size, trace->data + operation->operation_type.input.operand->data_offset, access_size);
			size += access_size;
		}
	}

	if (argSet_add_input(set, &arg) < 0){
		printf("ERROR: in %s, unable to add element to array structure\n", __func__);
		inputArgument_clean(&arg);
	}

	quit:
	free(mapping);
}

void argCluster_create_adjacent_arg_max(struct trace* trace, struct argCluster* cluster, struct argSet* set){
	uint32_t* 				mapping;
	uint32_t 				i;
	uint32_t 				j;
	uint32_t 				k;
	struct irOperation* 	operation;
	ADDRESS 				start_address 	= 0;
	uint32_t 				size 			= 0;
	uint32_t 				access_size 	= 0;
	struct inputArgument 	arg;
	uint32_t 				copy_size;

	mapping = array_create_mapping(&(cluster->node_array), (int32_t(*)(void*,void*))argCluster_compare_address);
	if (mapping == NULL){
		printf("ERROR: in %s, unable to create array mapping\n", __func__);
		return;
	}

	for (i = 0; i < argCluster_get_size(cluster); i = k){
		operation 		= ir_node_get_operation(*(struct node**)argCluster_get_node(cluster, mapping[i]));
		start_address 	= operation->operation_type.input.operand->location.address;
		size 			= operation->operation_type.input.operand->size;
		access_size 	= ARGUMENT_ACCESS_SIZE_UNDEFINED;

		for (k = i + 1; k < argCluster_get_size(cluster); k++){
			operation = ir_node_get_operation(*(struct node**)argCluster_get_node(cluster, mapping[k]));

			if (start_address + size >= operation->operation_type.input.operand->location.address){
				size += operation->operation_type.input.operand->location.address + operation->operation_type.input.operand->size - (start_address + size);
			}
			else{
				break;
			}
		}

		if (inputArgument_init(&arg, size, 1, access_size)){
			printf("ERROR: in %s, unable to init input argument\n", __func__);
			continue;
		}

		arg.desc[0].type 				= ARGFRAG_MEM;
		arg.desc[0].location.address 	= start_address;
		arg.desc[0].size 				= size;

		for (j = i, size = 0; j < k; j++){
			operation = ir_node_get_operation(*(struct node**)argCluster_get_node(cluster, mapping[j]));
			
			if (operation->operation_type.input.operand->location.address + operation->operation_type.input.operand->size > start_address + size){
				copy_size = operation->operation_type.input.operand->location.address + operation->operation_type.input.operand->size - (start_address + size);

				memcpy(arg.data + size, trace->data + operation->operation_type.input.operand->data_offset + (operation->operation_type.input.operand->size - copy_size), copy_size);
				size += copy_size;
			}
		}

		if (argSet_add_input(set, &arg) < 0){
			printf("ERROR: in %s, unable to add element to array structure\n", __func__);
			inputArgument_clean(&arg);
		}

	}

	free(mapping);
}

void argCluster_brute_force_small(struct trace* trace, struct argCluster* cluster, struct argSet* set){
	uint32_t 				nb_combination;
	uint32_t 				i;
	uint32_t 				j;
	uint32_t 				k;
	uint8_t* 				permutation;
	struct inputArgument 	arg;
	uint32_t 				access_size;
	struct irOperation* 	operation;

	for (i = 0; i < argCluster_get_size(cluster); i++){
		operation = ir_node_get_operation(*(struct node**)argCluster_get_node(cluster, i));

		if (i == 0){
			access_size = operation->operation_type.input.operand->size;
		}
		else{
			if (access_size != operation->operation_type.input.operand->size){
				printf("WARNING: in %s, operand access size varies across the cluster -> drop\n", __func__);
				return;
			}
		}
	}

	nb_combination = 0x00000001 << argCluster_get_size(cluster);
	for (i = 1; i < nb_combination; i++){
		uint8_t nb_operand = __builtin_popcount(i);
		
		PERMUTATION_INIT(nb_operand)

		PERMUTATION_GET_FIRST(permutation)
		while(permutation != NULL){
			if (inputArgument_init(&arg, nb_operand * access_size, nb_operand, access_size)){
				printf("ERROR: in %s, unable to init input argument\n", __func__);
				break;
			}

			for (j = 0, k = 0; j < argCluster_get_size(cluster); j++){
				if ((i >> j) & 0x00000001){
					operation = ir_node_get_operation(*(struct node**)argCluster_get_node(cluster, j));
					inputArgument_set_operand(&arg, permutation[k], permutation[k]*access_size, operation->operation_type.input.operand, (char*)(trace->data + operation->operation_type.input.operand->data_offset));
					k++;
				}
			}

			if (argSet_add_input(set, &arg) < 0){
				printf("ERROR: in %s, unable to add element to array structure\n", __func__);
				inputArgument_clean(&arg);
			}

			PERMUTATION_GET_NEXT(permutation)
		}
		PERMUTATION_CLEAN()
	}
}

void ir_extract_arg(struct ir* ir, struct argSet* set){
	struct node* 			node_cursor;
	struct irOperation* 	operation_cursor;
	struct array 			cluster_array;
	uint32_t 				i;
	struct argCluster 		new_cluster;
	struct argCluster* 		cluster_ptr;
	enum irOpcode* 			opcode_seq_ptr = new_cluster.opcode_seq;
	uint8_t 				nb_opcode;

	/* INPUT */
	if (array_init(&cluster_array, sizeof(struct argCluster))){
		printf("ERROR: in %s, unable to init array\n", __func__);
		return;
	}

	node_cursor = ir->input_linkedList;
	while(node_cursor != NULL){
		nb_opcode = argCluster_create_opcode_seq(node_cursor, opcode_seq_ptr);
		operation_cursor = ir_node_get_operation(node_cursor);

		for (i = 0; i < array_get_length(&cluster_array); i++){
			cluster_ptr = (struct argCluster*)array_get(&cluster_array, i);
			if (!argCluster_compare_opcode_seq(cluster_ptr->opcode_seq, cluster_ptr->nb_opcode, opcode_seq_ptr, nb_opcode)){
				if (argCluster_add_node(cluster_ptr, &node_cursor) < 0){
					printf("ERROR: in %s, unable to add node to node_array\n", __func__);
				}
				else{
					if (cluster_ptr->location_type == ARG_LOCATION_MEMORY && OPERAND_IS_REG(*(operation_cursor->operation_type.input.operand))){
						cluster_ptr->location_type = ARG_LOCATION_MIX;
					}
					if (cluster_ptr->location_type == ARG_LOCATION_REGISTER && OPERAND_IS_MEM(*(operation_cursor->operation_type.input.operand))){
						cluster_ptr->location_type = ARG_LOCATION_MIX;
					}
				}

				goto next_input;
			}
		}

		if (OPERAND_IS_MEM(*(operation_cursor->operation_type.input.operand))){
			new_cluster.location_type = ARG_LOCATION_MEMORY;
		}
		else if (OPERAND_IS_REG(*(operation_cursor->operation_type.input.operand))){
			new_cluster.location_type = ARG_LOCATION_REGISTER;
		}
		else{
			printf("ERROR: in %s, incorrect operand type\n", __func__);
			goto next_input;
		}

		new_cluster.nb_opcode 		= nb_opcode;
		new_cluster.mem_base 		= NULL;
		new_cluster.mem_base_set 	= 0;

		if (array_init(&(new_cluster.node_array), sizeof(struct node*))){
			printf("ERROR: in %s, unable to init array\n", __func__);
			goto next_input;
		}
		if (argCluster_add_node(&new_cluster, &node_cursor) < 0){
			printf("ERROR: in %s, unable to add node to node_array\n", __func__);
			argCluster_clean(&new_cluster);
			goto next_input;
		}
		if (array_add(&cluster_array, &new_cluster) < 0){
			printf("ERROR: in %s, unable to add argCluster to cluster_array\n", __func__);
			argCluster_clean(&new_cluster);
		}

		next_input:
		node_cursor = operation_cursor->operation_type.input.next;
	}

	for (i = 0; i < array_get_length(&cluster_array); i++){
		cluster_ptr = (struct argCluster*)array_get(&cluster_array, i);
		if (argCluster_get_size(cluster_ptr) > ARGCLUSTER_MAX_SIZE_BRUTE_FORCE){
			if (cluster_ptr->location_type == ARG_LOCATION_MEMORY){
				if (cluster_ptr->mem_base_set){
					argCluster_create_adjacent_arg_strict(ir->trace, cluster_ptr, set);
				}
				else{
					argCluster_create_adjacent_arg_max(ir->trace, cluster_ptr, set);
					argCluster_split_mem_base(&cluster_array, i);
				}
			}
			else{
				printf("ERROR: in %s, I don't know how to deal with this case (large MIX)\n", __func__);
			}
		}
		else{
			argCluster_brute_force_small(ir->trace, cluster_ptr, set);
		}

		argCluster_clean((struct argCluster*)array_get(&cluster_array, i));
	}

	array_clean(&cluster_array);

	/* OUTPUT */
	node_cursor = ir->output_linkedList;
	while(node_cursor != NULL){
		operation_cursor = ir_node_get_operation(node_cursor);
		if (argSet_add_output(set, operation_cursor->operation_type.output.operand, ir->trace->data + operation_cursor->operation_type.output.operand->data_offset)){
			printf("ERROR: in %s, unable to add output argument to argSet\n", __func__);
		}

		node_cursor = operation_cursor->operation_type.output.next;
	}
}

/* ===================================================================== */
/* Printing functions						                             */
/* ===================================================================== */

void ir_dotPrint_node(void* data, FILE* file){
	struct irOperation* operation = (struct irOperation*)data;

	switch(operation->type){
		case IR_OPERATION_TYPE_INPUT 		: {
			if (OPERAND_IS_MEM(*(operation->operation_type.input.operand))){
				#if defined ARCH_32
				fprintf(file, "[shape=\"box\",label=\"@%08x\"]", operation->operation_type.input.operand->location.address);
				#elif defined ARCH_64
				#pragma GCC diagnostic ignored "-Wformat" /* ISO C90 does not support the ‘ll’ gnu_printf length modifier */
				fprintf(file, "[shape=\"box\",label=\"@%llx\"]", operation->operation_type.input.operand->location.address);
				#else
				#error Please specify an architecture {ARCH_32 or ARCH_64}
				#endif
			}
			else if (OPERAND_IS_REG(*(operation->operation_type.input.operand))){
				fprintf(file, "[shape=\"box\",label=\"%s\"]", reg_2_string(operation->operation_type.input.operand->location.reg));
			}
			else{
				printf("ERROR: in %s, unexpected data type (REG or MEM)\n", __func__);
				break;
			}
			break;
		}
		case IR_OPERATION_TYPE_OUTPUT 		: {
			fprintf(file, "[shape=\"invhouse\",label=\"%s\"]", irOpcode_2_string(operation->operation_type.output.opcode));
			break;
		}
		case IR_OPERATION_TYPE_INNER 		: {
			fprintf(file, "[label=\"%s\"]", irOpcode_2_string(operation->operation_type.inner.opcode));
			break;
		}
	}
}

void ir_dotPrint_edge(void* data, FILE* file){
	struct irDependence* dependence = (struct irDependence*)data;

	switch(dependence->type){
		case IR_DEPENDENCE_TYPE_DIRECT 	: {
			break;
		}
		case IR_DEPENDENCE_TYPE_BASE 	: {
			fprintf(file, "[label=\"base\"]");
			break;
		}
		case IR_DEPENDENCE_TYPE_INDEX 	: {
			fprintf(file, "[label=\"index\"]");
			break;
		}
	}
}

char* irOpcode_2_string(enum irOpcode opcode){
	switch(opcode){
		case IR_ADD 	: {return "add";}
		case IR_AND 	: {return "and";}
		case IR_BSWAP 	: {return "bswap";}
		case IR_DEC 	: {return "dec";}
		case IR_MOVZX 	: {return "movzx";}
		case IR_NOT 	: {return "not";}
		case IR_OR 		: {return "or";}
		case IR_PART 	: {return "part";}
		case IR_ROR 	: {return "ror";}
		case IR_SAR 	: {return "sar";}
		case IR_SHL 	: {return "shl";}
		case IR_SHR 	: {return "shr";}
		case IR_SUB 	: {return "sub";}
		case IR_XOR 	: {return "xor";}
	}

	return NULL;
}

void ir_print_io(struct ir* ir){
	struct node* 				node_cursor;
	struct irOperation* 		operation_cursor;
	struct multiColumnPrinter* 	printer;
	char 						value_str[20];
	char 						desc_str[20];

	printer = multiColumnPrinter_create(stdout, 3, NULL, NULL, NULL);
	if (printer != NULL){
		multiColumnPrinter_set_column_type(printer, 2, MULTICOLUMN_TYPE_UINT32);

		multiColumnPrinter_set_title(printer, 0, (char*)"VALUE");
		multiColumnPrinter_set_title(printer, 1, (char*)"DESC");
		multiColumnPrinter_set_title(printer, 2, (char*)"SIZE");

		printf("*** INPUT ***\n");
		multiColumnPrinter_print_header(printer);
		node_cursor = ir->input_linkedList;
		while(node_cursor != NULL){
			operation_cursor = ir_node_get_operation(node_cursor);

			switch(operation_cursor->operation_type.input.operand->size){
			case 1 	: {snprintf(value_str, 20, "%02x", *(uint32_t*)(ir->trace->data + operation_cursor->operation_type.input.operand->data_offset) & 0x000000ff); break;}
			case 2 	: {snprintf(value_str, 20, "%04x", *(uint32_t*)(ir->trace->data + operation_cursor->operation_type.input.operand->data_offset) & 0x0000ffff); break;}
			case 4 	: {snprintf(value_str, 20, "%08x", *(uint32_t*)(ir->trace->data + operation_cursor->operation_type.input.operand->data_offset) & 0xffffffff); break;}
			default : {printf("WARNING: in %s, unexpected data size\n", __func__); break;}
			}

			if (OPERAND_IS_MEM(*(operation_cursor->operation_type.input.operand))){
				#if defined ARCH_32
				snprintf(desc_str, 20, "0x%08x", operation_cursor->operation_type.input.operand->location.address);
				#elif defined ARCH_64
				#pragma GCC diagnostic ignored "-Wformat" /* ISO C90 does not support the ‘ll’ gnu_printf length modifier */
				snprintf(desc_str, 20, "0x%llx", operation_cursor->operation_type.input.operand->location.address);
				#else
				#error Please specify an architecture {ARCH_32 or ARCH_64}
				#endif
			}
			else if (OPERAND_IS_REG(*(operation_cursor->operation_type.input.operand))){
				snprintf(desc_str, 20, "%s", reg_2_string(operation_cursor->operation_type.input.operand->location.reg));
			}
			else{
				printf("WARNING: in %s, unexpected operand type\n", __func__);
			}

			multiColumnPrinter_print(printer, value_str, desc_str, operation_cursor->operation_type.input.operand->size, NULL);

			node_cursor = operation_cursor->operation_type.input.next;
		}

		printf("\n*** OUTPUT ***\n");
		multiColumnPrinter_print_header(printer);
		node_cursor = ir->output_linkedList;
		while(node_cursor != NULL){
			operation_cursor = ir_node_get_operation(node_cursor);

			switch(operation_cursor->operation_type.output.operand->size){
			case 1 	: {snprintf(value_str, 20, "%02x", *(uint32_t*)(ir->trace->data + operation_cursor->operation_type.output.operand->data_offset) & 0x000000ff); break;}
			case 2 	: {snprintf(value_str, 20, "%04x", *(uint32_t*)(ir->trace->data + operation_cursor->operation_type.output.operand->data_offset) & 0x0000ffff); break;}
			case 4 	: {snprintf(value_str, 20, "%08x", *(uint32_t*)(ir->trace->data + operation_cursor->operation_type.output.operand->data_offset) & 0xffffffff); break;}
			default : {printf("WARNING: in %s, unexpected data size\n", __func__); break;}
			}

			if (OPERAND_IS_MEM(*(operation_cursor->operation_type.output.operand))){
				#if defined ARCH_32
				snprintf(desc_str, 20, "0x%08x", operation_cursor->operation_type.output.operand->location.address);
				#elif defined ARCH_64
				#pragma GCC diagnostic ignored "-Wformat" /* ISO C90 does not support the ‘ll’ gnu_printf length modifier */
				snprintf(desc_str, 20, "0x%llx", operation_cursor->operation_type.output.operand->location.address);
				#else
				#error Please specify an architecture {ARCH_32 or ARCH_64}
				#endif
			}
			else if (OPERAND_IS_REG(*(operation_cursor->operation_type.output.operand))){
				snprintf(desc_str, 20, "%s", reg_2_string(operation_cursor->operation_type.output.operand->location.reg));
			}
			else{
				printf("WARNING: in %s, unexpected operand type\n", __func__);
			}

			multiColumnPrinter_print(printer, value_str, desc_str, operation_cursor->operation_type.output.operand->size, NULL);
			
			node_cursor = operation_cursor->operation_type.output.next;
		}

		multiColumnPrinter_delete(printer);
	}
	else{
		printf("ERROR: in %s, unable to create multi column printer\n", __func__);
	}
}