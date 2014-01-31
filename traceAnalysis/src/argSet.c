#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "argSet.h"


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

void argSet_clean(struct argSet* set){
	uint32_t 			i;
	struct argBuffer* 	arg;

	if (set->input != NULL){
		for (i = 0; i < array_get_length(set->input); i++){
			arg = (struct argBuffer*)array_get(set->input, i);
			free(arg->data);
		}
		array_delete(set->input);
		set->input = NULL;
	}

	if (set->output != NULL){
		for (i = 0; i < array_get_length(set->output); i++){
			arg = (struct argBuffer*)array_get(set->output, i);
			free(arg->data);
		}
		array_delete(set->output);
		set->output = NULL;
	}
}