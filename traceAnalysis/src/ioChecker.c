#include <stdlib.h>
#include <stdio.h>

#include "ioChecker.h"
#include "primitiveReference.h"
#include "MD5.h"


void ioChecker_wrapper_md5(void** input, uint8_t nb_input, void** output, uint8_t nb_output);

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
			if (array_add(&(checker->reference_array), &primitive)){
				printf("ERROR: in %s, unable to add element to array\n", __func__);
			}
		}
	}

	/* TEA cipher */
	{
		/* in order to add tea cipher new arg mode in primitive ref output size equal to input size */
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

	/* We can check the padding of the first argument */
	if (nb_input != 2 || nb_output != 1){
		printf("ERROR: in %s, incorrect arguement(s) to call md5\n", __func__);
	}
	else{
		size = *(uint32_t*)input[1];
		md5((uint32_t*)input[0], size, (uint32_t*)output[0]);
	}
}