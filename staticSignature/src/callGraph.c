#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>

#include "callGraph.h"
#include "base.h"

enum blockLabel{
	BLOCK_LABEL_CALL,
	BLOCK_LABEL_RET,
	BLOCK_LABEL_NONE
};

#define CALLGRAPH_MAX_DEPTH 	4096
#define CALLGRAPH_START_DEPTH 	256

#define CALLGRAPH_ENABLE_LABEL_CHECK 1

static enum blockLabel* callGraph_label_blocks(struct assembly* assembly);

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

struct callGraph* callGraph_create(struct trace* trace, uint32_t start, uint32_t stop){
	struct callGraph* call_graph;

	call_graph = (struct callGraph*)malloc(sizeof(struct callGraph));
	if (call_graph != NULL){
		if (callGraph_init(call_graph, trace, start, stop)){
			free(call_graph);
			call_graph = NULL;
		}
	}

	return call_graph;
}

int32_t callGraph_init(struct callGraph* call_graph, struct trace* trace, uint32_t start, uint32_t stop){
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

	label_buffer = callGraph_label_blocks(&(trace->assembly));
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

	call_graph->assembly_ref = &(trace->assembly);

	for (i = 0; i < trace->assembly.nb_dyn_block; i++){
		if (trace->assembly.dyn_blocks[i].instruction_count < start){
			if (dynBlock_is_valid(trace->assembly.dyn_blocks + i) && trace->assembly.dyn_blocks[i].instruction_count + trace->assembly.dyn_blocks[i].block->header.nb_ins > start){
				log_warn_m("index %u is not at a the begining of a basic block, rounding", start);
			}
			else{
				continue;
			}
		}
		if (trace->assembly.dyn_blocks[i].instruction_count >= stop){
			break;
		}

		if (dynBlock_is_invalid(trace->assembly.dyn_blocks + i)){
			if (call_graph_stack[stack_index] != NULL){
				current_func = callGraph_node_get_function(call_graph_stack[stack_index]);
				if (function_is_valid(current_func)){
					function_add_snippet(call_graph, current_func, snippet_start, trace->assembly.dyn_blocks[i].instruction_count, 0);
					function_set_invalid(current_func);
				}
			}
			else{
				call_graph_stack[stack_index] = graph_add_node_(&(call_graph->graph));
				if (call_graph_stack[stack_index] == NULL){
					log_err("unable to create node");
				}
				else{
					current_func = callGraph_node_get_function(call_graph_stack[stack_index]);
					function_init_invalid(current_func)
					snippet_start = trace->assembly.dyn_blocks[i].instruction_count;
				}
			}
		}
		else{
			snippet_stop = trace->assembly.dyn_blocks[i].instruction_count + trace->assembly.dyn_blocks[i].block->header.nb_ins;

			if (call_graph_stack[stack_index] != NULL){
				current_func = callGraph_node_get_function(call_graph_stack[stack_index]);
				if (function_is_invalid(current_func)){
					if (stack_index > 0 && call_graph_stack[stack_index - 1] != NULL){
						if (callGraph_node_get_function(call_graph_stack[stack_index - 1])->last_snippet_offset >= 0){
							if (((struct assemblySnippet*)array_get(&(call_graph->snippet_array), callGraph_node_get_function(call_graph_stack[stack_index - 1])->last_snippet_offset))->expected_next_address == trace->assembly.dyn_blocks[i].block->header.address){
								snippet_start = trace->assembly.dyn_blocks[i].instruction_count;
								stack_index --;

								call_graph_edge.type = CALLGRAPH_EDGE_RET;

								if (graph_add_edge(&(call_graph->graph), call_graph_stack[stack_index + 1], call_graph_stack[stack_index], &call_graph_edge) == NULL){
									log_err("unable to add edge to callGraph");
								}
							}
							else{
								snippet_start = trace->assembly.dyn_blocks[i].instruction_count;
								stack_index ++;

								call_graph_stack[stack_index] = graph_add_node_(&(call_graph->graph));
								if (call_graph_stack[stack_index] == NULL){
									log_err("unable to create node");
								}
								else{
									current_func = callGraph_node_get_function(call_graph_stack[stack_index]);
									function_init_valid(current_func)

									call_graph_edge.type = CALLGRAPH_EDGE_CALL;

									if (graph_add_edge(&(call_graph->graph), call_graph_stack[stack_index - 1], call_graph_stack[stack_index], &call_graph_edge) == NULL){
										log_err("unable to add edge to callGraph");
									}
								}
							}
						}
						else{
							log_err("the previous function call on the stack is empty");
						}
					}
					else{
						log_warn_m("unable to classify current basic block %u as a RET or a CALL", i);

						snippet_start = trace->assembly.dyn_blocks[i].instruction_count;
						stack_index ++;

						call_graph_stack[stack_index] = graph_add_node_(&(call_graph->graph));
						if (call_graph_stack[stack_index] == NULL){
							log_err("unable to create node");
						}
						else{
							current_func = callGraph_node_get_function(call_graph_stack[stack_index]);
							function_init_valid(current_func)

							call_graph_edge.type = CALLGRAPH_EDGE_CALL;

							if (graph_add_edge(&(call_graph->graph), call_graph_stack[stack_index - 1], call_graph_stack[stack_index], &call_graph_edge) == NULL){
								log_err("unable to add edge to callGraph");
							}
						}
					}
				}
			}
			else{
				call_graph_stack[stack_index] = graph_add_node_(&(call_graph->graph));
				if (call_graph_stack[stack_index] == NULL){
					log_err("unable to create node");
					continue;
				}
				else{
					current_func = callGraph_node_get_function(call_graph_stack[stack_index]);
					function_init_valid(current_func)
					snippet_start = trace->assembly.dyn_blocks[i].instruction_count;
				}
			}

			current_func = callGraph_node_get_function(call_graph_stack[stack_index]);
			switch(label_buffer[trace->assembly.dyn_blocks[i].block->header.id - 1]){
				case BLOCK_LABEL_CALL 	: {
					function_add_snippet(call_graph, current_func, snippet_start, snippet_stop, trace->assembly.dyn_blocks[i].block->header.address + trace->assembly.dyn_blocks[i].block->header.size);

					if (trace->assembly.dyn_blocks[i].instruction_count + trace->assembly.dyn_blocks[i].block->header.nb_ins < stop){
						stack_index ++;

						if (stack_index == CALLGRAPH_MAX_DEPTH){
							log_err("the top of the stack has been reached");
							result = -1;
							goto exit;
						}

						call_graph_stack[stack_index] = graph_add_node_(&(call_graph->graph));
						if (call_graph_stack[stack_index] == NULL){
							log_err("unable to create node");
						}
						else{
							current_func = callGraph_node_get_function(call_graph_stack[stack_index]);
							function_init_valid(current_func)
							snippet_start = snippet_stop;

							call_graph_edge.type = CALLGRAPH_EDGE_CALL;

							if (graph_add_edge(&(call_graph->graph), call_graph_stack[stack_index - 1], call_graph_stack[stack_index], &call_graph_edge) == NULL){
								log_err("unable to add edge to callGraph");
							}
						}
					}
					break;
				}
				case BLOCK_LABEL_RET 	: {
					function_add_snippet(call_graph, current_func, snippet_start, snippet_stop, trace->assembly.dyn_blocks[i].block->header.address + trace->assembly.dyn_blocks[i].block->header.size);

					if (trace->assembly.dyn_blocks[i].instruction_count + trace->assembly.dyn_blocks[i].block->header.nb_ins < stop){
						if (stack_index == 0){
							log_err("the bottom of the stack has been reached");
							result = -1;
							goto exit;
						}

						stack_index --;

						if (call_graph_stack[stack_index] == NULL){
							call_graph_stack[stack_index] = graph_add_node_(&(call_graph->graph));
							if (call_graph_stack[stack_index] == NULL){
								log_err("unable to create node");
							}
							else{
								current_func = callGraph_node_get_function(call_graph_stack[stack_index]);
								function_init_valid(current_func)
							}
						}

						snippet_start = snippet_stop;

						if (call_graph_stack[stack_index] != NULL){
							call_graph_edge.type = CALLGRAPH_EDGE_RET;

							if (graph_add_edge(&(call_graph->graph), call_graph_stack[stack_index + 1], call_graph_stack[stack_index], &call_graph_edge) == NULL){
								log_err("unable to add edge to callGraph");
							}
						}
					}
					break;
				}
				case BLOCK_LABEL_NONE 	: {
					if (trace->assembly.dyn_blocks[i].instruction_count + trace->assembly.dyn_blocks[i].block->header.nb_ins >= stop){
						function_add_snippet(call_graph, current_func, snippet_start, snippet_stop, trace->assembly.dyn_blocks[i].block->header.address + trace->assembly.dyn_blocks[i].block->header.size);
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

static enum blockLabel* callGraph_label_blocks(struct assembly* assembly){
	uint32_t 			block_offset;
	struct asmBlock* 	block;
	uint32_t 			nb_block;
	enum blockLabel* 	result;
	xed_decoded_inst_t 	xedd;
	uint8_t 			request_write_permission = 0;

	for (block_offset = 0, nb_block = 0; block_offset != assembly->mapping_size_block; ){
		block = (struct asmBlock*)((char*)assembly->mapping_block + block_offset);
		if (block_offset + block->header.size + sizeof(struct asmBlockHeader) > assembly->mapping_size_block){
			log_err("the last asmBlock is incomplete");
			break;
		}

		block_offset += sizeof(struct asmBlockHeader) + block->header.size;
		nb_block ++;
	}

	result = (enum blockLabel*)malloc(sizeof(enum blockLabel) * nb_block);
	if (result == NULL){
		log_err("unable to allocate memory");
		return NULL;
	}

	for (block_offset = 0, nb_block = 0; block_offset != assembly->mapping_size_block; ){
		block = (struct asmBlock*)((char*)assembly->mapping_block + block_offset);
		if (block_offset + block->header.size + sizeof(struct asmBlockHeader) > assembly->mapping_size_block){
			log_err("the last asmBlock is incomplete");
			break;
		}

		result[nb_block] = BLOCK_LABEL_NONE;
		if (block->header.id != nb_block + 1){
			log_warn_m("resetting block id %u -> %u", block->header.id, nb_block + 1);
			if (assembly->allocation_type == ALLOCATION_MMAP && !request_write_permission){
				if (mprotect(assembly->mapping_block, assembly->mapping_size_block, PROT_READ | PROT_WRITE)){
					log_err("unable to change memory protection");
				}
				else{
					request_write_permission = 1;
				}
			}
			block->header.id = nb_block + 1;
		}


		if (assembly_get_last_instruction(block, &xedd)){
			log_err_m("unable to get last instruction of block %u", nb_block);
		}
		else{
			switch(xed_decoded_inst_get_iclass(&xedd)){
				case XED_ICLASS_CALL_FAR 	:
				case XED_ICLASS_CALL_NEAR 	: {
					const xed_inst_t* 		xi = xed_decoded_inst_inst(&xedd);
					const xed_operand_t* 	xed_op;
					xed_operand_enum_t 		op_name;
					uint32_t 				i;

					result[nb_block] = BLOCK_LABEL_CALL;

					for (i = 0; i < xed_inst_noperands(xi); i++){
						xed_op = xed_inst_operand(xi, i);
						if (xed_operand_read_only(xed_op)){
							op_name = xed_operand_name(xed_op);

							if (op_name == XED_OPERAND_RELBR && (xed_decoded_inst_get_branch_displacement(&xedd) & (0xffffffff >> (32 - 8 * xed_decoded_inst_get_branch_displacement_width(&xedd)))) == 0){
								result[nb_block] = BLOCK_LABEL_NONE;
								break;
							}
						}
					}
					
					break;
				}
				case XED_ICLASS_RET_FAR 	:
				case XED_ICLASS_RET_NEAR 	: {
					result[nb_block] = BLOCK_LABEL_RET;
					break;
				}
				default 					: {
					break;
				}
			}
		}

		block_offset += sizeof(struct asmBlockHeader) + block->header.size;
		nb_block ++;
	}

	return result;
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

void callGraph_locate_in_codeMap_linux(struct callGraph* call_graph, struct trace* trace, struct codeMap* code_map){
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

			while(routine == NULL || (routine != NULL && !strncmp(routine->name, ".plt", CODEMAP_DEFAULT_NAME_SIZE))){
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

			if (routine != NULL && strncmp(routine->name, ".plt", CODEMAP_DEFAULT_NAME_SIZE)){
				function_cursor->routine = routine;
				break;
			}
		}
	}
}

void callGraph_locate_in_codeMap_windows(struct callGraph* call_graph, struct trace* trace, struct codeMap* code_map){
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

void callGraph_check(struct callGraph* call_graph, struct codeMap* code_map){
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
				if (assembly_get_instruction(call_graph->assembly_ref, &it, prev_snippet->offset)){
					log_err_m("unable to fetch instruction @ %u", prev_snippet->offset);
				}
				else{
					if (it.instruction_address != snippet->expected_next_address){
						#if defined ARCH_32
						if (function_cursor->routine){
							log_err_m("address mismatch for func %s, snippet %u: 0x%08x vs 0x%08x", function_cursor->routine->name, i, snippet->expected_next_address, it.instruction_address);
						}
						else{
							log_err_m("address mismatch for func %p, snippet %u: 0x%08x vs 0x%08x", (void*)function_cursor, i, snippet->expected_next_address, it.instruction_address);
						}
						#elif defined ARCH_64
						if (function_cursor->routine){
							log_err_m("address mismatch for func %s, snippet %u: 0x%llx vs 0x%llx", function_cursor->routine->name, i, snippet->expected_next_address, it.instruction_address);
						}
						else{
							log_err_m("address mismatch for func %p, snippet %u: 0x%llx vs 0x%llx", (void*)function_cursor, i, snippet->expected_next_address, it.instruction_address);
						}
						#else
						#error Please specify an architecture {ARCH_32 or ARCH_64}
						#endif
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

				if (assembly_get_instruction(call_graph->assembly_ref, &it, snippet->offset)){
					log_err_m("unable to fetch instruction @ %u", snippet->offset);
				}
				else{
					routine = codeMap_search_routine(code_map, it.instruction_address);
					if (routine != NULL && !strncmp(routine->name, ".plt", CODEMAP_DEFAULT_NAME_SIZE)){
						continue;
					}

					if (routine != NULL && function_cursor->routine != NULL){
						if (strncmp(routine->name, function_cursor->routine->name, CODEMAP_DEFAULT_NAME_SIZE)){
							log_warn_m("snippet of function %s, appars to be in %s", function_cursor->routine->name, routine->name);
						}
					}
					else if (routine != NULL && function_cursor->routine == NULL){
						log_warn_m("snippet of function NULL, appars to be in %s", routine->name);
					}
				}
			}
		}
	}
	#endif
}

void callGraph_print_stack(struct callGraph* call_graph, uint32_t index){
	struct node* 				node_cursor;
	struct edge*				edge_cursor;
	struct node*				caller_node;
	struct function*			caller_function;
	struct function* 			function_cursor;
	int32_t 					cursor;
	struct assemblySnippet* 	snippet;

	for (node_cursor = graph_get_head_node(&(call_graph->graph)); node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
		function_cursor = callGraph_node_get_function(node_cursor);

		for (cursor = function_cursor->last_snippet_offset; cursor >= 0; cursor = snippet->prev_snippet_offset){
			snippet = (struct assemblySnippet*)array_get(&(call_graph->snippet_array), cursor);

			if (snippet->offset <= index && snippet->offset + snippet->length > index){
				if (function_cursor->routine != NULL){
					log_info_m("found index in %s", function_cursor->routine->name);
				}
				else{
					log_info_m("found index in %p", (void*)function_cursor);
				}

				for (caller_node = node_cursor; ; ){
					for (edge_cursor = node_get_head_edge_dst(caller_node); edge_cursor != NULL; edge_cursor = edge_get_next_dst(edge_cursor)){
						if (callGraph_edge_get_data(edge_cursor)->type == CALLGRAPH_EDGE_CALL){
							break;
						}
					}

					if (edge_cursor != NULL){
						caller_node = edge_get_src(edge_cursor);
						caller_function = callGraph_node_get_function(caller_node);

						if (function_is_valid(caller_function)){
							if (caller_function->routine != NULL){
								printf("\t%s\n", caller_function->routine->name);
							}
							else{
								printf("\t%p\n", (void*)caller_function);
							}
						}
						else{
							fputs("\t-\n", stdout);
						}
					}
					else{
						break;
					}
				}
			}
		}
	}
}

int32_t callGraph_export_inclusive(struct callGraph* call_graph, struct trace* trace, struct array* frag_array, char* name_filter){
	struct node* 				node;
	struct function* 			func;
	int32_t 					first_snippet_index;
	struct assemblySnippet* 	snippet;
	uint32_t 					start_index;
	uint32_t 					stop_index;
	struct trace 				fragment;

	for (node = graph_get_head_node(&(call_graph->graph)); node != NULL; node = node_get_next(node)){
		func = callGraph_node_get_function(node);
		if (name_filter == NULL || (name_filter != NULL && func->routine != NULL && !strncmp(name_filter, func->routine->name, CODEMAP_DEFAULT_NAME_SIZE))){
			if (func->last_snippet_offset < 0){
				log_err("no code snippet for the current callGraph node");
				continue;
			}

			snippet = (struct assemblySnippet*)array_get(&(call_graph->snippet_array), func->last_snippet_offset);
			stop_index = snippet->offset + snippet->length;

			first_snippet_index = function_get_first_snippet(call_graph, func);
			if (first_snippet_index < 0){
				log_err("unable to get first snippet for a callGraph node");
				continue;
			}

			snippet = (struct assemblySnippet*)array_get(&(call_graph->snippet_array), first_snippet_index);
			start_index = snippet->offset;

			if (trace_extract_segment(trace, &fragment, start_index, stop_index - start_index)){
				log_err("unable to extract traceFragment");
				return -1;
			}
			
			log_info_m("export trace fragment [%u:%u]", start_index, stop_index);
			if (func->routine != NULL){
				snprintf(fragment.trace_type.frag.tag, TRACE_TAG_LENGTH, "rtn_inc:%s", func->routine->name);
			}

			if (array_add(frag_array, &fragment) < 0){
				log_err("unable to add traceFragment to array");
				trace_clean(&fragment);
				return -1;
			}
		}
	}

	return 0;
}