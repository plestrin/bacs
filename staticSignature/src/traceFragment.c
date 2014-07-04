#include <stdlib.h>
#include <stdio.h>

#include "traceFragment.h"

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

				#if defined ARCH_32
				printf("\t- Image: \"%s\", Section: \"%s\", Routine: \"%s\", Offset: 0x%08x\n", image->name, section->name, routine->name, frag->trace.instructions[i].pc - routine->address_start);
				#elif defined ARCH_64
				#pragma GCC diagnostic ignored "-Wformat" /* ISO C90 does not support the ‘ll’ gnu_printf length modifier */
				printf("\t- Image: \"%s\", Section: \"%s\", Routine: \"%s\", Offset: 0x%llx\n", image->name, section->name, routine->name, frag->trace.instructions[i].pc - routine->address_start);
				#else
				#error Please specify an architecture {ARCH_32 or ARCH_64}
				#endif
			}
			else{
				printf("WARNING: in %s, instruction at offset %u does not belong to a routine\n", __func__, i);
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
