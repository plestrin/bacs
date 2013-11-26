#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "traceFragment.h"
#include "argBuffer.h"

int memAccess_compare_address(const void* mem_access1, const void* mem_access2); /* Order memAccess array in address order */


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

struct array* traceFragement_extract_mem_arg_adjacent(struct memAccess* mem_access, int nb_mem_access){
	int 				i;
	int 				j;
	struct array* 		array = NULL;
	uint64_t 			mem_address_upper_bound;
	struct argBuffer 	arg;

	if (mem_access != NULL && nb_mem_access > 0){
		qsort(mem_access, nb_mem_access, sizeof(struct memAccess), memAccess_compare_address);

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
					break;
				}

				while (j <  i){
					/* warning we do not raise alarm for overlapping access with different value */
					memcpy(arg.data + (mem_access[j].address - arg.location.address), &(mem_access[j].value), mem_access[j].size);
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
			while (j < i){
				/* warning we do not raise alarm for overlapping access with different value */
				memcpy(arg.data + (mem_access[j].address - arg.location.address), &(mem_access[j].value), mem_access[j].size);
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

int memAccess_compare_address(const void* mem_access1, const void* mem_access2){
	int64_t diff = ((struct memAccess*)mem_access1)->address - ((struct memAccess*)mem_access2)->address;

	if (diff > 0){
		return 1;
	}
	else if (diff < 0){
		return -1;
	}
	else{
		return 0;
	}
}