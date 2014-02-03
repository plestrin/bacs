#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "memAccess.h"
#include "argBuffer.h"
#include "argSet.h"
#include "instruction.h"
#include "multiColumn.h"

int memAccess_compare_address_then_order(const void* mem_access1, const void* mem_access2); 		/* Order memAccess array in address order and in order order as a second option */
int memAccess_compare_address_then_inv_order(const void* mem_access1, const void* mem_access2); 	/* Order memAccess array in address order and in inv order order as a second option */


void memAccess_print(struct memAccess* mem_access, int nb_mem_access){
	struct multiColumnPrinter* 	printer;
	int 						i;
	char 						value_str[20];

	printer = multiColumnPrinter_create(stdout, 5, NULL, NULL, NULL);
	if (printer != NULL){
		multiColumnPrinter_set_column_size(printer, 3, 4);

		multiColumnPrinter_set_column_type(printer, 0, MULTICOLUMN_TYPE_UINT32);
		#if defined ARCH_32
		multiColumnPrinter_set_column_type(printer, 2, MULTICOLUMN_TYPE_HEX_32);
		#elif defined ARCH_64
		multiColumnPrinter_set_column_type(printer, 2, MULTICOLUMN_TYPE_HEX_64);
		#else
		#error Please specify an architecture {ARCH_32 or ARCH_64}
		#endif
		multiColumnPrinter_set_column_type(printer, 3, MULTICOLUMN_TYPE_UINT32);

		multiColumnPrinter_set_title(printer, 0, (char*)"ORDER");
		multiColumnPrinter_set_title(printer, 1, (char*)"VALUE");
		multiColumnPrinter_set_title(printer, 2, (char*)"ADDRESS");
		multiColumnPrinter_set_title(printer, 3, (char*)"SIZE");
		multiColumnPrinter_set_title(printer, 4, (char*)"OPCODE");

		multiColumnPrinter_print_header(printer);

		for (i = 0; i < nb_mem_access; i++){
			switch(mem_access[i].size){
			case 1 	: {snprintf(value_str, 20, "%02x", mem_access[i].value & 0x000000ff); break;}
			case 2 	: {snprintf(value_str, 20, "%04x", mem_access[i].value & 0x0000ffff); break;}
			case 4 	: {snprintf(value_str, 20, "%08x", mem_access[i].value & 0xffffffff); break;}
			default : {printf("WARNING: in %s, unexpected data size\n", __func__); break;}
			}

			multiColumnPrinter_print(printer, mem_access[i].order, value_str, mem_access[i].address, mem_access[i].size, instruction_opcode_2_string(mem_access[i].opcode), NULL);
		}

		multiColumnPrinter_delete(printer);
	}
	else{
		printf("ERROR: in %s, unable to create multi column printer\n", __func__);
	}
}

#define TRACEFRAGMENT_EXTRACT_MEM_ARG_ADJACENT(name, sort_rtn)																																	\
int32_t (name)(struct array* array, struct memAccess* mem_access, int nb_mem_access){																											\
	int 				i;																																										\
	int 				j;																																										\
	ADDRESS 			mem_address_upper_bound;																																				\
	ADDRESS 			mem_address_written;																																					\
	struct argBuffer 	arg;																																									\
	int8_t 				nb_byte;																																								\
																																																\
	if (mem_access != NULL && nb_mem_access > 0){																																				\
		qsort(mem_access, nb_mem_access, sizeof(struct memAccess), (sort_rtn));																													\
																																																\
		for (i = 1, j = 0, mem_address_upper_bound = mem_access[0].address + mem_access[0].size; i < nb_mem_access; i++){																		\
			if (mem_access[i].address > mem_address_upper_bound){																																\
				arg.location_type 		= ARG_LOCATION_MEMORY;																																	\
				arg.location.address 	= mem_access[j].address;																																\
				arg.size 				= (uint32_t)(mem_address_upper_bound - mem_access[j].address);																							\
				arg.access_size 		= ARGBUFFER_ACCESS_SIZE_UNDEFINED;																														\
				arg.data 				= (char*)malloc(arg.size);																																\
				if (arg.data == NULL){																																							\
					printf("ERROR: in %s, unable to allocate memory\n", __func__);																												\
					return -1;																																									\
				}																																												\
																																																\
				mem_address_written = mem_access[j].address;																																	\
				while (j <  i){																																									\
					nb_byte = (int8_t)((mem_access[j].address + mem_access[j].size) - mem_address_written);																						\
					if (nb_byte > 0){																																							\
						memcpy(arg.data + (mem_address_written - arg.location.address), (char*)&(mem_access[j].value) + (mem_access[j].size - nb_byte), nb_byte);								\
						mem_address_written += nb_byte;																																			\
					}																																											\
					j++;																																										\
				}																																												\
																																																\
				if (array_add(array, &arg) < 0){																																				\
					printf("ERROR: in %s, unable to add element to array structure\n", __func__);																								\
				}																																												\
																																																\
				mem_address_upper_bound = mem_access[i].address + mem_access[i].size;																											\
			}																																													\
			else{																																												\
				mem_address_upper_bound = (mem_address_upper_bound > (mem_access[i].address + mem_access[i].size)) ? mem_address_upper_bound : (mem_access[i].address + mem_access[i].size);	\
			}																																													\
		}																																														\
																																																\
		arg.location_type 		= ARG_LOCATION_MEMORY;																																			\
		arg.location.address 	= mem_access[j].address;																																		\
		arg.size 				= (uint32_t)(mem_address_upper_bound - mem_access[j].address);																									\
		arg.access_size 		= ARGBUFFER_ACCESS_SIZE_UNDEFINED;																																\
		arg.data 				= (char*)malloc(arg.size);																																		\
		if (arg.data == NULL){																																									\
			printf("ERROR: in %s, unable to allocate memory\n", __func__);																														\
		}																																														\
		else{																																													\
			mem_address_written = mem_access[j].address;																																		\
			while (j < i){																																										\
				nb_byte = (int8_t)((mem_access[j].address + mem_access[j].size) - mem_address_written);																							\
				if (nb_byte > 0){																																								\
					memcpy(arg.data + (mem_address_written - arg.location.address), (char*)&(mem_access[j].value) + (mem_access[j].size - nb_byte), nb_byte);									\
					mem_address_written += nb_byte;																																				\
				}																																												\
				j++;																																											\
			}																																													\
																																																\
			if (array_add(array, &arg) < 0){																																					\
				printf("ERROR: in %s, unable to add element to array structure\n", __func__);																									\
			}																																													\
		}																																														\
	}																																															\
																																																\
	return 0;																																													\
}

TRACEFRAGMENT_EXTRACT_MEM_ARG_ADJACENT(memAccess_extract_arg_adjacent_read, memAccess_compare_address_then_order)
TRACEFRAGMENT_EXTRACT_MEM_ARG_ADJACENT(memAccess_extract_arg_adjacent_write, memAccess_compare_address_then_inv_order)

#define TRACEFRAGMENT_EXTRACT_MEM_ARG_ADJACENT_SIZE(name, sort_rtn) 																															\
int32_t (name)(struct array* array, struct memAccess* mem_access, int nb_mem_access){ 																											\
	uint8_t*			taken; 																																									\
	int 				i; 																																										\
	int 				j; 																																										\
	int 				k; 																																										\
	struct argBuffer 	arg; 																																									\
	uint32_t			size; 																																									\
																																																\
	if (mem_access != NULL && nb_mem_access > 0){ 																																				\
		qsort(mem_access, nb_mem_access, sizeof(struct memAccess), (sort_rtn)); 																												\
																																																\
		taken = (uint8_t*)calloc(nb_mem_access, sizeof(uint8_t)); 																																\
		if (taken == NULL){ 																																									\
			printf("ERROR: in %s, unable to allocate memory\n", __func__); 																														\
			return -1; 																																											\
		} 																																														\
																																																\
		for (i = 0; i < nb_mem_access; i++){ 																																					\
			if (taken[i] == 0){ 																																								\
				taken[i] = 1; 																																									\
				size = mem_access[i].size; 																																						\
				for (j = i + 1; j < nb_mem_access; j++){ 																																		\
					if ((taken[j] == 0) && (mem_access[j].size == mem_access[i].size)){ 																										\
						if (mem_access[j].address == mem_access[i].address + size){ 																											\
							size += mem_access[j].size; 																																		\
							taken[j] = 1; 																																						\
						} 																																										\
						else if (mem_access[j].address > mem_access[i].address + size){ 																										\
							break; 																																								\
						} 																																										\
						else{ 																																									\
							taken[j] = 2; 																																						\
						} 																																										\
					} 																																											\
				} 																																												\
																																																\
				arg.location_type 		= ARG_LOCATION_MEMORY; 																																	\
				arg.location.address 	= mem_access[i].address; 																																\
				arg.size 				= size; 																																				\
				arg.access_size 		= mem_access[i].size; 																																	\
				arg.data 				= (char*)malloc(arg.size); 																																\
				if (arg.data == NULL){ 																																							\
					printf("ERROR: in %s, unable to allocate memory\n", __func__); 																												\
					return -1; 																																									\
				} 																																												\
																																																\
				for (k = i, size = 0; k < j; k++){ 																																				\
					if (taken[k] == 1){ 																																						\
						memcpy(arg.data + size, &(mem_access[k].value), mem_access[i].size); 																									\
						size += mem_access[i].size; 																																			\
						taken[k] = 2; 																																							\
					} 																																											\
				} 																																												\
																																																\
				if (array_add(array, &arg) < 0){ 																																				\
					printf("ERROR: in %s, unable to add element to array structure\n", __func__); 																								\
				} 																																												\
			} 																																													\
		} 																																														\
																																																\
		free(taken); 																																											\
	} 																																															\
																																																\
	return 0; 																																													\
}

TRACEFRAGMENT_EXTRACT_MEM_ARG_ADJACENT_SIZE(memAccess_extract_arg_adjacent_size_read, memAccess_compare_address_then_order)
TRACEFRAGMENT_EXTRACT_MEM_ARG_ADJACENT_SIZE(memAccess_extract_arg_adjacent_size_write, memAccess_compare_address_then_inv_order)

#define TRACEFRAGMENT_EXTRACT_MEM_ARG_ADJACENT_SIZE_OPCODE(name, sort_rtn) 																														\
int32_t (name)(struct array* array, struct memAccess* mem_access, int nb_mem_access){ 																											\
	uint8_t*			taken; 																																									\
	int 				i; 																																										\
	int 				j; 																																										\
	int 				k; 																																										\
	struct argBuffer 	arg; 																																									\
	uint32_t			size; 																																									\
	 																																															\
	if (mem_access != NULL && nb_mem_access > 0){ 																																				\
		qsort(mem_access, nb_mem_access, sizeof(struct memAccess), (sort_rtn)); 																												\
 																																																\
		taken = (uint8_t*)calloc(nb_mem_access, sizeof(uint8_t)); 																																\
		if (taken == NULL){ 																																									\
			printf("ERROR: in %s, unable to allocate memory\n", __func__); 																														\
			return -1; 																																											\
		} 																																														\
 																																																\
		for (i = 0; i < nb_mem_access; i++){ 																																					\
			if (taken[i] == 0){ 																																								\
				taken[i] = 1; 																																									\
				size = mem_access[i].size; 																																						\
				for (j = i + 1; j < nb_mem_access; j++){ 																																		\
					if ((taken[j] == 0) && (mem_access[j].size == mem_access[i].size) && (mem_access[j].opcode == mem_access[i].opcode)){ 														\
						if (mem_access[j].address == mem_access[i].address + size){ 																											\
							size += mem_access[j].size; 																																		\
							taken[j] = 1; 																																						\
						} 																																										\
						else if (mem_access[j].address > mem_access[i].address + size){ 																										\
							break; 																																								\
						} 																																										\
						else{ 																																									\
							taken[j] = 2; 																																						\
						} 																																										\
					} 																																											\
				} 																																												\
 																																																\
				arg.location_type 		= ARG_LOCATION_MEMORY; 																																	\
				arg.location.address 	= mem_access[i].address; 																																\
				arg.size 				= size; 																																				\
				arg.access_size 		= mem_access[i].size; 																																	\
				arg.data 				= (char*)malloc(arg.size); 																																\
				if (arg.data == NULL){ 																																							\
					printf("ERROR: in %s, unable to allocate memory\n", __func__); 																												\
					return -1; 																																									\
				} 																																												\
 																																																\
				for (k = i, size = 0; k < j; k++){ 																																				\
					if (taken[k] == 1){ 																																						\
						memcpy(arg.data + size, &(mem_access[k].value), mem_access[i].size); 																									\
						size += mem_access[i].size; 																																			\
						taken[k] = 2; 																																							\
					} 																																											\
				} 																																												\
 																																																\
				if (array_add(array, &arg) < 0){ 																																				\
					printf("ERROR: in %s, unable to add element to array structure\n", __func__); 																								\
				} 																																												\
			} 																																													\
		} 																																														\
 																																																\
		free(taken); 																																											\
	} 																																															\
 																																																\
	return 0; 																																													\
}

TRACEFRAGMENT_EXTRACT_MEM_ARG_ADJACENT_SIZE_OPCODE(memAccess_extract_arg_adjacent_size_opcode_read, memAccess_compare_address_then_order)
TRACEFRAGMENT_EXTRACT_MEM_ARG_ADJACENT_SIZE_OPCODE(memAccess_extract_arg_adjacent_size_opcode_write, memAccess_compare_address_then_inv_order)

/* ===================================================================== */
/* Comparison functions 	                                         	 */
/* ===================================================================== */

int memAccess_compare_address_then_order(const void* mem_access1, const void* mem_access2){
	if (((struct memAccess*)mem_access1)->address > ((struct memAccess*)mem_access2)->address){
		return 1;
	}
	else if (((struct memAccess*)mem_access2)->address > ((struct memAccess*)mem_access1)->address){
		return -1;
	}
	else{
		return (int32_t)(((struct memAccess*)mem_access1)->order - ((struct memAccess*)mem_access2)->order);
	}
}

int memAccess_compare_address_then_inv_order(const void* mem_access1, const void* mem_access2){
	if (((struct memAccess*)mem_access1)->address > ((struct memAccess*)mem_access2)->address){
		return 1;
	}
	else if (((struct memAccess*)mem_access2)->address > ((struct memAccess*)mem_access1)->address){
		return -1;
	}
	else{
		return (int32_t)(((struct memAccess*)mem_access2)->order - ((struct memAccess*)mem_access1)->order);
	}
}
