#include <stdlib.h>
#include <stdio.h>
#include <dirent.h> 
#include <string.h>

#include "trace.h"
#include "assemblyElfLoader.h"
#include "result.h"

#define TRACE_BLOCK_FILE_NAME 	"block.bin"
#define TRACE_NB_MAX_THREAD 	64

int32_t trace_init(struct trace* trace, enum traceType type){
	trace->tag[0]	= '\0';
	trace->ir 		= NULL;
	trace->type 	= type;

	return array_init(&(trace->result_array), sizeof(struct result));
}

struct trace* trace_load(const char* directory_path){
	struct trace* 	trace;
	char 			file1_path[TRACE_PATH_MAX_LENGTH];
	char 			file2_path[TRACE_PATH_MAX_LENGTH];
	DIR* 			directory;
	struct dirent* 	entry;
	uint32_t 		thread_id[TRACE_NB_MAX_THREAD];
	uint32_t 		thread_counter = 0;
	uint32_t 		i;

	trace = (struct trace*)malloc(sizeof(struct trace));
	if (trace != NULL){
		strncpy(trace->directory_path, directory_path, TRACE_PATH_MAX_LENGTH);

		directory = opendir(directory_path);
		if (directory != NULL){
			while ((entry = readdir(directory)) != NULL){
				if (!memcmp(entry->d_name, "blockId", 7) && !strcmp(entry->d_name + 7 + strspn(entry->d_name + 7, "0123456789"), ".bin")){
					if (thread_counter == TRACE_NB_MAX_THREAD){
						printf("WARNING: in %s, the max number of thread has been reached, increment TRACE_NB_MAX_THREAD\n", __func__);
						break;
					}
					else{
						thread_id[thread_counter ++] = atoi(entry->d_name + 7);
					}
				}
			}
			closedir(directory);

			if (thread_counter > 1){
				printf("INFO: in %s, several thread traces have been found, loading the first (%u):\n", __func__, thread_id[0]);
				for (i = 0; i < thread_counter; i++){
					if (i == 0){
						printf("\t- Thread: %u (loaded)\n", thread_id[i]);
					}
					else{
						printf("\t- Thread: %u\n", thread_id[i]);
					}
				}
				printf("Use: \"change thread\" command to load a different thread\n");
			}

			snprintf(file1_path, TRACE_PATH_MAX_LENGTH, "%s/blockId%u.bin", directory_path, thread_id[0]);
			snprintf(file2_path, TRACE_PATH_MAX_LENGTH, "%s/%s", directory_path, TRACE_BLOCK_FILE_NAME);
			
			if (assembly_load_trace(&(trace->assembly), file1_path, file2_path)){
				printf("ERROR: in %s, unable to init assembly structure\n", __func__);
				free(trace);
				trace = NULL;
			}
			else{
				if (trace_init(trace, EXECUTION_TRACE)){
					printf("ERROR: in %s, unable to init executionTrace\n", __func__);
					assembly_clean(&(trace->assembly));
					free(trace);
					trace = NULL;
				}
			}
		}
		else{
			printf("ERROR: in %s, unable to open directory: \"%s\"\n", __func__, directory_path);
			free(trace);
			trace = NULL;
		}
	}

	return trace;
}

void trace_change_thread(struct trace* trace, uint32_t thread_id){
	char file1_path[TRACE_PATH_MAX_LENGTH];
	char file2_path[TRACE_PATH_MAX_LENGTH];

	if (trace->type == EXECUTION_TRACE){
		if (trace->ir != NULL){
			ir_delete(trace->ir)
			trace->ir = NULL;
		}

		assembly_clean(&(trace->assembly));

		snprintf(file1_path, TRACE_PATH_MAX_LENGTH, "%s/blockId%u.bin", trace->directory_path, thread_id);
		snprintf(file2_path, TRACE_PATH_MAX_LENGTH, "%s/%s", trace->directory_path, TRACE_BLOCK_FILE_NAME);
			
		if (assembly_load_trace(&(trace->assembly), file1_path, file2_path)){
			printf("ERROR: in %s, unable to init assembly structure\n", __func__);
		}
	}
	else{
		printf("ERROR: in %s, the trace was made from and ELF file there is no thread\n", __func__);
	}
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
			if (trace_init(trace, ELF_TRACE)){
				printf("ERROR: in %s, unabvle to init elfTrace\n", __func__);
				assembly_clean(&(trace->assembly));
				free(trace);
				trace = NULL;
			}
		}
	}

	return trace;
}

void trace_create_ir(struct trace* trace){
	uint32_t 		i;
	struct result* 	result;

	if (trace->ir != NULL){
		printf("WARNING: in %s, an IR has already been built for fragment \"%s\" - deleting\n", __func__, trace->tag);
		ir_delete(trace->ir);

		if(array_get_length(&(trace->result_array))){
			printf("WARNING: in %s, discarding outdated result(s) for fragment \"%s\"\n", __func__, trace->tag);
			for (i = 0; i < array_get_length(&(trace->result_array)); i++){
				result = (struct result*)array_get(&(trace->result_array), i);
				result_clean(result)
			}
			array_empty(&(trace->result_array));
		}
	}
	trace->ir = ir_create(&(trace->assembly));
	if (trace->ir == NULL){
		printf("ERROR: in %s, unable to create IR for fragment \"%s\"\n", __func__, trace->tag);
	}
}

void trace_normalize_ir(struct trace* trace){
	uint32_t 		i;
	struct result* 	result;

	if (trace->ir == NULL){
		printf("ERROR: in %s, the IR is NULL for fragment \"%s\"\n", __func__, trace->tag);
		return;
	}

	for (i = 0; i < array_get_length(&(trace->result_array)); i++){
		result = (struct result*)array_get(&(trace->result_array), i);
		if (result->state != RESULTSTATE_IDLE){
			printf("ERROR: in %s, cannot normalize IR of fragment \"%s\" resulls have been exported\n", __func__, trace->tag);
			return;
		}
	}

	if(array_get_length(&(trace->result_array))){
		printf("WARNING: in %s, discarding outdated result(s) for fragment \"%s\"\n", __func__, trace->tag);
		for (i = 0; i < array_get_length(&(trace->result_array)); i++){
			result = (struct result*)array_get(&(trace->result_array), i);
			result_clean(result)
		}
		array_empty(&(trace->result_array));
	}

	ir_normalize(trace->ir);
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
	uint32_t i;

	for (i = 0; i < trace->assembly.nb_dyn_block; i++){
		if (dynBlock_is_valid(trace->assembly.dyn_blocks + i)){
			#if defined ARCH_32
			printf("\t-BBL %u [%u:%u] 0x%08x:", trace->assembly.dyn_blocks[i].block->header.id, trace->assembly.dyn_blocks[i].instruction_count, trace->assembly.dyn_blocks[i].instruction_count + trace->assembly.dyn_blocks[i].block->header.nb_ins, trace->assembly.dyn_blocks[i].block->header.address);
			#elif defined ARCH_64
			printf("\t-BBL %u [%u:%u] 0x%0llx:", trace->assembly.dyn_blocks[i].block->header.id, trace->assembly.dyn_blocks[i].instruction_count, trace->assembly.dyn_blocks[i].instruction_count + trace->assembly.dyn_blocks[i].block->header.nb_ins, trace->assembly.dyn_blocks[i].block->header.address);
			#else
			#error Please specify an architecture {ARCH_32 or ARCH_64}
			#endif
			codeMap_print_address_info(cm, trace->assembly.dyn_blocks[i].block->header.address, stdout);
			printf("\n");
		}
		else{
			printf("\t-[...]\n");
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

void trace_export_result(struct trace* trace, void** signature_buffer, uint32_t nb_signature){
	uint32_t 		i;
	uint32_t 		j;
	struct result* 	result;
	uint32_t* 		exported_result 	= NULL;
	uint32_t 		nb_exported_result;
	struct set* 	node_set 			= NULL;
	struct node** 	footprint 			= NULL;
	uint32_t 		nb_node_footprint;

	if (trace->ir == NULL){
		printf("ERROR: in %s, the IR is NULL for fragment \"%s\"\n", __func__, trace->tag);
		goto exit;
	}

	exported_result = (uint32_t*)malloc(sizeof(uint32_t) * array_get_length(&(trace->result_array)));
	if (exported_result == NULL){
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
		goto exit;
	}

	for (i = 0, nb_exported_result = 0; i < array_get_length(&(trace->result_array)); i++){
		result = (struct result*)array_get(&(trace->result_array), i);
		if (result->state != RESULTSTATE_IDLE){
			printf("ERROR: in %s, results have already been exported (%s), unable to export twice - rebuild IR\n", __func__, result->signature->name);
			goto exit;
		}
		else{
			for (j = 0; j < nb_signature; j++){
				if (signature_buffer[j] == result->signature){
					#ifdef VERBOSE
					printf("Export %u occurrence(s) of %s in fragment %s\n", result->nb_occurrence, result->signature->name, trace->tag);
					#endif
					exported_result[nb_exported_result ++] = i;
				}
			}
		}
	}

	if (nb_exported_result == 0){
		printf("WARNING: in %s, no exported result\n", __func__);
		goto exit;
	}

	node_set = set_create(sizeof(struct node*), 512);
	if (node_set == NULL){
		printf("ERROR: in %s, unable to create set\n", __func__);
		goto exit;
	}

	for (i = 0; i < nb_exported_result; i++){
		result = (struct result*)array_get(&(trace->result_array), exported_result[i]);
		result_push(result, trace->ir);
		
		for (j = 0; j < result->nb_occurrence; j++){
			result_get_footprint(result, j, node_set);
		}
	}

	footprint = (struct node**)set_export_buffer_unique(node_set, &nb_node_footprint);
	if (footprint == NULL){
		printf("ERROR: in %s, unable to export set\n", __func__);
		goto exit;
	}

	set_delete(node_set);
	node_set = NULL;

	ir_remove_footprint(trace->ir, footprint, nb_node_footprint);
	ir_normalize_remove_dead_code(trace->ir, NULL);

	exit:
	if (exported_result != NULL){
		free(exported_result);
	}
	if (node_set != NULL){
		set_delete(node_set);
	}
	if (footprint != NULL){
		free(footprint);
	}
}

void trace_clean(struct trace* trace){
	struct result* 	result;
	uint32_t 		i; 

	for (i = 0; i < array_get_length(&(trace->result_array)); i++){
		result = (struct result*)array_get(&(trace->result_array), i);
		result_clean(result)
	}
	array_clean(&(trace->result_array));

	if (trace->ir != NULL){
		ir_delete(trace->ir)
	}

	assembly_clean(&(trace->assembly));
}