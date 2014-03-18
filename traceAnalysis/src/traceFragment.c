#include <stdlib.h>
#include <stdio.h>

#include "traceFragment.h"


int32_t traceFragment_init(struct traceFragment* frag, enum fragmentType type, void* specific_data, struct fragmentCallback* callback){
	frag->tag[0]					= '\0';
	frag->type 						= type;
	frag->specific_data 			= specific_data;
	frag->callback 					= callback;

	frag->read_memory_array 		= NULL;
	frag->write_memory_array 		= NULL;
	frag->nb_memory_read_access 	= 0;
	frag->nb_memory_write_access 	= 0;

	frag->read_register_array 		= NULL;
	frag->write_register_array 		= NULL;
	frag->nb_register_read_access 	= 0;
	frag->nb_register_write_access 	= 0;

	return 0;
}

double traceFragment_opcode_percent(struct traceFragment* frag, int nb_opcode, uint32_t* opcode, int nb_excluded_opcode, uint32_t* excluded_opcode){
	double 		result = 0;
	uint32_t 	i;
	int 		j;
	int 		nb_effective_instruction = 0;
	int 		nb_found_instruction = 0;
	char 		excluded;

	if (frag != NULL){
		for (i = 0; i < frag->trace.nb_instruction; i++){
			excluded = 0;
			if (excluded_opcode != NULL){
				for (j = 0; j < nb_excluded_opcode; j++){
					if (frag->trace.instructions[i].opcode == excluded_opcode[j]){
						excluded = 1;
						break;
					}
				}
			}

			if (!excluded){
				nb_effective_instruction++;
				if (opcode != NULL){
					for (j = 0; j < nb_opcode; j++){
						if (frag->trace.instructions[i].opcode == opcode[j]){
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
	uint32_t 			i;
	uint32_t 			j;
	struct operand* 	operands;

	if (frag != NULL){
		for (i = 0; i < frag->trace.nb_instruction; i++){
			operands = trace_get_ins_operands(&(frag->trace), i);
			for (j = 0; j < frag->trace.instructions[i].nb_operand; j++){
				if (OPERAND_IS_MEM(operands[j])){
					if(operands[j].size <= 4){
						if (OPERAND_IS_READ(operands[j])){
							nb_read_mem ++;
						}
						else if (OPERAND_IS_WRITE(operands[j])){
							nb_write_mem ++;
						}
						else{
							printf("ERROR: in %s, incorrect operand type\n", __func__);
						}
					}
					else{
						printf("ERROR: in %s, operand size is too big (%u) - this case is not implemented yet\n", __func__, operands[j].size);
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
				return -1;
			}
		}

		if (nb_write_mem != 0){
			frag->write_memory_array = (struct memAccess*)malloc(sizeof(struct memAccess) * nb_write_mem);
			if (frag->write_memory_array == NULL){
				printf("ERROR: in %s, unable to allocate memory\n", __func__);
				return -1;
			}
		}

		frag->nb_memory_read_access = nb_read_mem;
		frag->nb_memory_write_access = nb_write_mem;

		nb_read_mem = 0;
		nb_write_mem = 0;

		for (i = 0; i < frag->trace.nb_instruction; i++){
			operands = trace_get_ins_operands(&(frag->trace), i);
			for (j = 0; j < frag->trace.instructions[i].nb_operand; j++){
				if (OPERAND_IS_MEM(operands[j])){
					if (operands[j].size <= 4){
						if (OPERAND_IS_READ(operands[j])){
							frag->read_memory_array[nb_read_mem].order		= frag->trace.instructions[i].operand_offset + j;
							memcpy(&(frag->read_memory_array[nb_read_mem].value), trace_get_ins_op_data(&(frag->trace), i, j), operands[j].size);
							frag->read_memory_array[nb_read_mem].address	= operands[j].location.address;
							frag->read_memory_array[nb_read_mem].size 		= operands[j].size;
							frag->read_memory_array[nb_read_mem].opcode 	= frag->trace.instructions[i].opcode;
							frag->read_memory_array[nb_read_mem].group 		= MEMACCESS_UNDEF_GROUP;
							nb_read_mem ++;
						}
						else if (OPERAND_IS_WRITE(operands[j])){
							frag->write_memory_array[nb_write_mem].order		= frag->trace.instructions[i].operand_offset + j;
							memcpy(&(frag->write_memory_array[nb_write_mem].value), trace_get_ins_op_data(&(frag->trace), i, j), operands[j].size);
							frag->write_memory_array[nb_write_mem].address		= operands[j].location.address;
							frag->write_memory_array[nb_write_mem].size 		= operands[j].size;
							frag->write_memory_array[nb_write_mem].opcode 		= frag->trace.instructions[i].opcode;
							frag->write_memory_array[nb_write_mem].group 		= MEMACCESS_UNDEF_GROUP;
							nb_write_mem ++;
						}
						else{
							printf("ERROR: in %s, incorrect operand type\n", __func__);
						}
					}
					else{
						printf("ERROR: in %s, operand size is too big (%u) - this case is not implemented yet\n", __func__, operands[j].size);
					}
				}
			}
		}
	}

	return 0;
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
	uint8_t 			register_read_state[NB_REGISTER];
	uint8_t 			register_write_state[NB_REGISTER];
	uint32_t 			i;
	uint8_t 			j;
	uint8_t 			index = 0;
	struct operand* 	operands;

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

		for (i = 0; i < frag->trace.nb_instruction; i++){
			operands = trace_get_ins_operands(&(frag->trace), i);

			/* READ ACCESS */
			for (j = 0; j < frag->trace.instructions[i].nb_operand; j++){
				if (OPERAND_IS_REG(operands[j]) && OPERAND_IS_READ(operands[j])){
					if (operands[j].size <= 4){
						REGISTER_TO_INDEX(operands[j].location.reg, index);
						switch (register_read_state[index]){
							case REGISTER_STATE_UNINIT 	: {
								memcpy(&(frag->read_register_array[index].value), trace_get_ins_op_data(&(frag->trace), i, j), operands[j].size);
								frag->read_register_array[index].size 	= operands[j].size;
								frag->read_register_array[index].order 	= frag->trace.instructions[i].operand_offset + j;
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
					else{
						printf("ERROR: in %s, operand size is too big (%u) - this case is not implemented yet\n", __func__, operands[j].size);
					}
				}
			}

			/* WRITE ACCESS */
			for (j = 0; j < frag->trace.instructions[i].nb_operand; j++){
				if (OPERAND_IS_REG(operands[j]) && OPERAND_IS_WRITE(operands[j])){
					if (operands[j].size <= 4){
						REGISTER_TO_INDEX(operands[j].location.reg, index);

						memcpy(&(frag->write_register_array[index].value), trace_get_ins_op_data(&(frag->trace), i, j), operands[j].size);
						frag->write_register_array[index].size 	= operands[j].size;
						frag->write_register_array[index].order = frag->trace.instructions[i].operand_offset + j;
						register_write_state[index] = REGISTER_STATE_VALID;

						REGISTER_SET_STATE_WRITTEN(register_read_state[index]);

						switch(operands[j].location.reg){
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
					}
					else{
						printf("ERROR: in %s, operand size is too big (%u) - this case is not implemented yet\n", __func__, operands[j].size);
					}
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

void traceFragment_print_location(struct traceFragment* frag, struct codeMap* cm){
	uint32_t 				i;
	struct cm_routine* 		routine  = NULL;
	struct cm_section* 		section;
	struct cm_image* 		image;

	for (i = 0; i < frag->trace.nb_instruction; i++){
		if (routine == NULL || !CODEMAP_IS_ADDRESS_IN_ROUTINE(routine, frag->trace.instructions[i].pc)){
			routine = codeMap_search_routine(cm, frag->trace.instructions[i].pc);
			if (routine != NULL){
				section = CODEMAP_ROUTINE_GET_SECTION(routine);
				image = CODEMAP_SECTION_GET_IMAGE(section);

				printf("\t- Image: \"%s\", Section: \"%s\", Routine: \"%s\"\n", image->name, section->name, routine->name);
			}
			else{
				printf("WARNING: in %s, instruction at offset %u does not belong to a routine\n", __func__, i);
			}
		}
	}
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

		if (frag->callback != NULL && frag->callback->specific_delete != NULL){
			frag->callback->specific_delete(frag->specific_data);
		}

		trace_clean(&(frag->trace));
	}
}
