#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>

#include "trace.h"
#include "assemblyElfLoader.h"
#include "result.h"
#include "base.h"

int32_t traceIdentifier_add(struct traceIdentifier* identifier, uint32_t pid, uint32_t tid){
	uint32_t i;

	for (i = 0; i < identifier->nb_process; i++){
		if (identifier->process[i].id == pid){
			if (identifier->process[i].nb_thread == TRACE_NB_MAX_THREAD){
				log_err("the max number of thread has been reached, increment TRACE_NB_MAX_THREAD");
				return -1;
			}

			identifier->process[i].thread_id[identifier->process[i].nb_thread] = tid;
			identifier->process[i].nb_thread ++;
			break;
		}
	}
	if (i == identifier->nb_process){
		if (identifier->nb_process == TRACE_NB_MAX_PROCESS){
			log_err("the max number of process has been reached, increment TRACE_NB_MAX_PROCESS");
			return -1;
		}
			
		identifier->process[i].id = pid;
		identifier->process[i].thread_id[0] = tid;
		identifier->process[i].nb_thread = 1;
		identifier->nb_process ++;
	}

	return 0;
}

int32_t traceIdentifier_select(struct traceIdentifier* identifier, uint32_t p_index, uint32_t t_index){
	uint32_t i;

	if (p_index >= identifier->nb_process){
		log_err_m("process index (%u) is out of bound", p_index);
		return -1;
	}

	if (t_index >= identifier->process[p_index].nb_thread){
		log_err_m("thread index (%u) is out of bound", t_index);
		return -1;
	}

	if (identifier->nb_process > 1){
		log_info_m("several processes have been found, loading %u:", p_index);
		for (i = 0; i < identifier->nb_process; i++){
			if (i == p_index){
				printf("\t- process: %u; pid=%u. %u thread(s) (loaded)\n", i, identifier->process[i].id, identifier->process[i].nb_thread);
			}
			else{
				printf("\t- process: %u; pid=%u; %u thread(s)\n", i, identifier->process[i].id, identifier->process[i].nb_thread);
			}
		}
		printf("Use: \"change trace\" command to load a different process/tread\n");
	}

	if (identifier->process[p_index].nb_thread > 1){
		log_info_m("several threads have been found for process %u, loading %u:", identifier->process[p_index].id, t_index);
		for (i = 0; i < identifier->process[p_index].nb_thread; i++){
			if (i == t_index){
				printf("\t- thread: %u; tid=%u (loaded)\n", i, identifier->process[p_index].thread_id[i]);
			}
			else{
				printf("\t- thread: %u; tid=%u\n", i, identifier->process[p_index].thread_id[i]);
			}
		}
		printf("Use: \"change trace\" command to load a different process/thread\n");
	}

	identifier->current_pid = identifier->process[p_index].id;
	identifier->current_tid = identifier->process[p_index].thread_id[t_index];

	return 0;
}

static inline int32_t trace_init(struct trace* trace, enum traceType type){
	int32_t result = 0;

	trace->type 		= type;
	trace->mem_trace 	= NULL;
	switch(type){
		case EXECUTION_TRACE 	: {
			traceIdentifier_init(&(trace->trace_type.exe.identifier));
			break;
		}
		case ELF_TRACE 			: {
			break;
		}
		case FRAGMENT_TRACE 	: {
			trace->trace_type.frag.tag[0]			= '\0';
			trace->trace_type.frag.ir 				= NULL;
			trace->trace_type.frag.synthesis_graph 	= NULL;
			result = array_init(&(trace->trace_type.frag.result_array), sizeof(struct result));
			break;
		}
	}

	return result;
}

struct trace* trace_load_exe(const char* directory_path){
	struct trace* 			trace;
	char 					file1_path[TRACE_PATH_MAX_LENGTH];
	char 					file2_path[TRACE_PATH_MAX_LENGTH];
	DIR* 					directory;
	struct dirent* 			entry;
	size_t 					offset;

	if ((trace = (struct trace*)malloc(sizeof(struct trace))) == NULL){
		log_err("unable to allocate memory");
		return NULL;
	}

	if (trace_init(trace, EXECUTION_TRACE)){
		log_err("unable to init executionTrace");
		goto error;
	}

	if ((directory = opendir(directory_path)) == NULL){
		log_err_m("unable to open directory: \"%s\"", directory_path);
		goto error;
	}

	while ((entry = readdir(directory)) != NULL){
		if (!memcmp(entry->d_name, "blockId", 7)){
			uint32_t pid;
			uint32_t tid;

			offset = strspn(entry->d_name + 7, "0123456789");
			if (offset == 0){
				continue;
			}

			offset += 7;

			if (entry->d_name[offset ++] != '_'){
				continue;
			}

			pid = atoi(entry->d_name + 7);
			tid = atoi(entry->d_name + offset);

			offset += strspn(entry->d_name + offset, "0123456789");

			if (strcmp(entry->d_name + offset, ".bin")){
				continue;
			}

			if (traceIdentifier_add(&(trace->trace_type.exe.identifier), pid, tid)){
				log_err_m("unable to add (pid=%u, tid=%u) to traceIdentifier", pid, tid);
			}
		}
	}
	closedir(directory);

	if (traceIdentifier_select(&(trace->trace_type.exe.identifier), 0, 0)){
		log_err("unable to load default trace identifier");
		goto error;
	}
		
	strncpy(trace->trace_type.exe.directory_path, directory_path, TRACE_PATH_MAX_LENGTH);

	snprintf(file1_path, TRACE_PATH_MAX_LENGTH, "%s/blockId%u_%u.bin", directory_path, trace->trace_type.exe.identifier.current_pid, trace->trace_type.exe.identifier.current_tid);
	snprintf(file2_path, TRACE_PATH_MAX_LENGTH, "%s/block%u.bin", directory_path, trace->trace_type.exe.identifier.current_pid);
			
	if (assembly_load_trace(&(trace->assembly), file1_path, file2_path)){
		log_err("unable to init assembly structure");
		goto error;
	}

	if (memTrace_is_trace_exist(trace->trace_type.exe.directory_path, trace->trace_type.exe.identifier.current_pid, trace->trace_type.exe.identifier.current_tid)){
		if ((trace->mem_trace = memTrace_create_trace(trace->trace_type.exe.directory_path, trace->trace_type.exe.identifier.current_pid, trace->trace_type.exe.identifier.current_tid, &(trace->assembly))) == NULL){
			log_err("unable to load associated memory addresses");
		}
	}

	return trace;

	error:
	free(trace);

	return NULL;
}

int32_t trace_change(struct trace* trace, uint32_t p_index, uint32_t t_index){
	char file1_path[TRACE_PATH_MAX_LENGTH];
	char file2_path[TRACE_PATH_MAX_LENGTH];

	if (trace->type == EXECUTION_TRACE){
		if (traceIdentifier_select(&(trace->trace_type.exe.identifier), p_index, t_index)){
			log_err_m("unable to load trace identifier (process index: %u; thread index: %u)", p_index, t_index);
			return -1;
		}

		trace_reset(trace);

		snprintf(file1_path, TRACE_PATH_MAX_LENGTH, "%s/blockId%u_%u.bin", trace->trace_type.exe.directory_path, trace->trace_type.exe.identifier.current_pid, trace->trace_type.exe.identifier.current_tid);
		snprintf(file2_path, TRACE_PATH_MAX_LENGTH, "%s/block%u.bin", trace->trace_type.exe.directory_path, trace->trace_type.exe.identifier.current_pid);
			
		if (assembly_load_trace(&(trace->assembly), file1_path, file2_path)){
			log_err("unable to init assembly structure");
			return -1;
		}

		if (memTrace_is_trace_exist(trace->trace_type.exe.directory_path, trace->trace_type.exe.identifier.current_pid, trace->trace_type.exe.identifier.current_tid)){
			if ((trace->mem_trace = memTrace_create_trace(trace->trace_type.exe.directory_path, trace->trace_type.exe.identifier.current_pid, trace->trace_type.exe.identifier.current_tid, &(trace->assembly))) == NULL){
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
	uint64_t 		index_mem_start;
	uint64_t 		index_mem_stop;
	struct array* 	extrude_array = NULL;

	if (assembly_extract_segment(&(trace_src->assembly), &(trace_dst->assembly), offset, length, &index_mem_start, &index_mem_stop)){
		log_err("unable to extract assembly fragment");
		return -1;
	}

	if (assembly_filter_blacklisted_function_call(&(trace_dst->assembly), &extrude_array)){
		log_err("unable to filter assembly fragment");
		assembly_clean(&(trace_dst->assembly));
		if (extrude_array != NULL){
			array_delete(extrude_array);
		}
		return - 1;
	}

	if (trace_init(trace_dst, FRAGMENT_TRACE)){
		log_err("unable to init traceFragment");
		assembly_clean(&(trace_dst->assembly));
		array_delete(extrude_array);
		return - 1;
	}

	if (trace_src->mem_trace != NULL && (trace_dst->mem_trace = memTrace_create_frag(trace_src->mem_trace, index_mem_start, index_mem_stop, extrude_array)) == NULL){
		log_err("unable to extract memory trace");
	}

	array_delete(extrude_array);

	snprintf(trace_dst->trace_type.frag.tag, TRACE_TAG_LENGTH, "trace [%u:%u]", offset, offset + length);

	return 0;
}

void trace_create_ir(struct trace* trace){
	uint32_t 		i;
	struct result* 	result;

	if (trace->type != FRAGMENT_TRACE){
		log_err("wrong trace type");
		return;
	}

	if (trace->trace_type.frag.ir != NULL){
		log_warn_m("an IR has already been built for fragment \"%s\" - deleting", trace->trace_type.frag.tag);
		ir_delete(trace->trace_type.frag.ir);

		if(array_get_length(&(trace->trace_type.frag.result_array))){
			log_warn_m("discarding outdated result(s) for fragment \"%s\"", trace->trace_type.frag.tag);
			for (i = 0; i < array_get_length(&(trace->trace_type.frag.result_array)); i++){
				result = (struct result*)array_get(&(trace->trace_type.frag.result_array), i);
				result_clean(result)
			}
			array_empty(&(trace->trace_type.frag.result_array));
		}
	}
	
	if (trace->mem_trace != NULL && trace->mem_trace->mem_addr_buffer != NULL){
		trace->trace_type.frag.ir = ir_create(&(trace->assembly), trace->mem_trace);
	}
	else{
		trace->trace_type.frag.ir = ir_create(&(trace->assembly), NULL);
	}

	
	if (trace->trace_type.frag.ir == NULL){
		log_err_m("unable to create IR for fragment \"%s\"", trace->trace_type.frag.tag);
	}
}

void trace_normalize_ir(struct trace* trace){
	uint32_t 		i;
	struct result* 	result;

	if (trace->type != FRAGMENT_TRACE){
		log_err("wrong trace type");
		return;
	}

	if (trace->trace_type.frag.ir == NULL){
		log_err_m("the IR is NULL for fragment \"%s\"", trace->trace_type.frag.tag);
		return;
	}

	for (i = 0; i < array_get_length(&(trace->trace_type.frag.result_array)); i++){
		result = (struct result*)array_get(&(trace->trace_type.frag.result_array), i);
		if (result->state != RESULTSTATE_IDLE){
			log_err_m("cannot normalize IR of fragment \"%s\" results have been exported", trace->trace_type.frag.tag);
			return;
		}
	}

	if(array_get_length(&(trace->trace_type.frag.result_array))){
		log_warn_m("discarding outdated result(s) for fragment \"%s\"", trace->trace_type.frag.tag);
		for (i = 0; i < array_get_length(&(trace->trace_type.frag.result_array)); i++){
			result = (struct result*)array_get(&(trace->trace_type.frag.result_array), i);
			result_clean(result)
		}
		array_empty(&(trace->trace_type.frag.result_array));
	}

	ir_normalize(trace->trace_type.frag.ir);
}

void trace_search_irComponent(struct trace* trace_ext, struct trace* trace_inn, struct array* ir_component_array){
	struct irComponent 			ir_component;
	struct instructionIterator 	it;
	int32_t 					status;

	if (trace_ext->type != FRAGMENT_TRACE){
		log_err("wrong trace type");
		return;
	}

	if (trace_inn->type != FRAGMENT_TRACE || trace_inn->trace_type.frag.ir == NULL){
		return;
	}

	if (trace_ext->mem_trace != NULL && trace_ext->mem_trace->mem_addr_buffer != NULL){
		if (trace_ext->mem_trace == NULL || trace_ext->mem_trace->mem_addr_buffer == NULL){
			return;
		}
	}
	else{
		if (trace_ext->mem_trace != NULL && trace_ext->mem_trace->mem_addr_buffer != NULL){
			return;
		}
	}
	
	for (status = assembly_get_first_instruction(&(trace_ext->assembly), &it);  status == 0 && !assembly_search_sub_sequence(&(trace_ext->assembly), &(trace_inn->assembly), &it); status = assembly_get_next_instruction(&(trace_ext->assembly), &it)){
		ir_component.instruction_start 	= it.instruction_index;
		ir_component.instruction_stop 	= ir_component.instruction_start + assembly_get_nb_instruction(&(trace_inn->assembly));
		ir_component.ir 				= trace_inn->trace_type.frag.ir;

		if (trace_ext->mem_trace != NULL && trace_ext->mem_trace->mem_addr_buffer != NULL){
			if (memAddress_buffer_compare(trace_ext->mem_trace->mem_addr_buffer + instructionIterator_get_mem_addr_index(&it), trace_inn->mem_trace->mem_addr_buffer, trace_inn->mem_trace->nb_mem_addr)){
				goto next;
			}
		}
		
		log_info_m("found component [%u:%u] fragment \"%s\"", ir_component.instruction_start, ir_component.instruction_stop, trace_inn->trace_type.frag.tag);

		if (array_add(ir_component_array, &ir_component) < 0){
			log_err("unable to add element to array");
		}

		next:
		if (ir_component.instruction_stop == assembly_get_nb_instruction(&(trace_ext->assembly))){
			break;
		}
	}
	if (status){
		log_err("unable to fetch next instruction from the assembly");
	}
}

static int32_t irComponent_compare(void* arg1, void* arg2){
	struct irComponent* component1 = (struct irComponent*)arg1;
	struct irComponent* component2 = (struct irComponent*)arg2;

	if (component1->instruction_start < component2->instruction_start){
		return -1;
	}
	else if (component1->instruction_start < component2->instruction_start){
		return 1;
	}
	else if (component1->instruction_stop > component2->instruction_stop){
		return -1;
	}
	else if (component1->instruction_stop < component2->instruction_stop){
		return 1;
	}
	else{
		return 0;
	}
}

void trace_create_compound_ir(struct trace* trace, struct array* ir_component_array){
	uint32_t* 				mapping;
	uint32_t 				i;
	uint32_t 				nb_component;
	struct result* 			result;
	struct irComponent* 	component1;
	struct irComponent* 	component2;
	struct irComponent** 	component_buffer;

	if (trace->type != FRAGMENT_TRACE){
		log_err("wrong trace type");
		return;
	}

	if (trace->trace_type.frag.ir != NULL){
		log_warn_m("an IR has already been built for fragment \"%s\" - deleting", trace->trace_type.frag.tag);
		ir_delete(trace->trace_type.frag.ir);

		if(array_get_length(&(trace->trace_type.frag.result_array))){
			log_warn_m("discarding outdated result(s) for fragment \"%s\"", trace->trace_type.frag.tag);
			for (i = 0; i < array_get_length(&(trace->trace_type.frag.result_array)); i++){
				result = (struct result*)array_get(&(trace->trace_type.frag.result_array), i);
				result_clean(result)
			}
			array_empty(&(trace->trace_type.frag.result_array));
		}
	}

	if ((mapping = array_create_mapping(ir_component_array, irComponent_compare)) == NULL){
		log_err("unable to create array mappping");
		return;
	}

	for (i = 1, nb_component = min(1, array_get_length(ir_component_array)); i < array_get_length(ir_component_array); i++){
		component1 = (struct irComponent*)array_get(ir_component_array, mapping[nb_component - 1]);
		component2 = (struct irComponent*)array_get(ir_component_array, mapping[i]);

		if (component2->instruction_start < component1->instruction_stop){
			log_warn("overlapping component, taking the largest");
		}
		else{
			mapping[nb_component] = mapping[i];
			nb_component ++;
		}
	}

	log_info_m("found %u already processed instructions sequence(s) in fragment \"%s\"", nb_component, trace->trace_type.frag.tag);

	if (nb_component > 0){
		component_buffer = (struct irComponent**)malloc(sizeof(struct irComponent*) * nb_component);
		if (component_buffer == NULL){
			log_err("unable to allocate memory");
		}
		else{
			for (i = 0; i < nb_component; i++){
				component_buffer[i] = (struct irComponent*)array_get(ir_component_array, mapping[i]);
			}

			if (trace->mem_trace != NULL && trace->mem_trace->mem_addr_buffer != NULL){
				trace->trace_type.frag.ir = ir_create_compound(&(trace->assembly), trace->mem_trace, component_buffer, nb_component);
			}
			else{
				trace->trace_type.frag.ir = ir_create_compound(&(trace->assembly), NULL, component_buffer, nb_component);
			}

			free(component_buffer);
		}
	}
	else{
		if (trace->mem_trace != NULL && trace->mem_trace->mem_addr_buffer != NULL){
			trace->trace_type.frag.ir = ir_create(&(trace->assembly), trace->mem_trace);
		}
		else{
			trace->trace_type.frag.ir = ir_create(&(trace->assembly), NULL);
		}
	}

	if (trace->trace_type.frag.ir == NULL){
		log_err_m("unable to create IR for fragment \"%s\"", trace->trace_type.frag.tag);
	}

	free(mapping);
}

int32_t trace_register_code_signature_result(void* signature, struct array* assignement_array, void* arg){
	struct result 	result;
	int32_t 		return_value;
	struct trace* 	trace = (struct trace*)arg;

	if (trace->type != FRAGMENT_TRACE){
		log_err("wrong trace type");
		return -1;
	}

	if (result_init(&result, signature, assignement_array)){
		log_err("unable to init result");
		return -1;
	}

	if ((return_value = array_add(&(trace->trace_type.frag.result_array), &result)) < 0){
		log_err("unable to add element to array");
	}

	return return_value;
}

void trace_push_code_signature_result(int32_t idx, void* arg){
	struct trace* 	trace = (struct trace*)arg;

	if (trace->type != FRAGMENT_TRACE){
		log_err("wrong trace type");
		return;
	}

	if (idx >= 0){
		result_push((struct result*)array_get(&(trace->trace_type.frag.result_array), idx), trace->trace_type.frag.ir);
	}
	else{
		log_err_m("incorrect index value %d", idx);
	}
}

void trace_pop_code_signature_result(int32_t idx, void* arg){
	struct trace* trace = (struct trace*)arg;

	if (trace->type != FRAGMENT_TRACE){
		log_err("wrong trace type");
		return;
	}

	if (idx >= 0){
		result_pop((struct result*)array_get(&(trace->trace_type.frag.result_array), idx), trace->trace_type.frag.ir);
	}
	else{
		log_err_m("incorrect index value %d", idx);
	}
}

void trace_create_synthesis(struct trace* trace){
	if (trace->type != FRAGMENT_TRACE){
		log_err("wrong trace type");
		return;
	}

	if (trace->trace_type.frag.synthesis_graph != NULL){
		log_warn_m("an synthesis has already been create for fragment \"%s\" - deleting", trace->trace_type.frag.tag);
		synthesisGraph_delete(trace->trace_type.frag.synthesis_graph);
		trace->trace_type.frag.synthesis_graph = NULL;
	}

	if (trace->trace_type.frag.ir == NULL){
		log_err_m("the IR is NULL for fragment \"%s\"", trace->trace_type.frag.tag);
		return;
	}

	if((trace->trace_type.frag.synthesis_graph = synthesisGraph_create(trace->trace_type.frag.ir)) == NULL){
		log_err_m("unable to create synthesis graph for fragment \"%s\"", trace->trace_type.frag.tag);
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
	if (trace->type == FRAGMENT_TRACE && trace->trace_type.frag.ir != NULL){
		if (ir_check(trace->trace_type.frag.ir)){
			log_info("error(s) have been found in IR");
		}
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

	if (trace->type != FRAGMENT_TRACE){
		log_err("wrong trace type");
		return;
	}

	if (trace->trace_type.frag.ir == NULL){
		log_err_m("the IR is NULL for fragment \"%s\"", trace->trace_type.frag.tag);
		return;
	}

	exported_result = (uint32_t*)malloc(sizeof(uint32_t) * array_get_length(&(trace->trace_type.frag.result_array)));
	if (exported_result == NULL){
		log_err("unable to allocate memory");
		return;
	}

	for (i = 0, nb_exported_result = 0; i < array_get_length(&(trace->trace_type.frag.result_array)); i++){
		result = (struct result*)array_get(&(trace->trace_type.frag.result_array), i);
		if (result->state != RESULTSTATE_IDLE){
			log_err_m("results have already been exported (%s), unable to export twice - rebuild IR", result->code_signature->signature.name);
			goto exit;
		}
		
		for (j = 0; j < nb_signature; j++){
			if (signature_buffer[j] == result->code_signature){
				#ifdef VERBOSE
				log_info_m("export %u occurrence(s) of %s in fragment %s", result->nb_occurrence, result->code_signature->signature.name, trace->trace_type.frag.tag);
				#endif
				exported_result[nb_exported_result ++] = i;
				continue;
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
		result = (struct result*)array_get(&(trace->trace_type.frag.result_array), exported_result[i]);
		result_push(result, trace->trace_type.frag.ir);
		
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

	ir_remove_footprint(trace->trace_type.frag.ir, footprint, nb_node_footprint);
	ir_normalize_remove_dead_code(trace->trace_type.frag.ir, NULL);

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

	if (trace->type == FRAGMENT_TRACE){
		if (trace->trace_type.frag.synthesis_graph != NULL){
			synthesisGraph_delete(trace->trace_type.frag.synthesis_graph);
			trace->trace_type.frag.synthesis_graph = NULL;
		}

		for (i = 0; i < array_get_length(&(trace->trace_type.frag.result_array)); i++){
			result = (struct result*)array_get(&(trace->trace_type.frag.result_array), i);
			result_clean(result)
		}
		array_empty(&(trace->trace_type.frag.result_array));

		if (trace->trace_type.frag.ir != NULL){
			ir_delete(trace->trace_type.frag.ir)
			trace->trace_type.frag.ir = NULL;
		}
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

	if (trace->type == FRAGMENT_TRACE){
		if (trace->trace_type.frag.synthesis_graph != NULL){
			synthesisGraph_delete(trace->trace_type.frag.synthesis_graph);
		}

		for (i = 0; i < array_get_length(&(trace->trace_type.frag.result_array)); i++){
			result = (struct result*)array_get(&(trace->trace_type.frag.result_array), i);
			result_clean(result)
		}
		array_clean(&(trace->trace_type.frag.result_array));

		if (trace->trace_type.frag.ir != NULL){
			ir_delete(trace->trace_type.frag.ir)
		}
	}

	assembly_clean(&(trace->assembly));

	if (trace->mem_trace != NULL){
		memTrace_delete(trace->mem_trace);
	}
}