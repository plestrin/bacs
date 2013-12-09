#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "ioChecker.h"
#include "primitiveReference.h"
#include "MD5.h"
#include "TEA.h"
#include "RC4.h"
#include "AES.h"


void ioChecker_wrapper_md5(void** input, void** output);
void ioChecker_wrapper_tea_encipher(void** input, void** output);
void ioChecker_wrapper_tea_decipher(void** input, void** output);
void ioChecker_wrapper_rc4(void** input, void** output);
void ioChecker_wrapper_aes128_key_expand_encrypt(void** input, void** output);
void ioChecker_wrapper_aes128_key_expand_decrypt(void** input, void** output);
void ioChecker_wrapper_aes128_encrypt(void** input, void** output);
void ioChecker_wrapper_aes128_decrypt(void** input, void** output);

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
	uint32_t 					i;
	struct primitiveReference* 	primitive_pointer;
	uint8_t 					nb_explicit_input;
	uint8_t 					nb_explicit_output;

	if (array_init(&(checker->reference_array), sizeof(struct primitiveReference))){
		printf("ERROR: in %s, unable to create array structure\n", __func__);
		return result;
	}

	#define IOCHECKER_ADD_PRIMITIVE_REFRENCE(name, function)																																		\
	if (primitiveReference_init(&primitive, (name), sizeof(input_specifier) / sizeof(uint32_t), sizeof(output_specifier) / sizeof(uint32_t), input_specifier, output_specifier, (function))){		\
		printf("ERROR: in %s, unable to init primitive reference structure\n", __func__);																											\
	}																																																\
	else{																																															\
		if (array_add(&(checker->reference_array), &primitive) < 0){																																\
			printf("ERROR: in %s, unable to add element to array\n", __func__);																														\
		}																																															\
	}

	/* MD5 hash */
	{
		uint32_t input_specifier[2];
		uint32_t output_specifier[1];

		PRIMITIVEREFERENCE_ARG_SPECIFIER_SET_SIZE_MULTIPLE(input_specifier[0], MD5_BLOCK_NB_BYTE);
		PRIMITIVEREFERENCE_ARG_SPECIFIER_SET_IMPLICIT_SIZE(input_specifier[1], 0);
		PRIMITIVEREFERENCE_ARG_SPECIFIER_SET_SIZE_EXACT_VALUE(output_specifier[0], MD5_HASH_NB_BYTE);

		IOCHECKER_ADD_PRIMITIVE_REFRENCE("MD5", ioChecker_wrapper_md5)
	}

	/* TEA cipher */
	{
		uint32_t input_specifier[3];
		uint32_t output_specifier[1];

		PRIMITIVEREFERENCE_ARG_SPECIFIER_SET_SIZE_MULTIPLE(input_specifier[0], TEA_BLOCK_NB_BYTE);
		PRIMITIVEREFERENCE_ARG_SPECIFIER_SET_IMPLICIT_SIZE(input_specifier[1], 0);
		PRIMITIVEREFERENCE_ARG_SPECIFIER_SET_SIZE_EXACT_VALUE(input_specifier[2], TEA_KEY_NB_BYTE);
		PRIMITIVEREFERENCE_ARG_SPECIFIER_SET_SIZE_OF_INPUT_ARG(output_specifier[0], 0);

		IOCHECKER_ADD_PRIMITIVE_REFRENCE("TEA encrypt", ioChecker_wrapper_tea_encipher)
	}
	{
		uint32_t input_specifier[3];
		uint32_t output_specifier[1];

		PRIMITIVEREFERENCE_ARG_SPECIFIER_SET_SIZE_MULTIPLE(input_specifier[0], TEA_BLOCK_NB_BYTE);
		PRIMITIVEREFERENCE_ARG_SPECIFIER_SET_IMPLICIT_SIZE(input_specifier[1], 0);
		PRIMITIVEREFERENCE_ARG_SPECIFIER_SET_SIZE_EXACT_VALUE(input_specifier[2], TEA_KEY_NB_BYTE);
		PRIMITIVEREFERENCE_ARG_SPECIFIER_SET_SIZE_OF_INPUT_ARG(output_specifier[0], 0);

		IOCHECKER_ADD_PRIMITIVE_REFRENCE("TEA decrypt", ioChecker_wrapper_tea_decipher)
	}

	/* RC4 cipher */
	{
		uint32_t input_specifier[4];
		uint32_t output_specifier[1];

		PRIMITIVEREFERENCE_ARG_SPECIFIER_SET_SIZE_UNDEFINED(input_specifier[0]);
		PRIMITIVEREFERENCE_ARG_SPECIFIER_SET_IMPLICIT_SIZE(input_specifier[1], 0);
		PRIMITIVEREFERENCE_ARG_SPECIFIER_SET_SIZE_MAX(input_specifier[2], RC4_KEY_MAX_NB_BYTE);
		PRIMITIVEREFERENCE_ARG_SPECIFIER_SET_IMPLICIT_SIZE(input_specifier[3], 1);
		PRIMITIVEREFERENCE_ARG_SPECIFIER_SET_SIZE_OF_INPUT_ARG(output_specifier[0], 0);

		IOCHECKER_ADD_PRIMITIVE_REFRENCE("RC4", ioChecker_wrapper_rc4)
	}

	/* AES 128 */
	{
		uint32_t input_specifier[1];
		uint32_t output_specifier[1];

		PRIMITIVEREFERENCE_ARG_SPECIFIER_SET_SIZE_EXACT_VALUE(input_specifier[0], AES_128_NB_BYTE_KEY);
		PRIMITIVEREFERENCE_ARG_SPECIFIER_SET_SIZE_EXACT_VALUE(output_specifier[0], AES_128_NB_BYTE_ROUND_KEY);

		IOCHECKER_ADD_PRIMITIVE_REFRENCE("AES128 key expand encrypt", ioChecker_wrapper_aes128_key_expand_encrypt)
	}
	{
		uint32_t input_specifier[1];
		uint32_t output_specifier[1];

		PRIMITIVEREFERENCE_ARG_SPECIFIER_SET_SIZE_EXACT_VALUE(input_specifier[0], AES_128_NB_BYTE_KEY);
		PRIMITIVEREFERENCE_ARG_SPECIFIER_SET_SIZE_EXACT_VALUE(output_specifier[0], AES_128_NB_BYTE_ROUND_KEY);

		IOCHECKER_ADD_PRIMITIVE_REFRENCE("AES128 key expand decrypt", ioChecker_wrapper_aes128_key_expand_decrypt)
	}
	{
		uint32_t input_specifier[2];
		uint32_t output_specifier[1];

		PRIMITIVEREFERENCE_ARG_SPECIFIER_SET_SIZE_EXACT_VALUE(input_specifier[0], AES_BLOCK_NB_BYTE);
		PRIMITIVEREFERENCE_ARG_SPECIFIER_SET_SIZE_EXACT_VALUE(input_specifier[1], AES_128_NB_BYTE_ROUND_KEY);
		PRIMITIVEREFERENCE_ARG_SPECIFIER_SET_SIZE_EXACT_VALUE(output_specifier[0], AES_BLOCK_NB_BYTE);

		IOCHECKER_ADD_PRIMITIVE_REFRENCE("AES128 encrypt", ioChecker_wrapper_aes128_encrypt)
	}
	{
		uint32_t input_specifier[2];
		uint32_t output_specifier[1];

		PRIMITIVEREFERENCE_ARG_SPECIFIER_SET_SIZE_EXACT_VALUE(input_specifier[0], AES_BLOCK_NB_BYTE);
		PRIMITIVEREFERENCE_ARG_SPECIFIER_SET_SIZE_EXACT_VALUE(input_specifier[1], AES_128_NB_BYTE_ROUND_KEY);
		PRIMITIVEREFERENCE_ARG_SPECIFIER_SET_SIZE_EXACT_VALUE(output_specifier[0], AES_BLOCK_NB_BYTE);

		IOCHECKER_ADD_PRIMITIVE_REFRENCE("AES128 key expand decrypt", ioChecker_wrapper_aes128_decrypt)
	}

	checker->max_nb_input = 0;
	checker->max_nb_output = 0;

	for (i = 0; i < array_get_length(&(checker->reference_array)); i++){
		primitive_pointer = (struct primitiveReference*)array_get(&(checker->reference_array), i);
		if (primitive_pointer != NULL){
			nb_explicit_input = primitiveReference_get_nb_explicit_input(primitive_pointer);
			nb_explicit_output = primitiveReference_get_nb_explicit_output(primitive_pointer);

			checker->max_nb_input = (checker->max_nb_input < nb_explicit_input) ? nb_explicit_input : checker->max_nb_input;
			checker->max_nb_output = (checker->max_nb_output < nb_explicit_output) ? nb_explicit_output : checker->max_nb_output;
		}
		else{
			printf("ERROR: in %s, array_get returns a NULL pointer\n", __func__);
		}
	}

	#ifdef VERBOSE
	printf("Create IOChecker: {");
	for (i = 0; i < array_get_length(&(checker->reference_array)); i++){
		primitive_pointer = (struct primitiveReference*)array_get(&(checker->reference_array), i);
		if (primitive_pointer != NULL){
			if (i != (array_get_length(&(checker->reference_array)) - 1)){
				printf("%s, ", primitive_pointer->name);
			}
			else{
				printf("%s} ", primitive_pointer->name);
			}
		}
		else{
			printf("ERROR: in %s, array_get returns a NULL pointer\n", __func__);
		}
	}
	printf("max input: %u, max output: %u\n", checker->max_nb_input, checker->max_nb_output);
	#endif

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
	for (i = 1; i <= ((nb_input > checker->max_nb_input) ? checker->max_nb_input : nb_input); i++){
		memset(current_input, 0, i);
		input_next = 1;

		while(input_next){

			/* Generate the output sub set */
			for (j = 1; j <= ((nb_output > checker->max_nb_output) ? checker->max_nb_output : nb_output); j++){
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

void ioChecker_print(struct ioChecker* checker){
	uint32_t 					i;
	struct primitiveReference* 	primitive;

	if (checker != NULL){
		printf("*** IoChecker (max input: %u, max output: %u) ***\n", checker->max_nb_input, checker->max_nb_output);

		for (i = 0; i < array_get_length(&(checker->reference_array)); i++){
			primitive = (struct primitiveReference*)array_get(&(checker->reference_array), i);
			primitiveReference_print(primitive);
		}
	}
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

void ioChecker_wrapper_md5(void** input, void** output){
	uint64_t size;

	size = *(uint32_t*)input[1];
	md5((uint32_t*)input[0], size, (uint32_t*)output[0]);
}

void ioChecker_wrapper_tea_encipher(void** input, void** output){
	uint64_t size;

	size = *(uint32_t*)input[1];
	tea_encipher((uint32_t*)input[0], size, (uint32_t*)input[2], (uint32_t*)output[0]);
}

void ioChecker_wrapper_tea_decipher(void** input, void** output){
	uint64_t size;

	size = *(uint32_t*)input[1];
	tea_decipher((uint32_t*)input[0], size, (uint32_t*)input[2], (uint32_t*)output[0]);
}

void ioChecker_wrapper_rc4(void** input, void** output){
	uint64_t input_length;
	uint8_t key_length;

	input_length = *(uint32_t*)input[1];
	key_length = *(uint32_t*)input[3];
	rc4((uint8_t*)input[0], input_length, (uint8_t*)input[2], key_length, (uint8_t*)output[0]);
}

void ioChecker_wrapper_aes128_key_expand_encrypt(void** input, void** output){
	aes128_key_expand_encrypt((uint32_t*)input[0], (uint32_t*)output[0]);
}

void ioChecker_wrapper_aes128_key_expand_decrypt(void** input, void** output){
	aes128_key_expand_decrypt((uint32_t*)input[0], (uint32_t*)output[0]);
}

void ioChecker_wrapper_aes128_encrypt(void** input, void** output){
	aes128_encrypt((uint32_t*)input[0], (uint32_t*)input[1], (uint32_t*)output[0]);
}

void ioChecker_wrapper_aes128_decrypt(void** input, void** output){
	aes128_decrypt((uint32_t*)input[0], (uint32_t*)input[1], (uint32_t*)output[0]);
}

/* ===================================================================== */
/* Special debug stuff - don't touch	                                 */
/* ===================================================================== */

void ioChecker_handmade_test(struct ioChecker* checker){
	struct argBuffer plt;
	struct argBuffer key;
	struct argBuffer cit;
	unsigned char plt_val[12] = {0x48, 0x65, 0x6c, 0x6c, 0x6f, 0x20, 0x57, 0x6f, 0x72, 0x6c, 0x64, 0x21};
	unsigned char key_val[3]  = {0x4b, 0x65, 0x79};
	unsigned char cit_val[12] = {0xa3, 0xfa, 0x1b, 0xed, 0xd8, 0x14, 0x9d, 0x1d, 0xd5, 0x75, 0x2e, 0x09};
	struct array* array_in;
	struct array* array_out;

	plt.location_type = ARG_LOCATION_MEMORY;
	plt.location.address = 1;
	plt.size = 12;
	plt.data = (char*)malloc(12); /* srry but we don't check allocation */
	memcpy(plt.data, plt_val, 12);

	key.location_type = ARG_LOCATION_MEMORY;
	key.location.address = 2;
	key.size = 3;
	key.data = (char*)malloc(3); /* srry but we don't check allocation */
	memcpy(key.data, key_val, 3);

	cit.location_type = ARG_LOCATION_MEMORY;
	cit.location.address = 3;
	cit.size = 12;
	cit.data = (char*)malloc(12); /* srry but we don't check allocation */
	memcpy(cit.data, cit_val, 12);

	argBuffer_print_raw(&plt);
	argBuffer_print_raw(&key);
	argBuffer_print_raw(&cit);

	array_in = array_create(sizeof(struct argBuffer));
	array_out = array_create(sizeof(struct argBuffer));

	if (array_in != NULL && array_out != NULL){
		array_add(array_in, (void*)&plt); 	/* we should test if the return value is >= 0 */
		array_add(array_in, (void*)&key); 	/* we should test if the return value is >= 0 */
		array_add(array_out, (void*)&cit);	/* we should test if the return value is >= 0*/

		ioChecker_submit_arguments(checker, array_in, array_out);

		array_delete(array_in);
		array_delete(array_out);

		free(plt.data);
		free(key.data);
		free(cit.data);
	}
	else{
		printf("ERROR: in %s, unable to create arrays\n", __func__);
	}
}