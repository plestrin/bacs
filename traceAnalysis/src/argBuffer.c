#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "argBuffer.h"
#include "printBuffer.h"

void argBuffer_print_fragment_table(uint32_t* table, uint32_t nb_element);
static void argBuffer_create_fragment_table(struct argBuffer* arg, uint32_t** table_, uint32_t* nb_element_);

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
			printf("Argument Register\n");
			/* a completer */
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

static void argBuffer_create_fragment_table(struct argBuffer* arg, uint32_t** table_, uint32_t* nb_element_){
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

void argument_fragment_input(struct argument* argument, struct array* array){
	uint32_t 			nb_input;
	uint32_t**			table_store;
	uint32_t*			nb_element_store;
	uint32_t			i;
	uint32_t			j;
	uint32_t			k;
	struct argument 	new_argument;
	struct argBuffer* 	argBuffer;
	struct argBuffer 	new_argBuffer;
	uint32_t 			nb_fragment = 0;
	uint32_t			local_index;
	uint32_t 			global_index;

	nb_input = array_get_length(argument->input);

	table_store = (uint32_t**)malloc(sizeof(uint32_t*) * nb_input);
	if (table_store == NULL){
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
		return;
	}

	nb_element_store = (uint32_t*)malloc(sizeof(uint32_t) * nb_input);
	if (nb_element_store == NULL){
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
		free(table_store);
		return;
	}

	for (i = 0; i < nb_input; i++){
		argBuffer_create_fragment_table((struct argBuffer*)array_get(argument->input, i), table_store + i, nb_element_store + i);
		if (table_store[i] != NULL && nb_element_store[i] != 0){
			if (nb_fragment == 0){
				nb_fragment = (0x00000001 << (nb_element_store[i] - 1)) - 1;
			}
			else{
				nb_fragment = nb_fragment * ((0x00000001 << (nb_element_store[i] - 1)) - 1);
			}
		}
	}

	if (nb_fragment != 0){
		#ifdef VERBOSE
		printf("Creating %u fragment argument(s)\n", nb_fragment);
		#endif

		for (i = 0; i < nb_fragment; i++){
			global_index = i;

			snprintf(new_argument.tag, ARGBUFFER_TAG_LENGTH, "frag %u: %s", i, argument->tag);

			new_argument.input = array_create(sizeof(struct argBuffer));
			if (new_argument.input == NULL){
				printf("ERROR: in %s, unable to create array\n", __func__);
				break;
			}

			new_argument.output = (struct array*)malloc(sizeof(struct array));
			if (new_argument.output == NULL){
				printf("ERROR: in %s, unable to allocate memory\n", __func__);
				break;
			}

			for (j = 0; j < nb_input; j++){
				if (table_store[j] != NULL && nb_element_store[j] != 0){
					local_index = global_index % ((0x00000001 << (nb_element_store[j] - 1)) - 1);
					global_index = global_index / ((0x00000001 << (nb_element_store[j] - 1)) - 1);
					argBuffer = (struct argBuffer*)array_get(argument->input, j);

					new_argBuffer.location_type = argBuffer->location_type;
					new_argBuffer.access_size = argBuffer->access_size;

					for (k = 0; k < nb_element_store[j]; k++){
						if (table_store[j][local_index * (2 * nb_element_store[j]) + 2*k + 1] != 0){
							new_argBuffer.size = table_store[j][local_index * (2 * nb_element_store[j]) + 2*k + 1] * new_argBuffer.access_size;
							if (new_argBuffer.location_type == ARG_LOCATION_MEMORY){
								new_argBuffer.location.address = argBuffer->location.address + table_store[j][local_index * (2 * nb_element_store[j]) + 2*k] * new_argBuffer.access_size;
							}
							else if (new_argBuffer.location_type == ARG_LOCATION_REGISTER){
								/* a completer */
							}
							else{
								printf("ERROR: in %s, incorrect location type in argBuffer\n", __func__);
							}

							new_argBuffer.data = (char*)malloc(new_argBuffer.size);
							if (new_argBuffer.data == NULL){
								printf("ERROR: in %s, unable to allocate memory\n", __func__);
								continue;
							}
							memcpy(new_argBuffer.data, argBuffer->data + table_store[j][local_index * (2 * nb_element_store[j]) + 2*k] * new_argBuffer.access_size, new_argBuffer.size);

							if (array_add(new_argument.input, &new_argBuffer) < 0){
								printf("ERROR: in %s, unable to add argBuffer in input array\n", __func__);
								free(new_argBuffer.data);
							}
						}
					}
				}
				else{
					if (argBuffer_clone((struct argBuffer*)array_get(argument->input, j), &new_argBuffer)){
						printf("ERROR: in %s, unable to clone argBuffer\n", __func__);
					}
					else{
						if (array_add(new_argument.input, &new_argBuffer) < 0){
							printf("ERROR: in %s, unable to add argBuffer to array\n", __func__);
						}
					}
				}
			}

			if (argBuffer_clone_array(argument->output, new_argument.output)){
				printf("ERROR: in %s, unable to clone output argBuffer array\n", __func__);
				argBuffer_delete_array(new_argument.input);
				break;
			}

			if (array_add(array, &new_argument) < 0){
				printf("ERROR: in %s, unable to add argument to array\n", __func__);
				break;
			}
		}

		for (i = 0;  i < nb_input; i++){
			if (table_store[i] != NULL){
				free(table_store[i]);
			}
		}
	}

	free(table_store);
	free(nb_element_store);

	return;
}