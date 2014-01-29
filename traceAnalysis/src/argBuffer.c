#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "argBuffer.h"
#include "printBuffer.h"
#include "instruction.h"


void argBuffer_print_reg(FILE* file, uint64_t reg);
void argBuffer_print_fragment_table(uint32_t* table, uint32_t nb_element);

void argBuffer_print_raw(struct argBuffer* arg){
	if (arg != NULL){
		if (arg->location_type == ARG_LOCATION_MEMORY){
			printf("Argument Memory\n");
			#if defined ARCH_32
			printf("\t-Address: \t0x%08x\n", arg->location.address);
			#elif defined ARCH_64
			#pragma GCC diagnostic ignored "-Wformat" /* ISO C90 does not support the ‘ll’ gnu_printf length modifier */
			printf("\t-Address: \t0x%llx\n", arg->location.address);
			#else
			#error Please specify an architecture {ARCH_32 or ARCH_64}
			#endif
		}
		else if (arg->location_type == ARG_LOCATION_REGISTER){
			printf("Argument Register\n\t-Register: \t");
			argBuffer_print_reg(stdout, arg->location.reg);
			printf("\n");
		}
		else{
			printf("ERROR: in %s, incorrect location type in argBuffer\n", __func__);
		}
		
		printf("\t-Size: \t\t%u\n", arg->size);

		if (arg->access_size == ARGBUFFER_ACCESS_SIZE_UNDEFINED){
			printf("\t-Access size: \tundefined\n");
		}
		else{
			printf("\t-Access size: \t%d\n", arg->access_size);
		}

		printf("\t-Value: \t");
		printBuffer_raw(stdout, arg->data, arg->size);
		printf("\n");
	}
}

int32_t argBuffer_clone(struct argBuffer* arg_src, struct argBuffer* arg_dst){
	arg_dst->location_type = arg_src->location_type;
	arg_dst->size = arg_src->size;
	arg_dst->access_size = arg_src->access_size;

	if (arg_src->location_type == ARG_LOCATION_MEMORY){
		arg_dst->location.address = arg_src->location.address;
	}
	else if (arg_src->location_type == ARG_LOCATION_REGISTER){
		arg_dst->location.reg = arg_src->location.reg;
	}
	else{
		printf("ERROR: in %s, incorrect location type in argBuffer\n", __func__);
	}

	arg_dst->data = (char*)malloc(arg_src->size);
	if (arg_dst->data == NULL){
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
		return -1;
	}
	memcpy(arg_dst->data, arg_src->data, arg_src->size);

	return 0;
}

int32_t argBuffer_search(struct argBuffer* arg, char* buffer, uint32_t buffer_size){
	uint32_t i;

	if (arg->size >= buffer_size){
		for (i = 0; i <= (arg->size - buffer_size); i++){
			if (!memcmp(arg->data + i, buffer, buffer_size)){
				return i;
			}
		}
		return -1;
	}
	else{
		return -1;
	}
}

struct argBuffer* argBuffer_compare(struct argBuffer* arg1, struct argBuffer* arg2){
	struct argBuffer* 	result = NULL;
	uint32_t			size;
	uint32_t			offset_arg1;
	uint32_t			offset_arg2;

	if (arg1->location_type == arg2->location_type){
		if (arg1->location_type == ARG_LOCATION_MEMORY){
			if (arg1->location.address > arg2->location.address){
				if (arg2->location.address + arg2->size <= arg1->location.address){
					return NULL;
				}
				offset_arg1 = 0;
				offset_arg2 = arg1->location.address - arg2->location.address;
			}
			else{
				if (arg1->location.address + arg1->size <= arg2->location.address){
					return NULL;
				}
				offset_arg1 = arg2->location.address - arg1->location.address;
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
					result->location.address = arg1->location.address + offset_arg1;
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
		}
		else if(arg1->location_type == ARG_LOCATION_REGISTER){
			/* a completer */
		}
		else{
			printf("ERROR: in %s, incorrect location type in argBuffer\n", __func__);
		}
	}

	return result;
}

int32_t argBuffer_try_merge(struct argBuffer* arg1, struct argBuffer* arg2){
	int32_t result = -1;
	char* 	new_data;

	if (arg1->location_type == arg2->location_type){
		if (arg1->location_type == ARG_LOCATION_MEMORY){
			/* For now no overlapping permitted - may change it afterward */
			if (arg2->location.address + arg2->size == arg1->location.address){
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
					arg1->location.address = arg2->location.address;

					result = 0;
				}
			}
			else if (arg1->location.address + arg1->size == arg2->location.address){
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
		}
		else if(arg1->location_type == ARG_LOCATION_REGISTER){
			/* a completer */
		}
		else{
			printf("ERROR: in %s, incorrect location type in argBuffer\n", __func__);
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

void argBuffer_print_reg(FILE* file, uint64_t reg){
	uint8_t i;
	uint8_t nb_register;

	nb_register = ARGBUFFER_GET_NB_REG(reg);
	for (i = 0; i < nb_register; i++){
		if (i == nb_register - 1){
			fprintf(file, "%s", reg_2_string((enum reg)ARGBUFFER_GET_REG_NAME(reg, i)));
		}
		else{
			fprintf(file, "%s, ", reg_2_string((enum reg)ARGBUFFER_GET_REG_NAME(reg, i)));
		}
	}
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

int32_t argBuffer_equal(struct argBuffer* arg1, struct argBuffer* arg2){
	int32_t result = -1;

	if (arg1->size == arg2->size && arg1->location_type == arg2->location_type){
		if (!memcmp(arg1->data, arg2->data, arg1->size)){
			if (arg1->location_type == ARG_LOCATION_MEMORY){
				if (arg1->location.address == arg2->location.address){
					result = 0;
				}
			}
			else if (arg1->location_type == ARG_LOCATION_REGISTER){
				if (arg1->location.reg == arg2->location.reg){
					result = 0;
				}
			}
			else{
				printf("ERROR: in %s, incorrect location type in argBuffer\n", __func__);
			}
		}
	}
	
	return result;
}

void argBuffer_print_fragment_table(uint32_t* table, uint32_t nb_element){
	uint32_t i;
	uint32_t j;
	uint32_t nb_partition = (0x00000001 << (nb_element - 1)) - 1;
	uint32_t verif_size;

	for (i = 0; i < nb_partition; i++){
		for (j = 0, verif_size = 0; j < nb_element; j++){
			if (table[i*nb_element*2 + j*2 + 1] != 0){
				printf("{@:%u, len:%u}", table[i*nb_element*2 + j*2], table[i*nb_element*2 + j*2 + 1]);
				verif_size += table[i*nb_element*2 + j*2 + 1];
			}
		}
		if (verif_size != nb_element){
			printf("ERROR (nb_element: %u, get: %u)", nb_element, verif_size);
		}

		printf("\n");
	} 
}

void argBuffer_create_fragment_table(struct argBuffer* arg, uint32_t** table_, uint32_t* nb_element_){
	uint32_t 	nb_element;
	uint32_t	partition_size;
	uint32_t 	nb_partition;
	uint32_t	*table;
	uint32_t 	i;
	uint32_t 	j;
	uint32_t 	k;
	uint32_t 	offset;
	uint8_t 	stop;

	if (arg->access_size != ARGBUFFER_ACCESS_SIZE_UNDEFINED){
		nb_element = arg->size / arg->access_size;

		if (nb_element > 1 && nb_element < ARGBUFFER_FRAGMENT_MAX_NB_ELEMENT){
			partition_size = 2 * nb_element;
			nb_partition = (0x00000001 << (nb_element - 1)) - 1;

			table = (uint32_t*)calloc(nb_partition * partition_size, sizeof(uint32_t));
			if (table == NULL){
				printf("ERROR: in %s, unable to allocate memory\n", __func__);
			}
			else{
				offset = 0;

				for (i = 2; i <= nb_element; i++){
					for (j = 0; j < i; j++){
						if (j == i - 1){
							table[offset * partition_size + 2*j + 0] = j;
							table[offset * partition_size + 2*j + 1] = nb_element - table[offset * partition_size + 2*j + 0];
						}
						else{
							table[offset * partition_size + 2*j + 0] = j;
							table[offset * partition_size + 2*j + 1] = 1;
						}
					}

					do{
						stop = 1;
						for (j = i - 1; j > 0; j--){
							if (table[offset * partition_size + 2 * j + 1] > 1){
								stop = 0;
								break;
							}
						}

						offset ++;

						if (!stop){
							for (k = 0; k < i; k++){
								if (k == j - 1){
									table[offset * partition_size + 2 * k + 0] = table[(offset - 1) * partition_size + 2 * k + 0];
									table[offset * partition_size + 2 * k + 1] = table[(offset - 1) * partition_size + 2 * k + 1] + 1;
								}
								else if (k >= j){
									if (k  == i - 1){
										table[offset * partition_size + 2 * k + 0] = table[offset * partition_size + 2 * (j - 1)] + table[offset * partition_size + 2 * (j - 1) + 1] + (k - j);
										table[offset * partition_size + 2 * k + 1] = nb_element - table[offset * partition_size + 2 * k + 0];
									}
									else{
										table[offset * partition_size + 2 * k + 0] = table[offset * partition_size + 2 * (j - 1) + 0] + table[offset * partition_size + 2 * (j - 1) + 1] + (k - j);
										table[offset * partition_size + 2 * k + 1] = 1;
									}
								}
								else{
									table[offset * partition_size + 2 * k + 0] = table[(offset - 1) * partition_size + 2 * k + 0];
									table[offset * partition_size + 2 * k + 1] = table[(offset - 1) * partition_size + 2 * k + 1];
								}
							}
						}
					}while(!stop);
				}
			}

			*table_ = table;
			*nb_element_ = nb_element;
		}
		else{
			#ifdef VERBOSE
			printf("\t- Cannot fragment input: too many or too few element(s) %u (max is fixed to %u)\n", nb_element, ARGBUFFER_FRAGMENT_MAX_NB_ELEMENT);
			#endif

			*table_ = NULL;
			*nb_element_ = 0;
		}
	}
	else{
		#ifdef VERBOSE
		printf("\t- Cannot fragment input: undefined size access\n");
		#endif

		*table_ = NULL;
		*nb_element_ = 0;
	}
}

void argBuffer_delete_array(struct array* arg_array){
	uint32_t 			i;
	struct argBuffer* 	arg;

	if (arg_array != NULL){
		for (i = 0; i < array_get_length(arg_array); i++){
			arg = (struct argBuffer*)array_get(arg_array, i);
			free(arg->data);
		}

		array_delete(arg_array);
	}
}