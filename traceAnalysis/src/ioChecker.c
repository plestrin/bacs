#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "ioChecker.h"
#include "primitiveReference.h"
#include "MD5.h"
#include "TEA.h"
#include "RC4.h"


void ioChecker_wrapper_md5(void** input, uint8_t nb_input, void** output, uint8_t nb_output);
void ioChecker_wrapper_tea_encipher(void** input, uint8_t nb_input, void** output, uint8_t nb_output);
void ioChecker_wrapper_tea_decipher(void** input, uint8_t nb_input, void** output, uint8_t nb_output);
void ioChecker_wrapper_rc4(void** input, uint8_t nb_input, void** output, uint8_t nb_output);

static int32_t ioChecker_ckeck(struct ioChecker* checker, uint8_t nb_input, uint8_t nb_output, struct argBuffer* input, struct argBuffer* output);

struct ioChecker* ioChecker_create(){
	struct ioChecker* checker;

	checker = (struct ioChecker*)malloc(sizeof(struct ioChecker));
	if (checker != NULL){
		if (ioChecker_init(checker)){
			free(checker);
			checker = NULL;
		}
	}

	return checker;
}

int32_t ioChecker_init(struct ioChecker* checker){
	int32_t 					result = -1;
	struct primitiveReference 	primitive;

	if (array_init(&(checker->reference_array), sizeof(struct primitiveReference))){
		printf("ERROR: in %s, unable to create array structure\n", __func__);
		return result;
	}

	/* MD5 hash */
	{
		uint32_t input_specifier[2];
		uint32_t output_specifier[1];

		PRIMITIVEREFERENCE_ARG_SPECIFIER_SET_SIZE_MULTIPLE(input_specifier[0], MD5_BLOCK_NB_BYTE);
		PRIMITIVEREFERENCE_ARG_SPECIFIER_SET_IMPLICIT_SIZE(input_specifier[1], 0);
		PRIMITIVEREFERENCE_ARG_SPECIFIER_SET_SIZE_EXACT_VALUE(output_specifier[0], MD5_HASH_NB_BYTE);

		if (primitiveReference_init(&primitive, "MD5", 2, 1, input_specifier, output_specifier, ioChecker_wrapper_md5)){
			printf("ERROR: in %s, unable to init primitive reference structure\n", __func__);
		}
		else{
			if (array_add(&(checker->reference_array), &primitive) < 0){
				printf("ERROR: in %s, unable to add element to array\n", __func__);
			}
		}
	}

	/* TEA cipher */
	{
		uint32_t input_specifier[3];
		uint32_t output_specifier[1];

		PRIMITIVEREFERENCE_ARG_SPECIFIER_SET_SIZE_MULTIPLE(input_specifier[0], TEA_BLOCK_NB_BYTE);
		PRIMITIVEREFERENCE_ARG_SPECIFIER_SET_IMPLICIT_SIZE(input_specifier[1], 0);
		PRIMITIVEREFERENCE_ARG_SPECIFIER_SET_SIZE_EXACT_VALUE(input_specifier[2], TEA_KEY_NB_BYTE);
		PRIMITIVEREFERENCE_ARG_SPECIFIER_SET_SIZE_OF_INPUT_ARG(output_specifier[0], 0);

		if (primitiveReference_init(&primitive, "TEA encypher", 3, 1, input_specifier, output_specifier, ioChecker_wrapper_tea_encipher)){
			printf("ERROR: in %s, unable to init primitive reference structure\n", __func__);
		}
		else{
			if (array_add(&(checker->reference_array), &primitive) < 0){
				printf("ERROR: in %s, unable to add element to array\n", __func__);
			}
		}
	}
	{
		uint32_t input_specifier[3];
		uint32_t output_specifier[1];

		PRIMITIVEREFERENCE_ARG_SPECIFIER_SET_SIZE_MULTIPLE(input_specifier[0], TEA_BLOCK_NB_BYTE);
		PRIMITIVEREFERENCE_ARG_SPECIFIER_SET_IMPLICIT_SIZE(input_specifier[1], 0);
		PRIMITIVEREFERENCE_ARG_SPECIFIER_SET_SIZE_EXACT_VALUE(input_specifier[2], TEA_KEY_NB_BYTE);
		PRIMITIVEREFERENCE_ARG_SPECIFIER_SET_SIZE_OF_INPUT_ARG(output_specifier[0], 0);

		if (primitiveReference_init(&primitive, "TEA decypher", 3, 1, input_specifier, output_specifier, ioChecker_wrapper_tea_decipher)){
			printf("ERROR: in %s, unable to init primitive reference structure\n", __func__);
		}
		else{
			if (array_add(&(checker->reference_array), &primitive) < 0){
				printf("ERROR: in %s, unable to add element to array\n", __func__);
			}
		}
	}

	/* RC4 cipher */
	{
		uint32_t input_specifier[4];
		uint32_t output_specifier[1];

		PRIMITIVEREFERENCE_ARG_SPECIFIER_SET_SIZE_UNDEFINED(input_specifier[0]);
		PRIMITIVEREFERENCE_ARG_SPECIFIER_SET_IMPLICIT_SIZE(input_specifier[1], 0);
		PRIMITIVEREFERENCE_ARG_SPECIFIER_SET_SIZE_MAX(input_specifier[2], RC4_KEY_MAX_NB_BYTE);
		PRIMITIVEREFERENCE_ARG_SPECIFIER_SET_IMPLICIT_SIZE(input_specifier[3], 2);
		PRIMITIVEREFERENCE_ARG_SPECIFIER_SET_SIZE_OF_INPUT_ARG(output_specifier[0], 0);

		if (primitiveReference_init(&primitive, "RC4", 4, 1, input_specifier, output_specifier, ioChecker_wrapper_rc4)){
			printf("ERROR: in %s, unable to init primitive reference structure\n", __func__);
		}
		else{
			if (array_add(&(checker->reference_array), &primitive) < 0){
				printf("ERROR: in %s, unable to add element to array\n", __func__);
			}
		}
	}

	/* Do not foget to update PRIMITIVEREFERENCE_MAX_NB_EXPLICIT_INPUT and PRIMITIVEREFERENCE_MAX_NB_EXPLICIT_OUTPUT */

	result = 0;

	return result;
}

void ioChecker_submit_arguments(struct ioChecker* checker, struct array* input_args, struct array* output_args){
	uint8_t 			nb_input;
	uint8_t 			nb_output;
	uint8_t 			i;
	uint8_t 			j;
	uint8_t 			k;
	uint8_t 			l;
	uint8_t 			m;
	uint8_t 			n;
	struct argBuffer* 	arg_in;
	struct argBuffer* 	arg_out;
	uint8_t* 			current_input;
	uint8_t* 			current_output;
	uint8_t 			input_next;
	uint8_t 			output_next;

	nb_input = (uint8_t)array_get_length(input_args);
	nb_output = (uint8_t)array_get_length(output_args);

	arg_in = (struct argBuffer*)malloc(sizeof(struct argBuffer) * nb_input);
	arg_out = (struct argBuffer*)malloc(sizeof(struct argBuffer) * nb_output);

	current_input = (uint8_t*)malloc(nb_input);
	current_output = (uint8_t*)malloc(nb_output);

	if (arg_in == NULL || arg_out == NULL || current_input == NULL || current_output == NULL){
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
		if (arg_in != NULL){
			free(arg_in);
		}
		if (arg_out != NULL){
			free(arg_out);
		}
		if (current_input == NULL){
			free(current_input);
		}
		if (current_output == NULL){
			free(current_output);
		}
		return;
	}

	/* Generate input sub set */
	for (i = 1; i <= ((nb_input > PRIMITIVEREFERENCE_MAX_NB_EXPLICIT_INPUT) ? PRIMITIVEREFERENCE_MAX_NB_EXPLICIT_INPUT : nb_input); i++){
		memset(current_input, 0, i);
		input_next = 1;

		while(input_next){

			/* Generate the output sub set */
			for (j = 1; j <= ((nb_output > PRIMITIVEREFERENCE_MAX_NB_EXPLICIT_OUTPUT) ? PRIMITIVEREFERENCE_MAX_NB_EXPLICIT_OUTPUT : nb_output); j++){
				memset(current_output, 0, j);
				output_next = 1;

				while(output_next){
					for (k = 0; k < i; k++){
						memcpy(arg_in + k, array_get(input_args, current_input[k]), sizeof(struct argBuffer));
					}
					for (l = 0; l < j; l++){
						memcpy(arg_out + l, array_get(output_args, current_output[l]), sizeof(struct argBuffer));
					}
					ioChecker_ckeck(checker, i, j, arg_in, arg_out);

					for (n = 0, output_next = 0; n < j; n++){
						output_next |= (current_output[n] != nb_output - 1);
					}

					if (output_next){
						l = j -1;
						while(current_output[l] == nb_output - 1){
							l--;
						}
						current_output[l] ++;
						for (n = l + 1; n < j; n++){
							current_output[n] = 0;
						}
					}
				}
			}

			for (m = 0, input_next = 0; m < i; m++){
				input_next |= (current_input[m] != nb_input - 1);
			}

			if (input_next){
				k = i -1;
				while(current_input[k] == nb_input - 1){
					k--;
				}
				current_input[k] ++;
				for (m = k + 1; m < i; m++){
					current_input[m] = 0;
				}
			}
		}
	}

	free(arg_in);
	free(arg_out);
	free(current_input);
	free(current_output);
}

static int32_t ioChecker_ckeck(struct ioChecker* checker, uint8_t nb_input, uint8_t nb_output, struct argBuffer* input, struct argBuffer* output){
	uint32_t i;
	int32_t result = -1;

	for (i = 0; i < array_get_length(&(checker->reference_array)); i++){
		if (!primitiveReference_test((struct primitiveReference*)array_get(&(checker->reference_array), i), nb_input, nb_output, input, output)){
			result = 0;
			break;
		}
	}

	return result;
}

void ioChecker_clean(struct ioChecker* checker){
	uint32_t 					i;
	struct primitiveReference* 	primitive;

	for (i = 0; i < array_get_length(&(checker->reference_array)); i++){
		primitive = (struct primitiveReference*)array_get(&(checker->reference_array), i);
		primitiveReference_clean(primitive);
	}

	array_clean(&(checker->reference_array));
}

void ioChecker_delete(struct ioChecker* checker){
	if (checker != NULL){
		ioChecker_clean(checker);
		free(checker);
	}
}

/* ===================================================================== */
/* Wrapper crypto function(s) 	                                         */
/* ===================================================================== */

void ioChecker_wrapper_md5(void** input, uint8_t nb_input, void** output, uint8_t nb_output){
	uint64_t size;

	if (nb_input != 2 || nb_output != 1){
		printf("ERROR: in %s, incorrect argument(s) to call md5\n", __func__);
	}
	else{
		size = *(uint32_t*)input[1];
		md5((uint32_t*)input[0], size, (uint32_t*)output[0]);
	}
}

void ioChecker_wrapper_tea_encipher(void** input, uint8_t nb_input, void** output, uint8_t nb_output){
	uint64_t size;

	if (nb_input != 3 || nb_output != 1){
		printf("ERROR: in %s, incorrect argument(s) to call tea encypher\n", __func__);
	}
	else{
		size = *(uint32_t*)input[1];
		tea_encipher((uint32_t*)input[0], size, (uint32_t*)input[2], (uint32_t*)output[0]);
	}
}

void ioChecker_wrapper_tea_decipher(void** input, uint8_t nb_input, void** output, uint8_t nb_output){
	uint64_t size;

	if (nb_input != 3 || nb_output != 1){
		printf("ERROR: in %s, incorrect argument(s) to call tea decypher\n", __func__);
	}
	else{
		size = *(uint32_t*)input[1];
		tea_decipher((uint32_t*)input[0], size, (uint32_t*)input[2], (uint32_t*)output[0]);
	}
}

void ioChecker_wrapper_rc4(void** input, uint8_t nb_input, void** output, uint8_t nb_output){
	uint64_t input_length;
	uint8_t key_length;

	if (nb_input != 4 || nb_output != 1){
		printf("ERROR: in %s, incorrect argument(s) to call rc4\n", __func__);
	}
	else{
		input_length = *(uint32_t*)input[1];
		key_length = *(uint32_t*)input[3];
		rc4((uint8_t*)input[0], input_length, (uint8_t*)input[2], key_length, (uint8_t*)output[0]);
	}
}