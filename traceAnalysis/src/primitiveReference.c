#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dlfcn.h>

#include "primitiveReference.h"
#include "printBuffer.h"

int32_t primitiveReference_init(struct primitiveReference* primitive, char* name, uint8_t nb_input, uint8_t nb_output, uint32_t* input_specifier, uint32_t* output_specifier, char* lib_name, char* func_name){
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
		return -1;
	}

	strncpy(primitive->name, name, PRIMITIVEREFERENCE_MAX_NAME_SIZE);
	primitive->nb_input = nb_input;
	primitive->nb_output = nb_output;

	primitive->input_specifier = (uint32_t*)malloc(nb_input * sizeof(uint32_t));
	if (primitive->input_specifier == NULL){
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
		return -1;
	}
	memcpy(primitive->input_specifier, input_specifier, nb_input * sizeof(uint32_t));

	primitive->output_specifier = (uint32_t*)malloc(nb_output * sizeof(uint32_t));
	if (primitive->output_specifier == NULL){
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
		free(primitive->input_specifier);
		return -1;
	}
	memcpy(primitive->output_specifier, output_specifier, nb_output * sizeof(uint32_t));

	primitive->lib_handle = dlopen(lib_name, RTLD_LAZY);
	if (primitive->lib_handle == NULL){
		printf("ERROR: in %s, unable to load lib: \"%s\"\n", __func__, lib_name);
		free(primitive->input_specifier);
		free(primitive->output_specifier);
		return -1;
	}

	#pragma GCC diagnostic ignored "-Wpedantic" /* ISO C forbids conversion of object pointer to function pointer type */
	primitive->func = (void(*)(void**, void**))dlsym(primitive->lib_handle, func_name);
	if (primitive->func == NULL){
		printf("ERROR: in %s, unable to get function (\"%s\") address\n", __func__, func_name);
		dlclose(primitive->lib_handle);
		free(primitive->input_specifier);
		free(primitive->output_specifier);
		return -1;
	}



	return 0;
}

int32_t primitiveReference_test(struct primitiveReference* primitive, uint8_t nb_input, struct argBuffer* input, struct argSet* set){
	int32_t 			result = -1;
	uint8_t 			i;
	uint8_t 			j;
	void** 				arg_in;
	void** 				arg_out;
	uint32_t*			arg_out_size;

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
	arg_out_size = (uint32_t*)alloca(primitive->nb_output * sizeof(uint32_t));

	memset(arg_out, 0, primitive->nb_output * sizeof(void*));

	for (i = 0; i < primitive->nb_output; i++){
		if (PRIMITIVEREFERENCE_ARG_SPECIFIER_IS_SIZE_EXACT_VALUE(primitive->output_specifier[i])){
			arg_out_size[i] = PRIMITIVEREFERENCE_ARG_SPECIFIER_GET_SIZE_EXACT_VALUE(primitive->output_specifier[i]);
			arg_out[i] = malloc(arg_out_size[i]);
			if (arg_out[i] == NULL){
				printf("ERROR: in %s, unable to allocate memory\n", __func__);
				goto exit;
			}
		}
		else if (PRIMITIVEREFERENCE_ARG_SPECIFIER_IS_SIZE_OF_INPUT_ARG(primitive->output_specifier[i])){
			if (PRIMITIVEREFERENCE_ARG_SPECIFIER_GET_INPUT_INDEX(primitive->output_specifier[i]) < nb_input){
				arg_out_size[i] = input[PRIMITIVEREFERENCE_ARG_SPECIFIER_GET_INPUT_INDEX(primitive->output_specifier[i])].size;
				arg_out[i] = malloc(arg_out_size[i]);
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
			printf("ERROR: in %s, this case is not suppose to happen\n", __func__);
			goto exit;
		}	
	}

	primitive->func(arg_in, arg_out);

	for (j = 0, result = 0; j < primitive->nb_output; j++){
		if (argSet_search_output(set, arg_out[j], arg_out_size[j]) < 0){
			result = 1;
		}
	}

	if (!result){
		printf("\n*** IO match for %s ****\n", primitive->name);
		for (j = 0; j < nb_input; j++){
			printf("\tArg %u in:  ", j);
			printBuffer_raw(stdout, input[j].data, input[j].size);
			printf("\n");
		}
		for (j = 0; j < primitive->nb_output; j++){
			printf("\tArg %u out: ", j);
			printBuffer_raw(stdout, arg_out[j], arg_out_size[j]);
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

void primitiveReference_snprint_inputs(struct primitiveReference* primitive, char* buffer, uint32_t buffer_length){
	uint8_t 	i;
	uint32_t 	offset = 0;

	if (primitive->nb_input ==0 && buffer_length > 0){
		buffer[0] = '\0';
	}

	for (i = 0; i < primitive->nb_input; i++){
		if (PRIMITIVEREFERENCE_ARG_SPECIFIER_IS_SIZE_EXACT_VALUE(primitive->input_specifier[i])){
			offset += snprintf(buffer + offset, buffer_length - offset, "(exp %u: size %u)", i, PRIMITIVEREFERENCE_ARG_SPECIFIER_GET_SIZE_EXACT_VALUE(primitive->input_specifier[i]));
		}
		else if (PRIMITIVEREFERENCE_ARG_SPECIFIER_IS_SIZE_MULTIPLE(primitive->input_specifier[i])){
			offset += snprintf(buffer + offset, buffer_length - offset, "(exp %u: size multiple of %u)", i, PRIMITIVEREFERENCE_ARG_SPECIFIER_GET_SIZE_MULTIPLE(primitive->input_specifier[i]));
		}
		else if (PRIMITIVEREFERENCE_ARG_SPECIFIER_IS_SIZE_UNDEFINED(primitive->input_specifier[i])){
			offset += snprintf(buffer + offset, buffer_length - offset, "(exp %u: undefined size)", i);
		}
		else if (PRIMITIVEREFERENCE_ARG_SPECIFIER_IS_IMPLICIT_SIZE(primitive->input_specifier[i])){
			offset += snprintf(buffer + offset, buffer_length - offset, "(imp %u: size of input %u)", i, PRIMITIVEREFERENCE_ARG_SPECIFIER_GET_INPUT_INDEX(primitive->input_specifier[i]));
		}
		else if (PRIMITIVEREFERENCE_ARG_SPECIFIER_IS_SIZE_MAX(primitive->input_specifier[i])){
			offset += snprintf(buffer + offset, buffer_length - offset, "(exp %u: max size %u)", i, PRIMITIVEREFERENCE_ARG_SPECIFIER_GET_SIZE_MAX(primitive->input_specifier[i]));
		}
		else{
			printf("ERROR: in %s, this case is not suppose to happen\n", __func__);
		}
		if (offset >= buffer_length){
			break;
		}
	}
}

void primitiveReference_snprint_outputs(struct primitiveReference* primitive, char* buffer, uint32_t buffer_length){
	uint8_t 	i;
	uint32_t 	offset = 0;

	if (primitive->nb_output ==0 && buffer_length > 0){
		buffer[0] = '\0';
	}
		
	for (i = 0; i < primitive->nb_output; i++){
		if (PRIMITIVEREFERENCE_ARG_SPECIFIER_IS_SIZE_EXACT_VALUE(primitive->output_specifier[i])){
			offset += snprintf(buffer + offset, buffer_length - offset, "(exp %u: size %u)", i, PRIMITIVEREFERENCE_ARG_SPECIFIER_GET_SIZE_EXACT_VALUE(primitive->output_specifier[i]));
		}
		else if (PRIMITIVEREFERENCE_ARG_SPECIFIER_IS_SIZE_OF_INPUT_ARG(primitive->output_specifier[i])){
			offset += snprintf(buffer + offset, buffer_length - offset, "(exp %u: size of input %u)", i, PRIMITIVEREFERENCE_ARG_SPECIFIER_GET_INPUT_INDEX(primitive->output_specifier[i]));
		}
		else{
			printf("ERROR: in %s, this case is not suppose to happen\n", __func__);
		}
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

	if (dlclose(primitive->lib_handle)){
		printf("ERROR: in %s, unable to unload lib\n", __func__);
	}
}
