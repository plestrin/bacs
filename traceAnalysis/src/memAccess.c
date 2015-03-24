#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "memAccess.h"
#include "argument.h"
#include "instruction.h"
#include "traceFragment.h"
#include "multiColumn.h"

int memAccess_compare_address_then_order(const void* mem_access1, const void* mem_access2); 		/* Order memAccess array in address order and in order order as a second option */
int memAccess_compare_address_then_inv_order(const void* mem_access1, const void* mem_access2); 	/* Order memAccess array in address order and in inv order order as a second option */

void memAccess_recursive_arg_grouping(struct memAccess* mem_access, uint32_t nb_element, uint32_t remove_id, uint32_t new_id);
void memAccess_extract_group(struct argSet* set, struct memAccess* mem_access, int32_t nb_mem_access, uint32_t max_nb_group);


void memAccess_print(struct memAccess* mem_access, int nb_mem_access){
	struct multiColumnPrinter* 	printer;
	int 						i;
	char 						value_str[20];

	printer = multiColumnPrinter_create(stdout, 6, NULL, NULL, NULL);
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
		multiColumnPrinter_set_column_type(printer, 5, MULTICOLUMN_TYPE_UINT32);

		multiColumnPrinter_set_title(printer, 0, (char*)"ORDER");
		multiColumnPrinter_set_title(printer, 1, (char*)"VALUE");
		multiColumnPrinter_set_title(printer, 2, (char*)"ADDRESS");
		multiColumnPrinter_set_title(printer, 3, (char*)"SIZE");
		multiColumnPrinter_set_title(printer, 4, (char*)"OPCODE");
		multiColumnPrinter_set_title(printer, 5, (char*)"GROUP");

		multiColumnPrinter_print_header(printer);

		for (i = 0; i < nb_mem_access; i++){
			switch(mem_access[i].size){
			case 1 	: {snprintf(value_str, 20, "%02x", mem_access[i].value & 0x000000ff); break;}
			case 2 	: {snprintf(value_str, 20, "%04x", mem_access[i].value & 0x0000ffff); break;}
			case 4 	: {snprintf(value_str, 20, "%08x", mem_access[i].value & 0xffffffff); break;}
			default : {printf("WARNING: in %s, unexpected data size\n", __func__); break;}
			}

			multiColumnPrinter_print(printer, mem_access[i].order, value_str, mem_access[i].address, mem_access[i].size, xed_iclass_enum_t2str(mem_access[i].opcode), mem_access[i].group, NULL);
		}

		multiColumnPrinter_delete(printer);
	}
	else{
		printf("ERROR: in %s, unable to create multi column printer\n", __func__);
	}
}

/* we may the use the group interface correctly for this problem */
#pragma GCC diagnostic ignored "-Wunused-parameter" /* Due to the interface format */
int32_t memAccess_extract_arg_adjacent_size_read(struct argSet* set, struct memAccess* mem_access, int nb_mem_access, void* frag){
	int 					i;
	int 					j;
	int 					k;
	struct inputArgument 	arg;
	uint32_t				size;

	if (mem_access != NULL && nb_mem_access > 0){
		qsort(mem_access, nb_mem_access, sizeof(struct memAccess), memAccess_compare_address_then_order);

		for (i = 0; i < nb_mem_access; i++){
			if (mem_access[i].group == 0){
				mem_access[i].group = 1;
				size = mem_access[i].size;
				for (j = i + 1; j < nb_mem_access; j++){
					if ((mem_access[j].group == 0) && (mem_access[j].size == mem_access[i].size)){
						if (mem_access[j].address == mem_access[i].address + size){
							size += mem_access[j].size;
							mem_access[j].group = 1;
						}
						else if (mem_access[j].address > mem_access[i].address + size){
							break;
						}
						else{
							mem_access[j].group = 2;
						}
					}
				}

				if (inputArgument_init(&arg, size, 1, mem_access[i].size)){
					printf("ERROR: in %s, unable to init inut argument\n", __func__);
					return -1;
				}
				
				arg.desc[0].type 				= ARGFRAG_MEM;
				arg.desc[0].location.address 	= mem_access[i].address;
				arg.desc[0].size 				= size;

				for (k = i, size = 0; k < j; k++){
					if (mem_access[k].group == 1){
						memcpy(arg.data + size, &(mem_access[k].value), mem_access[i].size);
						size += mem_access[i].size;
						mem_access[k].group = 2;
					}
				}

				if (argSet_add_input(set, &arg) < 0){
					printf("ERROR: in %s, unable to add element to array structure\n", __func__);
					inputArgument_clean(&arg);
				}
			}
		}
	}

	return 0;
}

/* we may use the group interface correctly for this problem */
#pragma GCC diagnostic ignored "-Wunused-parameter" /* Due to the interface format */
int32_t memAccess_extract_arg_adjacent_size_opcode_read(struct argSet* set, struct memAccess* mem_access, int nb_mem_access, void* frag){
	int 					i;
	int 					j;
	int 					k;
	struct inputArgument 	arg;
	uint32_t				size;
	
	if (mem_access != NULL && nb_mem_access > 0){
		qsort(mem_access, nb_mem_access, sizeof(struct memAccess), memAccess_compare_address_then_order);

		for (i = 0; i < nb_mem_access; i++){
			if (mem_access[i].group == 0){
				mem_access[i].group = 1;
				size = mem_access[i].size;
				for (j = i + 1; j < nb_mem_access; j++){
					if ((mem_access[j].group == 0) && (mem_access[j].size == mem_access[i].size) && (mem_access[j].opcode == mem_access[i].opcode)){
						if (mem_access[j].address == mem_access[i].address + size){
							size += mem_access[j].size;
							mem_access[j].group = 1;
						}
						else if (mem_access[j].address > mem_access[i].address + size){
							break;
						}
						else{
							mem_access[j].group = 2;
						}
					}
				}

				if (inputArgument_init(&arg, size, 1, mem_access[i].size)){
					printf("ERROR: in %s, unable to init inut argument\n", __func__);
					return -1;
				}
				
				arg.desc[0].type 				= ARGFRAG_MEM;
				arg.desc[0].location.address 	= mem_access[i].address;
				arg.desc[0].size 				= size;

				for (k = i, size = 0; k < j; k++){
					if (mem_access[k].group == 1){
						memcpy(arg.data + size, &(mem_access[k].value), mem_access[i].size);
						size += mem_access[i].size;
						mem_access[k].group = 2;
					}
				}

				if (argSet_add_input(set, &arg) < 0){
					printf("ERROR: in %s, unable to add element to array structure\n", __func__);
					inputArgument_clean(&arg);
				}
			}
		}
	}

	return 0;
}

void memAccess_recursive_arg_grouping(struct memAccess* mem_access, uint32_t nb_element, uint32_t remove_id, uint32_t new_id){
	uint32_t i;
	uint32_t min_index = nb_element;
	uint32_t max_index = 0;
	
	for (i = 0; i< nb_element; i++){
		if (mem_access[i].group == remove_id){
			mem_access[i].group = new_id;
			min_index = (min_index == nb_element)?i:min_index;
			max_index = i;
		}
		else if (mem_access[i].group == new_id){
			min_index = (min_index == nb_element)?i:min_index;
			max_index = i;
		}
	}

	for (i = min_index + 1; i < max_index; i++){
		if (mem_access[i].group != new_id){
			memAccess_recursive_arg_grouping(mem_access, nb_element, mem_access[i].group, new_id);
		}
	}
}

int32_t memAccess_extract_arg_loop_adjacent_size_opcode_read(struct argSet* set, struct memAccess* mem_access, int nb_mem_access, void* frag){
	uint32_t 			group_id_generator = 1;
	int 				i;
	int 				j;
	uint32_t			size;
	uint32_t 			loop_length_ins;
	uint32_t 			loop_length_op;

	if (((struct traceFragment*)frag)->type == TRACEFRAGMENT_TYPE_LOOP){
		loop_length_ins = (uint32_t)((struct traceFragment*)frag)->specific_data;
		loop_length_op = ((struct traceFragment*)frag)->trace.instructions[loop_length_ins].operand_offset;
	}
	else{
		printf("ERROR: in %s, extraction routine expect a loop fragment\n", __func__);
		return -1;
	}

	if (mem_access != NULL && nb_mem_access > 0){
		qsort(mem_access, nb_mem_access, sizeof(struct memAccess), memAccess_compare_address_then_order);

		/* STEP 1: regroup using adj size and opcode */
		for (i = 0; i < nb_mem_access; i++){
			if (mem_access[i].group == 0){
				mem_access[i].group = group_id_generator ++;
				size = mem_access[i].size;
				for (j = i + 1; j < nb_mem_access; j++){
					if ((mem_access[j].group == 0) && (mem_access[j].size == mem_access[i].size) && (mem_access[j].opcode == mem_access[i].opcode)){
						if (mem_access[j].address == mem_access[i].address + size){
							size += mem_access[j].size;
							mem_access[j].group = mem_access[i].group;
						}
						else if (mem_access[j].address > mem_access[i].address + size){
							break;
						}
						else{
							mem_access[j].group = mem_access[i].group;
						}
					}
				}
			}
		}

		/* STEP 2: regroup using order */
		for (i = 0; i < nb_mem_access; i++){
			for (j = i + 1; j < nb_mem_access; j++){
				if (mem_access[i].order % loop_length_op == mem_access[j].order % loop_length_op){
					if (mem_access[i].group != mem_access[j].group){
						memAccess_recursive_arg_grouping(mem_access, nb_mem_access, mem_access[j].group, mem_access[i].group);
					}
					break;
				}
			}
		}

		memAccess_extract_group(set, mem_access, nb_mem_access, group_id_generator);
	}

	return 0;
}

#pragma GCC diagnostic ignored "-Wunused-parameter" /* Due to the interface format */
int32_t memAccess_extract_arg_large_write(struct argSet* set, struct memAccess* mem_access, int nb_mem_access, void* frag){
	int 					i;
	int 					j;
	struct outputArgument 	arg;

	if (mem_access != NULL && nb_mem_access > 0){
		qsort(mem_access, nb_mem_access, sizeof(struct memAccess), memAccess_compare_address_then_inv_order);

		for (i = 0; i < nb_mem_access; i += j){
			j = 1;
			if (mem_access[i].size == 4){
				arg.desc.type 				= ARGFRAG_MEM;
				arg.desc.location.address 	= mem_access[i].address;
				arg.desc.size 				= 4;
				memcpy(&(arg.data), &(mem_access[i].value), 4);

				if (array_add(set->output, &arg) < 0){
					printf("ERROR: in %s, unable to add element to array structure\n", __func__);
					return -1;
				}
				
				while(i + j < nb_mem_access && mem_access[i].address == mem_access[i + j].address){
					j++;
				}
			}
		}
	}

	return 0;
}

void memAccess_extract_group(struct argSet* set, struct memAccess* mem_access, int32_t nb_mem_access, uint32_t max_nb_group){
	int32_t 				i;
	int32_t 				j;
	struct inputArgument 	arg;
	uint32_t				size;
	uint8_t* 				taken;

	taken = (uint8_t*)alloca(max_nb_group);
	memset(taken, 0, max_nb_group);

	for (i = 0; i < nb_mem_access; i++){
		if (mem_access[i].group != MEMACCESS_UNDEF_GROUP && !taken[mem_access[i].group]){
			for (j = i + 1, size = mem_access[i].size; j < nb_mem_access; j++){
				if (mem_access[i].group == mem_access[j].group){
					if (mem_access[j].address > mem_access[i].address + size){
						#ifdef VERBOSE
						#if defined ARCH_32
						printf("WARNING: in %s, found incomplete argument (missing access between 0x%08x and 0x%08x) -> throwing away group %u\n", __func__, mem_access[i].address + size, mem_access[j].address, mem_access[i].group);
						#elif defined ARCH_64
						#pragma GCC diagnostic ignored "-Wformat" /* ISO C90 does not support the ‘ll’ gnu_printf length modifier */
						printf("WARNING: in %s, found incomplete argument (missing access between 0x%llx and 0x%llx) -> throwing away group %u\n", __func__, mem_access[i].address + size, mem_access[j].address, mem_access[i].group);
						#else
						#error Please specify an architecture {ARCH_32 or ARCH_64}
						#endif
						#endif
						taken[mem_access[i].group] = 1;
						break;
					}
					else{
						if (mem_access[j].address + mem_access[j].size > mem_access[i].address + size){
							size = (mem_access[j].address + mem_access[j].size) - mem_access[i].address;
						}
					}
				}
			}

			if (!taken[mem_access[i].group]){
				if (inputArgument_init(&arg, size, 1, mem_access[i].size)){
					printf("ERROR: in %s, unable to init inut argument\n", __func__);
					return;
				}
				
				arg.desc[0].type 				= ARGFRAG_MEM;
				arg.desc[0].location.address 	= mem_access[i].address;
				arg.desc[0].size 				= size;

				for (j = i, size = 0; j < nb_mem_access; j++){
					if (mem_access[i].group == mem_access[j].group){
						if (mem_access[j].address + mem_access[j].size > mem_access[i].address + size){
							memcpy(arg.data + (mem_access[j].address - mem_access[i].address), &(mem_access[j].value), mem_access[j].size);
							size = (mem_access[j].address + mem_access[j].size) - mem_access[i].address;
						}
					}
				}

				if (size != arg.size){
					printf("ERROR: in %s, incorrect size, expect %u but get %u\n", __func__, arg.size, size);
				}

				if (argSet_add_input(set, &arg) < 0){
					printf("ERROR: in %s, unable to add element to array structure\n", __func__);
					inputArgument_clean(&arg);
				}

				taken[mem_access[i].group] = 1;
			}
		}
	}
}


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
