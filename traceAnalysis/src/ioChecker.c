#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "ioChecker.h"
#include "primitiveReference.h"
#include "refReaderJSON.h"
#include "multiColumn.h"

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
	if (array_init(&(checker->reference_array), sizeof(struct primitiveReference))){
		printf("ERROR: in %s, unable to create array structure\n", __func__);
		return -1;
	}

	if (workQueue_init(&(checker->queue), IOCHECKER_NB_THREAD)){
		printf("ERROR: in %s, unable to init work queue\n", __func__);
		array_clean(&(checker->reference_array));
		return -1;
	}

	#ifdef VERBOSE
	if (multiWorkPercent_init(&(checker->multi_percent), IOCHECKER_NB_THREAD, WORKPERCENT_ACCURACY_0)){
		printf("ERROR: in %s, unable to create multiWorkPercent\n", __func__);
	}

	printf("IOChecker optimization: %u threads and accelerator for output larger than %u element(s)\n", IOCHECKER_NB_THREAD, IOCHECKER_MIN_SIZE_ACCELERATOR);
	#endif

	return 0;
}

void ioChecker_load(struct ioChecker* checker, void* arg){
	if (arg != NULL){
		if (refReaderJSON_parse(arg, &(checker->reference_array))){
			printf("ERROR: in %s, unable to parse reference file: \"%s\"\n", __func__, (char*)arg);
		}
	}
	else{
		printf("ERROR: in %s, a valid file name must be specified\n", __func__);
	}
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
	struct multiColumnPrinter* 	printer;
	#define INPUT_BUFFER_LENGTH 	80
	#define OUTPUT_BUFFER_LENGTH 	32
	char 						buffer_inputs[INPUT_BUFFER_LENGTH];
	char 						buffer_outputs[OUTPUT_BUFFER_LENGTH];

	printer = multiColumnPrinter_create(stdout, 3, NULL, NULL, NULL);
	if (printer != NULL){
		multiColumnPrinter_set_column_size(printer, 0, 32);
		multiColumnPrinter_set_column_size(printer, 1, INPUT_BUFFER_LENGTH);
		multiColumnPrinter_set_column_size(printer, 2, OUTPUT_BUFFER_LENGTH);

		multiColumnPrinter_set_title(printer, 0, (char*)"NAME");
		multiColumnPrinter_set_title(printer, 1, (char*)"INPUTS");
		multiColumnPrinter_set_title(printer, 2, (char*)"OUTPUTS");

		multiColumnPrinter_print_header(printer);

		for (i = 0; i < array_get_length(&(checker->reference_array)); i++){
			primitive = (struct primitiveReference*)array_get(&(checker->reference_array), i);
			
			primitiveReference_snprint_inputs(primitive, buffer_inputs, INPUT_BUFFER_LENGTH);
			primitiveReference_snprint_outputs(primitive, buffer_outputs, OUTPUT_BUFFER_LENGTH);

			multiColumnPrinter_print(printer, primitive->name, buffer_inputs, buffer_outputs, NULL);
		}

		multiColumnPrinter_delete(printer);
	}
	else{
		printf("ERROR: in %s, unable to create multi column printer\n", __func__);
	}

	#undef INPUT_BUFFER_LENGTH
	#undef OUTPUT_BUFFER_LENGTH
}

void ioChecker_empty(struct ioChecker* checker){
	uint32_t 					i;
	struct primitiveReference* 	primitive;

	for (i = 0; i < array_get_length(&(checker->reference_array)); i++){
		primitive = (struct primitiveReference*)array_get(&(checker->reference_array), i);
		primitiveReference_clean(primitive);
	}

	array_empty(&(checker->reference_array));
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
