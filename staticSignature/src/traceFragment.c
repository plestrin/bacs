#include <stdlib.h>
#include <stdio.h>

#include "traceFragment.h"

double traceFragment_opcode_percent(struct traceFragment* frag, uint32_t nb_opcode, uint32_t* opcode, uint32_t nb_excluded_opcode, uint32_t* excluded_opcode){
	uint32_t 					j;
	uint32_t 					nb_effective_instruction = 0;
	uint32_t 					nb_found_instruction = 0;
	uint8_t 					excluded;
	struct instructionIterator 	it;

	if (assembly_get_instruction(&(frag->trace.assembly), &it, 0)){
		printf("ERROR: in %s, unable to fetch first instruction from the assembly\n", __func__);
		return 0.0;
	}

	for (;;){
		excluded = 0;
		if (excluded_opcode != NULL){
			for (j = 0; j < nb_excluded_opcode; j++){
				if (xed_decoded_inst_get_iclass(&(it.xedd)) == excluded_opcode[j]){
					excluded = 1;
					break;
				}
			}
		}

		if (!excluded){
			nb_effective_instruction++;
			if (opcode != NULL){
				for (j = 0; j < nb_opcode; j++){
					if (xed_decoded_inst_get_iclass(&(it.xedd)) == opcode[j]){
						nb_found_instruction ++;
						break;
					}
				}
			}
		}

		if (instructionIterator_get_instruction_index(&it) ==  assembly_get_nb_instruction(&(frag->trace.assembly)) - 1){
			break;
		}
		else{
			if (assembly_get_next_instruction(&(frag->trace.assembly), &it)){
				printf("ERROR: in %s, unable to fetch next instruction from the assembly\n", __func__);
				break;
			}
		}
	}

	return (double)nb_found_instruction / (double)((nb_effective_instruction == 0) ? 1 : nb_effective_instruction);
}

void traceFragment_print_location(struct traceFragment* frag, struct codeMap* cm){
	struct cm_routine* 			routine  = NULL;
	struct cm_section* 			section;
	struct cm_image* 			image;
	struct instructionIterator 	it;

	if (assembly_get_instruction(&(frag->trace.assembly), &it, 0)){
		printf("ERROR: in %s, unable to fetch first instruction from the assembly\n", __func__);
		return;
	}

	for (;;){
		if (routine == NULL || !CODEMAP_IS_ADDRESS_IN_ROUTINE(routine, it.instruction_address)){
			routine = codeMap_search_routine(cm, it.instruction_address);
			if (routine != NULL){
				section = CODEMAP_ROUTINE_GET_SECTION(routine);
				image = CODEMAP_SECTION_GET_IMAGE(section);

				#if defined ARCH_32
				printf("\t- Image: \"%s\", Section: \"%s\", Routine: \"%s\", Offset: 0x%08x\n", image->name, section->name, routine->name, it.instruction_address- routine->address_start);
				#elif defined ARCH_64
				#pragma GCC diagnostic ignored "-Wformat" /* ISO C90 does not support the ‘ll’ gnu_printf length modifier */
				printf("\t- Image: \"%s\", Section: \"%s\", Routine: \"%s\", Offset: 0x%llx\n", image->name, section->name, routine->name, it.instruction_address - routine->address_start);
				#else
				#error Please specify an architecture {ARCH_32 or ARCH_64}
				#endif
			}
			else{
				printf("WARNING: in %s, instruction at offset %u does not belong to a routine\n", __func__, it.instruction_index);
			}
		}

		if (instructionIterator_get_instruction_index(&it) ==  assembly_get_nb_instruction(&(frag->trace.assembly)) - 1){
			break;
		}
		else{
			if (assembly_get_next_instruction(&(frag->trace.assembly), &it)){
				printf("ERROR: in %s, unable to fetch next instruction from the assembly\n", __func__);
				break;
			}
		}
	}
}

void traceFragment_clean(struct traceFragment* frag){
	if (frag->ir != NULL){
		ir_delete(frag->ir)
	}

	trace_clean(&(frag->trace));
}
