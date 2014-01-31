#include <stdlib.h>
#include <stdio.h>

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

int32_t regAccess_extract_arg_large_pure(struct array* array, struct regAccess* reg_access, int nb_reg_access){
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
				ARGBUFFER_SET_NB_REG(arg.location.reg, nb_register);
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
							ARGBUFFER_SET_REG_NAME(arg.location.reg, permutation[k], large_reg_access[j]->reg);
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