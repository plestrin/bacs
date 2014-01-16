#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "argSet.h"

void argSet_fragment_input(struct argSet* arg_set, struct array* array){
	uint32_t 			nb_input;
	uint32_t**			table_store;
	uint32_t*			nb_element_store;
	uint32_t			i;
	uint32_t			j;
	uint32_t			k;
	struct argSet 		new_set;
	struct argBuffer* 	argBuffer;
	struct argBuffer 	new_argBuffer;
	uint32_t 			nb_fragment = 0;
	uint32_t			local_index;
	uint32_t 			global_index;

	nb_input = array_get_length(arg_set->input);

	table_store = (uint32_t**)malloc(sizeof(uint32_t*) * nb_input);
	if (table_store == NULL){
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
		return;
	}

	nb_element_store = (uint32_t*)malloc(sizeof(uint32_t) * nb_input);
	if (nb_element_store == NULL){
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
		free(table_store);
		return;
	}

	for (i = 0; i < nb_input; i++){
		argBuffer_create_fragment_table((struct argBuffer*)array_get(arg_set->input, i), table_store + i, nb_element_store + i);
		if (table_store[i] != NULL && nb_element_store[i] != 0){
			if (nb_fragment == 0){
				nb_fragment = (0x00000001 << (nb_element_store[i] - 1)) - 1;
			}
			else{
				nb_fragment = nb_fragment * ((0x00000001 << (nb_element_store[i] - 1)) - 1);
			}
		}
	}

	if (nb_fragment != 0){
		#ifdef VERBOSE
		printf("Creating %u fragment argument(s)\n", nb_fragment);
		#endif

		for (i = 0; i < nb_fragment; i++){
			global_index = i;

			snprintf(new_set.tag, ARGSET_TAG_MAX_LENGTH, "frag %u: %s", i, arg_set->tag);

			new_set.input = array_create(sizeof(struct argBuffer));
			if (new_set.input == NULL){
				printf("ERROR: in %s, unable to create array\n", __func__);
				break;
			}

			new_set.output = (struct array*)malloc(sizeof(struct array));
			if (new_set.output == NULL){
				printf("ERROR: in %s, unable to allocate memory\n", __func__);
				break;
			}

			for (j = 0; j < nb_input; j++){
				if (table_store[j] != NULL && nb_element_store[j] != 0){
					local_index = global_index % ((0x00000001 << (nb_element_store[j] - 1)) - 1);
					global_index = global_index / ((0x00000001 << (nb_element_store[j] - 1)) - 1);
					argBuffer = (struct argBuffer*)array_get(arg_set->input, j);

					new_argBuffer.location_type = argBuffer->location_type;
					new_argBuffer.access_size = argBuffer->access_size;

					for (k = 0; k < nb_element_store[j]; k++){
						if (table_store[j][local_index * (2 * nb_element_store[j]) + 2*k + 1] != 0){
							new_argBuffer.size = table_store[j][local_index * (2 * nb_element_store[j]) + 2*k + 1] * new_argBuffer.access_size;
							if (new_argBuffer.location_type == ARG_LOCATION_MEMORY){
								new_argBuffer.location.address = argBuffer->location.address + table_store[j][local_index * (2 * nb_element_store[j]) + 2*k] * new_argBuffer.access_size;
							}
							else if (new_argBuffer.location_type == ARG_LOCATION_REGISTER){
								/* a completer */
							}
							else{
								printf("ERROR: in %s, incorrect location type in argBuffer\n", __func__);
							}

							new_argBuffer.data = (char*)malloc(new_argBuffer.size);
							if (new_argBuffer.data == NULL){
								printf("ERROR: in %s, unable to allocate memory\n", __func__);
								continue;
							}
							memcpy(new_argBuffer.data, argBuffer->data + table_store[j][local_index * (2 * nb_element_store[j]) + 2*k] * new_argBuffer.access_size, new_argBuffer.size);

							if (array_add(new_set.input, &new_argBuffer) < 0){
								printf("ERROR: in %s, unable to add argBuffer in input array\n", __func__);
								free(new_argBuffer.data);
							}
						}
					}
				}
				else{
					if (argBuffer_clone((struct argBuffer*)array_get(arg_set->input, j), &new_argBuffer)){
						printf("ERROR: in %s, unable to clone argBuffer\n", __func__);
					}
					else{
						if (array_add(new_set.input, &new_argBuffer) < 0){
							printf("ERROR: in %s, unable to add argBuffer to array\n", __func__);
						}
					}
				}
			}

			if (argBuffer_clone_array(arg_set->output, new_set.output)){
				printf("ERROR: in %s, unable to clone output argBuffer array\n", __func__);
				argBuffer_delete_array(new_set.input);
				break;
			}

			if (array_add(array, &new_set) < 0){
				printf("ERROR: in %s, unable to add argSet to array\n", __func__);
				break;
			}
		}

		for (i = 0;  i < nb_input; i++){
			if (table_store[i] != NULL){
				free(table_store[i]);
			}
		}
	}

	free(table_store);
	free(nb_element_store);

	return;
}

int32_t argSet_combine(struct argSet* set_dst, struct argSet** set_src, uint32_t nb_set_src, struct argSet* set_result){
	uint32_t 			i;
	uint32_t 			j;
	uint32_t 			k;
	uint32_t 			print_length;
	struct array* 		input = NULL;
	struct array*		output = NULL;
	struct argBuffer*	arg_src;
	struct argBuffer* 	arg_dst;
	struct argBuffer 	new_arg;

	print_length = snprintf(set_result->tag, ARGSET_TAG_MAX_LENGTH, "Pack: %s <- ", set_dst->tag);
	for (i = 0; i < nb_set_src && print_length < ARGSET_TAG_MAX_LENGTH; i++){
		if (i == nb_set_src -1){
			print_length += snprintf(set_result->tag + print_length, ARGSET_TAG_MAX_LENGTH - print_length, "%s", set_src[i]->tag);
		}
		else{
			print_length += snprintf(set_result->tag + print_length, ARGSET_TAG_MAX_LENGTH - print_length, "%s ", set_src[i]->tag);
		}
	}

	input = (struct array*)malloc(sizeof(struct array));
	output = (struct array*)malloc(sizeof(struct array));

	if (input == NULL || output == NULL){
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
		goto error;
	}

	if (argBuffer_clone_array(set_dst->output, output)){
		printf("ERROR: in %s, unable to clone dst output arguemnts\n", __func__);
		goto error;
	}

	if (argBuffer_clone_array(set_dst->input, input)){
		printf("ERROR: in %s, unable to clone dst input arguemnts\n", __func__);
		goto error;
	}

	for (i = 0; i < nb_set_src; i++){

		/* INPUT */
		for (j = 0; j < array_get_length(set_src[i]->input); j++){
			arg_src = (struct argBuffer*)array_get(set_src[i]->input, j);

			for (k = 0; k < array_get_length(input); k++){
				arg_dst = (struct argBuffer*)array_get(input, k);

				if (!argBuffer_equal(arg_src, arg_dst)){
					break;
				}
			}
			if (k == array_get_length(input)){
				if (argBuffer_clone(arg_src, &new_arg)){
					printf("ERROR: in %s, unable to clone argBuffer\n", __func__);
				}
				else{
					if (array_add(input, &new_arg) < 0){
						printf("ERROR: in %s, unable to add argBuffer to array\n", __func__);
					}
				}
			}
		}

		/* OUTPUT */
		for (j = 0; j < array_get_length(set_src[i]->output); j++){
			arg_src = (struct argBuffer*)array_get(set_src[i]->output, j);

			for (k = 0; k < array_get_length(output); k++){
				arg_dst = (struct argBuffer*)array_get(output, k);

				/* not necessary if the merge routine (below) is more complete */
				if (!argBuffer_equal(arg_src, arg_dst)){
					break;
				}

				if (!argBuffer_try_merge(arg_dst, arg_src)){
					break;
				}
			}
			if (k == array_get_length(output)){
				if (argBuffer_clone(arg_src, &new_arg)){
					printf("ERROR: in %s, unable to clone argBuffer\n", __func__);
				}
				else{
					if (array_add(output, &new_arg) < 0){
						printf("ERROR: in %s, unable to add argBuffer to array\n", __func__);
					}
				}
			}
		}
	}

	set_result->input = input;
	set_result->output = output;

	return 0;

	error:

	if (input != NULL){
		free(input);
	}
	if (output != NULL){
		free(output);
	}

	return -1;
}