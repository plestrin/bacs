#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "primitiveReference.h"
#include "printBuffer.h"

int32_t primitiveReference_init(struct primitiveReference* primitive, char* name, uint8_t nb_input, uint8_t nb_output, uint32_t* input_specifier, uint32_t* output_specifier, void(*func)(void** input, uint8_t nb_input, void** output, uint8_t nb_output)){
	int32_t result = -1;

	if (nb_input < 1){
		printf("WARNING: in %s, at least one input argument is expected to perform a match\n", __func__);
		nb_input = 1;
	}

	if (nb_output < 1){
		printf("WARNING: in %s, at least one output argument is expected to perform a match\n", __func__);
		nb_output = 1;
	}

	if (input_specifier == NULL || output_specifier == NULL){
		printf("ERROR: in %s, specifier is NULL pointer\n", __func__);
		return result;
	}

	strncpy(primitive->name, name, PRIMITIVEREFERENCE_MAX_NAME_SIZE);
	primitive->nb_input = nb_input;
	primitive->nb_output = nb_output;

	primitive->input_specifier = (uint32_t*)malloc(nb_input * sizeof(uint32_t));
	if (primitive->input_specifier == NULL){
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
		return result;
	}
	memcpy(primitive->input_specifier, input_specifier, nb_input * sizeof(uint32_t));

	primitive->output_specifier = (uint32_t*)malloc(nb_output * sizeof(uint32_t));
	if (primitive->output_specifier == NULL){
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
		free(primitive->input_specifier);
		return result;
	}
	memcpy(primitive->output_specifier, output_specifier, nb_output * sizeof(uint32_t));

	primitive->func = func;
	result = 0;

	return result;
}

int32_t primitiveReference_test(struct primitiveReference* primitive, uint8_t nb_input, uint8_t nb_output, struct argBuffer* input, struct argBuffer* output){
	uint32_t 	result = -1;
	uint8_t 	i;
	uint8_t 	j;
	void** 		arg_in;
	void** 		arg_out;

	/* There is no implicit output - for faster test*/
	if (primitive->nb_output != nb_output){
		return result;
	}

	arg_in = (void**)alloca(primitive->nb_input * sizeof(void*));

	for (i = 0, j = 0; i < primitive->nb_input; i++){
		if (PRIMITIVEREFERENCE_ARG_SPECIFIER_IS_SIZE_EXACT_VALUE(primitive->input_specifier[i])){
			if (j < nb_input){
				if (input[j].size == PRIMITIVEREFERENCE_ARG_SPECIFIER_GET_SIZE_EXACT_VALUE(primitive->input_specifier[i])){
					arg_in[i] = (void*)input[j].data;
					j ++;
				}
				else{
					return result;
				}
			}
			else{
				return result;
			}
		}
		else if (PRIMITIVEREFERENCE_ARG_SPECIFIER_IS_SIZE_MULTIPLE(primitive->input_specifier[i])){
			if (j < nb_input){
				if (input[j].size % PRIMITIVEREFERENCE_ARG_SPECIFIER_GET_SIZE_MULTIPLE(primitive->input_specifier[i]) == 0){
					arg_in[i] = (void*)input[j].data;
					j ++;
				}
				else{
					return result;
				}
			}
			else{
				return result;
			}

		}
		else if (PRIMITIVEREFERENCE_ARG_SPECIFIER_IS_SIZE_UNDEFINED(primitive->input_specifier[i])){
			if (j < nb_input){
				arg_in[i] = (void*)input[j].data;
				j ++;
			}
			else{
				return result;
			}
		}
		else if (PRIMITIVEREFERENCE_ARG_SPECIFIER_IS_IMPLICIT_SIZE(primitive->input_specifier[i])){
			if (PRIMITIVEREFERENCE_ARG_SPECIFIER_GET_INPUT_INDEX(primitive->input_specifier[i]) < nb_input){
				arg_in[i] = (void*)&(input[PRIMITIVEREFERENCE_ARG_SPECIFIER_GET_INPUT_INDEX(primitive->input_specifier[i])].size);
			}
			else{
				return result;
			}
		}
		else if (PRIMITIVEREFERENCE_ARG_SPECIFIER_IS_SIZE_MAX(primitive->input_specifier[i])){
			if (j < nb_input){
				if (input[j].size <= PRIMITIVEREFERENCE_ARG_SPECIFIER_GET_SIZE_MAX(primitive->input_specifier[i])){
					arg_in[i] = (void*)input[j].data;
					j ++;
				}
				else{
					return result;
				}
			}
			else{
				return result;
			}
		}
		else{
			printf("ERROR: in %s, this case is not suppose to happen\n", __func__);
			return result;
		}	
	}

	if (j != nb_input || i != primitive->nb_input){
		return result;
	}

	arg_out = (void**)alloca(primitive->nb_output * sizeof(void*));

	memset(arg_out, 0, primitive->nb_output * sizeof(void*));

	for (i = 0; i < primitive->nb_output; i++){
		if (PRIMITIVEREFERENCE_ARG_SPECIFIER_IS_SIZE_EXACT_VALUE(primitive->output_specifier[i])){
			if (output[i].size == PRIMITIVEREFERENCE_ARG_SPECIFIER_GET_SIZE_EXACT_VALUE(primitive->output_specifier[i])){
				arg_out[i] = malloc(output[i].size);
				if (arg_out[i] == NULL){
					printf("ERROR: in %s, unable to allocate memory\n", __func__);
					goto exit;
				}
			}
			else{
				goto exit;
			}
		}
		else if (PRIMITIVEREFERENCE_ARG_SPECIFIER_IS_SIZE_OF_INPUT_ARG(primitive->output_specifier[i])){
			if (PRIMITIVEREFERENCE_ARG_SPECIFIER_GET_INPUT_INDEX(primitive->output_specifier[i]) < nb_input){
				if (output[i].size == input[PRIMITIVEREFERENCE_ARG_SPECIFIER_GET_INPUT_INDEX(primitive->output_specifier[i])].size){
					arg_out[i] = malloc(output[i].size);
					if (arg_out[i] == NULL){
						printf("ERROR: in %s, unable to allocate memory\n", __func__);
						goto exit;
					}
				}
				else{
					goto exit;
				}
			}
			else{
				goto exit;
			}
		}
		else{
			printf("ERROR: in %s, this case is not suppose to happen\n", __func__);
			goto exit;
		}	
	}

	primitive->func(arg_in, primitive->nb_input, arg_out, primitive->nb_output);

	for (j = 0, result = 0; j < nb_output; j++){
		result |= memcmp(arg_out[j], output[j].data, output[j].size);
	}

	if (!result){
		printf("*** IO match for %s ****\n", primitive->name);
		for (j = 0; j < nb_input; j++){
			printf("\tArg %u in:  ", j);
			printBuffer_raw(stdout, input[j].data, input[j].size);
			printf("\n");
		}
		for (j = 0; j < nb_output; j++){
			printf("\tArg %u out: ", j);
			printBuffer_raw(stdout, output[j].data, output[j].size);
			printf("\n");
		}
	}

	exit:
	for (i = 0; i < primitive->nb_output; i++){
		if (arg_out[i] != NULL){
			free(arg_out[i]);
		}
	}

	return result;
}

void primitiveReference_print(struct primitiveReference* primitive){
	uint8_t i;

	if (primitive != NULL){
		printf("Primitive reference %s: ", primitive->name);
		printf("input(s) -> {");
		for (i = 0; i < primitive->nb_input; i++){
			if (PRIMITIVEREFERENCE_ARG_SPECIFIER_IS_SIZE_EXACT_VALUE(primitive->input_specifier[i])){
				printf("(explicit %u: size %u)", i, PRIMITIVEREFERENCE_ARG_SPECIFIER_GET_SIZE_EXACT_VALUE(primitive->input_specifier[i]));
			}
			else if (PRIMITIVEREFERENCE_ARG_SPECIFIER_IS_SIZE_MULTIPLE(primitive->input_specifier[i])){
				printf("(explicit %u: size multiple of %u)", i, PRIMITIVEREFERENCE_ARG_SPECIFIER_GET_SIZE_MULTIPLE(primitive->input_specifier[i]));
			}
			else if (PRIMITIVEREFERENCE_ARG_SPECIFIER_IS_SIZE_UNDEFINED(primitive->input_specifier[i])){
				printf("(explicit %u: undefined size)", i);
			}
			else if (PRIMITIVEREFERENCE_ARG_SPECIFIER_IS_IMPLICIT_SIZE(primitive->input_specifier[i])){
				printf("(implicit %u: size of input %u)", i, PRIMITIVEREFERENCE_ARG_SPECIFIER_GET_INPUT_INDEX(primitive->input_specifier[i]));
			}
			else if (PRIMITIVEREFERENCE_ARG_SPECIFIER_IS_SIZE_MAX(primitive->input_specifier[i])){
				printf("(explicit %u: max size %u)", i, PRIMITIVEREFERENCE_ARG_SPECIFIER_GET_SIZE_MAX(primitive->input_specifier[i]));
			}
			else{
				printf("ERROR: in %s, this case is not suppose to happen\n", __func__);
			}
		}
		printf("}, output(s) -> {");
		for (i = 0; i < primitive->nb_output; i++){
			if (PRIMITIVEREFERENCE_ARG_SPECIFIER_IS_SIZE_EXACT_VALUE(primitive->output_specifier[i])){
				printf("(explicit %u: size %u)", i, PRIMITIVEREFERENCE_ARG_SPECIFIER_GET_SIZE_EXACT_VALUE(primitive->output_specifier[i]));
			}
			else if (PRIMITIVEREFERENCE_ARG_SPECIFIER_IS_SIZE_OF_INPUT_ARG(primitive->output_specifier[i])){
				printf("(explicit %u: size of input %u)", i, PRIMITIVEREFERENCE_ARG_SPECIFIER_GET_INPUT_INDEX(primitive->output_specifier[i]));
			}
			else{
				printf("ERROR: in %s, this case is not suppose to happen\n", __func__);
			}
		}
		printf("}\n");
	}
}

uint8_t primitiveReference_get_nb_explicit_input(struct primitiveReference* primitive){
	uint8_t result = 0;
	uint8_t i;

	if (primitive != NULL){
		for (i = 0; i < primitive->nb_input; i++){
			if (PRIMITIVEREFERENCE_ARG_SPECIFIER_IS_SIZE_EXACT_VALUE(primitive->input_specifier[i]) ||
				PRIMITIVEREFERENCE_ARG_SPECIFIER_IS_SIZE_MULTIPLE(primitive->input_specifier[i]) 	||
				PRIMITIVEREFERENCE_ARG_SPECIFIER_IS_SIZE_UNDEFINED(primitive->input_specifier[i]) 	||
				PRIMITIVEREFERENCE_ARG_SPECIFIER_IS_SIZE_MAX(primitive->input_specifier[i])){
				result ++;
			}
		}
	}

	return result;
}

uint8_t primitiveReference_get_nb_explicit_output(struct primitiveReference* primitive){
	uint8_t result = 0;
	uint8_t i;

	if (primitive != NULL){
		for (i = 0; i < primitive->nb_output; i++){
			if (PRIMITIVEREFERENCE_ARG_SPECIFIER_IS_SIZE_EXACT_VALUE(primitive->output_specifier[i]) ||
				PRIMITIVEREFERENCE_ARG_SPECIFIER_IS_SIZE_OF_INPUT_ARG(primitive->output_specifier[i])){
				result ++;
			}
		}
	}

	return result;
}

void primitiveReference_clean(struct primitiveReference* primitive){
	if (primitive->input_specifier != NULL){
		free(primitive->input_specifier);
	}
	if (primitive->output_specifier != NULL){
		free(primitive->output_specifier);
	}
}
