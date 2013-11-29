#include <stdlib.h>
#include <stdio.h>

#include "ioChecker.h"
#include "primitiveReference.h"
#include "MD5.h"
#include "TEA.h"
#include "RC4.h"


void ioChecker_wrapper_md5(void** input, uint8_t nb_input, void** output, uint8_t nb_output);
void ioChecker_wrapper_tea_encipher(void** input, uint8_t nb_input, void** output, uint8_t nb_output);
void ioChecker_wrapper_tea_decipher(void** input, uint8_t nb_input, void** output, uint8_t nb_output);
void ioChecker_wrapper_rc4(void** input, uint8_t nb_input, void** output, uint8_t nb_output);


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

	result = 0;

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