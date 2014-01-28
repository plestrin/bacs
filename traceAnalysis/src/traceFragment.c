#include <stdlib.h>
#include <stdio.h>

#include "traceFragment.h"
#include "argBuffer.h"
#include "argSet.h"
#include "multiColumn.h"

int memAccess_compare_address_then_order(const void* mem_access1, const void* mem_access2); 		/* Order memAccess array in address order and in order order as a second option */
int memAccess_compare_address_then_inv_order(const void* mem_access1, const void* mem_access2); 	/* Order memAccess array in address order and in inv order order as a second option */


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
		frag->tag[0]					= '\0';
		frag->read_memory_array 		= NULL;
		frag->write_memory_array 		= NULL;
		frag->nb_memory_read_access 	= 0;
		frag->nb_memory_write_access 	= 0;
		frag->read_register_array 		= NULL;
		frag->write_register_array 		= NULL;
		frag->nb_register_read_access 	= 0;
		frag->nb_register_write_access 	= 0;
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
	if (frag_src->read_register_array != NULL){
		printf("WARNING: in %s, this method does not copy the read_register_array buffer\n", __func__);
	}
	if (frag_src->write_register_array != NULL){
		printf("WARNING: in %s, this method does not copy the write_register_array buffer\n", __func__);
	}

	strncpy(frag_dst->tag, frag_src->tag, TRACEFRAGMENT_TAG_LENGTH);

	frag_dst->read_memory_array 		= NULL;
	frag_dst->write_memory_array 		= NULL;
	frag_dst->nb_memory_read_access 	= frag_src->nb_memory_read_access;
	frag_dst->nb_memory_write_access 	= frag_src->nb_memory_write_access;
	frag_dst->read_register_array 		= NULL;
	frag_dst->write_register_array 		= NULL;
	frag_dst->nb_register_read_access 	= frag_src->nb_register_read_access;
	frag_dst->nb_register_write_access 	= frag_src->nb_register_write_access;

	return 0;
}

double traceFragment_opcode_percent(struct traceFragment* frag, int nb_opcode, uint32_t* opcode, int nb_excluded_opcode, uint32_t* excluded_opcode){
	double 				result = 0;
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

		result = (double)nb_found_instruction/(double)((nb_effective_instruction == 0)?1:nb_effective_instruction);
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
						frag->read_memory_array[nb_read_mem].opcode 	= instruction->opcode;
						nb_read_mem ++;
					}
					if (INSTRUCTION_DATA_TYPE_IS_WRITE(instruction->data[j].type)){
						frag->write_memory_array[nb_write_mem].order		= i * INSTRUCTION_MAX_NB_DATA + j;
						frag->write_memory_array[nb_write_mem].value 		= instruction->data[j].value;
						frag->write_memory_array[nb_write_mem].address		= instruction->data[j].location.address;
						frag->write_memory_array[nb_write_mem].size 		= instruction->data[j].size;
						frag->write_memory_array[nb_write_mem].opcode 		= instruction->opcode;
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

void traceFragment_remove_read_after_write(struct traceFragment* frag){
	uint32_t			i;
	uint32_t 			j;
	uint32_t 			writing_pointer = 0;
	struct memAccess* 	new_array;
	uint32_t 			mask;
	uint8_t 			found;


	if (frag != NULL){
		if (frag->write_memory_array != NULL && frag->read_memory_array != NULL){
			for (i = 0; i < frag->nb_memory_read_access; i++){
				mask = ~(0xffffffff << (frag->read_memory_array[i].size * 8));

				for (j = 0, found = 0; j < frag->nb_memory_write_access; j++){
					if (frag->read_memory_array[i].order > frag->write_memory_array[j].order){
						if (frag->read_memory_array[i].address == frag->write_memory_array[j].address){
							if (frag->read_memory_array[i].size <= frag->write_memory_array[j].size){
								if ((frag->read_memory_array[i].value & mask) == (frag->write_memory_array[j].value & mask)){
									found = 1;
									continue;
								}
							}
						}
						else if (frag->read_memory_array[i].address > frag->write_memory_array[j].address){
							if (frag->read_memory_array[i].address + frag->read_memory_array[i].size < frag->write_memory_array[j].address + frag->write_memory_array[j].size){
								if ((frag->read_memory_array[i].value & mask) == ((frag->write_memory_array[j].value >> (frag->read_memory_array[i].address - frag->write_memory_array[j].address)*8) & mask)){
									found = 1;
									continue;
								}
							}
						}
					}
				}

				if (!found){
					if (writing_pointer != i){
						memcpy(frag->read_memory_array + writing_pointer, frag->read_memory_array + i, sizeof(struct memAccess));
					}
					writing_pointer ++;
				}
			}

			#ifdef VERBOSE
			if (i != writing_pointer){
				printf("Removing %u read after write memory access (before: %u, after: %u) in frag: \"%s\"\n", (i - writing_pointer), i, writing_pointer, frag->tag);
			}
			#endif

			if (writing_pointer != 0){
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
				free(frag->read_memory_array);
				frag->read_memory_array = NULL;
				frag->nb_memory_read_access = 0;
			}
		}
		else{
			printf("ERROR: in %s, create the mem array before calling this routine\n", __func__);
		}
	}
}

int32_t traceFragment_create_reg_array(struct traceFragment* frag){
	uint8_t 				register_read_state[NB_REGISTER];
	uint8_t 				register_write_state[NB_REGISTER];
	uint32_t 				i;
	uint8_t 				j;
	uint8_t 				index = 0;
	struct instruction* 	instruction;

	#define REGISTER_STATE_UNINIT 		0
	#define REGISTER_STATE_VALID  		1
	#define REGISTER_STATE_WRITTEN 		2 /* only for read access */
	#define REGISTER_STATE_OVER_WRITTEN 3 /* only for write access */ 

	if (frag != NULL){
		if (frag->read_register_array != NULL){
			free(frag->read_register_array);
		}
		if (frag->write_register_array != NULL){
			free(frag->write_register_array);
		}
		frag->nb_register_read_access = 0;
		frag->nb_register_write_access = 0;

		frag->read_register_array = (struct regAccess*)malloc(sizeof(struct regAccess) * NB_REGISTER);
		frag->write_register_array = (struct regAccess*)malloc(sizeof(struct regAccess) * NB_REGISTER);

		if (frag->read_register_array == NULL || frag->write_register_array == NULL){
			printf("ERROR: in %s, unable to allocate memory\n", __func__);
			if (frag->read_register_array != NULL){
				free(frag->read_register_array);
			}
			if (frag->write_register_array != NULL){
				free(frag->write_register_array);
			}
			return -1;
		}
		
		memset(register_read_state, REGISTER_STATE_UNINIT, NB_REGISTER);
		memset(register_write_state, REGISTER_STATE_UNINIT, NB_REGISTER);

		#define INDEX_REGISTER_EAX 	0
		#define INDEX_REGISTER_AX 	1
		#define INDEX_REGISTER_AH 	2
		#define INDEX_REGISTER_AL 	3
		#define INDEX_REGISTER_EBX 	4
		#define INDEX_REGISTER_BX 	5
		#define INDEX_REGISTER_BH 	6
		#define INDEX_REGISTER_BL 	7
		#define INDEX_REGISTER_ECX 	8
		#define INDEX_REGISTER_CX 	9
		#define INDEX_REGISTER_CH 	10
		#define INDEX_REGISTER_CL 	11
		#define INDEX_REGISTER_EDX 	12
		#define INDEX_REGISTER_DX 	13
		#define INDEX_REGISTER_DH 	14
		#define INDEX_REGISTER_DL 	15
		#define INDEX_REGISTER_ESI 	16
		#define INDEX_REGISTER_EDI 	17
		#define INDEX_REGISTER_EBP 	18

		#define INIT_REGISTER_ARRAY(array) 																\
		{																								\
			(array)[INDEX_REGISTER_EAX].reg = REGISTER_EAX; 											\
			(array)[INDEX_REGISTER_AX].reg  = REGISTER_AX; 												\
			(array)[INDEX_REGISTER_AH].reg  = REGISTER_AH; 												\
			(array)[INDEX_REGISTER_AL].reg  = REGISTER_AL; 												\
			(array)[INDEX_REGISTER_EBX].reg = REGISTER_EBX; 											\
			(array)[INDEX_REGISTER_BX].reg  = REGISTER_BX; 												\
			(array)[INDEX_REGISTER_BH].reg  = REGISTER_BH; 												\
			(array)[INDEX_REGISTER_BL].reg  = REGISTER_BL; 												\
			(array)[INDEX_REGISTER_ECX].reg = REGISTER_ECX; 											\
			(array)[INDEX_REGISTER_CX].reg  = REGISTER_CX; 												\
			(array)[INDEX_REGISTER_CH].reg  = REGISTER_CH; 												\
			(array)[INDEX_REGISTER_CL].reg  = REGISTER_CL; 												\
			(array)[INDEX_REGISTER_EDX].reg = REGISTER_EDX; 											\
			(array)[INDEX_REGISTER_DX].reg  = REGISTER_DX; 												\
			(array)[INDEX_REGISTER_DH].reg  = REGISTER_DH; 												\
			(array)[INDEX_REGISTER_DL].reg  = REGISTER_DL; 												\
			(array)[INDEX_REGISTER_ESI].reg = REGISTER_ESI; 											\
			(array)[INDEX_REGISTER_EDI].reg = REGISTER_EDI; 											\
			(array)[INDEX_REGISTER_EBP].reg = REGISTER_EBP; 											\
		}

		#define REGISTER_TO_INDEX(reg, index)															\
		{																								\
			switch(reg){ 																				\
			case REGISTER_EAX 	: {index = INDEX_REGISTER_EAX; break;}									\
			case REGISTER_AX 	: {index = INDEX_REGISTER_AX; break;}									\
			case REGISTER_AH 	: {index = INDEX_REGISTER_AH; break;}									\
			case REGISTER_AL 	: {index = INDEX_REGISTER_AL; break;}									\
			case REGISTER_EBX 	: {index = INDEX_REGISTER_EBX; break;}									\
			case REGISTER_BX 	: {index = INDEX_REGISTER_BX; break;}									\
			case REGISTER_BH 	: {index = INDEX_REGISTER_BH; break;}									\
			case REGISTER_BL 	: {index = INDEX_REGISTER_BL; break;}									\
			case REGISTER_ECX 	: {index = INDEX_REGISTER_ECX; break;}									\
			case REGISTER_CX 	: {index = INDEX_REGISTER_CX; break;}									\
			case REGISTER_CH 	: {index = INDEX_REGISTER_CH; break;}									\
			case REGISTER_CL 	: {index = INDEX_REGISTER_CL; break;}									\
			case REGISTER_EDX 	: {index = INDEX_REGISTER_EDX; break;}									\
			case REGISTER_DX 	: {index = INDEX_REGISTER_DX; break;}									\
			case REGISTER_DH 	: {index = INDEX_REGISTER_DH; break;}									\
			case REGISTER_DL 	: {index = INDEX_REGISTER_DL; break;}									\
			case REGISTER_ESI 	: {index = INDEX_REGISTER_ESI; break;}									\
			case REGISTER_EDI 	: {index = INDEX_REGISTER_EDI; break;}									\
			case REGISTER_EBP 	: {index = INDEX_REGISTER_EBP; break;}									\
			default 			: {printf("ERROR: in %s, incorrect register\n", __func__); break;} 		\
			}																							\
		}

		#define REGISTER_SET_STATE_WRITTEN(state)														\
		{																								\
			switch (state){																				\
			case REGISTER_STATE_UNINIT 	: {																\
				(state) = REGISTER_STATE_WRITTEN;														\
				break;																					\
			}																							\
			case REGISTER_STATE_VALID 	: 																\
			case REGISTER_STATE_WRITTEN : {																\
				break;																					\
			}																							\
			default 					: {																\
				printf("ERROR: in %s, incorrect state for register\n", __func__);						\
				break;																					\
			}																							\
			}																							\
		}

		INIT_REGISTER_ARRAY(frag->read_register_array);
		INIT_REGISTER_ARRAY(frag->write_register_array);

		for (i = 0; i < array_get_length(&(frag->instruction_array)); i++){
			instruction = (struct instruction*)array_get(&(frag->instruction_array), i);

			/* READ ACCESS */
			for (j = 0; j < INSTRUCTION_MAX_NB_DATA; j++){
				if (INSTRUCTION_DATA_TYPE_IS_VALID(instruction->data[j].type) && INSTRUCTION_DATA_TYPE_IS_REG(instruction->data[j].type) && INSTRUCTION_DATA_TYPE_IS_READ(instruction->data[j].type)){
					REGISTER_TO_INDEX(instruction->data[j].location.reg, index);
					switch (register_read_state[index]){
					case REGISTER_STATE_UNINIT 	: {
						frag->read_register_array[index].value = instruction->data[j].value;
						frag->read_register_array[index].size = instruction->data[j].size;
						register_read_state[index] = REGISTER_STATE_VALID;
						break;
					}
					case REGISTER_STATE_VALID 	:
					case REGISTER_STATE_WRITTEN : {
						break;
					}
					default 					: {
						printf("ERROR: in %s, incorrect state for register read\n", __func__);
						break;
					}
					}
				}
			}

			/* WRITE ACCESS */
			for (j = 0; j < INSTRUCTION_MAX_NB_DATA; j++){
				if (INSTRUCTION_DATA_TYPE_IS_VALID(instruction->data[j].type) && INSTRUCTION_DATA_TYPE_IS_REG(instruction->data[j].type) && INSTRUCTION_DATA_TYPE_IS_WRITE(instruction->data[j].type)){
					REGISTER_TO_INDEX(instruction->data[j].location.reg, index);

					frag->write_register_array[index].value = instruction->data[j].value;
					frag->write_register_array[index].size = instruction->data[j].size;
					register_write_state[index] = REGISTER_STATE_VALID;

					REGISTER_SET_STATE_WRITTEN(register_read_state[index]);

					switch(instruction->data[j].location.reg){
					case REGISTER_EAX 	: {
						register_write_state[INDEX_REGISTER_AX] = REGISTER_STATE_OVER_WRITTEN;
						register_write_state[INDEX_REGISTER_AH] = REGISTER_STATE_OVER_WRITTEN;
						register_write_state[INDEX_REGISTER_AL] = REGISTER_STATE_OVER_WRITTEN;

						REGISTER_SET_STATE_WRITTEN(register_read_state[INDEX_REGISTER_AX]);
						REGISTER_SET_STATE_WRITTEN(register_read_state[INDEX_REGISTER_AH]);
						REGISTER_SET_STATE_WRITTEN(register_read_state[INDEX_REGISTER_AL]);
						break;
					}
					case REGISTER_AX 	: {
						register_write_state[INDEX_REGISTER_AH] = REGISTER_STATE_OVER_WRITTEN;
						register_write_state[INDEX_REGISTER_AL] = REGISTER_STATE_OVER_WRITTEN;

						REGISTER_SET_STATE_WRITTEN(register_read_state[INDEX_REGISTER_AH]);
						REGISTER_SET_STATE_WRITTEN(register_read_state[INDEX_REGISTER_AL]);
						break;
					}
					case REGISTER_EBX 	: {
						register_write_state[INDEX_REGISTER_BX] = REGISTER_STATE_OVER_WRITTEN;
						register_write_state[INDEX_REGISTER_BH] = REGISTER_STATE_OVER_WRITTEN;
						register_write_state[INDEX_REGISTER_BL] = REGISTER_STATE_OVER_WRITTEN;

						REGISTER_SET_STATE_WRITTEN(register_read_state[INDEX_REGISTER_BX]);
						REGISTER_SET_STATE_WRITTEN(register_read_state[INDEX_REGISTER_BH]);
						REGISTER_SET_STATE_WRITTEN(register_read_state[INDEX_REGISTER_BL]);
						break;
					}
					case REGISTER_BX 	: {
						register_write_state[INDEX_REGISTER_BH] = REGISTER_STATE_OVER_WRITTEN;
						register_write_state[INDEX_REGISTER_BL] = REGISTER_STATE_OVER_WRITTEN;

						REGISTER_SET_STATE_WRITTEN(register_read_state[INDEX_REGISTER_BH]);
						REGISTER_SET_STATE_WRITTEN(register_read_state[INDEX_REGISTER_BL]);
						break;
					}
					case REGISTER_ECX 	: {
						register_write_state[INDEX_REGISTER_CX] = REGISTER_STATE_OVER_WRITTEN;
						register_write_state[INDEX_REGISTER_CH] = REGISTER_STATE_OVER_WRITTEN;
						register_write_state[INDEX_REGISTER_CL] = REGISTER_STATE_OVER_WRITTEN;

						REGISTER_SET_STATE_WRITTEN(register_read_state[INDEX_REGISTER_CX]);
						REGISTER_SET_STATE_WRITTEN(register_read_state[INDEX_REGISTER_CH]);
						REGISTER_SET_STATE_WRITTEN(register_read_state[INDEX_REGISTER_CL]);
						break;
					}
					case REGISTER_CX 	: {
						register_write_state[INDEX_REGISTER_CH] = REGISTER_STATE_OVER_WRITTEN;
						register_write_state[INDEX_REGISTER_CL] = REGISTER_STATE_OVER_WRITTEN;

						REGISTER_SET_STATE_WRITTEN(register_read_state[INDEX_REGISTER_CH]);
						REGISTER_SET_STATE_WRITTEN(register_read_state[INDEX_REGISTER_CL]);
						break;
					}
					case REGISTER_EDX 	: {
						register_write_state[INDEX_REGISTER_DX] = REGISTER_STATE_OVER_WRITTEN;
						register_write_state[INDEX_REGISTER_DH] = REGISTER_STATE_OVER_WRITTEN;
						register_write_state[INDEX_REGISTER_DL] = REGISTER_STATE_OVER_WRITTEN;

						REGISTER_SET_STATE_WRITTEN(register_read_state[INDEX_REGISTER_DX]);
						REGISTER_SET_STATE_WRITTEN(register_read_state[INDEX_REGISTER_DH]);
						REGISTER_SET_STATE_WRITTEN(register_read_state[INDEX_REGISTER_DL]);
						break;
					}
					case REGISTER_DX 	: {
						register_write_state[INDEX_REGISTER_DH] = REGISTER_STATE_OVER_WRITTEN;
						register_write_state[INDEX_REGISTER_DL] = REGISTER_STATE_OVER_WRITTEN;
						
						REGISTER_SET_STATE_WRITTEN(register_read_state[INDEX_REGISTER_DH]);
						REGISTER_SET_STATE_WRITTEN(register_read_state[INDEX_REGISTER_DL]);
						break;
					}
					default 			: {
						break;
					}
					}
					/* we have to deal with write register, it's a bit more tricky */
				}
			}
		}
		
		for (i = 0; i < NB_REGISTER; i++){
			if (register_read_state[i] == REGISTER_STATE_VALID){
				if (i != frag->nb_register_read_access){
					memcpy(frag->read_register_array + frag->nb_register_read_access, frag->read_register_array + i, sizeof(struct regAccess));
				}
				frag->nb_register_read_access ++;
			}
			if (register_write_state[i] == REGISTER_STATE_VALID){
				if (i != frag->nb_register_write_access){
					memcpy(frag->write_register_array + frag->nb_register_write_access, frag->write_register_array + i, sizeof(struct regAccess));
				}
				frag->nb_register_write_access ++;
			}
		}

		if (frag->nb_register_read_access != NB_REGISTER){
			if (frag->nb_register_read_access > 0){
				struct regAccess* new_buffer = (struct regAccess*)realloc(frag->read_register_array, sizeof(struct regAccess) * frag->nb_register_read_access);
				if (new_buffer != NULL){
					frag->read_register_array = new_buffer;
				}
				else{
					printf("ERROR: in %s, unable to realloc memory\n", __func__);
				}
			}
			else{
				free(frag->read_register_array);
				frag->read_register_array = NULL;
			}
		}

		if (frag->nb_register_write_access != NB_REGISTER){
			if (frag->nb_register_write_access > 0){
				struct regAccess* new_buffer = (struct regAccess*)realloc(frag->write_register_array, sizeof(struct regAccess) * frag->nb_register_write_access);
				if (new_buffer != NULL){
					frag->write_register_array = new_buffer;
				}
				else{
					printf("ERROR: in %s, unable to realloc memory\n", __func__);
				}
			}
			else{
				free(frag->write_register_array);
				frag->write_register_array = NULL;
			}
		}
	}

	#undef INDEX_REGISTER_EAX
	#undef INDEX_REGISTER_AX
	#undef INDEX_REGISTER_AH
	#undef INDEX_REGISTER_AL
	#undef INDEX_REGISTER_EBX
	#undef INDEX_REGISTER_BX
	#undef INDEX_REGISTER_BH
	#undef INDEX_REGISTER_BL
	#undef INDEX_REGISTER_ECX
	#undef INDEX_REGISTER_CX
	#undef INDEX_REGISTER_CH
	#undef INDEX_REGISTER_CL
	#undef INDEX_REGISTER_EDX
	#undef INDEX_REGISTER_DX
	#undef INDEX_REGISTER_DH
	#undef INDEX_REGISTER_DL
	#undef INDEX_REGISTER_ESI
	#undef INDEX_REGISTER_EDI
	#undef INDEX_REGISTER_EBP

	#undef REGISTER_STATE_UNINIT
	#undef REGISTER_STATE_VALID
	#undef REGISTER_STATE_WRITTEN
	#undef REGISTER_STATE_OVER_WRITTEN

	#undef INIT_REGISTER_ARRAY
	#undef REGISTER_TO_INDEX
	#undef REGISTER_SET_STATE_WRITTEN

	return 0;
}

void traceFragment_print_reg_array(struct regAccess* reg_access, int nb_reg_access){
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

#define TRACEFRAGMENT_EXTRACT_MEM_ARG_ADJACENT(name, sort_rtn)																																	\
struct array* (name)(struct memAccess* mem_access, int nb_mem_access){																															\
	int 				i;																																										\
	int 				j;																																										\
	struct array* 		array = NULL;																																							\
	ADDRESS 			mem_address_upper_bound;																																				\
	ADDRESS 			mem_address_written;																																					\
	struct argBuffer 	arg;																																									\
	int8_t 				nb_byte;																																								\
																																																\
	if (mem_access != NULL && nb_mem_access > 0){																																				\
		qsort(mem_access, nb_mem_access, sizeof(struct memAccess), (sort_rtn));																													\
																																																\
		array = array_create(sizeof(struct argBuffer));																																			\
		if (array == NULL){																																										\
			printf("ERROR: in %s, unable to create array structure\n", __func__);																												\
			return array;																																										\
		}																																														\
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
					return array;																																								\
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
	return array;																																												\
}

TRACEFRAGMENT_EXTRACT_MEM_ARG_ADJACENT(traceFragment_extract_mem_arg_adjacent_read, memAccess_compare_address_then_order)
TRACEFRAGMENT_EXTRACT_MEM_ARG_ADJACENT(traceFragment_extract_mem_arg_adjacent_write, memAccess_compare_address_then_inv_order)

struct array* traceFragment_extract_mem_arg_adjacent_size_read(struct memAccess* mem_access, int nb_mem_access){
	struct array* 		array = NULL;
	uint8_t*			taken;
	int 				i;
	int 				j;
	int 				k;
	struct argBuffer 	arg;
	uint32_t			size;
	
	if (mem_access != NULL && nb_mem_access > 0){
		qsort(mem_access, nb_mem_access, sizeof(struct memAccess), memAccess_compare_address_then_order);

		taken = (uint8_t*)calloc(nb_mem_access, sizeof(uint8_t));
		if (taken == NULL){
			printf("ERROR: in %s, unable to allocate memory\n", __func__);
			return array;
		}

		array = array_create(sizeof(struct argBuffer));
		if (array == NULL){
			printf("ERROR: in %s, unable to create array structure\n", __func__);
		}
		else{
			for (i = 0; i < nb_mem_access; i++){
				if (taken[i] == 0){
					taken[i] = 1;
					size = mem_access[i].size;
					for (j = i + 1; j < nb_mem_access; j++){
						if ((taken[j] == 0) && (mem_access[j].size == mem_access[i].size)){
							if (mem_access[j].address == mem_access[i].address + size){
								size += mem_access[j].size;
								taken[j] = 1;
							}
							else if (mem_access[j].address > mem_access[i].address + size){
								break;
							}
							else{
								taken[j] = 2;
							}
						}
					}

					arg.location_type 		= ARG_LOCATION_MEMORY;
					arg.location.address 	= mem_access[i].address;
					arg.size 				= size;
					arg.access_size 		= mem_access[i].size;
					arg.data 				= (char*)malloc(arg.size);
					if (arg.data == NULL){
						printf("ERROR: in %s, unable to allocate memory\n", __func__);
						return array;
					}

					for (k = i, size = 0; k < j; k++){
						if (taken[k] == 1){
							memcpy(arg.data + size, &(mem_access[k].value), mem_access[i].size);
							size += mem_access[i].size;
							taken[k] = 2;
						}
					}

					if (array_add(array, &arg) < 0){
						printf("ERROR: in %s, unable to add element to array structure\n", __func__);
					}
				}
			}
		}

		free(taken);
	}

	return array;
}

struct array* traceFragment_extract_mem_arg_adjacent_size_opcode_read(struct memAccess* mem_access, int nb_mem_access){
	struct array* 		array = NULL;
	uint8_t*			taken;
	int 				i;
	int 				j;
	int 				k;
	struct argBuffer 	arg;
	uint32_t			size;
	
	if (mem_access != NULL && nb_mem_access > 0){
		qsort(mem_access, nb_mem_access, sizeof(struct memAccess), memAccess_compare_address_then_order);

		taken = (uint8_t*)calloc(nb_mem_access, sizeof(uint8_t));
		if (taken == NULL){
			printf("ERROR: in %s, unable to allocate memory\n", __func__);
			return array;
		}

		array = array_create(sizeof(struct argBuffer));
		if (array == NULL){
			printf("ERROR: in %s, unable to create array structure\n", __func__);
		}
		else{
			for (i = 0; i < nb_mem_access; i++){
				if (taken[i] == 0){
					taken[i] = 1;
					size = mem_access[i].size;
					for (j = i + 1; j < nb_mem_access; j++){
						if ((taken[j] == 0) && (mem_access[j].size == mem_access[i].size) && (mem_access[j].opcode == mem_access[i].opcode)){
							if (mem_access[j].address == mem_access[i].address + size){
								size += mem_access[j].size;
								taken[j] = 1;
							}
							else if (mem_access[j].address > mem_access[i].address + size){
								break;
							}
							else{
								taken[j] = 2;
							}
						}
					}

					arg.location_type 		= ARG_LOCATION_MEMORY;
					arg.location.address 	= mem_access[i].address;
					arg.size 				= size;
					arg.access_size 		= mem_access[i].size;
					arg.data 				= (char*)malloc(arg.size);
					if (arg.data == NULL){
						printf("ERROR: in %s, unable to allocate memory\n", __func__);
						return array;
					}

					for (k = i, size = 0; k < j; k++){
						if (taken[k] == 1){
							memcpy(arg.data + size, &(mem_access[k].value), mem_access[i].size);
							size += mem_access[i].size;
							taken[k] = 2;
						}
					}

					if (array_add(array, &arg) < 0){
						printf("ERROR: in %s, unable to add element to array structure\n", __func__);
					}
				}
			}
		}

		free(taken);
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
		if (frag->read_register_array != NULL){
			free(frag->read_register_array);
		}
		if (frag->write_register_array != NULL){
			free(frag->write_register_array);
		}
		array_clean(&(frag->instruction_array));
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
