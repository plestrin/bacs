#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <alloca.h>

#include "argument.h"
#include "printBuffer.h"

int32_t inputArgument_init(struct inputArgument* arg, uint32_t size, uint8_t nb_desc, int8_t access_size){
	arg->dyn_mem = malloc(sizeof(struct argFragDesc) * nb_desc + size);
	if (arg->dyn_mem != NULL){
		arg->desc 			= (struct argFragDesc*)arg->dyn_mem;
		arg->nb_desc 		= nb_desc;
		arg->data 			= (char*)arg->dyn_mem + sizeof(struct argFragDesc) * nb_desc;
		arg->size 			= size;
		arg->access_size 	= access_size;
	}
	else{
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
		return -1;
	}

	return 0;
}

void inputArgument_set_operand(struct inputArgument* arg, uint8_t index, uint32_t offset, struct operand* operand, char* data){
	if (index >= arg->nb_desc){
		printf("ERROR: in %s, index value is too high\n", __func__);
		return;
	}

	if (offset + operand->size > arg->size){
		printf("ERROR: in %s, offset value is too high\n", __func__);
		return;
	}
	
	if (OPERAND_IS_MEM(*operand)){
		arg->desc[index].type 				= ARGFRAG_MEM;
		arg->desc[index].location.address 	= operand->location.address;
		arg->desc[index].size 				= operand->size;
	}
	else if (OPERAND_IS_REG(*operand)){
		arg->desc[index].type 				= ARGFRAG_REG;
		arg->desc[index].location.reg 		= operand->location.reg;
		arg->desc[index].size 				= operand->size;
	}
	else{
		printf("ERROR: in %s, this case is not supposed to happen\n", __func__);
		return;
	}

	
	memcpy(arg->data + offset, data, operand->size);
}

uint32_t inputArgument_print_desc(char* string, uint32_t string_length, struct inputArgument* arg){
	uint32_t i;
	uint32_t offset = 0;

	for (i = 0; i < arg->nb_desc; i++){
		if (i == (uint32_t)arg->nb_desc - 1){
			switch(arg->desc[i].type){
				case ARGFRAG_MEM : {
					#if defined ARCH_32
					offset += snprintf(string + offset, string_length - offset, "Mem 0x%08x size: %u", arg->desc[i].location.address, arg->desc[i].size);
					#elif defined ARCH_64
					#pragma GCC diagnostic ignored "-Wformat" /* ISO C90 does not support the ‘ll’ gnu_printf length modifier */
					offset += snprintf(string + offset, string_length - offset, "Mem 0x%llx size: %u", arg->desc[i].location.address, arg->desc[i].size);
					#else
					#error Please specify an architecture {ARCH_32 or ARCH_64}
					#endif
					break;
				}
				case ARGFRAG_REG : {
					offset += snprintf(string + offset, string_length - offset, "%s", reg_2_string(arg->desc[i].location.reg));
					break;
				}
			}
		}
		else{
			switch(arg->desc[i].type){
				case ARGFRAG_MEM : {
					#if defined ARCH_32
					offset += snprintf(string + offset, string_length - offset, "Mem 0x%08x size: %u ", arg->desc[i].location.address, arg->desc[i].size);
					#elif defined ARCH_64
					#pragma GCC diagnostic ignored "-Wformat" /* ISO C90 does not support the ‘ll’ gnu_printf length modifier */
					offset += snprintf(string + offset, string_length - offset, "Mem 0x%llx size: %u ", arg->desc[i].location.address, arg->desc[i].size);
					#else
					#error Please specify an architecture {ARCH_32 or ARCH_64}
					#endif
					break;
				}
				case ARGFRAG_REG : {
					offset += snprintf(string + offset, string_length - offset, "%s ", reg_2_string(arg->desc[i].location.reg));
					break;
				}
			}
		}

		if (offset >= string_length){
			return string_length;
		}
	}

	return offset;
}

void inputArgument_print(struct inputArgument* arg, struct multiColumnPrinter* printer){
	char 	desc[ARGUMENT_STRING_DESC_LENGTH];
	char* 	value;

	if (printer != NULL){
		inputArgument_print_desc(desc, ARGUMENT_STRING_DESC_LENGTH, arg);

		value = (char*)alloca(PRINTBUFFER_GET_STRING_SIZE(arg->size));
		printBuffer_raw_string(value, PRINTBUFFER_GET_STRING_SIZE(arg->size), arg->data, arg->size);

		multiColumnPrinter_print(printer, desc, arg->size, arg->desc[0].size, value, NULL);
	}
	else{
		printf("WARNING: in %s, this case (printer is NULL) is not implemented yet\n", __func__);
	}
}

int32_t inputArgument_search(struct inputArgument* arg, char* buffer, uint32_t buffer_size){
	uint32_t i;

	if (arg->size >= buffer_size){
		if (arg->access_size != ARGUMENT_ACCESS_SIZE_UNDEFINED){
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

int32_t inputArgument_is_mem(struct inputArgument* arg){
	uint32_t i;

	for (i = 0; i < arg->nb_desc; i++){
		if (arg->desc[i].type != ARGFRAG_MEM){
			return 0;
		}
	}

	return 1;
}

int32_t inputArgument_is_reg(struct inputArgument* arg){
	uint32_t i;

	for (i = 0; i < arg->nb_desc; i++){
		if (arg->desc[i].type != ARGFRAG_REG){
			return 0;
		}
	}

	return 1;
}

int32_t outputArgument_init(struct outputArgument* arg, struct operand* operand, char* data){
	if (OPERAND_IS_MEM(*operand)){
		arg->desc.type 				= ARGFRAG_MEM;
		arg->desc.location.address 	= operand->location.address;
		arg->desc.size 				= operand->size;
	}
	else if (OPERAND_IS_REG(*operand)){
		arg->desc.type 				= ARGFRAG_REG;
		arg->desc.location.reg 		= operand->location.reg;
		arg->desc.size 				= operand->size;
	}
	else{
		printf("ERROR: in %s, this case is not supposed to happen\n", __func__);
		return - 1;
	}

	if (arg->desc.size > OUTPUTARGUMENT_MAX_SIZE){
		printf("ERROR: in %s, output argument size is too big (%u)\n", __func__, arg->desc.size);
		return -1;
	}
	else{
		memcpy(&(arg->data), data, arg->desc.size);
	}

	return 0;
}

uint32_t outputArgument_print_desc(char* string, uint32_t string_length, struct outputArgument* arg){
	uint32_t offset = 0;

	switch(arg->desc.type){
		case ARGFRAG_MEM : {
			#if defined ARCH_32
			offset = snprintf(string, string_length, "Mem 0x%08x size: %u", arg->desc.location.address, arg->desc.size);
			#elif defined ARCH_64
			#pragma GCC diagnostic ignored "-Wformat" /* ISO C90 does not support the ‘ll’ gnu_printf length modifier */
			offset = snprintf(string, string_length, "Mem 0x%llx size: %u", arg->desc.location.address, arg->desc.size);
			#else
			#error Please specify an architecture {ARCH_32 or ARCH_64}
			#endif
			break;
		}
		case ARGFRAG_REG : {
			offset = snprintf(string, string_length, "%s", reg_2_string(arg->desc.location.reg));
			break;
		}
	}

	return offset;
}

void outputArgument_print(struct outputArgument* arg, struct multiColumnPrinter* printer){
	char 	desc[ARGUMENT_STRING_DESC_LENGTH];
	char* 	value;

	if (printer != NULL){
		outputArgument_print_desc(desc, ARGUMENT_STRING_DESC_LENGTH, arg);

		value = (char*)alloca(PRINTBUFFER_GET_STRING_SIZE(arg->desc.size));
		printBuffer_raw_string(value, PRINTBUFFER_GET_STRING_SIZE(arg->desc.size), arg->data, arg->desc.size);

		multiColumnPrinter_print(printer, desc, arg->desc.size, arg->desc.size, value, NULL);
	}
	else{
		printf("WARNING: in %s, this case (printer is NULL) is not implemented yet\n", __func__);
	}
}
