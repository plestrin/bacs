#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <alloca.h>

#include "argSet.h"
#include "multiColumn.h"
#include "printBuffer.h"

int32_t argSet_init(struct argSet* set, char* tag){
	set->input = array_create(sizeof(struct argBuffer));
	if (set->input == NULL){
		printf("ERROR: in %s, unable to create array\n", __func__);
		return -1;
	}
	set->output = array_create(sizeof(struct argBuffer));
	if (set->output == NULL){
		printf("ERROR: in %s, unable to create array\n", __func__);
		array_delete(set->input);
		return -1;
	}

	strncpy(set->tag, tag, ARGSET_TAG_MAX_LENGTH);

	set->output_mapping = NULL;

	return 0;
}

void argSet_print(struct argSet* set, enum argLocationType* type){
	struct multiColumnPrinter* 	printer;
	uint32_t 					i;
	struct argBuffer* 			arg;
	char 						desc[ARGBUFFER_STRING_DESC_LENGTH];
	char* 						value;

	printer = multiColumnPrinter_create(stdout, 4, NULL, NULL, NULL);
	if (printer != NULL){
		multiColumnPrinter_set_column_size(printer, 0, ARGBUFFER_STRING_DESC_LENGTH);
		multiColumnPrinter_set_column_size(printer, 1, 4);
		multiColumnPrinter_set_column_size(printer, 2, 5);

		multiColumnPrinter_set_title(printer, 0, "Description");
		multiColumnPrinter_set_title(printer, 1, "Size");
		multiColumnPrinter_set_title(printer, 2, "Asize");
		multiColumnPrinter_set_title(printer, 3, "Value");

		multiColumnPrinter_set_column_type(printer, 1, MULTICOLUMN_TYPE_UINT32);
		multiColumnPrinter_set_column_type(printer, 2, MULTICOLUMN_TYPE_INT8);
		multiColumnPrinter_set_column_type(printer, 3, MULTICOLUMN_TYPE_UNBOUND_STRING);

		#ifdef VERBOSE
		printf("*** Input argBuffer(s) %u ***\n", array_get_length(set->input));
		#endif

		multiColumnPrinter_print_header(printer);

		for (i = 0; i < array_get_length(set->input); i++){
			arg = (struct argBuffer*)array_get(set->input, i);
			if ((type != NULL && *type == arg->location_type) || type == NULL){
				argBuffer_print(desc, ARGBUFFER_STRING_DESC_LENGTH, arg);

				value = (char*)malloc(PRINTBUFFER_GET_STRING_SIZE(arg->size));
				if (value == NULL){
					printf("ERROR: in %s, unable to allocate memory\n", __func__);
					break;
				}
				printBuffer_raw_string(value, PRINTBUFFER_GET_STRING_SIZE(arg->size), arg->data, arg->size);

				multiColumnPrinter_print(printer, desc, arg->size, arg->access_size, value, NULL);
				
				free(value);
			}
		}

		#ifdef VERBOSE
		printf("\n*** Output argBuffer(s) %u ***\n", array_get_length(set->output));
		#endif

		multiColumnPrinter_print_header(printer);

		for (i = 0; i < array_get_length(set->output); i++){
			arg = (struct argBuffer*)array_get(set->output, i);
			if ((type != NULL && *type == arg->location_type) || type == NULL){
				argBuffer_print(desc, ARGBUFFER_STRING_DESC_LENGTH, arg);

				value = (char*)malloc(PRINTBUFFER_GET_STRING_SIZE(arg->size));
				if (value == NULL){
					printf("ERROR: in %s, unable to allocate memory\n", __func__);
					break;
				}
				printBuffer_raw_string(value, PRINTBUFFER_GET_STRING_SIZE(arg->size), arg->data, arg->size);

				multiColumnPrinter_print(printer, desc, arg->size, arg->access_size, value, NULL);
				
				free(value);
			}
		}

		multiColumnPrinter_delete(printer);
	}
	else{
		printf("ERROR: in %s, unable to init multiColumnPrinter\n", __func__);
	}
}

void argSet_get_nb_mem(struct argSet* set, uint32_t* nb_in, uint32_t* nb_out){
	uint32_t 			i;
	uint32_t 			result_in = 0;
	uint32_t 			result_out = 0;

	for (i = 0; i < array_get_length(set->input); i++){
		if (ARGBUFFER_IS_MEM((struct argBuffer*)array_get(set->input, i))){
			result_in ++;
		}
	}

	for (i = 0; i < array_get_length(set->output); i++){
		if (ARGBUFFER_IS_MEM((struct argBuffer*)array_get(set->output, i))){
			result_out ++;
		}
	}

	*nb_in = result_in;
	*nb_out = result_out;
}

void argSet_get_nb_reg(struct argSet* set, uint32_t* nb_in, uint32_t* nb_out){
	uint32_t 			i;
	uint32_t 			result_in = 0;
	uint32_t 			result_out = 0;

	for (i = 0; i < array_get_length(set->input); i++){
		if (ARGBUFFER_IS_REG((struct argBuffer*)array_get(set->input, i))){
			result_in ++;
		}
	}

	for (i = 0; i < array_get_length(set->output); i++){
		if (ARGBUFFER_IS_REG((struct argBuffer*)array_get(set->output, i))){
			result_out ++;
		}
	}

	*nb_in = result_in;
	*nb_out = result_out;
}

void argSet_get_nb_mix(struct argSet* set, uint32_t* nb_in, uint32_t* nb_out){
	uint32_t 			i;
	uint32_t 			result_in = 0;
	uint32_t 			result_out = 0;

	for (i = 0; i < array_get_length(set->input); i++){
		if (ARGBUFFER_IS_MIX((struct argBuffer*)array_get(set->input, i))){
			result_in ++;
		}
	}

	for (i = 0; i < array_get_length(set->output); i++){
		if (ARGBUFFER_IS_MIX((struct argBuffer*)array_get(set->output, i))){
			result_out ++;
		}
	}

	*nb_in = result_in;
	*nb_out = result_out;
}

int32_t argSet_sort_output(struct argSet* set){
	if (set->output_mapping != NULL){
		free(set->output_mapping);
	}

	set->output_mapping = array_create_mapping(set->output, (int32_t(*)(void*,void*))argBuffer_compare_data);
	if (set->output_mapping == NULL){
		printf("ERROR: in %s, unable to create array mapping\n", __func__);
		return -1;
	}

	return 0;
}

int32_t argSet_search_input(struct argSet* set, char* buffer, uint32_t buffer_length){
	uint32_t 			i;
	struct argBuffer* 	arg;
	int32_t 			index;
	char 				desc[ARGBUFFER_STRING_DESC_LENGTH];

	for (i = 0; i < array_get_length(set->input); i++){
		arg = (struct argBuffer*)array_get(set->input, i);
		index = argBuffer_search(arg, buffer, buffer_length);
		if (index >= 0){
			argBuffer_print(desc, ARGBUFFER_STRING_DESC_LENGTH, arg);
			printf("Found in argset \"%s\", input %u %s\n", set->tag, i, desc);
			printBuffer_raw_color(arg->data, arg->size, index, buffer_length);
			printf("\n");
			return i;
		}
	}

	return -1;
}

/* This routine works only for 32 bits arg */
int32_t argSet_search_output(struct argSet* set, char* buffer, uint32_t buffer_length){
	uint32_t 	i;
	uint32_t* 	arg_index;
	char 		desc[ARGBUFFER_STRING_DESC_LENGTH];

	if (set->output_mapping == NULL){
		printf("ERROR: in %s, output mapping is NULL, please create before calling this method\n", __func__);
		return -1;
	}

	arg_index = alloca(sizeof(uint32_t) * (buffer_length / 4));

	for (i = 0; i < (buffer_length / 4); i++){
		uint32_t 			low = 0;
		uint32_t 			up = array_get_length(set->output);
		uint32_t 			idx;
		struct argBuffer*	arg;
		int32_t 			comparison;
		int8_t 				found = 0;

		while (low < up){
			idx = (low + up) / 2;
			arg = (struct argBuffer*)array_get(set->output, set->output_mapping[idx]);
			if (arg->size == 4){
				comparison = memcmp(buffer + 4*i, arg->data, 4);
			}
			else{
				comparison = (arg->size > 4) ? -1 : 1;
			}
			if (comparison < 0){
				up = idx;
			}
			else if (comparison > 0){
				low = idx + 1;
			}
			else{
				arg_index[i] = set->output_mapping[idx];
				found = 1;
				break;
			}
		}

		if (!found){
			return -1;
		}
	}
	printf("Found in argset \"%s\", output %u {", set->tag, i);
	for (i = 0; i < (buffer_length / 4); i++){
		argBuffer_print(desc, ARGBUFFER_STRING_DESC_LENGTH, (struct argBuffer*)array_get(set->output, arg_index[i]));
		if (i == (buffer_length / 4) - 1){
			printf("%s}\n", desc);
		}
		else{
			printf("%s ", desc);
		}
	}

	return 0;
}

void argSet_clean(struct argSet* set){
	uint32_t 			i;
	struct argBuffer* 	arg;

	if (set->output_mapping != NULL){
		free(set->output_mapping);
	}

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