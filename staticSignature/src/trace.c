#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>

#include "trace.h"
#include "assemblyElfLoader.h"
#include "result.h"
#include "base.h"

#define TRACE_BLOCK_FILE_NAME 	"block.bin"
#define TRACE_NB_MAX_THREAD 	64

static inline int32_t trace_init(struct trace* trace, enum traceType type){
	trace->tag[0]			= '\0';
	trace->ir 				= NULL;
	trace->type 			= type;
	trace->mem_trace 		= NULL;
	trace->synthesis_graph 	= NULL;

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

	if ((directory = opendir(directory_path)) == NULL){
		log_err_m("unable to open directory: \"%s\"", directory_path);
		return NULL;
	}

	while ((entry = readdir(directory)) != NULL){
		if (!memcmp(entry->d_name, "blockId", 7) && !strcmp(entry->d_name + 7 + strspn(entry->d_name + 7, "0123456789"), ".bin")){
			if (thread_counter == TRACE_NB_MAX_THREAD){
				log_warn("the max number of thread has been reached, increment TRACE_NB_MAX_THREAD");
				break;
			}
			else{
				thread_id[thread_counter ++] = atoi(entry->d_name + 7);
			}
		}
	}
	closedir(directory);

	if (thread_counter > 1){
		log_info_m("several thread traces have been found, loading the first (%u):", thread_id[0]);
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

	if ((trace = (struct trace*)malloc(sizeof(struct trace))) == NULL){
		log_err("unable to allocate memory");
		return NULL;
	}
		
	strncpy(trace->directory_path, directory_path, TRACE_PATH_MAX_LENGTH);

	snprintf(file1_path, TRACE_PATH_MAX_LENGTH, "%s/blockId%u.bin", directory_path, thread_id[0]);
	snprintf(file2_path, TRACE_PATH_MAX_LENGTH, "%s/%s", directory_path, TRACE_BLOCK_FILE_NAME);
			
	if (assembly_load_trace(&(trace->assembly), file1_path, file2_path)){
		log_err("unable to init assembly structure");
		free(trace);
		return NULL;
	}

	if (trace_init(trace, EXECUTION_TRACE)){
		log_err("unable to init executionTrace");
		assembly_clean(&(trace->assembly));
		free(trace);
		return NULL;
	}

	if (memTrace_is_trace_exist(trace->directory_path, thread_id[0])){
		if ((trace->mem_trace = memTrace_create_trace(trace->directory_path, thread_id[0], &(trace->assembly))) == NULL){
			log_err("unable to load associated memory addresses");
		}
	}

	return trace;
}

int32_t trace_change_thread(struct trace* trace, uint32_t thread_id){
	char file1_path[TRACE_PATH_MAX_LENGTH];
	char file2_path[TRACE_PATH_MAX_LENGTH];

	if (trace->type == EXECUTION_TRACE){
		trace_reset(trace);

		snprintf(file1_path, TRACE_PATH_MAX_LENGTH, "%s/blockId%u.bin", trace->directory_path, thread_id);
		snprintf(file2_path, TRACE_PATH_MAX_LENGTH, "%s/%s", trace->directory_path, TRACE_BLOCK_FILE_NAME);
			
		if (assembly_load_trace(&(trace->assembly), file1_path, file2_path)){
			log_err("unable to init assembly structure");
			return -1;
		}

		if (memTrace_is_trace_exist(trace->directory_path, thread_id)){
			if ((trace->mem_trace = memTrace_create_trace(trace->directory_path, thread_id, &(trace->assembly))) == NULL){
				log_err("unable to load associated memory addresses");
			}
		}
	}
	else{
		log_err("the trace was made from and ELF file there is no thread");
	}

	return 0;
}

struct trace* trace_load_elf(const char* file_path){
	struct trace* 	trace;

	trace = (struct trace*)malloc(sizeof(struct trace));
	if (trace != NULL){
		if (assembly_load_elf(&(trace->assembly), file_path)){
			log_err("unable to init assembly structure from ELF file");
			free(trace);
			return NULL;
		}
		
		if (trace_init(trace, ELF_TRACE)){
			log_err("unable to init elfTrace");
			assembly_clean(&(trace->assembly));
			free(trace);
			return NULL;
		}
	}

	return trace;
}

int32_t trace_extract_segment(struct trace* trace_src, struct trace* trace_dst, uint32_t offset, uint32_t length){
	uint64_t index_mem_start;
	uint64_t index_mem_stop;

	if (assembly_extract_segment(&(trace_src->assembly), &(trace_dst->assembly), offset, length, &index_mem_start, &index_mem_stop)){
		log_err("unable to extract assembly fragment");
		return -1;
	}

	if (trace_init(trace_dst, FRAGMENT_TRACE)){
		log_err("unable to init traceFragment");
		assembly_clean(&(trace_dst->assembly));
		return - 1;
	}

	if (trace_src->mem_trace != NULL && (trace_dst->mem_trace = memTrace_create_frag(trace_src->mem_trace, index_mem_start, index_mem_stop)) == NULL){
		log_err("unable to extract memory trace");
	}

	return 0;
}

void trace_create_ir(struct trace* trace){
	uint32_t 		i;
	struct result* 	result;

	if (trace->ir != NULL){
		log_warn_m("an IR has already been built for fragment \"%s\" - deleting", trace->tag);
		ir_delete(trace->ir);

		if(array_get_length(&(trace->result_array))){
			log_warn_m("discarding outdated result(s) for fragment \"%s\"", trace->tag);
			for (i = 0; i < array_get_length(&(trace->result_array)); i++){
				result = (struct result*)array_get(&(trace->result_array), i);
				result_clean(result)
			}
			array_empty(&(trace->result_array));
		}
	}
	
	if (trace->mem_trace != NULL && trace->mem_trace->mem_addr_buffer != NULL){
		trace->ir = ir_create(&(trace->assembly), trace->mem_trace);
	}
	else{
		trace->ir = ir_create(&(trace->assembly), NULL);
	}

	
	if (trace->ir == NULL){
		log_err_m("unable to create IR for fragment \"%s\"", trace->tag);
	}
}

void trace_normalize_ir(struct trace* trace){
	uint32_t 		i;
	struct result* 	result;

	if (trace->ir == NULL){
		log_err_m("the IR is NULL for fragment \"%s\"", trace->tag);
		return;
	}

	for (i = 0; i < array_get_length(&(trace->result_array)); i++){
		result = (struct result*)array_get(&(trace->result_array), i);
		if (result->state != RESULTSTATE_IDLE){
			log_err_m("cannot normalize IR of fragment \"%s\" resulls have been exported", trace->tag);
			return;
		}
	}

	if(array_get_length(&(trace->result_array))){
		log_warn_m("discarding outdated result(s) for fragment \"%s\"", trace->tag);
		for (i = 0; i < array_get_length(&(trace->result_array)); i++){
			result = (struct result*)array_get(&(trace->result_array), i);
			result_clean(result)
		}
		array_empty(&(trace->result_array));
	}

	ir_normalize(trace->ir);
}

int32_t trace_register_code_signature_result(void* signature, struct array* assignement_array, void* arg){
	struct result 	result;
	int32_t 		return_value;

	if (result_init(&result, signature, assignement_array)){
		log_err("unable to init result");
		return -1;
	}

	if ((return_value = array_add(&(((struct trace*)arg)->result_array), &result)) < 0){
		log_err("unable to add element to array");
	}

	return return_value;
}

void trace_push_code_signature_result(int32_t idx, void* arg){
	struct trace* fragment = (struct trace*)arg; 

	if (idx >= 0){
		result_push((struct result*)array_get(&(fragment->result_array), idx), fragment->ir);
	}
	else{
		log_err_m("incorrect index value %d", idx);
	}
}

void trace_pop_code_signature_result(int32_t idx, void* arg){
	struct trace* fragment = (struct trace*)arg; 

	if (idx >= 0){
		result_pop((struct result*)array_get(&(fragment->result_array), idx), fragment->ir);
	}
	else{
		log_err_m("incorrect index value %d", idx);
	}
}

void trace_create_synthesis(struct trace* trace){
	if (trace->synthesis_graph != NULL){
		log_warn_m("an synthesis has already been create for fragment \"%s\" - deleting", trace->tag);
		synthesisGraph_delete(trace->synthesis_graph);
		trace->synthesis_graph = NULL;
	}

	if (trace->ir == NULL){
		log_err_m("the IR is NULL for fragment \"%s\"", trace->tag);
		return;
	}

	if((trace->synthesis_graph = synthesisGraph_create(trace->ir)) == NULL){
		log_err_m("unable to create synthesis graph for fragment \"%s\"", trace->tag);
	}
}

int32_t trace_concat(struct trace** trace_src_buffer, uint32_t nb_trace_src, struct trace* trace_dst){
	void** 				src_buffer;
	uint32_t 			i;

	src_buffer = (void**)alloca((sizeof(void*) * nb_trace_src));
	for (i = 0; i < nb_trace_src; i++){
		src_buffer[i] = &(trace_src_buffer[i]->assembly);
	}

	if (assembly_concat((struct assembly**)src_buffer, nb_trace_src, &(trace_dst->assembly))){
		log_err("unable to concat assembly");
		return -1;
	}
	
	if (trace_init(trace_dst, FRAGMENT_TRACE)){
		log_err("unable to init traceFragment");
		assembly_clean(&(trace_dst->assembly));
		return -1;
	}

	for (i = 0; i < nb_trace_src; i++){
		if (trace_src_buffer[i]->mem_trace == NULL){
			break;
		}
		src_buffer[i] = trace_src_buffer[i]->mem_trace;
	}
	if (i == nb_trace_src){
		if ((trace_dst->mem_trace = memTrace_create_concat((struct memTrace**)src_buffer, nb_trace_src)) == NULL){
			log_err("unable to concat memory trace");
		}
	}

	return 0;
}

void trace_check(struct trace* trace){
	if (assembly_check(&(trace->assembly))){
		log_err("assembly check failed");
	}
	if (trace->ir != NULL){
		ir_check(trace->ir);
	}
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
		log_err("unable to fetch first instruction from the assembly");
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
				log_err("unable to fetch next instruction from the assembly");
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
		log_err_m("the IR is NULL for fragment \"%s\"", trace->tag);
		goto exit;
	}

	exported_result = (uint32_t*)malloc(sizeof(uint32_t) * array_get_length(&(trace->result_array)));
	if (exported_result == NULL){
		log_err("unable to allocate memory");
		goto exit;
	}

	for (i = 0, nb_exported_result = 0; i < array_get_length(&(trace->result_array)); i++){
		result = (struct result*)array_get(&(trace->result_array), i);
		if (result->state != RESULTSTATE_IDLE){
			log_err_m("results have already been exported (%s), unable to export twice - rebuild IR", result->code_signature->signature.name);
			goto exit;
		}
		
		for (j = 0; j < nb_signature; j++){
			if (signature_buffer[j] == result->code_signature){
				#ifdef VERBOSE
				log_info_m("export %u occurrence(s) of %s in fragment %s", result->nb_occurrence, result->code_signature->signature.name, trace->tag);
				#endif
				exported_result[nb_exported_result ++] = i;
			}
		}
	}

	if (nb_exported_result == 0){
		log_warn("no exported result");
		goto exit;
	}

	node_set = set_create(sizeof(struct node*), 512);
	if (node_set == NULL){
		log_err("unable to create set");
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
		log_err("unable to export set");
		goto exit;
	}

	set_delete(node_set);
	node_set = NULL;

	/*ir_remove_footprint(trace->ir, footprint, nb_node_footprint);*/
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

void trace_reset(struct trace* trace){
	struct result* 	result;
	uint32_t 		i;

	if (trace->synthesis_graph != NULL){
		synthesisGraph_delete(trace->synthesis_graph);
		trace->synthesis_graph = NULL;
	}

	for (i = 0; i < array_get_length(&(trace->result_array)); i++){
		result = (struct result*)array_get(&(trace->result_array), i);
		result_clean(result)
	}
	array_empty(&(trace->result_array));

	if (trace->ir != NULL){
		ir_delete(trace->ir)
		trace->ir = NULL;
	}

	assembly_clean(&(trace->assembly));
	if (trace->mem_trace != NULL){
		memTrace_delete(trace->mem_trace);
		trace->mem_trace = NULL;
	}
}

void trace_clean(struct trace* trace){
	struct result* 	result;
	uint32_t 		i;

	if (trace->synthesis_graph != NULL){
		synthesisGraph_delete(trace->synthesis_graph);
	}

	for (i = 0; i < array_get_length(&(trace->result_array)); i++){
		result = (struct result*)array_get(&(trace->result_array), i);
		result_clean(result)
	}
	array_clean(&(trace->result_array));

	if (trace->ir != NULL){
		ir_delete(trace->ir)
	}

	assembly_clean(&(trace->assembly));

	if (trace->mem_trace != NULL){
		memTrace_delete(trace->mem_trace);
	}
}