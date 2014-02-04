#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "regAccess.h"
#include "argBuffer.h"
#include "argSet.h"
#include "multiColumn.h"
#include "permutation.h"

void regAccess_print(struct regAccess* reg_access, int nb_reg_access){
	struct multiColumnPrinter* 	printer;
	int 						i;
	char 						value_str[20];

	printer = multiColumnPrinter_create(stdout, 2, NULL, NULL, NULL);
	if (printer != NULL){
		multiColumnPrinter_set_title(printer, 0, (char*)"REGISTER");
		multiColumnPrinter_set_title(printer, 1, (char*)"VALUE");

		multiColumnPrinter_print_header(printer);

		for (i = 0; i < nb_reg_access; i++){
			switch(reg_access[i].size){
			case 1 	: {snprintf(value_str, 20, "%02x", reg_access[i].value & 0x000000ff); break;}
			case 2 	: {snprintf(value_str, 20, "%04x", reg_access[i].value & 0x0000ffff); break;}
			case 4 	: {snprintf(value_str, 20, "%08x", reg_access[i].value & 0xffffffff); break;}
			default : {printf("WARNING: in %s, unexpected data size\n", __func__); break;}
			}

			multiColumnPrinter_print(printer, reg_2_string(reg_access[i].reg), value_str, NULL);
		}

		multiColumnPrinter_delete(printer);
	}
	else{
		printf("ERROR: in %s, unable to create multi column printer\n", __func__);
	}
}

int32_t regAccess_extract_arg_large_pure_read(struct array* array, struct regAccess* reg_access, int nb_reg_access){
	uint32_t 			i;
	uint8_t 			j;
	uint8_t 			k;
	uint8_t 			nb_large_access;
	struct regAccess**	large_reg_access;
	uint32_t 			nb_argBuffer;
	struct argBuffer 	arg;

	if (nb_reg_access > 0){
		large_reg_access = (struct regAccess**)alloca(sizeof(struct regAccess*) * nb_reg_access);

		for (i = 0, nb_large_access = 0; i < (uint32_t)nb_reg_access; i++){
			if (reg_access[i].size == 4){
				large_reg_access[nb_large_access] = reg_access + i;
				nb_large_access ++;
			}
		}

		if (nb_large_access > 0){
			nb_argBuffer = 0x00000001 << nb_large_access;
			for (i = 1; i < nb_argBuffer; i++){
				uint8_t 	nb_register = __builtin_popcount(i);
				uint8_t* 	permutation;
				PERMUTATION_INIT(nb_register)

				arg.location_type 		= ARG_LOCATION_REGISTER;
				#pragma GCC diagnostic ignored "-Wlong-long" /* ISO C90 does not support long long integer constant and pragma in macro */
				ARGBUFFER_SET_NB_REG(arg.reg, nb_register);
				arg.size 				= nb_register * 4;
				arg.access_size 		= 4;

				PERMUTATION_GET_FIRST(permutation)
				while(permutation != NULL){
					arg.data = (char*)malloc(arg.size);
					if (arg.data == NULL){
						printf("ERROR: in %s, unable to allocate memory\n", __func__);
						return -1;
					}

					for (j = 0, k = 0; j < nb_large_access; j++){
						if ((i >> j) & 0x00000001){
							*((uint32_t*)arg.data + permutation[k]) = large_reg_access[j]->value;
							#pragma GCC diagnostic ignored "-Wlong-long" /* ISO C90 does not support long long integer constant and pragma in macro */
							ARGBUFFER_SET_REG_NAME(arg.reg, permutation[k], large_reg_access[j]->reg);
							k++;
						}
					}

					if (array_add(array, &arg) < 0){
						printf("ERROR: in %s, unable to add element to array structure\n", __func__);
					}

					PERMUTATION_GET_NEXT(permutation)
				}
				PERMUTATION_CLEAN()
			}
		}
	}
	
	return 0;
}

int32_t regAccess_extract_arg_large_pure_write(struct array* array, struct regAccess* reg_access, int nb_reg_access){
	uint32_t 			i;
	uint8_t 			nb_large_access;
	struct regAccess**	large_reg_access;
	struct argBuffer 	arg;

	if (nb_reg_access > 0){
		large_reg_access = (struct regAccess**)alloca(sizeof(struct regAccess*) * nb_reg_access);

		for (i = 0, nb_large_access = 0; i < (uint32_t)nb_reg_access; i++){
			if (reg_access[i].size == 4){
				large_reg_access[nb_large_access] = reg_access + i;
				nb_large_access ++;
			}
		}

		if (nb_large_access > 0){
			uint8_t 	nb_register = nb_large_access;
			uint8_t* 	permutation;
			PERMUTATION_INIT(nb_register)

			arg.location_type 		= ARG_LOCATION_REGISTER;
			#pragma GCC diagnostic ignored "-Wlong-long" /* ISO C90 does not support long long integer constant and pragma in macro */
			ARGBUFFER_SET_NB_REG(arg.reg, nb_register);
			arg.size 				= nb_register * 4;
			arg.access_size 		= 4;

			PERMUTATION_GET_FIRST(permutation)
			while(permutation != NULL){
				arg.data = (char*)malloc(arg.size);
				if (arg.data == NULL){
					printf("ERROR: in %s, unable to allocate memory\n", __func__);
					return -1;
				}

				for (i = 0; i < nb_register; i++){
					*((uint32_t*)arg.data + permutation[i]) = large_reg_access[i]->value;
					#pragma GCC diagnostic ignored "-Wlong-long" /* ISO C90 does not support long long integer constant and pragma in macro */
					ARGBUFFER_SET_REG_NAME(arg.reg, permutation[i], large_reg_access[i]->reg);
				}

				if (array_add(array, &arg) < 0){
					printf("ERROR: in %s, unable to add element to array structure\n", __func__);
				}

				PERMUTATION_GET_NEXT(permutation)
			}
			PERMUTATION_CLEAN()
		}
	}
	
	return 0;
}

/* Warning this routine must be called after memory extraction. The array must contain memory arguments */
int32_t regAccess_extract_arg_large_mix_read(struct array* array, struct regAccess* reg_access, int nb_reg_access){
	uint32_t 			i;
	uint8_t 			j;
	uint8_t 			k;
	uint32_t 			l;
	uint8_t 			nb_large_access;
	struct regAccess**	large_reg_access;
	uint32_t 			nb_argBuffer;
	struct argBuffer 	arg;
	uint32_t 			nb_mem;
	struct argBuffer* 	arg_mem;

	if (nb_reg_access > 0){
		large_reg_access = (struct regAccess**)alloca(sizeof(struct regAccess*) * nb_reg_access);

		for (i = 0, nb_large_access = 0; i < (uint32_t)nb_reg_access; i++){
			if (reg_access[i].size == 4){
				large_reg_access[nb_large_access] = reg_access + i;
				nb_large_access ++;
			}
		}

		if (nb_large_access > 0){
			nb_mem = array_get_length(array);
			nb_argBuffer = 0x00000001 << (nb_large_access + ((nb_mem == 0)?0:1));
			for (i = 1; i < nb_argBuffer; i++){
				uint8_t 	nb_element = __builtin_popcount(i);
				uint8_t* 	permutation;
				PERMUTATION_INIT(nb_element)

				if ((i >> nb_large_access) & 0x00000001){
					if (nb_element != 1){
						arg.location_type 		= ARG_LOCATION_MIX;
						#pragma GCC diagnostic ignored "-Wlong-long" /* ISO C90 does not support long long integer constant and pragma in macro */
						ARGBUFFER_SET_NB_REG(arg.reg, nb_element);
						arg.access_size 		= 4;

						PERMUTATION_GET_FIRST(permutation)
						while(permutation != NULL){
							for (l = 0; l < nb_mem; l++){
								arg_mem = (struct argBuffer*)array_get(array, l);
								if (arg_mem->access_size == 4){
									arg.address = arg_mem->address;
									arg.size  	= (nb_element - 1) * 4 + arg_mem->size;

									arg.data = (char*)malloc(arg.size);
									if (arg.data == NULL){
										printf("ERROR: in %s, unable to allocate memory\n", __func__);
										return -1;
									}

									for (j = 0, k = 0; j < nb_large_access; j++){
										if ((i >> j) & 0x00000001){
											if (permutation[k] < permutation[nb_element - 1]){
												*((uint32_t*)arg.data + permutation[k]) = large_reg_access[j]->value;
											}
											else{
												*((uint32_t*)(arg.data + arg_mem->size) + (permutation[k] - 1)) = large_reg_access[j]->value;
											}
											#pragma GCC diagnostic ignored "-Wlong-long" /* ISO C90 does not support long long integer constant and pragma in macro */
											ARGBUFFER_SET_REG_NAME(arg.reg, permutation[k], large_reg_access[j]->reg);
											k++;
										}
									}
									memcpy(arg.data + permutation[k] * 4, arg_mem->data, arg_mem->size);
									#pragma GCC diagnostic ignored "-Wlong-long" /* ISO C90 does not support long long integer constant and pragma in macro */
									ARGBUFFER_SET_REG_NAME(arg.reg, permutation[k], ARGBUFFER_MEM_SLOT);

									if (array_add(array, &arg) < 0){
										printf("ERROR: in %s, unable to add element to array structure\n", __func__);
									}
								}
							}
							PERMUTATION_GET_NEXT(permutation)
						}
					}
				}
				else{
					arg.location_type 		= ARG_LOCATION_REGISTER;
					#pragma GCC diagnostic ignored "-Wlong-long" /* ISO C90 does not support long long integer constant and pragma in macro */
					ARGBUFFER_SET_NB_REG(arg.reg, nb_element);
					arg.size 				= nb_element * 4;
					arg.access_size 		= 4;

					PERMUTATION_GET_FIRST(permutation)
					while(permutation != NULL){
						arg.data = (char*)malloc(arg.size);
						if (arg.data == NULL){
							printf("ERROR: in %s, unable to allocate memory\n", __func__);
							return -1;
						}

						for (j = 0, k = 0; j < nb_large_access; j++){
							if ((i >> j) & 0x00000001){
								*((uint32_t*)arg.data + permutation[k]) = large_reg_access[j]->value;
								#pragma GCC diagnostic ignored "-Wlong-long" /* ISO C90 does not support long long integer constant and pragma in macro */
								ARGBUFFER_SET_REG_NAME(arg.reg, permutation[k], large_reg_access[j]->reg);
								k++;
							}
						}

						if (array_add(array, &arg) < 0){
							printf("ERROR: in %s, unable to add element to array structure\n", __func__);
						}

						PERMUTATION_GET_NEXT(permutation)
					}
				}
				PERMUTATION_CLEAN()
			}
		}
	}

	return 0;
}

/* Warning this routine must be called after memory extraction. The array must contain memory arguments */
int32_t regAccess_extract_arg_large_mix_write(struct array* array, struct regAccess* reg_access, int nb_reg_access){
	uint32_t 			i;
	uint32_t 			l;
	uint8_t 			nb_large_access;
	struct regAccess**	large_reg_access;
	struct argBuffer 	arg;
	struct argBuffer* 	arg_mem;

	if (nb_reg_access > 0){
		large_reg_access = (struct regAccess**)alloca(sizeof(struct regAccess*) * nb_reg_access);

		for (i = 0, nb_large_access = 0; i < (uint32_t)nb_reg_access; i++){
			if (reg_access[i].size == 4){
				large_reg_access[nb_large_access] = reg_access + i;
				nb_large_access ++;
			}
		}

		if (nb_large_access > 0){
			uint32_t 	nb_mem = array_get_length(array);
			uint8_t 	nb_element = nb_large_access + ((nb_mem == 0)?0:1);
			uint8_t* 	permutation;
			PERMUTATION_INIT(nb_element)

			arg.location_type 		= ARG_LOCATION_MIX;
			#pragma GCC diagnostic ignored "-Wlong-long" /* ISO C90 does not support long long integer constant and pragma in macro */
			ARGBUFFER_SET_NB_REG(arg.reg, nb_element);
			arg.access_size 		= 4;

			PERMUTATION_GET_FIRST(permutation)
			while(permutation != NULL){
				for (l = 0; l < nb_mem; l++){
					arg_mem = (struct argBuffer*)array_get(array, l);
					if (arg_mem->access_size == 4){
						arg.address = arg_mem->address;
						arg.size  	= (nb_element - 1) * 4 + arg_mem->size;

						arg.data = (char*)malloc(arg.size);
						if (arg.data == NULL){
							printf("ERROR: in %s, unable to allocate memory\n", __func__);
							return -1;
						}

						for (i = 0; i < nb_large_access; i++){
							if (permutation[i] < permutation[nb_element - 1]){
								*((uint32_t*)arg.data + permutation[i]) = large_reg_access[i]->value;
							}
							else{
								*((uint32_t*)(arg.data + arg_mem->size) + (permutation[i] - 1)) = large_reg_access[i]->value;
							}
							#pragma GCC diagnostic ignored "-Wlong-long" /* ISO C90 does not support long long integer constant and pragma in macro */
							ARGBUFFER_SET_REG_NAME(arg.reg, permutation[i], large_reg_access[i]->reg);
						}
							
						memcpy(arg.data + permutation[nb_large_access] * 4, arg_mem->data, arg_mem->size);
						#pragma GCC diagnostic ignored "-Wlong-long" /* ISO C90 does not support long long integer constant and pragma in macro */
						ARGBUFFER_SET_REG_NAME(arg.reg, permutation[nb_large_access], ARGBUFFER_MEM_SLOT);

						if (array_add(array, &arg) < 0){
							printf("ERROR: in %s, unable to add element to array structure\n", __func__);
						}
					}
				}
				PERMUTATION_GET_NEXT(permutation)
			}
			PERMUTATION_CLEAN()
		}
	}

	return 0;
}