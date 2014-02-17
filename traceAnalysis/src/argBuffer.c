#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "argBuffer.h"
#include "instruction.h"


uint32_t argBuffer_print_reg(char* string, uint32_t string_length, uint64_t reg);


uint32_t argBuffer_print(char* string, uint32_t string_length, struct argBuffer* arg){
	uint32_t offset = 0;

	switch(arg->location_type){
		case ARG_LOCATION_MEMORY : {
			#if defined ARCH_32
			offset = snprintf(string, string_length, "Mem 0x%08x size: %u", arg->address, arg->size);
			#elif defined ARCH_64
			#pragma GCC diagnostic ignored "-Wformat" /* ISO C90 does not support the ‘ll’ gnu_printf length modifier */
			offset = snprintf(string, string_length, "Mem 0x%llx size: %u", arg->address, arg->size);
			#else
			#error Please specify an architecture {ARCH_32 or ARCH_64}
			#endif
			break;
		}
		case ARG_LOCATION_REGISTER : {
			offset = argBuffer_print_reg(string, string_length, arg->reg);
			break;
		}
		case ARG_LOCATION_MIX : {
			offset = argBuffer_print_reg(string, string_length, arg->reg);
			if (offset < string_length){
				#if defined ARCH_32
				offset += snprintf(string + offset, string_length - offset, " (mem 0x%08x)", arg->address);
				#elif defined ARCH_64
				#pragma GCC diagnostic ignored "-Wformat" /* ISO C90 does not support the ‘ll’ gnu_printf length modifier */
				offset += snprintf(string + offset, string_length - offset, " (mem 0x%llx)", arg->address);
				#else
				#error Please specify an architecture {ARCH_32 or ARCH_64}
				#endif
			}
			break;
		}
	}

	return offset;
}

uint32_t argBuffer_print_reg(char* string, uint32_t string_length, uint64_t reg){
	uint8_t 	i;
	uint8_t 	nb_register;
	uint32_t 	offset = 0;

	nb_register = ARGBUFFER_GET_NB_REG(reg);
	for (i = 0; i < nb_register; i++){
		if (ARGBUFFER_GET_REG_NAME(reg, i) == ARGBUFFER_MEM_SLOT){
			if (i == nb_register - 1){
				offset += snprintf(string + offset, string_length - offset, "MEM");
			}
			else{
				offset += snprintf(string + offset, string_length - offset, "MEM, ");
			}
		}
		else{
			if (i == nb_register - 1){
				offset += snprintf(string + offset, string_length - offset, "%s", reg_2_string((enum reg)ARGBUFFER_GET_REG_NAME(reg, i)));
			}
			else{
				offset += snprintf(string + offset, string_length - offset, "%s, ", reg_2_string((enum reg)ARGBUFFER_GET_REG_NAME(reg, i)));
			}
		}
		if (offset >= string_length){
			break;
		}
	}

	return offset;
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

int32_t argBuffer_compare_data(struct argBuffer* arg1, struct argBuffer* arg2){
	int32_t cmp_size;
	int32_t cmp_rslt;

	cmp_size = (arg1->size > arg2->size) ? arg2->size : arg1->size;
	cmp_rslt = memcmp(arg1->data, arg2->data, cmp_size);

	if (cmp_rslt == 0){
		return (int32_t)arg1->size - (int32_t)arg2->size;
	}
	
	return cmp_rslt;
}

