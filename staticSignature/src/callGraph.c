#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>

#include "callGraph.h"
#include "base.h"

enum blockLabel{
	BLOCK_LABEL_UNKOWN 	= 0,
	BLOCK_LABEL_CALL 	= 1,
	BLOCK_LABEL_RET 	= 2,
	BLOCK_LABEL_NONE 	= 3
};

#define CALLGRAPH_MAX_DEPTH 	4096
#define CALLGRAPH_START_DEPTH 	256

#define CALLGRAPH_ENABLE_LABEL_CHECK 1

static enum blockLabel callGraph_label_block(struct asmBlock* block);
static enum blockLabel* callGraph_alloc_label_buffer(struct assembly* assembly);
static enum blockLabel* callGraph_fill_label_buffer(struct assembly* assembly);

static int32_t function_add_snippet(struct callGraph* call_graph, struct function* func, uint32_t start, uint32_t stop, ADDRESS expected_next_address);
static int32_t function_get_first_snippet(struct callGraph* call_graph, struct function* func);

#pragma GCC diagnostic ignored "-Wunused-parameter"
void callGraph_dotPrint_node(void* data, FILE* file, void* arg){
	struct function* func = (struct function*)data;

	if (function_is_invalid(func)){
		fputs("[label=\"-\"]", file);
	}
	else{
		if (func->routine != NULL){
			fprintf(file, "[label=\"%s\"]", func->routine->name);
		}
		else{
			fputs("[label=\"NULL\"]", file);
		}
	}
}

#pragma GCC diagnostic ignored "-Wunused-parameter"
void callGraph_dotPrint_edge(void* data, FILE* file, void* arg){
	struct callGraphEdge* edge = (struct callGraphEdge*)data;

	switch(edge->type){
		case CALLGRAPH_EDGE_CALL 	: {fputs("[label=\"call\"]", file); break;}
		case CALLGRAPH_EDGE_RET 	: {fputs("[label=\"ret\"]", file); break;}
	}
}

struct callGraph* callGraph_create(struct assembly* assembly, uint32_t start, uint32_t stop){
	struct callGraph* call_graph;

	call_graph = (struct callGraph*)malloc(sizeof(struct callGraph));
	if (call_graph != NULL){
		if (callGraph_init(call_graph, assembly, start, stop)){
			log_err("unable to init callGraph");
			free(call_graph);
			call_graph = NULL;
		}
	}
	else{
		log_err("unable to allocate memory");
	}

	return call_graph;
}

int32_t callGraph_init(struct callGraph* call_graph, struct assembly* assembly, uint32_t start, uint32_t stop){
	struct node** 				call_graph_stack 	= NULL;
	uint32_t 					stack_index			= CALLGRAPH_START_DEPTH;
	enum blockLabel* 			label_buffer 		= NULL;
	uint32_t 					snippet_start 		= 0;
	uint32_t 					snippet_stop;
	struct callGraphEdge 		call_graph_edge;
	int32_t 					result 				= 0;
	uint32_t 					i;
	struct function* 			current_func;

	call_graph_stack = (struct node**)calloc(sizeof(struct node*), CALLGRAPH_MAX_DEPTH);
	if (call_graph_stack == NULL){
		log_err("unable to allocate memory");
		result = -1;
		goto exit;
	}

	label_buffer = callGraph_fill_label_buffer(assembly);
	if (label_buffer == NULL){
		log_err("unable to label assembly blocks");
		result = -1;
		goto exit;
	}

	if (array_init(&(call_graph->snippet_array), sizeof(struct assemblySnippet))){
		log_err("unable to init array");
		result = -1;
		goto exit;
	}

	graph_init(&(call_graph->graph), sizeof(struct function), sizeof(struct callGraphEdge))
	graph_register_dotPrint_callback(&(call_graph->graph), NULL, callGraph_dotPrint_node, callGraph_dotPrint_edge, NULL)

#define callGraph_is_last_dyn_block(assembly, i, stop) ((assembly)->dyn_blocks[i].instruction_count + (assembly)->dyn_blocks[i].block->header.nb_ins >= (stop))

#define callGraph_stack_call() 																															\
	{ 																																					\
		if (++ stack_index == CALLGRAPH_MAX_DEPTH){ 																									\
			log_err("the top of the stack has been reached"); 																							\
			result = -1; 																																\
			goto exit; 																																	\
		} 																																				\
																																						\
		if ((call_graph_stack[stack_index] = graph_add_node_(&(call_graph->graph))) == NULL){ 															\
			log_err("unable to create node"); 																											\
			result = -1; 																																\
			goto exit; 																																	\
		} 																																				\
																																						\
		current_func = callGraph_node_get_function(call_graph_stack[stack_index]); 																		\
		function_init_valid(current_func) 																												\
																																						\
		call_graph_edge.type = CALLGRAPH_EDGE_CALL; 																									\
		if (graph_add_edge(&(call_graph->graph), call_graph_stack[stack_index - 1], call_graph_stack[stack_index], &call_graph_edge) == NULL){ 			\
			log_err("unable to add edge to callGraph"); 																								\
		} 																																				\
	}

#define callGraph_stack_ret() 																															\
	{ 																																					\
		if (stack_index-- == 0){ 																														\
			log_err("the bottom of the stack has been reached"); 																						\
			result = -1; 																																\
			goto exit; 																																	\
		} 																																				\
																																						\
		if (call_graph_stack[stack_index] == NULL){ 																									\
			if ((call_graph_stack[stack_index] = graph_add_node_(&(call_graph->graph))) == NULL){ 														\
				log_err("unable to create node"); 																										\
				result = -1; 																															\
				goto exit; 																																\
			} 																																			\
																																						\
			current_func = callGraph_node_get_function(call_graph_stack[stack_index]); 																	\
			function_init_valid(current_func) 																											\
		} 																																				\
																																						\
		call_graph_edge.type = CALLGRAPH_EDGE_RET; 																										\
		if (graph_add_edge(&(call_graph->graph), call_graph_stack[stack_index + 1], call_graph_stack[stack_index], &call_graph_edge) == NULL){ 			\
			log_err("unable to add edge to callGraph"); 																								\
		} 																																				\
	}

	for (i = 0; i < assembly->nb_dyn_block; i++){
		if (assembly->dyn_blocks[i].instruction_count < start){
			if (dynBlock_is_valid(assembly->dyn_blocks + i) && assembly->dyn_blocks[i].instruction_count + assembly->dyn_blocks[i].block->header.nb_ins > start){
				log_warn_m("index %u is not at a the beginning of a basic block, rounding", start);
			}
			else{
				continue;
			}
		}
		if (assembly->dyn_blocks[i].instruction_count >= stop){
			break;
		}

		if (dynBlock_is_invalid(assembly->dyn_blocks + i)){
			if (call_graph_stack[stack_index] != NULL){
				current_func = callGraph_node_get_function(call_graph_stack[stack_index]);
				if (function_is_valid(current_func)){
					function_add_snippet(call_graph, current_func, snippet_start, assembly->dyn_blocks[i].instruction_count, 0);
					function_set_invalid(current_func);
				}
			}
			else{
				call_graph_stack[stack_index] = graph_add_node_(&(call_graph->graph));
				if (call_graph_stack[stack_index] == NULL){
					log_err("unable to create node");
					result= -1;
					goto exit;
				}

				current_func = callGraph_node_get_function(call_graph_stack[stack_index]);
				function_init_invalid(current_func)
				snippet_start = assembly->dyn_blocks[i].instruction_count;
			}
		}
		else{
			snippet_stop = assembly->dyn_blocks[i].instruction_count + assembly->dyn_blocks[i].block->header.nb_ins;

			if (call_graph_stack[stack_index] != NULL){
				current_func = callGraph_node_get_function(call_graph_stack[stack_index]);
				if (function_is_invalid(current_func)){
					if (stack_index > 0 && call_graph_stack[stack_index - 1] != NULL){
						if (callGraph_node_get_function(call_graph_stack[stack_index - 1])->last_snippet_offset >= 0){
							if (((struct assemblySnippet*)array_get(&(call_graph->snippet_array), callGraph_node_get_function(call_graph_stack[stack_index - 1])->last_snippet_offset))->expected_next_address == assembly->dyn_blocks[i].block->header.address){
								callGraph_stack_ret()
								snippet_start = assembly->dyn_blocks[i].instruction_count;
							}
							else{
								callGraph_stack_call()
								snippet_start = assembly->dyn_blocks[i].instruction_count;
							}
						}
						else{
							log_err("the previous function call on the stack is empty");
							result = -1;
							goto exit;
						}
					}
					else{
						log_warn_m("unable to classify current basic block %u as a RET or a CALL", i);
						callGraph_stack_call()
						snippet_start = assembly->dyn_blocks[i].instruction_count;
					}
				}
			}
			else{
				call_graph_stack[stack_index] = graph_add_node_(&(call_graph->graph));
				if (call_graph_stack[stack_index] == NULL){
					log_err("unable to create node");
					result = -1;
					goto exit;
				}

				current_func = callGraph_node_get_function(call_graph_stack[stack_index]);
				function_init_valid(current_func)
				snippet_start = assembly->dyn_blocks[i].instruction_count;
			}

			current_func = callGraph_node_get_function(call_graph_stack[stack_index]);
			switch(label_buffer[assembly->dyn_blocks[i].block->header.id - FIRST_BLOCK_ID]){
				case BLOCK_LABEL_UNKOWN : {
					log_err("this case is not supposed to happen");
					result = -1;
					goto exit;
				}
				case BLOCK_LABEL_CALL 	: {
					function_add_snippet(call_graph, current_func, snippet_start, snippet_stop, assembly->dyn_blocks[i].block->header.address + assembly->dyn_blocks[i].block->header.size);
					if (!callGraph_is_last_dyn_block(assembly, i, stop)){
						callGraph_stack_call()
						snippet_start = snippet_stop;
					}
					break;
				}
				case BLOCK_LABEL_RET 	: {
					function_add_snippet(call_graph, current_func, snippet_start, snippet_stop, assembly->dyn_blocks[i].block->header.address + assembly->dyn_blocks[i].block->header.size);
					if (!callGraph_is_last_dyn_block(assembly, i, stop)){
						callGraph_stack_ret()
						snippet_start = snippet_stop;
					}
					break;
				}
				case BLOCK_LABEL_NONE 	: {
					if (callGraph_is_last_dyn_block(assembly, i, stop)){
						function_add_snippet(call_graph, current_func, snippet_start, snippet_stop, assembly->dyn_blocks[i].block->header.address + assembly->dyn_blocks[i].block->header.size);
					}
					break;
				}
			}
		}
	}

	exit:
	if (label_buffer != NULL){
		free(label_buffer);
	}
	if (call_graph_stack != NULL){
		free(call_graph_stack);
	}

	return result;
}

void callGraph_print_frame(struct assembly* assembly, uint32_t index, struct codeMap* code_map){
	uint32_t 			index_start;
	uint32_t 			index_stop;
	uint32_t 			base_block_index;
	uint32_t 			i;
	enum blockLabel* 	label_buffer 	= NULL;
	uint32_t 			stack;
	uint32_t 			is_not_leaf 	= 0;

	if (assembly_get_dyn_block_ins(assembly, index, &base_block_index)){
		log_err_m("unable to locate the dyn block containing the index %u", index);
		return;
	}

	if ((label_buffer = callGraph_alloc_label_buffer(assembly)) == NULL){
		log_err("alloc of label buffer failed");
		return;
	}

	for (i = base_block_index, stack = 0; i + 1 < assembly->nb_dyn_block; i++){
		if (dynBlock_is_invalid(assembly->dyn_blocks + i)){
			log_warn_m("found a black listed block @ %u, this case is not handled yet. Result might be incorrect", assembly->dyn_blocks[i].instruction_count);
		}
		else{
			if (label_buffer[assembly->dyn_blocks[i].block->header.id - FIRST_BLOCK_ID] == BLOCK_LABEL_UNKOWN){
				label_buffer[assembly->dyn_blocks[i].block->header.id - FIRST_BLOCK_ID] = callGraph_label_block(assembly->dyn_blocks[i].block);
			}

			if (label_buffer[assembly->dyn_blocks[i].block->header.id - FIRST_BLOCK_ID] == BLOCK_LABEL_CALL){
				stack ++;
				is_not_leaf ++;
			}
			else if (label_buffer[assembly->dyn_blocks[i].block->header.id - FIRST_BLOCK_ID] == BLOCK_LABEL_RET){
				if (stack-- == 0){
					break;
				}
			}
		}
	}

	index_stop = i;

	for (i = base_block_index, stack = 0; i; i--){
		if (dynBlock_is_invalid(assembly->dyn_blocks + i - 1)){
			log_warn_m("found a black listed block @ %u, this case is not handled yet. Result might be incorrect", assembly->dyn_blocks[i - 1].instruction_count);
		}
		else{
			if (label_buffer[assembly->dyn_blocks[i - 1].block->header.id - FIRST_BLOCK_ID] == BLOCK_LABEL_UNKOWN){
				label_buffer[assembly->dyn_blocks[i - 1].block->header.id - FIRST_BLOCK_ID] = callGraph_label_block(assembly->dyn_blocks[i - 1].block);
			}

			if (label_buffer[assembly->dyn_blocks[i - 1].block->header.id - FIRST_BLOCK_ID] == BLOCK_LABEL_CALL){
				if (stack -- == 0){
					break;
				}
			}
			else if (label_buffer[assembly->dyn_blocks[i - 1].block->header.id - FIRST_BLOCK_ID] == BLOCK_LABEL_RET){
				stack ++;
				is_not_leaf ++;
			}
		}
	}

	index_start = i;

	if (dynBlock_is_invalid(assembly->dyn_blocks + index_start) || dynBlock_is_invalid(assembly->dyn_blocks + index_stop)){
		log_err("frame boundary is a black listed block");
	}
	else{
		printf("Frame: index = %u, leaf = %s, size = %u", index, (is_not_leaf) ? (ANSI_COLOR_BOLD_RED "N" ANSI_COLOR_RESET) : (ANSI_COLOR_BOLD_GREEN "Y" ANSI_COLOR_RESET), assembly->dyn_blocks[index_stop].instruction_count + assembly->dyn_blocks[index_stop].block->header.nb_ins - assembly->dyn_blocks[index_start].instruction_count);
		if (code_map != NULL){
			struct cm_routine* rtn;

			if ((rtn = codeMap_search_routine(code_map, assembly->dyn_blocks[index_start].block->header.address)) != NULL){
				printf(", RTN:%s+" PRINTF_ADDR_SHORT, rtn->name, assembly->dyn_blocks[index_start].block->header.address - rtn->address_start);
			}
		}

		printf("\n\t- Start: index = %u , addr = " PRINTF_ADDR "\n", assembly->dyn_blocks[index_start].instruction_count, assembly->dyn_blocks[index_start].block->header.address);
		printf("\t- Stop : index = %u , addr = " PRINTF_ADDR "\n", assembly->dyn_blocks[index_stop].instruction_count + assembly->dyn_blocks[index_stop].block->header.nb_ins, assembly->dyn_blocks[index_stop].block->header.address + assembly->dyn_blocks[index_stop].block->header.size);
	}

	free(label_buffer);

	return;
}

static enum blockLabel callGraph_label_block(struct asmBlock* block){
	xed_decoded_inst_t 	xedd;

	if (asmBlock_get_last_instruction(block, &xedd)){
		log_err_m("unable to get last instruction of block %u", block->header.id);
		return BLOCK_LABEL_NONE;
	}

	switch(xed_decoded_inst_get_iclass(&xedd)){
		case XED_ICLASS_CALL_FAR 	:
		case XED_ICLASS_CALL_NEAR 	: {
			const xed_inst_t* 		xi = xed_decoded_inst_inst(&xedd);
			const xed_operand_t* 	xed_op;
			xed_operand_enum_t 		op_name;
			uint32_t 				i;

			for (i = 0; i < xed_inst_noperands(xi); i++){
				xed_op = xed_inst_operand(xi, i);
				if (xed_operand_read_only(xed_op)){
					op_name = xed_operand_name(xed_op);

					if (op_name == XED_OPERAND_RELBR && (xed_decoded_inst_get_branch_displacement(&xedd) & (0xffffffff >> (32 - 8 * xed_decoded_inst_get_branch_displacement_width(&xedd)))) == 0){
						return BLOCK_LABEL_NONE;
					}
				}
			}

			return BLOCK_LABEL_CALL;
		}
		case XED_ICLASS_RET_FAR 	:
		case XED_ICLASS_RET_NEAR 	: {
			return BLOCK_LABEL_RET;
		}
		default 					: {
			return BLOCK_LABEL_NONE;
		}
	}
}

static enum blockLabel* callGraph_alloc_label_buffer(struct assembly* assembly){
	uint32_t 			block_offset;
	struct asmBlock* 	block;
	uint32_t 			nb_block;
	enum blockLabel* 	label_buffer;
	uint8_t 			request_write_permission = 0;

	for (block_offset = 0, nb_block = 0; block_offset != assembly->mapping_size_block; block_offset += sizeof(struct asmBlockHeader) + block->header.size){
		block = (struct asmBlock*)((char*)assembly->mapping_block + block_offset);
		if (block_offset + block->header.size + sizeof(struct asmBlockHeader) > assembly->mapping_size_block){
			log_err("the last asmBlock is incomplete");
			break;
		}

		if (block->header.id != nb_block + FIRST_BLOCK_ID){
			log_warn_m("resetting block id %u -> %u", block->header.id, nb_block + FIRST_BLOCK_ID);
			if (assembly->allocation_type == ALLOCATION_MMAP && !request_write_permission){
				if (mprotect(assembly->mapping_block, assembly->mapping_size_block, PROT_READ | PROT_WRITE)){
					log_err("unable to change memory protection");
				}
				else{
					request_write_permission = 1;
				}
			}
			block->header.id = nb_block + FIRST_BLOCK_ID;
		}
		nb_block ++;
	}

	if ((label_buffer = (enum blockLabel*)calloc(nb_block, sizeof(enum blockLabel))) == NULL){
		log_err("unable to allocate memory");
	}

	return label_buffer;
}

static enum blockLabel* callGraph_fill_label_buffer(struct assembly* assembly){
	uint32_t 			block_offset;
	struct asmBlock* 	block;
	uint32_t 			nb_block;
	enum blockLabel* 	label_buffer;

	if ((label_buffer = callGraph_alloc_label_buffer(assembly)) == NULL){
		log_err("alloc of label buffer failed");
	}
	else{
		for (block_offset = 0, nb_block = 0; block_offset != assembly->mapping_size_block; block_offset += sizeof(struct asmBlockHeader) + block->header.size){
			block = (struct asmBlock*)((char*)assembly->mapping_block + block_offset);
			label_buffer[nb_block ++] = callGraph_label_block(block);
		}
	}

	return label_buffer;
}

static int32_t function_add_snippet(struct callGraph* call_graph, struct function* func, uint32_t start, uint32_t stop, ADDRESS expected_next_address){
	int32_t 				new_snippet_index;
	struct assemblySnippet 	new_snippet;

	new_snippet.offset 					= start;
	new_snippet.length 					= stop - start;
	new_snippet.expected_next_address 	= expected_next_address;
	new_snippet.next_snippet_offset 	= -1;
	new_snippet.prev_snippet_offset 	= func->last_snippet_offset;

	if (new_snippet.length){
		new_snippet_index = array_add(&(call_graph->snippet_array), &new_snippet);
		if (new_snippet_index < 0){
			log_err("unable to add snippet to array");
			return -1;
		}

		func->last_snippet_offset = new_snippet_index;
	}

	return 0;
}

static int32_t function_get_first_snippet(struct callGraph* call_graph, struct function* func){
	int32_t 					index;
	struct assemblySnippet* 	snippet;

	index = func->last_snippet_offset;
	while (index >= 0){
		snippet = (struct assemblySnippet*)array_get(&(call_graph->snippet_array), index);
		if (snippet->prev_snippet_offset >= 0){
			index = snippet->prev_snippet_offset;
		}
		else{
			break;
		}
	}

	return index;
}

void callGraph_locate_in_codeMap_linux(struct callGraph* call_graph, const struct trace* trace, struct codeMap* code_map){
	struct node* 				node_cursor;
	struct function* 			function_cursor;
	int32_t 					snippet_index;
	struct assemblySnippet* 	snippet;
	struct instructionIterator 	it;
	struct cm_routine* 			routine;

	for (node_cursor = graph_get_head_node(&(call_graph->graph)); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
		function_cursor = callGraph_node_get_function(node_cursor);
		function_cursor->routine = NULL;

		if (function_is_invalid(function_cursor)){
			continue;
		}

		snippet_index = function_get_first_snippet(call_graph, function_cursor);

		for (routine = NULL; snippet_index >= 0; snippet_index = snippet->next_snippet_offset){
			snippet = (struct assemblySnippet*)array_get(&(call_graph->snippet_array), snippet_index);

			if (assembly_get_instruction(&(trace->assembly), &it, snippet->offset)){
				log_err_m("unable to fetch first instruction of snippet: start @ %u, stop @ %u", snippet->offset, snippet->offset + snippet->length);
				break;
			}

			while(routine == NULL || (routine != NULL && !strncmp(routine->name, ".plt", CODEMAP_DEFAULT_NAME_SIZE) && !strncmp(routine->name, ".text", CODEMAP_DEFAULT_NAME_SIZE))){
				routine = codeMap_search_routine(code_map, it.instruction_address);

				if (instructionIterator_get_instruction_index(&it) <  snippet->offset + snippet->length){
					if (assembly_get_next_instruction(&(trace->assembly), &it)){
						log_err("unable to fetch next instruction from the assembly");
						break;
					}
				}
				else{
					break;
				}
			}

			if (routine != NULL && strncmp(routine->name, ".plt", CODEMAP_DEFAULT_NAME_SIZE) && strncmp(routine->name, ".text", CODEMAP_DEFAULT_NAME_SIZE)){
				function_cursor->routine = routine;
				break;
			}
		}
	}
}

void callGraph_locate_in_codeMap_windows(struct callGraph* call_graph, const struct trace* trace, struct codeMap* code_map){
	struct node* 				node;
	struct function* 			func;
	int32_t 					snippet_index;
	struct assemblySnippet* 	snippet;
	struct instructionIterator 	it;
	struct cm_routine* 			routine;

	for (node = graph_get_head_node(&(call_graph->graph)); node != NULL; node = node_get_next(node)){
		func = callGraph_node_get_function(node);

		if (function_is_invalid(func)){
			continue;
		}

		snippet_index = function_get_first_snippet(call_graph, func);
		if (snippet_index < 0){
			log_err("unable to get first snippet for a callGraph node");
			continue;
		}

		/* same question as above + why here it is not mandatory to iterate over the code snippets? */
		snippet = (struct assemblySnippet*)array_get(&(call_graph->snippet_array), snippet_index);
		if (assembly_get_instruction(&(trace->assembly), &it, snippet->offset)){
			log_err_m("unable to fetch first instruction of snippet: start @ %u, stop @ %u", snippet->offset, snippet->offset + snippet->length);
			continue;
		}

		do{
			routine = codeMap_search_routine(code_map, it.instruction_address);

			if (instructionIterator_get_instruction_index(&it) <  snippet->offset + snippet->length){
				if (assembly_get_next_instruction(&(trace->assembly), &it)){
					log_err("unable to fetch next instruction from the assembly");
					break;
				}
			}
			else{
				break;
			}
		} while(routine == NULL || (routine != NULL && !strncmp(routine->name, "unnamedImageEntryPoint", CODEMAP_DEFAULT_NAME_SIZE)));

		func->routine = codeMap_search_routine(code_map, it.instruction_address);
	}
}

void callGraph_check(struct callGraph* call_graph, const struct assembly* assembly, struct codeMap* code_map){
	struct node* 				node_cursor;
	struct function* 			function_cursor;
	int32_t 					index;
	struct assemblySnippet* 	snippet;
	struct assemblySnippet* 	prev_snippet;
	struct instructionIterator 	it;
	uint32_t 					i;
	uint32_t 					nb_edge;
	struct edge*				edge_cursor;
	struct cm_routine* 			routine;

	/* Snippet check */
	for (node_cursor = graph_get_head_node(&(call_graph->graph)); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
		function_cursor = callGraph_node_get_function(node_cursor);

		for (index = function_cursor->last_snippet_offset, snippet = NULL, i = 0; index >= 0; index = snippet->prev_snippet_offset, i++){
			prev_snippet = snippet;
			snippet = (struct assemblySnippet*)array_get(&(call_graph->snippet_array), index);

			if (prev_snippet != NULL){
				if (assembly_get_instruction(assembly, &it, prev_snippet->offset)){
					log_err_m("unable to fetch instruction @ %u", prev_snippet->offset);
				}
				else{
					if (it.instruction_address != snippet->expected_next_address){
						if (function_cursor->routine){
							log_err_m("address mismatch for func %s, snippet %u: " PRINTF_ADDR " vs " PRINTF_ADDR, function_cursor->routine->name, i, snippet->expected_next_address, it.instruction_address);
						}
						else{
							log_err_m("address mismatch for func %p, snippet %u: " PRINTF_ADDR " vs " PRINTF_ADDR, (void*)function_cursor, i, snippet->expected_next_address, it.instruction_address);
						}
					}
				}
			}
		}
	}

	/* Connectivity check */
	for (node_cursor = graph_get_head_node(&(call_graph->graph)); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
		function_cursor = callGraph_node_get_function(node_cursor);

		for (edge_cursor = node_get_head_edge_dst(node_cursor), nb_edge = 0; edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
			if (callGraph_edge_get_data(edge_cursor)->type == CALLGRAPH_EDGE_CALL){
				nb_edge ++;
			}
		}

		if (nb_edge > 1){
			if (function_cursor->routine){
				log_err_m("function %s is called %u times", function_cursor->routine->name, nb_edge);
			}
			else{
				log_err_m("function %p is called %u times", (void*)function_cursor, nb_edge);
			}
		}

		for (edge_cursor = node_get_head_edge_src(node_cursor), nb_edge = 0; edge_cursor != NULL; edge_cursor = edge_get_next_src(edge_cursor)){
			if (callGraph_edge_get_data(edge_cursor)->type == CALLGRAPH_EDGE_RET){
				nb_edge ++;
			}
		}

		if (nb_edge > 1){
			if (function_cursor->routine){
				log_err_m("function %s returns %u times", function_cursor->routine->name, nb_edge);
			}
			else{
				log_err_m("function %p returns %u times", (void*)function_cursor, nb_edge);
			}
		}
	}

	/* Label check */
	#if CALLGRAPH_ENABLE_LABEL_CHECK == 1
	if (code_map != NULL){
		for (node_cursor = graph_get_head_node(&(call_graph->graph)); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
			function_cursor = callGraph_node_get_function(node_cursor);

			if (function_is_invalid(function_cursor)){
				continue;
			}

			for (index = function_cursor->last_snippet_offset; index >= 0; index = snippet->prev_snippet_offset){
				snippet = (struct assemblySnippet*)array_get(&(call_graph->snippet_array), index);

				if (assembly_get_instruction(assembly, &it, snippet->offset)){
					log_err_m("unable to fetch instruction @ %u", snippet->offset);
				}
				else{
					routine = codeMap_search_routine(code_map, it.instruction_address);
					if (routine != NULL && !strncmp(routine->name, ".plt", CODEMAP_DEFAULT_NAME_SIZE)){
						continue;
					}

					if (routine != NULL && function_cursor->routine != NULL){
						if (strncmp(routine->name, function_cursor->routine->name, CODEMAP_DEFAULT_NAME_SIZE)){
							log_warn_m("snippet of function %s, appears to be in %s", function_cursor->routine->name, routine->name);
						}
					}
					else if (routine != NULL && function_cursor->routine == NULL){
						log_warn_m("snippet of function NULL, appears to be in %s", routine->name);
					}
				}
			}
		}
	}
	#endif
}

struct node* callGraph_get_index(struct callGraph* call_graph, uint32_t index){
	struct node* 			node_cursor;
	struct function* 		function_cursor;
	int32_t 				cursor;
	struct assemblySnippet* snippet;

	for (node_cursor = graph_get_head_node(&(call_graph->graph)); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
		function_cursor = callGraph_node_get_function(node_cursor);

		for (cursor = function_cursor->last_snippet_offset; cursor >= 0; cursor = snippet->prev_snippet_offset){
			snippet = (struct assemblySnippet*)array_get(&(call_graph->snippet_array), cursor);

			if (snippet->offset <= index && snippet->offset + snippet->length > index){
				return node_cursor;
			}
		}
	}

	log_err_m("unable to find a callGraph node associated to index %u", index);

	return NULL;
}

void callGraph_fprint_stack(struct callGraph* call_graph, struct node* node, FILE* file){
	struct edge* edge_cursor;
	struct node* node_cursor;

	for (node_cursor = node; node_cursor != NULL; ){
		fputs("\t-", file); callGraph_fprint_node(call_graph, node_cursor, file); fputc('\n', file);

		for (edge_cursor = node_get_head_edge_dst(node_cursor), node_cursor = NULL; edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
			if (callGraph_edge_get_data(edge_cursor)->type == CALLGRAPH_EDGE_CALL){
				node_cursor = edge_get_src(edge_cursor);
				break;
			}
		}
	}
}

void callGraph_fprint_node(struct callGraph* call_graph, const struct node* node, FILE* file){
	struct function* 			function;
	int32_t 					snippet_index;
	struct assemblySnippet* 	snippet;

	function = callGraph_node_get_function(node);
	if (function_is_valid(function)){
		snippet_index = function_get_first_snippet(call_graph, function);
		if (snippet_index < 0){
			log_err("unable to get first snippet for a callGraph node");
			return;
		}

		snippet = (struct assemblySnippet*)array_get(&(call_graph->snippet_array), snippet_index);
		fprintf(file, "%9u", snippet->offset);

		if (function->routine != NULL){
			fprintf(file, " (%s)", function->routine->name);
		}
	}
	else{
		fputs("inv", file);
	}
}

int32_t callGraphNode_is_leaf(const struct node* node){
	struct edge* 			edge_cursor;
	struct callGraphEdge* 	call_graph_edge;

	for (edge_cursor = node_get_head_edge_src(node); edge_cursor != NULL; edge_cursor = edge_get_next_src(edge_cursor)){
		call_graph_edge = callGraph_edge_get_data(edge_cursor);
		if (call_graph_edge->type == CALLGRAPH_EDGE_CALL){
			return 0;
		}
	}

	return 1;
}

int32_t callGraph_export_node_inclusive(struct callGraph* call_graph, struct node* node, struct trace* trace, struct array* frag_array){
	struct function* 		function;
	int32_t 				first_snippet_index;
	struct assemblySnippet* snippet;
	uint32_t 				start_index;
	uint32_t 				stop_index;
	struct trace 			fragment;
	uint32_t 				i;

	if (node == NULL){
		log_err("callGraph node is NULL");
		return -1;
	}

	function = callGraph_node_get_function(node);

	if (function_is_invalid(function)){
		log_err("callGraph node is invalid");
		return -1;
	}

	if (function->last_snippet_offset < 0){
		log_err("no code snippet for this callGraph node");
		return -1;
	}

	snippet = (struct assemblySnippet*)array_get(&(call_graph->snippet_array), function->last_snippet_offset);
	stop_index = snippet->offset + snippet->length;

	first_snippet_index = function_get_first_snippet(call_graph, function);
	if (first_snippet_index < 0){
		log_err("unable to get first snippet");
		return -1;
	}

	snippet = (struct assemblySnippet*)array_get(&(call_graph->snippet_array), first_snippet_index);
	start_index = snippet->offset;

	if (trace_extract_segment(trace, &fragment, start_index, stop_index - start_index)){
		log_err("unable to extract traceFragment");
		return -1;
	}

	log_info_m("export trace fragment [%u:%u]", start_index, stop_index);
	if (function->routine != NULL){
		snprintf(fragment.trace_type.frag.tag, TRACE_TAG_LENGTH, "rtn_inc:%s", function->routine->name);
	}
	else{
		snprintf(fragment.trace_type.frag.tag, TRACE_TAG_LENGTH, "rtn_inc:[%u:%u]", start_index, stop_index);
	}

	for (i = 0; i < array_get_length(frag_array); i++){
		if (trace_compare(&fragment, (struct trace*)array_get(frag_array, i)) == 0){
			log_info_m("an equivalent fragment (%u) has already been exported", i);
			trace_clean(&fragment);
			return 0;
		}
	}

	if (array_add(frag_array, &fragment) < 0){
		log_err("unable to add traceFragment to array");
		trace_clean(&fragment);
		return -1;
	}

	return 0;
}