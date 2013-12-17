#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "traceFragment.h"
#include "argBuffer.h"
#include "multiColumn.h"

int memAccess_compare_address_then_order(const void* mem_access1, const void* mem_access2); 		/* Order memAccess array in address order and in order order as a second option */
int memAccess_compare_address_then_lower_order(const void* mem_access1, const void* mem_access2); 	/* Search the given address for a lower order */


struct traceFragment* codeSegment_create(){
	struct traceFragment* frag = (struct traceFragment*)malloc(sizeof(struct traceFragment));

	if (frag != NULL){
		if (traceFragment_init(frag)){
			printf("ERROR: in %s, unable to init traceFragment\n", __func__);
			free(frag);
			frag = NULL;
		}
	}
	else{
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
	}

	return frag;
}

int32_t traceFragment_init(struct traceFragment* frag){
	int32_t result = -1;

	result = array_init(&frag->instruction_array, sizeof(struct instruction));
	if (result){
		printf("ERROR: in %s, unable to init instruction array\n", __func__);
	}
	else{
		frag->read_memory_array 		= NULL;
		frag->write_memory_array 		= NULL;
		frag->nb_memory_read_access 	= 0;
		frag->nb_memory_write_access 	= 0;
	}

	return result;
}

struct instruction* traceFragment_get_last_instruction(struct traceFragment* frag){
	struct instruction* instruction = NULL;

	if (frag != NULL){
		if (array_get_length(&(frag->instruction_array)) > 0){
			instruction = (struct instruction*)array_get(&(frag->instruction_array), array_get_length(&(frag->instruction_array)) - 1);
		}
	}

	return instruction;
}

int32_t traceFragment_clone(struct traceFragment* frag_src, struct traceFragment* frag_dst){
	if (array_clone(&(frag_src->instruction_array), &(frag_dst->instruction_array))){
		printf("ERROR: in %s, unable to clone array\n", __func__);
		return -1;
	}
	
	if (frag_src->read_memory_array != NULL){
		printf("WARNING: in %s, this method does not copy the read_memory_array buffer\n", __func__);
	}
	if (frag_src->write_memory_array != NULL){
		printf("WARNING: in %s, this method does not copy the write_memory_array buffer\n", __func__);
	}

	frag_dst->read_memory_array 		= NULL;
	frag_dst->write_memory_array 		= NULL;
	frag_dst->nb_memory_read_access 	= frag_src->nb_memory_read_access;
	frag_dst->nb_memory_write_access 	= frag_src->nb_memory_write_access;

	return 0;
}

float traceFragment_opcode_percent(struct traceFragment* frag, int nb_opcode, uint32_t* opcode, int nb_excluded_opcode, uint32_t* excluded_opcode){
	float 				result = 0;
	uint32_t 			i;
	struct instruction*	instruction;
	int 				j;
	int 				nb_effective_instruction = 0;
	int 				nb_found_instruction = 0;
	char 				excluded;

	if (frag != NULL){
		for (i = 0; i < array_get_length(&(frag->instruction_array)); i++){
			instruction = (struct instruction*)array_get(&(frag->instruction_array), i);
			excluded = 0;
			if (excluded_opcode != NULL){
				for (j = 0; j < nb_excluded_opcode; j++){
					if (instruction->opcode == excluded_opcode[j]){
						excluded = 1;
						break;
					}
				}
			}

			if (!excluded){
				nb_effective_instruction++;
				if (opcode != NULL){
					for (j = 0; j < nb_opcode; j++){
						if (instruction->opcode == opcode[j]){
							nb_found_instruction ++;
							break;
						}
					}
				}
			}
		}

		result = (float)nb_found_instruction/(float)((nb_effective_instruction == 0)?1:nb_effective_instruction);
	}

	return result;
}

int32_t traceFragment_create_mem_array(struct traceFragment* frag){
	uint32_t 			nb_read_mem 	= 0;
	uint32_t 			nb_write_mem 	= 0;
	int32_t 			result 			= -1;
	uint32_t 			i;
	uint32_t 			j;
	struct instruction* instruction;

	if (frag != NULL){
		for (i = 0; i < array_get_length(&(frag->instruction_array)); i++){
			instruction = (struct instruction*)array_get(&(frag->instruction_array), i);
			for (j = 0; j < INSTRUCTION_MAX_NB_DATA; j++){
				if (INSTRUCTION_DATA_TYPE_IS_VALID(instruction->data[j].type) && INSTRUCTION_DATA_TYPE_IS_MEM(instruction->data[j].type)){
					if (INSTRUCTION_DATA_TYPE_IS_READ(instruction->data[j].type)){
						nb_read_mem ++;
					}
					if (INSTRUCTION_DATA_TYPE_IS_WRITE(instruction->data[j].type)){
						nb_write_mem ++;
					}
				}
			}
		}

		if (frag->read_memory_array != NULL){
			free(frag->read_memory_array);
			frag->read_memory_array = NULL;
		}
		if (frag->write_memory_array != NULL){
			free(frag->write_memory_array);
			frag->write_memory_array = NULL;
		}

		if (nb_read_mem != 0){
			frag->read_memory_array = (struct memAccess*)malloc(sizeof(struct memAccess) * nb_read_mem);
			if (frag->read_memory_array == NULL){
				printf("ERROR: in %s, unable to allocate memory\n", __func__);
				return result;
			}
		}

		if (nb_write_mem != 0){
			frag->write_memory_array = (struct memAccess*)malloc(sizeof(struct memAccess) * nb_write_mem);
			if (frag->write_memory_array == NULL){
				printf("ERROR: in %s, unable to allocate memory\n", __func__);
				return result;
			}
		}

		frag->nb_memory_read_access = nb_read_mem;
		frag->nb_memory_write_access = nb_write_mem;

		nb_read_mem = 0;
		nb_write_mem = 0;

		for (i = 0; i < array_get_length(&(frag->instruction_array)); i++){
			instruction = (struct instruction*)array_get(&(frag->instruction_array), i);
			for (j = 0; j < INSTRUCTION_MAX_NB_DATA; j++){
				if (INSTRUCTION_DATA_TYPE_IS_VALID(instruction->data[j].type) && INSTRUCTION_DATA_TYPE_IS_MEM(instruction->data[j].type)){
					if (INSTRUCTION_DATA_TYPE_IS_READ(instruction->data[j].type)){
						frag->read_memory_array[nb_read_mem].order		= i * INSTRUCTION_MAX_NB_DATA + j;
						frag->read_memory_array[nb_read_mem].value 		= instruction->data[j].value;
						frag->read_memory_array[nb_read_mem].address	= instruction->data[j].location.address;
						frag->read_memory_array[nb_read_mem].size 		= instruction->data[j].size;
						nb_read_mem ++;
					}
					if (INSTRUCTION_DATA_TYPE_IS_WRITE(instruction->data[j].type)){
						frag->write_memory_array[nb_write_mem].order		= i * INSTRUCTION_MAX_NB_DATA + j;
						frag->write_memory_array[nb_write_mem].value 		= instruction->data[j].value;
						frag->write_memory_array[nb_write_mem].address		= instruction->data[j].location.address;
						frag->write_memory_array[nb_write_mem].size 		= instruction->data[j].size;
						nb_write_mem ++;
					}
				}
			}
		}
	}

	result = 0;

	return result;
}

void traceFragment_print_mem_array(struct memAccess* mem_access, int nb_mem_access){
	struct multiColumnPrinter* 	printer;
	int 						i;
	char 						value_str[20];
	char 						addr_str[20];

	printer = multiColumnPrinter_create(stdout, 4, NULL, NULL, NULL);
	if (printer != NULL){
		multiColumnPrinter_set_column_size(printer, 3, 4);

		multiColumnPrinter_set_column_type(printer, 0, MULTICOLUMN_TYPE_UINT);
		multiColumnPrinter_set_column_type(printer, 3, MULTICOLUMN_TYPE_UINT);

		multiColumnPrinter_set_title(printer, 0, (char*)"ORDER");
		multiColumnPrinter_set_title(printer, 1, (char*)"VALUE");
		multiColumnPrinter_set_title(printer, 2, (char*)"ADDRESS");
		multiColumnPrinter_set_title(printer, 3, (char*)"SIZE");

		multiColumnPrinter_print_header(printer);

		for (i = 0; i < nb_mem_access; i++){
			switch(mem_access[i].size){
			case 1 	: {snprintf(value_str, 20, "%02x", mem_access[i].value & 0x000000ff); break;}
			case 2 	: {snprintf(value_str, 20, "%04x", mem_access[i].value & 0x0000ffff); break;}
			case 4 	: {snprintf(value_str, 20, "%08x", mem_access[i].value & 0xffffffff); break;}
			default : {printf("WARNING: in %s, unexpected data size\n", __func__); break;}
			}
			#pragma GCC diagnostic ignored "-Wformat" /* ISO C90 does not support the ‘ll’ gnu_printf length modifier */
			snprintf(addr_str, 20, "%llx", mem_access[i].address);

			multiColumnPrinter_print(printer, mem_access[i].order, value_str, addr_str, mem_access[i].size, NULL);
		}

		multiColumnPrinter_delete(printer);
	}
	else{
		printf("ERROR: in %s, unable to create multi column printer\n", __func__);
	}
}

int32_t traceFragment_remove_read_after_write(struct traceFragment* frag){
	int 				result = -1;
	uint32_t 			i;
	uint32_t 			writing_pointer = 0;
	struct memAccess* 	write_access;
	struct memAccess* 	new_array;


	if (frag != NULL){
		if (frag->write_memory_array != NULL && frag->read_memory_array != NULL){
			qsort(frag->read_memory_array, frag->nb_memory_read_access, sizeof(struct memAccess), memAccess_compare_address_then_order);
			qsort(frag->write_memory_array, frag->nb_memory_write_access, sizeof(struct memAccess), memAccess_compare_address_then_order);

			/* Warning! This is kind of dumb because it does not take into account the access size */
			for (i = 0; i < frag->nb_memory_read_access; i++){
				write_access = (struct memAccess*)bsearch (frag->read_memory_array + i, frag->write_memory_array, frag->nb_memory_write_access, sizeof(struct memAccess), memAccess_compare_address_then_lower_order);
				if (write_access != NULL){
					if (frag->read_memory_array[i].order < write_access->order){
						printf("ERROR: in %s, something screws up in the bsearch - must be due to the asym compare routine\n", __func__);
					}
				}
				else{
					if (writing_pointer != i){
						memcpy(frag->read_memory_array + writing_pointer, frag->read_memory_array + i, sizeof(struct memAccess));
					}
					writing_pointer ++;
				}
			}

			#ifdef VERBOSE
			printf("Removing %u read write memory access (before: %u, after: %u)\n", (i - writing_pointer), i, writing_pointer);
			#endif

			new_array = (struct memAccess*)realloc(frag->read_memory_array, sizeof(struct memAccess) * writing_pointer);
			if (new_array != NULL){
				frag->read_memory_array = new_array;
			}
			else{
				printf("ERROR: in %s, unable to realloc memory\n", __func__);
			}
			frag->nb_memory_read_access = writing_pointer;
		}
		else{
			printf("ERROR: in %s, create the mem array before calling theis routine\n", __func__);
		}
	}

	return result;

}

/* this routine must be adapted to take into account different heristics like: opcode value end access size */
struct array* traceFragment_extract_mem_arg_adjacent(struct memAccess* mem_access, int nb_mem_access){
	int 				i;
	int 				j;
	struct array* 		array = NULL;
	uint64_t 			mem_address_upper_bound;
	uint64_t 			mem_address_written;
	struct argBuffer 	arg;
	int8_t 				nb_byte;

	if (mem_access != NULL && nb_mem_access > 0){
		qsort(mem_access, nb_mem_access, sizeof(struct memAccess), memAccess_compare_address_then_order);

		array = array_create(sizeof(struct argBuffer));
		if (array == NULL){
			printf("ERROR: in %s, unable to create array struture\n", __func__);
			return array;
		}

		for (i = 1, j = 0, mem_address_upper_bound = mem_access[0].address + mem_access[0].size; i < nb_mem_access; i++){
			if (mem_access[i].address > mem_address_upper_bound){
				arg.location_type 		= ARG_LOCATION_MEMORY;
				arg.location.address 	= mem_access[j].address;
				arg.size 				= (uint32_t)(mem_address_upper_bound - mem_access[j].address);
				arg.data 				= (char*)malloc(arg.size);
				if (arg.data == NULL){
					printf("ERROR: in %s, unable to allocate memory\n", __func__);
					return array;
				}

				mem_address_written = mem_access[j].address;
				while (j <  i){
					nb_byte = (int8_t)((mem_access[j].address + mem_access[j].size) - mem_address_written);
					if (nb_byte > 0){
						memcpy(arg.data + (mem_address_written - arg.location.address), (char*)&(mem_access[j].value) + (mem_access[j].size - nb_byte), nb_byte);
						mem_address_written += nb_byte;
					}
					j++;
				}

				if (array_add(array, &arg) < 0){
					printf("ERROR: in %s, unable to add element to array structure\n", __func__);
				}

				mem_address_upper_bound = mem_access[i].address + mem_access[i].size;
			}
			else{
				mem_address_upper_bound = (mem_address_upper_bound > (mem_access[i].address + mem_access[i].size)) ? mem_address_upper_bound : (mem_access[i].address + mem_access[i].size);
			}
		}

		arg.location_type 		= ARG_LOCATION_MEMORY;
		arg.location.address 	= mem_access[j].address;
		arg.size 				= (uint32_t)(mem_address_upper_bound - mem_access[j].address);
		arg.data 				= (char*)malloc(arg.size);
		if (arg.data == NULL){
			printf("ERROR: in %s, unable to allocate memory\n", __func__);
		}
		else{
			mem_address_written = mem_access[j].address;
			while (j < i){
				nb_byte = (int8_t)((mem_access[j].address + mem_access[j].size) - mem_address_written);
				if (nb_byte > 0){
					memcpy(arg.data + (mem_address_written - arg.location.address), (char*)&(mem_access[j].value) + (mem_access[j].size - nb_byte), nb_byte);
					mem_address_written += nb_byte;
				}
				j++;
			}

			if (array_add(array, &arg) < 0){
				printf("ERROR: in %s, unable to add element to array structure\n", __func__);
			}
		}
	}

	return array;
}

void traceFragment_delete(struct traceFragment* frag){
	if (frag != NULL){
		traceFragment_clean(frag);	
		free(frag);
	}
}

void traceFragment_clean(struct traceFragment* frag){
	if (frag != NULL){
		if (frag->read_memory_array != NULL){
			free(frag->read_memory_array);
		}
		if (frag->write_memory_array != NULL){
			free(frag->write_memory_array);
		}
		array_clean(&(frag->instruction_array));
	}
}

/* ===================================================================== */
/* Comparison functions 	                                         */
/* ===================================================================== */

int memAccess_compare_address_then_order(const void* mem_access1, const void* mem_access2){
	int64_t diff = ((struct memAccess*)mem_access1)->address - ((struct memAccess*)mem_access2)->address;

	if (diff > 0){
		return 1;
	}
	else if (diff < 0){
		return -1;
	}
	else{
		return ((struct memAccess*)mem_access1)->order - ((struct memAccess*)mem_access2)->order;
	}
}

int memAccess_compare_address_then_inv_order(const void* mem_access1, const void* mem_access2){
	int64_t diff = ((struct memAccess*)mem_access1)->address - ((struct memAccess*)mem_access2)->address;

	if (diff > 0){
		return 1;
	}
	else if (diff < 0){
		return -1;
	}
	else{
		return ((struct memAccess*)mem_access2)->order - ((struct memAccess*)mem_access1)->order;
	}
}

int memAccess_compare_address_then_lower_order(const void* mem_access1, const void* mem_access2){
	int64_t addr_diff = ((struct memAccess*)mem_access1)->address - ((struct memAccess*)mem_access2)->address;

	if (addr_diff > 0){
		return 1;
	}
	else if (addr_diff < 0){
		return -1;
	}
	else{
		int32_t order_diff = ((struct memAccess*)mem_access1)->order - ((struct memAccess*)mem_access2)->order;
		if (order_diff < 0){
			return -1;
		}
		else{
			return 0;
		}
	}
}