#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "argBuffer.h"
#include "instruction.h"
#include "printBuffer.h"
#include "multiColumn.h"


void argBuffer_fprint_reg(FILE* file, uint64_t reg);
void argBuffer_snprint_reg(char* string, uint32_t string_length, uint64_t reg);


/* ===================================================================== */
/* argBuffer functions						                             */
/* ===================================================================== */

void argBuffer_print_metadata(struct argBuffer* arg){
	switch(arg->location_type){
	case ARG_LOCATION_MEMORY : {
		#if defined ARCH_32
		printf("Mem 0x%08x size: %u", arg->address, arg->size);
		#elif defined ARCH_64
		#pragma GCC diagnostic ignored "-Wformat" /* ISO C90 does not support the ‘ll’ gnu_printf length modifier */
		printf("Mem 0x%llx size: %u", arg->address, arg->size);
		#else
		#error Please specify an architecture {ARCH_32 or ARCH_64}
		#endif
		break;
	}
	case ARG_LOCATION_REGISTER : {
		argBuffer_fprint_reg(stdout, arg->reg);
		break;
	}
	case ARG_LOCATION_MIX : {
		argBuffer_fprint_reg(stdout, arg->reg);
		#if defined ARCH_32
		printf(" (mem 0x%08x)", arg->address);
		#elif defined ARCH_64
		#pragma GCC diagnostic ignored "-Wformat" /* ISO C90 does not support the ‘ll’ gnu_printf length modifier */
		printf(" (mem 0x%llx)", arg->address);
		#else
		#error Please specify an architecture {ARCH_32 or ARCH_64}
		#endif
		break;
	}
	}
}

int32_t argBuffer_clone(struct argBuffer* arg_src, struct argBuffer* arg_dst){
	arg_dst->location_type = arg_src->location_type;
	arg_dst->size = arg_src->size;
	arg_dst->access_size = arg_src->access_size;

	switch(arg_src->location_type){
	case ARG_LOCATION_MEMORY : {
		arg_dst->address = arg_src->address;
		break;
	}
	case ARG_LOCATION_REGISTER : {
		arg_dst->reg = arg_src->reg;
		break;
	}
	case ARG_LOCATION_MIX : {
		printf("ERROR: in %s, incorrect location type in argBuffer\n", __func__);
		break;
	}
	}

	arg_dst->data = (char*)malloc(arg_src->size);
	if (arg_dst->data == NULL){
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
		return -1;
	}
	memcpy(arg_dst->data, arg_src->data, arg_src->size);

	return 0;
}

int32_t argBuffer_equal(struct argBuffer* arg1, struct argBuffer* arg2){
	int32_t result = -1;

	if (arg1->size == arg2->size && arg1->location_type == arg2->location_type){
		if (!memcmp(arg1->data, arg2->data, arg1->size)){
			switch(arg1->location_type){
			case ARG_LOCATION_MEMORY : {
				if (arg1->address == arg2->address){
					result = 0;
				}
				break;
			}
			case ARG_LOCATION_REGISTER : {
				if (arg1->reg == arg2->reg){
					result = 0;
				}
				break;
			}
			case ARG_LOCATION_MIX : {
				printf("ERROR: in %s, incorrect location type in argBuffer\n", __func__);
				break;
			}
			}
		}
	}
	
	return result;
}

int32_t argBuffer_search(struct argBuffer* arg, char* buffer, uint32_t buffer_size){
	uint32_t i;

	if (arg->size >= buffer_size){
		if (arg->access_size != ARGBUFFER_ACCESS_SIZE_UNDEFINED){
			for (i = 0; i <= (arg->size - buffer_size); i += arg->access_size){
				if (!memcmp(arg->data + i, buffer, buffer_size)){
					return i;
				}
			}
		}
		else{
			for (i = 0; i <= (arg->size - buffer_size); i++){
				if (!memcmp(arg->data + i, buffer, buffer_size)){
					return i;
				}
			}
		}
	}

	return -1;
}

struct argBuffer* argBuffer_compare(struct argBuffer* arg1, struct argBuffer* arg2){
	struct argBuffer* 	result = NULL;
	uint32_t			size;
	uint32_t			offset_arg1;
	uint32_t			offset_arg2;

	if (arg1->location_type == arg2->location_type){
		switch(arg1->location_type){
		case ARG_LOCATION_MEMORY : {
			if (arg1->address > arg2->address){
				if (arg2->address + arg2->size <= arg1->address){
					return NULL;
				}
				offset_arg1 = 0;
				offset_arg2 = arg1->address - arg2->address;
			}
			else{
				if (arg1->address + arg1->size <= arg2->address){
					return NULL;
				}
				offset_arg1 = arg2->address - arg1->address;
				offset_arg2 = 0;
			}

			size = ((arg1->size - offset_arg1) > (arg2->size - offset_arg2)) ? (arg2->size - offset_arg2) : (arg1->size - offset_arg1);
			if (!memcmp(arg1->data + offset_arg1, arg2->data + offset_arg2, size)){
				result = (struct argBuffer*)malloc(sizeof(struct argBuffer));
				if (result == NULL){
					printf("ERROR: in %s, unable to allocate memory\n", __func__);
				}
				else{
					result->location_type = ARG_LOCATION_MEMORY;
					result->address = arg1->address + offset_arg1;
					result->size = size;
					result->access_size = (arg1->access_size == arg2->access_size) ? arg1->access_size : ARGBUFFER_ACCESS_SIZE_UNDEFINED;
					result->data = (char*)malloc(size);
					if (result->data == NULL){
						printf("ERROR: in %s, unable to allocate memory\n", __func__);
						free(result);
						return NULL;
					}
					memcpy(result->data, arg1->data + offset_arg1, size);
				}
			}
			break;
		}
		case ARG_LOCATION_REGISTER :
		case ARG_LOCATION_MIX : {

			/* a completer */
			break;
		}
		}
	}

	return result;
}

int32_t argBuffer_try_merge(struct argBuffer* arg1, struct argBuffer* arg2){
	int32_t result = -1;
	char* 	new_data;

	if (arg1->location_type == arg2->location_type){
		switch(arg1->location_type){
		case ARG_LOCATION_MEMORY : {
			/* For now no overlapping permitted - may change it afterward */
			if (arg2->address + arg2->size == arg1->address){
				new_data = (char*)malloc(arg1->size + arg2->size);
				if (new_data == NULL){
					printf("ERROR: in %s, unable to realloc memory\n", __func__);
				}
				else{
					memcpy(new_data, arg2->data, arg2->size);
					memcpy(new_data + arg2->size, arg1->data, arg1->size);
					free(arg1->data);
					arg1->data = new_data;
					arg1->size += arg2->size;
					arg1->access_size = (arg1->access_size < arg2->access_size) ? arg1->access_size : arg2->access_size;
					arg1->address = arg2->address;

					result = 0;
				}
			}
			else if (arg1->address + arg1->size == arg2->address){
				new_data = (char*)realloc(arg1->data, arg1->size + arg2->size);
				if (new_data == NULL){
					printf("ERROR: in %s, unable to realloc memory\n", __func__);
				}
				else{
					memcpy(new_data + arg1->size, arg2->data, arg2->size);
					arg1->data = new_data;
					arg1->size += arg2->size;
					arg1->access_size = (arg1->access_size < arg2->access_size) ? arg1->access_size : arg2->access_size;

					result = 0;
				}
			}
			break;
		}
		case ARG_LOCATION_REGISTER :
		case ARG_LOCATION_MIX : {

			/* a completer */
			break;
		}
		}
	}

	return result;
}

void argBuffer_delete(struct argBuffer* arg){
	if (arg != NULL){
		free(arg->data);
		free(arg);
	}
}

void argBuffer_fprint_reg(FILE* file, uint64_t reg){
	uint8_t i;
	uint8_t nb_register;

	nb_register = ARGBUFFER_GET_NB_REG(reg);
	for (i = 0; i < nb_register; i++){
		if (ARGBUFFER_GET_REG_NAME(reg, i) == ARGBUFFER_MEM_SLOT){
			if (i == nb_register - 1){
				fprintf(file, "MEM");
			}
			else{
				fprintf(file, "MEM, ");
			}
		}
		else{
			if (i == nb_register - 1){
				fprintf(file, "%s", reg_2_string((enum reg)ARGBUFFER_GET_REG_NAME(reg, i)));
			}
			else{
				fprintf(file, "%s, ", reg_2_string((enum reg)ARGBUFFER_GET_REG_NAME(reg, i)));
			}
		}
	}
}

void argBuffer_snprint_reg(char* string, uint32_t string_length, uint64_t reg){
	uint8_t 	i;
	uint8_t 	nb_register;
	uint32_t 	pointer = 0;

	nb_register = ARGBUFFER_GET_NB_REG(reg);
	for (i = 0; i < nb_register; i++){
		if (ARGBUFFER_GET_REG_NAME(reg, i) == ARGBUFFER_MEM_SLOT){
			if (i == nb_register - 1){
				pointer += snprintf(string + pointer, string_length - pointer, "MEM");
			}
			else{
				pointer += snprintf(string + pointer, string_length - pointer, "MEM, ");
			}
		}
		else{
			if (i == nb_register - 1){
				pointer += snprintf(string + pointer, string_length - pointer, "%s", reg_2_string((enum reg)ARGBUFFER_GET_REG_NAME(reg, i)));
			}
			else{
				pointer += snprintf(string + pointer, string_length - pointer, "%s, ", reg_2_string((enum reg)ARGBUFFER_GET_REG_NAME(reg, i)));
			}
		}
		if (pointer >= string_length){
			break;
		}
	}
}


/* ===================================================================== */
/* argBuffer array functions						                     */
/* ===================================================================== */

void argBuffer_print_array(struct array* array, enum argLocationType* type){
	struct multiColumnPrinter* 	printer;
	uint32_t 					i;
	struct argBuffer* 			arg;
	#define DESC_SIZE			32
	char 						desc[DESC_SIZE];
	char* 						value;

	printer = multiColumnPrinter_create(stdout, 4, NULL, NULL, NULL);
	if (printer != NULL){
		multiColumnPrinter_set_column_size(printer, 0, DESC_SIZE);
		multiColumnPrinter_set_column_size(printer, 1, 4);
		multiColumnPrinter_set_column_size(printer, 2, 5);

		multiColumnPrinter_set_title(printer, 0, "Description");
		multiColumnPrinter_set_title(printer, 1, "Size");
		multiColumnPrinter_set_title(printer, 2, "Asize");
		multiColumnPrinter_set_title(printer, 3, "Value");

		multiColumnPrinter_set_column_type(printer, 1, MULTICOLUMN_TYPE_UINT32);
		multiColumnPrinter_set_column_type(printer, 2, MULTICOLUMN_TYPE_INT8);
		multiColumnPrinter_set_column_type(printer, 3, MULTICOLUMN_TYPE_UNBOUND_STRING);

		multiColumnPrinter_print_header(printer);

		for (i = 0; i < array_get_length(array); i++){
			arg = (struct argBuffer*)array_get(array, i);
			if ((type != NULL && *type == arg->location_type) || type == NULL){
				switch(arg->location_type){
				case ARG_LOCATION_MEMORY : {
					#if defined ARCH_32
					snprintf(desc, DESC_SIZE, "Mem 0x%08x", arg->address);
					#elif defined ARCH_64
					#pragma GCC diagnostic ignored "-Wformat" /* ISO C90 does not support the ‘ll’ gnu_printf length modifier */
					snprintf(desc, DESC_SIZE, "Mem 0x%llx", arg->address);
					#else
					#error Please specify an architecture {ARCH_32 or ARCH_64}
					#endif
					break;
				}
				case ARG_LOCATION_REGISTER : {
					argBuffer_snprint_reg(desc, DESC_SIZE, arg->reg);
					break;
				}
				case ARG_LOCATION_MIX : {
					printf("ERROR: in %s, incorrect location type in argBuffer\n", __func__);
					break;
				}
				}

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

	#undef DESC_SIZE
}

int32_t argBuffer_clone_array(struct array* array_src, struct array* array_dst){
	uint32_t 			i;
	struct argBuffer* 	arg_src;
	struct argBuffer* 	arg_dst;

	if (array_clone(array_src, array_dst)){
		printf("ERROR: in %s, unable to clone array\n", __func__);
		return -1;
	}

	for (i = 0; i < array_get_length(array_src); i++){
		arg_src = (struct argBuffer*)array_get(array_src, i);
		arg_dst = (struct argBuffer*)array_get(array_dst, i);

		arg_dst->data = (char*)malloc(arg_src->size);
		if (arg_dst->data == NULL){
			printf("ERROR: in %s, unable to allocate memory\n", __func__);
			return -1;
		}
		memcpy(arg_dst->data, arg_src->data, arg_src->size);
	}

	return 0;
}