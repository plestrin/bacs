#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

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
void ioChecker_wrapper_aes128_inner_loop_enc(void** input, void** output);
void ioChecker_wrapper_aes128_inner_loop_dec(void** input, void** output);

void ioChecker_wrapper_aes192_key_expand_encrypt(void** input, void** output);
void ioChecker_wrapper_aes192_inner_loop_enc(void** input, void** output);
void ioChecker_wrapper_aes192_inner_loop_dec(void** input, void** output);

void ioChecker_wrapper_aes256_key_expand_encrypt(void** input, void** output);
void ioChecker_wrapper_aes256_inner_loop_enc(void** input, void** output);
void ioChecker_wrapper_aes256_inner_loop_dec(void** input, void** output);

void ioChecker_thread_job(void* arg);


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
	struct primitiveReference 	primitive;
	#ifdef VERBOSE
	uint32_t 					i;
	struct primitiveReference* 	primitive_pointer;
	#endif

	if (array_init(&(checker->reference_array), sizeof(struct primitiveReference))){
		printf("ERROR: in %s, unable to create array structure\n", __func__);
		return -1;
	}

	if (workQueue_init(&(checker->queue), IOCHECKER_NB_THREAD)){
		printf("ERROR: in %s, unable to init work queue\n", __func__);
		array_clean(&(checker->reference_array));
		return -1;
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
	/*{
		uint32_t input_specifier[4];
		uint32_t output_specifier[1];

		PRIMITIVEREFERENCE_ARG_SPECIFIER_SET_SIZE_UNDEFINED(input_specifier[0]);
		PRIMITIVEREFERENCE_ARG_SPECIFIER_SET_IMPLICIT_SIZE(input_specifier[1], 0);
		PRIMITIVEREFERENCE_ARG_SPECIFIER_SET_SIZE_MAX(input_specifier[2], RC4_KEY_MAX_NB_BYTE);
		PRIMITIVEREFERENCE_ARG_SPECIFIER_SET_IMPLICIT_SIZE(input_specifier[3], 1);
		PRIMITIVEREFERENCE_ARG_SPECIFIER_SET_SIZE_OF_INPUT_ARG(output_specifier[0], 0);

		IOCHECKER_ADD_PRIMITIVE_REFRENCE("RC4", ioChecker_wrapper_rc4)
	}*/

	/* AES 128 */
	{
		uint32_t input_specifier[1];
		uint32_t output_specifier[1];

		PRIMITIVEREFERENCE_ARG_SPECIFIER_SET_SIZE_EXACT_VALUE(input_specifier[0], AES_128_NB_BYTE_KEY);
		PRIMITIVEREFERENCE_ARG_SPECIFIER_SET_SIZE_EXACT_VALUE(output_specifier[0], AES_128_NB_BYTE_ROUND_KEY);

		IOCHECKER_ADD_PRIMITIVE_REFRENCE("AES128 key expand encrypt", ioChecker_wrapper_aes128_key_expand_encrypt)
	}
	/*{
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
	}*/
	{
		uint32_t input_specifier[2];
		uint32_t output_specifier[1];

		PRIMITIVEREFERENCE_ARG_SPECIFIER_SET_SIZE_EXACT_VALUE(input_specifier[0], AES_BLOCK_NB_BYTE);
		PRIMITIVEREFERENCE_ARG_SPECIFIER_SET_SIZE_EXACT_VALUE(input_specifier[1], AES_128_NB_BYTE_ROUND_KEY - 32);
		PRIMITIVEREFERENCE_ARG_SPECIFIER_SET_SIZE_EXACT_VALUE(output_specifier[0], AES_BLOCK_NB_BYTE);

		IOCHECKER_ADD_PRIMITIVE_REFRENCE("AES128 inner enc", ioChecker_wrapper_aes128_inner_loop_enc)
	}
	{
		uint32_t input_specifier[2];
		uint32_t output_specifier[1];

		PRIMITIVEREFERENCE_ARG_SPECIFIER_SET_SIZE_EXACT_VALUE(input_specifier[0], AES_BLOCK_NB_BYTE);
		PRIMITIVEREFERENCE_ARG_SPECIFIER_SET_SIZE_EXACT_VALUE(input_specifier[1], AES_128_NB_BYTE_ROUND_KEY - 32);
		PRIMITIVEREFERENCE_ARG_SPECIFIER_SET_SIZE_EXACT_VALUE(output_specifier[0], AES_BLOCK_NB_BYTE);

		IOCHECKER_ADD_PRIMITIVE_REFRENCE("AES128 inner dec", ioChecker_wrapper_aes128_inner_loop_dec)
	}

	/* AES 192 */
	{
		uint32_t input_specifier[1];
		uint32_t output_specifier[1];

		PRIMITIVEREFERENCE_ARG_SPECIFIER_SET_SIZE_EXACT_VALUE(input_specifier[0], AES_192_NB_BYTE_KEY);
		PRIMITIVEREFERENCE_ARG_SPECIFIER_SET_SIZE_EXACT_VALUE(output_specifier[0], AES_192_NB_BYTE_ROUND_KEY);

		IOCHECKER_ADD_PRIMITIVE_REFRENCE("AES192 key expand encrypt", ioChecker_wrapper_aes192_key_expand_encrypt)
	}
	{
		uint32_t input_specifier[2];
		uint32_t output_specifier[1];

		PRIMITIVEREFERENCE_ARG_SPECIFIER_SET_SIZE_EXACT_VALUE(input_specifier[0], AES_BLOCK_NB_BYTE);
		PRIMITIVEREFERENCE_ARG_SPECIFIER_SET_SIZE_EXACT_VALUE(input_specifier[1], AES_192_NB_BYTE_ROUND_KEY - 32);
		PRIMITIVEREFERENCE_ARG_SPECIFIER_SET_SIZE_EXACT_VALUE(output_specifier[0], AES_BLOCK_NB_BYTE);

		IOCHECKER_ADD_PRIMITIVE_REFRENCE("AES192 inner enc", ioChecker_wrapper_aes192_inner_loop_enc)
	}
	{
		uint32_t input_specifier[2];
		uint32_t output_specifier[1];

		PRIMITIVEREFERENCE_ARG_SPECIFIER_SET_SIZE_EXACT_VALUE(input_specifier[0], AES_BLOCK_NB_BYTE);
		PRIMITIVEREFERENCE_ARG_SPECIFIER_SET_SIZE_EXACT_VALUE(input_specifier[1], AES_192_NB_BYTE_ROUND_KEY - 32);
		PRIMITIVEREFERENCE_ARG_SPECIFIER_SET_SIZE_EXACT_VALUE(output_specifier[0], AES_BLOCK_NB_BYTE);

		IOCHECKER_ADD_PRIMITIVE_REFRENCE("AES192 inner dec", ioChecker_wrapper_aes192_inner_loop_dec)
	}

	/* AES 256 */
	{
		uint32_t input_specifier[1];
		uint32_t output_specifier[1];

		PRIMITIVEREFERENCE_ARG_SPECIFIER_SET_SIZE_EXACT_VALUE(input_specifier[0], AES_256_NB_BYTE_KEY);
		PRIMITIVEREFERENCE_ARG_SPECIFIER_SET_SIZE_EXACT_VALUE(output_specifier[0], AES_256_NB_BYTE_ROUND_KEY);

		IOCHECKER_ADD_PRIMITIVE_REFRENCE("AES256 key expand encrypt", ioChecker_wrapper_aes256_key_expand_encrypt)
	}
	{
		uint32_t input_specifier[2];
		uint32_t output_specifier[1];

		PRIMITIVEREFERENCE_ARG_SPECIFIER_SET_SIZE_EXACT_VALUE(input_specifier[0], AES_BLOCK_NB_BYTE);
		PRIMITIVEREFERENCE_ARG_SPECIFIER_SET_SIZE_EXACT_VALUE(input_specifier[1], AES_256_NB_BYTE_ROUND_KEY - 32);
		PRIMITIVEREFERENCE_ARG_SPECIFIER_SET_SIZE_EXACT_VALUE(output_specifier[0], AES_BLOCK_NB_BYTE);

		IOCHECKER_ADD_PRIMITIVE_REFRENCE("AES256 inner enc", ioChecker_wrapper_aes256_inner_loop_enc)
	}
	{
		uint32_t input_specifier[2];
		uint32_t output_specifier[1];

		PRIMITIVEREFERENCE_ARG_SPECIFIER_SET_SIZE_EXACT_VALUE(input_specifier[0], AES_BLOCK_NB_BYTE);
		PRIMITIVEREFERENCE_ARG_SPECIFIER_SET_SIZE_EXACT_VALUE(input_specifier[1], AES_256_NB_BYTE_ROUND_KEY - 32);
		PRIMITIVEREFERENCE_ARG_SPECIFIER_SET_SIZE_EXACT_VALUE(output_specifier[0], AES_BLOCK_NB_BYTE);

		IOCHECKER_ADD_PRIMITIVE_REFRENCE("AES256 inner dec", ioChecker_wrapper_aes256_inner_loop_dec)
	}

	#ifdef VERBOSE
	if (multiWorkPercent_init(&(checker->multi_percent), IOCHECKER_NB_THREAD, WORKPERCENT_ACCURACY_0)){
		printf("ERROR: in %s, unable to create multiWorkPercent\n", __func__);
	}

	printf("Create IOChecker: {");
	for (i = 0; i < array_get_length(&(checker->reference_array)); i++){
		primitive_pointer = (struct primitiveReference*)array_get(&(checker->reference_array), i);
		if (primitive_pointer != NULL){
			if (i != (array_get_length(&(checker->reference_array)) - 1)){
				printf("%s, ", primitive_pointer->name);
			}
			else{
				printf("%s}\n", primitive_pointer->name);
			}
		}
		else{
			printf("ERROR: in %s, array_get returns a NULL pointer\n", __func__);
		}
	}
	printf("IOChecker optimization: %u threads and accelerator for output larger than %u element(s)\n", IOCHECKER_NB_THREAD, IOCHECKER_MIN_SIZE_ACCELERATOR);
	#endif

	return 0;
}

int32_t ioChecker_submit_argSet(struct ioChecker* checker, struct argSet* arg_set){
	struct checkJob* 			job;
	uint32_t 					i;
	struct fastOutputSearch* 	accelerator = NULL;
	uint32_t 					accelerator_length[] = {8, 16, 24, 32, 40, 48, 56};

	if (array_get_length(arg_set->input) > 0  && array_get_length(arg_set->output) > 0){
		if (array_get_length(arg_set->output) >= IOCHECKER_MIN_SIZE_ACCELERATOR){
			#ifdef VERBOSE
			printf("Building accelerator to search arg_set: \"%s\" (%u output(s))\n", arg_set->tag, array_get_length(arg_set->output));
			#endif

			accelerator = fastOutputSearch_create(arg_set->output, accelerator_length, sizeof(accelerator_length) / sizeof(uint32_t));
			if (accelerator == NULL){
				printf("ERROR: in %s, unable to create fastOutputSearch\n", __func__);
			}
		}

		for (i = 0; i < array_get_length(&(checker->reference_array)); i++){
			job = (struct checkJob*)malloc(sizeof(struct checkJob));
			if (job == NULL){
				printf("ERROR: in %s, unable to allocate memory\n", __func__);
				return -1;
			}

			job->checker 			= checker;
			job->primitive_index 	= i;
			job->arg_set 			= arg_set;
			job->accelerator 		= accelerator;
			#ifdef VERBOSE
			job->multi_percent 		= &(checker->multi_percent);
			#endif

			if (accelerator != NULL){
				fastOutputSearch_incr_ref(accelerator);
			}

			if (workQueue_submit(&(checker->queue), ioChecker_thread_job, job)){
				printf("ERROR: in %s, unable to submit job to workQueue\n", __func__);
				free(job);
			}
		}

		if (accelerator != NULL){
			fastOutputSearch_delete(accelerator);
		}
	}

	return 0;
}

void ioChecker_thread_job(void* arg){
	struct checkJob* 				job = (struct checkJob*)arg;
	struct ioChecker* 				checker = job->checker;
	struct argSet* 					arg_set = job->arg_set;
	struct primitiveReference* 		primitive;
	uint8_t 						nb_input;
	uint8_t 						j;
	uint8_t 						k;
	struct argBuffer* 				input_combination;
	uint32_t* 						input_combination_index;
	uint32_t 						input_has_next;
	#ifdef VERBOSE
	uint32_t 						printer_index = multiWorkPercent_get_thread_index(job->multi_percent);
	#endif

	primitive = (struct primitiveReference*)array_get(&(checker->reference_array), job->primitive_index);
	nb_input = primitiveReference_get_nb_explicit_input(primitive);
	input_has_next = 1;

	input_combination = (struct argBuffer*)malloc(sizeof(struct argBuffer) * nb_input);
	input_combination_index = (uint32_t*)calloc(nb_input, sizeof(uint32_t));

	if (input_combination == NULL || input_combination_index == NULL){
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
		return;
	}

	#ifdef VERBOSE
	multiWorkPercent_start(job->multi_percent, printer_index, pow(array_get_length(arg_set->input), nb_input));
	#endif

	while(input_has_next){
		for (j = 0; j < nb_input; j++){
			memcpy(input_combination + j, array_get(arg_set->input, input_combination_index[j]), sizeof(struct argBuffer));
		}
		if (!primitiveReference_test(primitive, nb_input, input_combination, arg_set->output, job->accelerator)){
			#if VERBOSE
			printf("\x1b[33mSuccess\x1b[0m: found primitive: \"%s\" in argSet: \"%s\"\n", primitive->name, arg_set->tag);
			#endif
		}

		#ifdef VERBOSE
		multiWorkPercent_notify(job->multi_percent, printer_index, 1);
		#endif

		for (k = 0, input_has_next = 0; k < nb_input; k++){
			input_has_next |= (input_combination_index[k] != array_get_length(arg_set->input) - 1);
		}

		if (input_has_next){
			j = nb_input -1;
			while(input_combination_index[j] == array_get_length(arg_set->input) - 1){
				j--;
			}
			input_combination_index[j] ++;
			for (k = j + 1; k < nb_input; k++){
				input_combination_index[k] = 0;
			}
		}
	}

	#ifdef VERBOSE
	multiWorkPercent_conclude(job->multi_percent, printer_index);
	#endif

	free(input_combination);
	free(input_combination_index);

	if (job->accelerator != NULL){
		fastOutputSearch_delete(job->accelerator);
	}

	free(job);
}

void ioChecker_print(struct ioChecker* checker){
	uint32_t 					i;
	struct primitiveReference* 	primitive;

	if (checker != NULL){
		printf("*** IoChecker ***\n");

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
	#ifdef VERBOSE
	multiWorkPercent_clean(&(checker->multi_percent));
	#endif
	workQueue_clean(&(checker->queue));
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

void ioChecker_wrapper_aes128_inner_loop_enc(void** input, void** output){
	aes128_inner_loop_enc((uint32_t*)input[0], (uint32_t*)input[1], (uint32_t*)output[0]);
}

void ioChecker_wrapper_aes128_inner_loop_dec(void** input, void** output){
	aes128_inner_loop_dec((uint32_t*)input[0], (uint32_t*)input[1], (uint32_t*)output[0]);
}

void ioChecker_wrapper_aes192_key_expand_encrypt(void** input, void** output){
	aes192_key_expand_encrypt((uint32_t*)input[0], (uint32_t*)output[0]);
}

void ioChecker_wrapper_aes192_inner_loop_enc(void** input, void** output){
	aes192_inner_loop_enc((uint32_t*)input[0], (uint32_t*)input[1], (uint32_t*)output[0]);
}

void ioChecker_wrapper_aes192_inner_loop_dec(void** input, void** output){
	aes192_inner_loop_dec((uint32_t*)input[0], (uint32_t*)input[1], (uint32_t*)output[0]);
}

void ioChecker_wrapper_aes256_key_expand_encrypt(void** input, void** output){
	aes256_key_expand_encrypt((uint32_t*)input[0], (uint32_t*)output[0]);
}

void ioChecker_wrapper_aes256_inner_loop_enc(void** input, void** output){
	aes256_inner_loop_enc((uint32_t*)input[0], (uint32_t*)input[1], (uint32_t*)output[0]);
}

void ioChecker_wrapper_aes256_inner_loop_dec(void** input, void** output){
	aes256_inner_loop_dec((uint32_t*)input[0], (uint32_t*)input[1], (uint32_t*)output[0]);
}
