#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "traceFragment.h"
#include "argBuffer.h"
#include "multiColumn.h"

int memAccess_compare_address_then_order(const void* mem_access1, const void* mem_access2); /* Order memAccess array in address order */


struct traceFragment* codeSegment_create(){
	struct traceFragment* frag = (struct traceFragment*)malloc(sizeof(struct traceFragment));

	if (frag != NULL){
		traceFragment_init(frag);
	}
	else{
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
	}

	return frag;
}

void traceFragment_init(struct traceFragment* frag){
	if (frag != NULL){
		frag->instructions = (struct instruction*)malloc(sizeof(struct instruction) * TRACEFRAGMENT_INSTRUCTION_BATCH);
		if (frag->instructions != NULL){
			frag->nb_allocated_instruction = TRACEFRAGMENT_INSTRUCTION_BATCH;
		}
		else{
			printf("ERROR: in %s, unable to allocate memory\n", __func__);
			frag->nb_allocated_instruction = 0;
		}
		frag->nb_instruction = 0;

		frag->read_memory_array 		= NULL;
		frag->write_memory_array 		= NULL;
		frag->nb_memory_read_access 	= 0;
		frag->nb_memory_write_access 	= 0;
	}
	else{
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
	}
}

int traceFragment_add_instruction(struct traceFragment* frag, struct instruction* ins){
	struct instruction* new_buffer;

	if (frag != NULL){
		if (frag->nb_allocated_instruction == frag->nb_instruction){
			new_buffer = (struct instruction*)realloc(frag->instructions, sizeof(struct instruction) * (frag->nb_allocated_instruction + TRACEFRAGMENT_INSTRUCTION_BATCH));
			if (new_buffer != NULL){
				frag->instructions = new_buffer;
				frag->nb_allocated_instruction += TRACEFRAGMENT_INSTRUCTION_BATCH;
			}
			else{
				printf("ERROR: in %s, unable to allocate memory\n", __func__);
				return -1;
			}
		}

		memcpy(frag->instructions + frag->nb_instruction, ins, sizeof(struct instruction));
		frag->nb_instruction ++;

		return frag->nb_instruction - 1;
	}

	return -1;
}

int traceFragment_search_pc(struct traceFragment* frag, struct instruction* ins){
	int result = -1;
	int i;

	if (frag != NULL){
		for (i = 0; i < frag->nb_instruction; i++){
			if (instruction_compare_pc(ins, frag->instructions + i)){
				result = i;
				break;
			}
		}
	}

	return result;
}

struct instruction* traceFragment_get_last_instruction(struct traceFragment* frag){
	struct instruction* instruction = NULL;

	if (frag != NULL){
		if (frag->nb_instruction > 0){
			instruction = frag->instructions + (frag->nb_instruction - 1);
		}
	}

	return instruction;
}

float traceFragment_opcode_percent(struct traceFragment* frag, int nb_opcode, uint32_t* opcode, int nb_excluded_opcode, uint32_t* excluded_opcode){
	float 	result = 0;
	int 	i;
	int 	j;
	int 	nb_effective_instruction = 0;
	int 	nb_found_instruction = 0;
	char 	excluded;

	if (frag != NULL){
		for (i = 0; i < frag->nb_instruction; i++){
			excluded = 0;
			if (excluded_opcode != NULL){
				for (j = 0; j < nb_excluded_opcode; j++){
					if (frag->instructions[i].opcode == excluded_opcode[j]){
						excluded = 1;
						break;
					}
				}
			}

			if (!excluded){
				nb_effective_instruction++;
				if (opcode != NULL){
					for (j = 0; j < nb_opcode; j++){
						if (frag->instructions[i].opcode == opcode[j]){
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

int traceFragment_create_mem_array(struct traceFragment* frag){
	uint32_t 	nb_read_mem 	= 0;
	uint32_t 	nb_write_mem 	= 0;
	int 		result 			= -1;
	int 		i;
	int 		j;

	if (frag != NULL){
		for (i = 0; i < frag->nb_instruction; i++){
			for (j = 0; j < INSTRUCTION_MAX_NB_DATA; j++){
				if (INSTRUCTION_DATA_TYPE_IS_VALID(frag->instructions[i].data[j].type) && INSTRUCTION_DATA_TYPE_IS_MEM(frag->instructions[i].data[j].type)){
					if (INSTRUCTION_DATA_TYPE_IS_READ(frag->instructions[i].data[j].type)){
						nb_read_mem ++;
					}
					if (INSTRUCTION_DATA_TYPE_IS_WRITE(frag->instructions[i].data[j].type)){
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

		for (i = 0; i < frag->nb_instruction; i++){
			for (j = 0; j < INSTRUCTION_MAX_NB_DATA; j++){
				if (INSTRUCTION_DATA_TYPE_IS_VALID(frag->instructions[i].data[j].type) && INSTRUCTION_DATA_TYPE_IS_MEM(frag->instructions[i].data[j].type)){
					if (INSTRUCTION_DATA_TYPE_IS_READ(frag->instructions[i].data[j].type)){
						frag->read_memory_array[nb_read_mem].order		= i * INSTRUCTION_MAX_NB_DATA + j;
						frag->read_memory_array[nb_read_mem].value 		= frag->instructions[i].data[j].value;
						frag->read_memory_array[nb_read_mem].address	= frag->instructions[i].data[j].location.address;
						frag->read_memory_array[nb_read_mem].size 		= frag->instructions[i].data[j].size;
						nb_read_mem ++;
					}
					if (INSTRUCTION_DATA_TYPE_IS_WRITE(frag->instructions[i].data[j].type)){
						frag->write_memory_array[nb_write_mem].order		= i * INSTRUCTION_MAX_NB_DATA + j;
						frag->write_memory_array[nb_write_mem].value 		= frag->instructions[i].data[j].value;
						frag->write_memory_array[nb_write_mem].address		= frag->instructions[i].data[j].location.address;
						frag->write_memory_array[nb_write_mem].size 		= frag->instructions[i].data[j].size;
						nb_write_mem ++;
					}
				}
			}
		}
	}

	return result;
}

void traceFragment_print_mem_array(struct memAccess* mem_access, int nb_mem_access){
	struct multiColumnPrinter* 	printer;
	int 						i;
	char 						order_str[20];
	char 						value_str[20];
	char 						addr_str[20];
	char 						size_str[20];

	printer = multiColumnPrinter_create(stdout, 4, NULL, NULL);
	if (printer != NULL){
		multiColumnPrinter_set_column_size(printer, 3, 4);

		multiColumnPrinter_set_title(printer, 0, (char*)"ORDER");
		multiColumnPrinter_set_title(printer, 1, (char*)"VALUE");
		multiColumnPrinter_set_title(printer, 2, (char*)"ADDRESS");
		multiColumnPrinter_set_title(printer, 3, (char*)"SIZE");
		multiColumnPrinter_print_header(printer);

		for (i = 0; i < nb_mem_access; i++){
			snprintf(order_str, 20, "%u", mem_access[i].order);
			switch(mem_access[i].size){
			case 1 	: {snprintf(value_str, 20, "%02x", mem_access[i].value & 0x000000ff); break;}
			case 2 	: {snprintf(value_str, 20, "%04x", mem_access[i].value & 0x0000ffff); break;}
			case 4 	: {snprintf(value_str, 20, "%08x", mem_access[i].value & 0xffffffff); break;}
			default : {printf("WARNING: in %s, unexpected data size\n", __func__); break;}
			}
			#pragma GCC diagnostic ignored "-Wformat" /* ISO C90 does not support the ‘ll’ gnu_printf length modifier */
			snprintf(addr_str, 20, "%llx", mem_access[i].address);
			snprintf(size_str, 20, "%u", mem_access[i].size);

			multiColumnPrinter_print(printer, order_str, value_str, addr_str, size_str, NULL);
		}

		multiColumnPrinter_delete(printer);
	}
	else{
		printf("ERROR: in %s, unable to create multi column printer\n", __func__);
	}
}

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
		if (frag->instructions != NULL){
			free(frag->instructions);
		}
		if (frag->read_memory_array != NULL){
			free(frag->read_memory_array);
		}
		if (frag->write_memory_array != NULL){
			free(frag->write_memory_array);
		}
	}
}

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