#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <alloca.h>

#include "argSet.h"
#include "multiColumn.h"
#include "printBuffer.h"

int32_t argSet_init(struct argSet* set, char* tag){
	set->input = array_create(sizeof(struct inputArgument));
	if (set->input == NULL){
		printf("ERROR: in %s, unable to create inputArgument array\n", __func__);
		return -1;
	}
	set->output = array_create(sizeof(struct outputArgument));
	if (set->output == NULL){
		printf("ERROR: in %s, unable to create outputArgument array\n", __func__);
		array_delete(set->input);
		return -1;
	}

	strncpy(set->tag, tag, ARGSET_TAG_MAX_LENGTH);

	return 0;
}

void argSet_print(struct argSet* set, enum argFragType* type){
	struct multiColumnPrinter* 	printer;
	uint32_t 					i;
	struct inputArgument* 		arg_input;
	struct outputArgument* 		arg_output;

	printer = multiColumnPrinter_create(stdout, 4, NULL, NULL, NULL);
	if (printer != NULL){
		multiColumnPrinter_set_column_size(printer, 0, ARGUMENT_STRING_DESC_LENGTH);
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
			arg_input = (struct inputArgument*)array_get(set->input, i);
			if ((type != NULL && ((*type == ARGFRAG_MEM && inputArgument_is_mem(arg_input)) || (*type == ARGFRAG_REG && inputArgument_is_reg(arg_input)))) || type == NULL){
				inputArgument_print(arg_input, printer);
			}
		}

		#ifdef VERBOSE
		printf("\n*** Output argBuffer(s) %u ***\n", array_get_length(set->output));
		#endif

		multiColumnPrinter_print_header(printer);

		for (i = 0; i < array_get_length(set->output); i++){
			arg_output = (struct outputArgument*)array_get(set->output, i);
			if ((type != NULL && *type == arg_output->desc.type) || type == NULL){
				outputArgument_print(arg_output, printer);
			}
		}

		multiColumnPrinter_delete(printer);
	}
	else{
		printf("ERROR: in %s, unable to init multiColumnPrinter\n", __func__);
	}
}

int32_t argSet_search_input(struct argSet* set, char* buffer, uint32_t buffer_length){
	uint32_t 				i;
	struct inputArgument* 	arg;
	int32_t 				index;
	char 					desc[ARGUMENT_STRING_DESC_LENGTH];

	for (i = 0; i < array_get_length(set->input); i++){
		arg = (struct inputArgument*)array_get(set->input, i);
		index = inputArgument_search(arg, buffer, buffer_length);
		if (index >= 0){
			inputArgument_print_desc(desc, ARGUMENT_STRING_DESC_LENGTH, arg);
			printf("Found in argset \"%s\", input %u %s\n", set->tag, i, desc);
			printBuffer_raw_color(arg->data, arg->size, index, buffer_length);
			printf("\n");
			return i;
		}
	}

	return -1;
}

int32_t argSet_add_output(struct argSet* set, struct operand* operand, uint8_t* data){
	struct outputArgument arg;

	if (!outputArgument_init(&arg, operand, (char*)data)){
		if (array_add(set->output, &arg) < 0){
			printf("ERROR: in %s, unable to add output argument to array\n", __func__);
			return -1;
		}
	}
	else{
		printf("ERROR: in %s, unable to init output argument\n", __func__);
		return -1;
	}

	return 0;
}

int32_t argSet_search_output(struct argSet* set, char* buffer, uint32_t buffer_length){
	int32_t* 				arg_index;
	char 					desc[ARGUMENT_STRING_DESC_LENGTH];
	uint32_t 				counter = 0;
	uint32_t 				i;
	uint32_t 				cmp_offset = 0;
	struct outputArgument* 	arg;

	arg_index = (int32_t*)alloca(sizeof(int32_t) * buffer_length);
	memset(arg_index, 0xff, sizeof(int32_t) * buffer_length);

	while(cmp_offset != buffer_length){
		for (i = arg_index[counter] + 1; i < array_get_length(set->output); i++){
			arg = (struct outputArgument*)array_get(set->output, i);
			if (buffer_length - cmp_offset >= arg->desc.size){
				if (!memcmp(buffer + cmp_offset, &(arg->data), arg->desc.size)){
					arg_index[counter] 	= i;
					counter 			= counter + 1;
					cmp_offset 			= cmp_offset + arg->desc.size;
					break;
				}
			}
		}

		if (i >= array_get_length(set->output)){
			if (counter == 0){
				return -1;
			}
			else{
				if (arg_index[counter] != -1){
					arg = (struct outputArgument*)array_get(set->output, arg_index[counter]);
					cmp_offset -= arg->desc.size;
					arg_index[counter] = -1;
				}					
				counter = counter - 1;
			}
		}
	}

	printf("Found in argset \"%s\", output(s): {", set->tag);
	for (i = 0; i < counter; i++){
		outputArgument_print_desc(desc, ARGUMENT_STRING_DESC_LENGTH, (struct outputArgument*)array_get(set->output, arg_index[i]));
		if (i == counter - 1){
			printf("%s}\n", desc);
		}
		else{
			printf("%s ", desc);
		}
	}

	return 0;
}

void argSet_clean(struct argSet* set){
	uint32_t 				i;
	struct inputArgument* 	arg;

	for (i = 0; i < array_get_length(set->input); i++){
		arg = (struct inputArgument*)array_get(set->input, i);
		inputArgument_clean(arg);
	}
	array_delete(set->input);
	set->input = NULL;

	array_delete(set->output);
	set->output = NULL;
}