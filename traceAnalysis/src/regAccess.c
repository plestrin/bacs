#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "regAccess.h"
#include "argument.h"
#include "multiColumn.h"
#include "permutation.h"

int regAccess_compare_order(const void* reg_access1, const void* reg_access2);


void regAccess_print(struct regAccess* reg_access, int nb_reg_access){
	struct multiColumnPrinter* 	printer;
	int 						i;
	char 						value_str[20];

	printer = multiColumnPrinter_create(stdout, 3, NULL, NULL, NULL);
	if (printer != NULL){
		multiColumnPrinter_set_title(printer, 0, (char*)"REGISTER");
		multiColumnPrinter_set_title(printer, 1, (char*)"VALUE");
		multiColumnPrinter_set_title(printer, 2, (char*)"ORDER");

		multiColumnPrinter_set_column_type(printer, 2, MULTICOLUMN_TYPE_UINT32);
		multiColumnPrinter_set_column_size(printer, 2, 5);

		multiColumnPrinter_print_header(printer);

		for (i = 0; i < nb_reg_access; i++){
			switch(reg_access[i].size){
				case 1 	: {snprintf(value_str, 20, "%02x", reg_access[i].value & 0x000000ff); break;}
				case 2 	: {snprintf(value_str, 20, "%04x", reg_access[i].value & 0x0000ffff); break;}
				case 4 	: {snprintf(value_str, 20, "%08x", reg_access[i].value & 0xffffffff); break;}
				default : {printf("WARNING: in %s, unexpected data size\n", __func__); break;}
			}

			multiColumnPrinter_print(printer, reg_2_string(reg_access[i].reg), value_str, reg_access[i].order, NULL);
		}

		multiColumnPrinter_delete(printer);
	}
	else{
		printf("ERROR: in %s, unable to create multi column printer\n", __func__);
	}
}

void regAccess_propagate_read(struct regAccess* reg_access, int nb_reg_access){
	uint32_t i;
	uint32_t eax_mask 	= 0xffffffff;
	uint32_t ebx_mask 	= 0xffffffff;
	uint32_t ecx_mask 	= 0xffffffff;
	uint32_t edx_mask 	= 0xffffffff;
	uint32_t eax_value 	= 0x00000000;
	uint32_t ebx_value 	= 0x00000000;
	uint32_t ecx_value 	= 0x00000000;
	uint32_t edx_value 	= 0x00000000;

	qsort(reg_access, nb_reg_access, sizeof(struct regAccess), regAccess_compare_order);

	for (i = 0; i < (uint32_t)nb_reg_access; i++){
		switch(reg_access[i].reg){
			case REGISTER_EAX 	 : {
				reg_access[i].value = (reg_access[i].value & eax_mask) | eax_value;
				break;
			}
			case REGISTER_AX  	 : {
				eax_mask &= 0xffff0000;
				eax_value = (eax_value & 0xffff0000) | (reg_access[i].value & 0x0000ffff);
				break;
			}
			case REGISTER_AH  	 : {
				eax_mask &= 0xffff00ff;
				eax_value = (eax_value & 0xffff00ff) | ((reg_access[i].value & 0x000000ff) << 8);
				break;
			}
			case REGISTER_AL  	 : {
				eax_mask &= 0xffffff00;
				eax_value = (eax_value & 0xffffff00) | (reg_access[i].value & 0x000000ff);
				break;
			}
			case REGISTER_EBX 	 : {
				reg_access[i].value = (reg_access[i].value & ebx_mask) | ebx_value;
				break;
			}
			case REGISTER_BX  	 : {
				ebx_mask &= 0xffff0000;
				ebx_value = (ebx_value & 0xffff0000) | (reg_access[i].value & 0x0000ffff);
				break;
			}
			case REGISTER_BH  	 : {
				ebx_mask &= 0xffff00ff;
				ebx_value = (ebx_value & 0xffff00ff) | ((reg_access[i].value & 0x000000ff) << 8);
				break;
			}
			case REGISTER_BL  	 : {
				ebx_mask &= 0xffffff00;
				ebx_value = (ebx_value & 0xffffff00) | (reg_access[i].value & 0x000000ff);
				break;
			}
			case REGISTER_ECX 	 : {
				reg_access[i].value = (reg_access[i].value & ecx_mask) | ecx_value;
				break;
			}
			case REGISTER_CX  	 : {
				ecx_mask &= 0xffff0000;
				ecx_value = (ecx_value & 0xffff0000) | (reg_access[i].value & 0x0000ffff);
				break;
			}
			case REGISTER_CH  	 : {
				ecx_mask &= 0xffff00ff;
				ecx_value = (ecx_value & 0xffff00ff) | ((reg_access[i].value & 0x000000ff) << 8);
				break;
			}
			case REGISTER_CL  	 : {
				ecx_mask &= 0xffffff00;
				ecx_value = (ecx_value & 0xffffff00) | (reg_access[i].value & 0x000000ff);
				break;
			}
			case REGISTER_EDX 	 : {
				reg_access[i].value = (reg_access[i].value & edx_mask) | edx_value;
				break;
			}
			case REGISTER_DX  	 : {
				edx_mask &= 0xffff0000;
				edx_value = (edx_value & 0xffff0000) | (reg_access[i].value & 0x0000ffff);
				break;
			}
			case REGISTER_DH  	 : {
				edx_mask &= 0xffff00ff;
				edx_value = (edx_value & 0xffff00ff) | ((reg_access[i].value & 0x000000ff) << 8);
				break;
			}
			case REGISTER_DL  	 : {
				edx_mask &= 0xffffff00;
				edx_value = (edx_value & 0xffffff00) | (reg_access[i].value & 0x000000ff);
				break;
			}
			default 			 : {break;}
		}
	}
}

void regAccess_propagate_write(struct regAccess* reg_access, int nb_reg_access){
	int32_t i;
	uint32_t eax_mask 	= 0xffffffff;
	uint32_t ebx_mask 	= 0xffffffff;
	uint32_t ecx_mask 	= 0xffffffff;
	uint32_t edx_mask 	= 0xffffffff;
	uint32_t eax_value 	= 0x00000000;
	uint32_t ebx_value 	= 0x00000000;
	uint32_t ecx_value 	= 0x00000000;
	uint32_t edx_value 	= 0x00000000;

	qsort(reg_access, nb_reg_access, sizeof(struct regAccess), regAccess_compare_order);

	for (i = nb_reg_access - 1; i >= 0; i--){
		switch(reg_access[i].reg){
			case REGISTER_EAX 	 : {
				reg_access[i].value = (reg_access[i].value & eax_mask) | eax_value;
				break;
			}
			case REGISTER_AX  	 : {
				eax_mask &= 0xffff0000;
				eax_value = (eax_value & 0xffff0000) | (reg_access[i].value & 0x0000ffff);
				break;
			}
			case REGISTER_AH  	 : {
				eax_mask &= 0xffff00ff;
				eax_value = (eax_value & 0xffff00ff) | ((reg_access[i].value & 0x000000ff) << 8);
				break;
			}
			case REGISTER_AL  	 : {
				eax_mask &= 0xffffff00;
				eax_value = (eax_value & 0xffffff00) | (reg_access[i].value & 0x000000ff);
				break;
			}
			case REGISTER_EBX 	 : {
				reg_access[i].value = (reg_access[i].value & ebx_mask) | ebx_value;
				break;
			}
			case REGISTER_BX  	 : {
				ebx_mask &= 0xffff0000;
				ebx_value = (ebx_value & 0xffff0000) | (reg_access[i].value & 0x0000ffff);
				break;
			}
			case REGISTER_BH  	 : {
				ebx_mask &= 0xffff00ff;
				ebx_value = (ebx_value & 0xffff00ff) | ((reg_access[i].value & 0x000000ff) << 8);
				break;
			}
			case REGISTER_BL  	 : {
				ebx_mask &= 0xffffff00;
				ebx_value = (ebx_value & 0xffffff00) | (reg_access[i].value & 0x000000ff);
				break;
			}
			case REGISTER_ECX 	 : {
				reg_access[i].value = (reg_access[i].value & ecx_mask) | ecx_value;
				break;
			}
			case REGISTER_CX  	 : {
				ecx_mask &= 0xffff0000;
				ecx_value = (ecx_value & 0xffff0000) | (reg_access[i].value & 0x0000ffff);
				break;
			}
			case REGISTER_CH  	 : {
				ecx_mask &= 0xffff00ff;
				ecx_value = (ecx_value & 0xffff00ff) | ((reg_access[i].value & 0x000000ff) << 8);
				break;
			}
			case REGISTER_CL  	 : {
				ecx_mask &= 0xffffff00;
				ecx_value = (ecx_value & 0xffffff00) | (reg_access[i].value & 0x000000ff);
				break;
			}
			case REGISTER_EDX 	 : {
				reg_access[i].value = (reg_access[i].value & edx_mask) | edx_value;
				break;
			}
			case REGISTER_DX  	 : {
				edx_mask &= 0xffff0000;
				edx_value = (edx_value & 0xffff0000) | (reg_access[i].value & 0x0000ffff);
				break;
			}
			case REGISTER_DH  	 : {
				edx_mask &= 0xffff00ff;
				edx_value = (edx_value & 0xffff00ff) | ((reg_access[i].value & 0x000000ff) << 8);
				break;
			}
			case REGISTER_DL  	 : {
				edx_mask &= 0xffffff00;
				edx_value = (edx_value & 0xffffff00) | (reg_access[i].value & 0x000000ff);
				break;
			}
			default 			 : {break;}
		}
	}
}

int32_t regAccess_extract_arg_large_pure_read(struct argSet* set, struct regAccess* reg_access, int nb_reg_access){
	uint32_t 				i;
	uint8_t 				j;
	uint8_t 				k;
	uint8_t 				nb_large_access;
	struct regAccess**		large_reg_access;
	uint32_t 				nb_argBuffer;
	struct inputArgument 	arg;

	if (nb_reg_access > 0){
		large_reg_access = (struct regAccess**)alloca(sizeof(struct regAccess*) * nb_reg_access);

		for (i = 0, nb_large_access = 0; i < (uint32_t)nb_reg_access; i++){
			if (reg_access[i].size == 4){
				large_reg_access[nb_large_access] = reg_access + i;
				nb_large_access ++;
			}
		}

		if (nb_large_access > 0){
			if (nb_large_access <= REGACCESS_MAX_NB_BRUTE_FORCE){
				nb_argBuffer = 0x00000001 << nb_large_access;
				for (i = 1; i < nb_argBuffer; i++){
					uint8_t 	nb_register = __builtin_popcount(i);
					uint8_t* 	permutation;

					PERMUTATION_CREATE(nb_register, malloc)

					PERMUTATION_GET_FIRST(permutation)
					while(permutation != NULL){
						if (inputArgument_init(&arg, nb_register * 4, nb_register, 4)){
							printf("ERROR: in %s, unable to init input argument\n", __func__);
							return -1;
						}

						for (j = 0, k = 0; j < nb_large_access; j++){
							if ((i >> j) & 0x00000001){
								*((uint32_t*)arg.data + permutation[k]) = large_reg_access[j]->value;
								arg.desc[permutation[k]].type 			= ARGFRAG_REG;
								arg.desc[permutation[k]].location.reg 	= large_reg_access[j]->reg;
								arg.desc[permutation[k]].size 			= 4;
								k++;
							}
						}

						if (argSet_add_input(set, &arg) < 0){
							printf("ERROR: in %s, unable to add element to array structure\n", __func__);
							inputArgument_clean(&arg);
						}

						PERMUTATION_GET_NEXT(permutation)
					}
					PERMUTATION_DELETE()
				}
			}
			else{
				printf("WARNING: in %s, the number of register is too high (%u) - brute force is disabled: \n", __func__, nb_large_access);
			}
		}
	}
	
	return 0;
}

/* Warning this routine must be called after memory extraction. The array must contain memory arguments */
int32_t regAccess_extract_arg_large_mix_read(struct argSet* set, struct regAccess* reg_access, int nb_reg_access){
	uint32_t 				i;
	uint8_t 				j;
	uint8_t 				k;
	uint32_t 				l;
	uint8_t 				nb_large_access;
	struct regAccess**		large_reg_access;
	uint32_t 				nb_argBuffer;
	struct inputArgument 	arg;
	uint32_t 				nb_mem;
	struct inputArgument* 	arg_mem;

	if (nb_reg_access > 0){
		large_reg_access = (struct regAccess**)alloca(sizeof(struct regAccess*) * nb_reg_access);

		for (i = 0, nb_large_access = 0; i < (uint32_t)nb_reg_access; i++){
			if (reg_access[i].size == 4){
				large_reg_access[nb_large_access] = reg_access + i;
				nb_large_access ++;
			}
		}

		if (nb_large_access > 0){
			if (nb_large_access + 1 <= REGACCESS_MAX_NB_BRUTE_FORCE){
				nb_mem = argSet_get_nb_input(set);
				nb_argBuffer = 0x00000001 << (nb_large_access + ((nb_mem == 0)?0:1));
				for (i = 1; i < nb_argBuffer; i++){
					uint8_t 	nb_element = __builtin_popcount(i);
					uint8_t* 	permutation;
					PERMUTATION_CREATE(nb_element, malloc)

					if ((i >> nb_large_access) & 0x00000001){
						if (nb_element != 1){

							PERMUTATION_GET_FIRST(permutation)
							while(permutation != NULL){
								for (l = 0; l < nb_mem; l++){
									arg_mem = (struct inputArgument*)array_get(set->input, l);
									if (arg_mem->access_size == 4){
										if (inputArgument_init(&arg, (nb_element - 1) * 4 + arg_mem->size, nb_element, 4)){
											printf("ERROR: in %s, unable to init input argument\n", __func__);
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

												arg.desc[permutation[k]].type 			= ARGFRAG_REG;
												arg.desc[permutation[k]].location.reg 	= large_reg_access[j]->reg;
												arg.desc[permutation[k]].size 			= 4;
												k++;
											}
										}
										memcpy(arg.data + permutation[k] * 4, arg_mem->data, arg_mem->size);

										arg.desc[permutation[k]].type 				= ARGFRAG_MEM;
										arg.desc[permutation[k]].location.address 	= arg_mem->desc[0].location.address;
										arg.desc[permutation[k]].size 				= arg_mem->size;

										if (argSet_add_input(set, &arg) < 0){
											printf("ERROR: in %s, unable to add element to array structure\n", __func__);
											inputArgument_clean(&arg);
										}
									}
								}
								PERMUTATION_GET_NEXT(permutation)
							}
						}
					}
					else{
						PERMUTATION_GET_FIRST(permutation)
						while(permutation != NULL){
							if (inputArgument_init(&arg, nb_element * 4, nb_element, 4)){
								printf("ERROR: in %s, unable to init input argument\n", __func__);
								return -1;
							}

							for (j = 0, k = 0; j < nb_large_access; j++){
								if ((i >> j) & 0x00000001){
									*((uint32_t*)arg.data + permutation[k]) = large_reg_access[j]->value;
									arg.desc[permutation[k]].type 			= ARGFRAG_REG;
									arg.desc[permutation[k]].location.reg 	= large_reg_access[j]->reg;
									arg.desc[permutation[k]].size 			= 4;
									k++;
								}
							}

							if (argSet_add_input(set, &arg) < 0){
								printf("ERROR: in %s, unable to add element to array structure\n", __func__);
								inputArgument_clean(&arg);
							}

							PERMUTATION_GET_NEXT(permutation)
						}
					}
					PERMUTATION_DELETE()
				}
			}
			else{
				printf("WARNING: in %s, the number of element is too high (%u) - downgrading to LP\n", __func__, nb_large_access + 1);
				if (regAccess_extract_arg_large_pure_read(set, reg_access, nb_reg_access)){
					printf("ERROR: in %s, downgrading LP routine failed\n", __func__);
				}
			}
		}
	}

	return 0;
}

int32_t regAccess_extract_arg_large_write(struct argSet* set, struct regAccess* reg_access, int nb_reg_access){
	uint32_t 				i;
	struct outputArgument 	arg;

	for (i = 0; i < (uint32_t)nb_reg_access; i++){
		if (reg_access[i].size == 4){
			arg.desc.type 			= ARGFRAG_REG;
			arg.desc.location.reg 	= reg_access[i].reg;
			arg.desc.size 			= 4;

			memcpy(&(arg.data), &(reg_access[i].value), 4);

			if (array_add(set->output, &arg) < 0){
				printf("ERROR: in %s, unable to add element to array structure\n", __func__);
			}
		}
	}

	return 0;
}

/* ===================================================================== */
/* Comparison functions 	                                         	 */
/* ===================================================================== */

int regAccess_compare_order(const void* reg_access1, const void* reg_access2){
	return (int32_t)(((struct regAccess*)reg_access1)->order - ((struct regAccess*)reg_access2)->order);
}