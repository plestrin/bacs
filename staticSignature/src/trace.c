#include <stdlib.h>
#include <stdio.h>

#include "trace.h"
#include "assemblyElfLoader.h"

#define TRACE_PATH_MAX_LENGTH 	256
#define TRACE_BLOCKID_FILE_NAME "blockId.bin"
#define TRACE_BLOCK_FILE_NAME 	"block.bin"

struct trace* trace_load(const char* directory_path){
	struct trace* 	trace;
	char 			file1_path[TRACE_PATH_MAX_LENGTH];
	char 			file2_path[TRACE_PATH_MAX_LENGTH];

	trace = (struct trace*)malloc(sizeof(struct trace));
	if (trace != NULL){
		snprintf(file1_path, TRACE_PATH_MAX_LENGTH, "%s/%s", directory_path, TRACE_BLOCKID_FILE_NAME);
		snprintf(file2_path, TRACE_PATH_MAX_LENGTH, "%s/%s", directory_path, TRACE_BLOCK_FILE_NAME);
		
		if (assembly_load_trace(&(trace->assembly), file1_path, file2_path)){
			printf("ERROR: in %s, unable to init assembly structure\n", __func__);
			free(trace);
			trace = NULL;
		}
		else{
			trace_init(trace);
		}
	}

	return trace;
}

struct trace* trace_load_elf(const char* file_path){
	struct trace* 	trace;

	trace = (struct trace*)malloc(sizeof(struct trace));
	if (trace != NULL){
		if (assembly_load_elf(&(trace->assembly), file_path)){
			printf("ERROR: in %s, unable to init assembly structure from ELF file\n", __func__);
			free(trace);
			trace = NULL;
		}
		else{
			trace_init(trace);
		}
	}

	return trace;
}

int32_t trace_concat(struct trace** trace_src_buffer, uint32_t nb_trace_src, struct trace* trace_dst){
	struct assembly** 	assembly_src_buffer;
	uint32_t 			i;

	assembly_src_buffer = (struct assembly**)alloca((sizeof(struct assembly*) * nb_trace_src));
	for (i = 0; i < nb_trace_src; i++){
		assembly_src_buffer[i] = &(trace_src_buffer[i]->assembly);
	}

	return assembly_concat(assembly_src_buffer, nb_trace_src, &(trace_dst->assembly));
}

void trace_print_location(struct trace* trace, struct codeMap* cm){
	struct cm_routine* 			routine  = NULL;
	struct cm_section* 			section;
	struct cm_image* 			image;
	struct instructionIterator 	it;

	if (assembly_get_instruction(&(trace->assembly), &it, 0)){
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

		if (instructionIterator_get_instruction_index(&it) ==  assembly_get_nb_instruction(&(trace->assembly)) - 1){
			break;
		}
		else{
			if (assembly_get_next_instruction(&(trace->assembly), &it)){
				printf("ERROR: in %s, unable to fetch next instruction from the assembly\n", __func__);
				break;
			}
		}
	}
}

double trace_opcode_percent(struct trace* trace, uint32_t nb_opcode, uint32_t* opcode, uint32_t nb_excluded_opcode, uint32_t* excluded_opcode){
	uint32_t 					j;
	uint32_t 					nb_effective_instruction = 0;
	uint32_t 					nb_found_instruction = 0;
	uint8_t 					excluded;
	struct instructionIterator 	it;

	if (assembly_get_instruction(&(trace->assembly), &it, 0)){
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

		if (instructionIterator_get_instruction_index(&it) ==  assembly_get_nb_instruction(&(trace->assembly)) - 1){
			break;
		}
		else{
			if (assembly_get_next_instruction(&(trace->assembly), &it)){
				printf("ERROR: in %s, unable to fetch next instruction from the assembly\n", __func__);
				break;
			}
		}
	}

	return (double)nb_found_instruction / (double)((nb_effective_instruction == 0) ? 1 : nb_effective_instruction);
}


void trace_clean(struct trace* trace){
	if (trace->ir != NULL){
		ir_delete(trace->ir)
	}

	assembly_clean(&(trace->assembly));
}