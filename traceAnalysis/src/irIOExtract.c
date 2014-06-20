#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "irIOExtract.h"
#include "permutation.h"

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

static uint8_t argCluster_create_opcode_seq(struct node* node, enum irOpcode* seq);
static int32_t argCluster_compare_opcode_seq(enum irOpcode* seq1, uint8_t nb_opcode1, enum irOpcode* seq2, uint8_t nb_opcode2);
static void argCluster_split_mem_base(struct array* cluster_array, uint32_t index);
static void argCluster_create_adjacent_arg_strict(struct trace* trace, struct argCluster* cluster, struct argSet* set);
static void argCluster_create_adjacent_arg_max(struct trace* trace, struct argCluster* cluster, struct argSet* set);
static void argCluster_brute_force_small(struct trace* trace, struct argCluster* cluster, struct argSet* set);

int32_t argCluster_compare_address(struct node** node1, struct node** node2);

#define argCluster_get_size(cluster) 		(array_get_length(&((cluster)->node_array)))
#define argCluster_add_node(cluster, node) 	(array_add(&((cluster)->node_array), (node)))
#define argCluster_get_node(cluster, index) (array_get(&((cluster)->node_array), (index)))
#define argCluster_clean(cluster) 			(array_clean(&((cluster)->node_array)))

static uint8_t argCluster_create_opcode_seq(struct node* node, enum irOpcode* seq){
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

static int32_t argCluster_compare_opcode_seq(enum irOpcode* seq1, uint8_t nb_opcode1, enum irOpcode* seq2, uint8_t nb_opcode2){
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

static void argCluster_split_mem_base(struct array* cluster_array, uint32_t index){
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

static void argCluster_create_adjacent_arg_strict(struct trace* trace, struct argCluster* cluster, struct argSet* set){
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

static void argCluster_create_adjacent_arg_max(struct trace* trace, struct argCluster* cluster, struct argSet* set){
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

static void argCluster_brute_force_small(struct trace* trace, struct argCluster* cluster, struct argSet* set){
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
		
		PERMUTATION_CREATE(nb_operand, malloc)

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
		PERMUTATION_DELETE()
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