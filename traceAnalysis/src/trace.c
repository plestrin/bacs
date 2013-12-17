#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "trace.h"
#include "instruction.h"
#include "graphPrintDot.h"
#include "argBuffer.h"
#include "simpleTraceStat.h"

struct trace* trace_create(const char* dir_name){
	struct trace* 	trace;
	char			file_name[TRACE_DIRECTORY_NAME_MAX_LENGTH];

	trace = (struct trace*)malloc(sizeof(struct trace));
	if (trace == NULL){
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
		return NULL;
	}

	trace->checker = ioChecker_create();
	if (trace->checker == NULL){
		printf("ERROR: in %s, unable to create ioChecker\n", __func__);
		free(trace);
		return NULL;
	}

	snprintf(file_name, TRACE_DIRECTORY_NAME_MAX_LENGTH, "%s/%s", dir_name, TRACE_INS_FILE_NAME);
	if (traceReaderJSON_init(&(trace->ins_reader.json), file_name)){
		printf("ERROR: in %s, unable to init trace JSON reader\n", __func__);
		ioChecker_delete(trace->checker);
		free(trace);
		return NULL;
	}

	snprintf(file_name, TRACE_DIRECTORY_NAME_MAX_LENGTH, "%s/%s", dir_name, TRACE_CM_FILE_NAME);
	trace->code_map = cmReaderJSON_parse_trace(file_name);
	if (trace->code_map == NULL){
		printf("WARNING: in %s, continue without code map information\n", __func__);
	}

	if (array_init(&(trace->frag_array), sizeof(struct traceFragment))){
		printf("ERROR: in %s, unable to init traceFragment array\n", __func__);
		ioChecker_delete(trace->checker);
		free(trace);
		return NULL;
	}

	trace->call_tree = NULL;
	trace->loop_engine = NULL;

	return trace;
}

void trace_delete(struct trace* trace){
	if (trace != NULL){
		trace_frag_clean(trace);
		array_clean(&(trace->frag_array));

		if (trace->loop_engine != NULL){
			loopEngine_delete(trace->loop_engine);
		}
		if (trace->call_tree != NULL){
			graph_delete(trace->call_tree);
		}

		traceReaderJSON_clean(&(trace->ins_reader.json));
		codeMap_delete(trace->code_map);
		ioChecker_delete(trace->checker);

		free(trace);
	}
}

void trace_instruction_print(struct trace* trace){
	struct instruction* ins;

	if (!traceReaderJSON_reset(&(trace->ins_reader.json))){
		do{
			ins = traceReaderJSON_get_next_instruction(&(trace->ins_reader.json));
			if (ins != NULL){
				instruction_print(ins);
			}
		} while (ins != NULL);
	}
	else{
		printf("ERROR: in %s, unable to reset JSON trace reader\n", __func__);
	}
}

/* ===================================================================== */
/* Calltree functions						                             */
/* ===================================================================== */

void trace_callTree_create(struct trace* trace){
	struct instruction*			ins;
	struct callTree_element		element;

	if (trace->call_tree != NULL){
		graph_delete(trace->call_tree);
	}
	trace->call_tree = graph_create();
	if (trace->call_tree != NULL){
		trace->call_tree->callback_node.create_data 		= callTree_create_node;
		trace->call_tree->callback_node.may_add_element 	= callTree_may_add_element;
		trace->call_tree->callback_node.add_element 		= callTree_add_element;
		trace->call_tree->callback_node.element_is_owned 	= callTree_element_is_owned;
		trace->call_tree->callback_node.print_dot 			= callTree_node_printDot;
		trace->call_tree->callback_node.delete_data 		= callTree_delete_node;

		element.cm = trace->code_map;
		if (!traceReaderJSON_reset(&(trace->ins_reader.json))){
			do{
				ins = traceReaderJSON_get_next_instruction(&(trace->ins_reader.json));
				if (ins != NULL){
					element.ins = ins;
					if (graph_add_element(trace->call_tree, &element)){
						printf("ERROR: in %s, unable to add instruction to call tree\n", __func__);
						break;
					}
				}
			} while (ins != NULL);
		}
		else{
			printf("ERROR: in %s, unable to reset JSON trace reader\n", __func__);
			graph_delete(trace->call_tree);
			trace->call_tree = NULL;
		}
	}
	else{
		printf("ERROR: in %s, unable to create graph\n", __func__);
	}
}

void trace_callTree_print_dot(struct trace* trace, char* file_name){
	if (trace->call_tree != NULL){
		if (graphPrintDot_print(trace->call_tree, file_name)){
			printf("ERROR: in %s, unable to print callTree to DOT format\n", __func__);
		}
	}
	else{
		printf("ERROR: in %s, callTree is NULL\n", __func__);
	}
}

/* Warning this function is not meant to last forever */
void trace_callTree_print_opcode_percent(struct trace* trace){
	if (trace->call_tree != NULL){
		callTree_print_opcode_percent(trace->call_tree);
	}
	else{
		printf("ERROR: in %s, callTree is NULL\n", __func__);
	}
}

void trace_callTree_export(struct trace* trace){
	int32_t 				i;
	struct callTree_node* 	node;
	struct traceFragment 	fragment;

	if (trace->call_tree != NULL){
		for (i = 0; i < trace->call_tree->nb_node; i++){
			node = (struct callTree_node*)trace->call_tree->nodes[i].data;

			if (traceFragment_clone(&(node->fragment), &fragment)){
				printf("ERROR: in %s, unable to clone traceFragment\n", __func__);
				break;
			}

			if (array_add(&(trace->frag_array), &fragment) < 0){
				printf("ERROR: in %s, unable to add fragment to frag_array\n", __func__);
				break;
			}
		}

		#ifdef VERBOSE
		printf("CallTree: %d/%d traceFragment(s) have been exported\n", i, trace->call_tree->nb_node);
		#endif
	}
	else{
		printf("ERROR: in %s, callTree is NULL\n", __func__);
	}
}

void trace_callTree_delete(struct trace* trace){
	if (trace->call_tree != NULL){
		graph_delete(trace->call_tree);
		trace->call_tree = NULL;
	}
	else{
		printf("ERROR: in %s, callTree is NULL\n", __func__);
	}
}

/* ===================================================================== */
/* Loop functions						                                 */
/* ===================================================================== */

void trace_loop_create(struct trace* trace){
	struct instruction* ins;

	if (trace->loop_engine != NULL){
		loopEngine_delete(trace->loop_engine);
	}

	trace->loop_engine = loopEngine_create();
	if (trace->loop_engine == NULL){
		printf("ERROR: in %s, unable to init loopEngine\n", __func__);
		return;
	}

	if (!traceReaderJSON_reset(&(trace->ins_reader.json))){
		do{
			ins = traceReaderJSON_get_next_instruction(&(trace->ins_reader.json));
			if (ins != NULL){
				if (loopEngine_add(trace->loop_engine, ins)){
					printf("ERROR: in %s, loopEngine failed to add element\n", __func__);
					break;
				}
			}
		} while (ins != NULL);
	}
	else{
		printf("ERROR: in %s, unable to reset JSON trace reader\n", __func__);
	}

	if (loopEngine_process(trace->loop_engine)){
		printf("ERROR: in %s, unable to process loopEngine\n", __func__);
	}
}

void trace_loop_remove_redundant(struct trace* trace){
	if (trace->loop_engine != NULL){
		if (loopEngine_remove_redundant_loop(trace->loop_engine)){
			printf("ERROR: in %s, unable to remove redundant loop\n", __func__);
		}
	}
	else{
		printf("ERROR: in %s, loopEngine is NULL\n", __func__);
	}
}

void trace_loop_pack_epilogue(struct trace* trace){
	if (trace->loop_engine != NULL){
		if (loopEngine_pack_epilogue(trace->loop_engine)){
			printf("ERROR: in %s, unable to pack epilogue\n", __func__);
		}
	}
	else{
		printf("ERROR: in %s, loopEngine is NULL\n", __func__);
	}
}

void trace_loop_print(struct trace* trace){
	if (trace->loop_engine != NULL){
		loopEngine_print_loop(trace->loop_engine);
	}
	else{
		printf("ERROR: in %s, loopEngine is NULL\n", __func__);
	}
}

void trace_loop_export(struct trace* trace){
	if (trace->loop_engine != NULL){
		if (loopEngine_export_traceFragment(trace->loop_engine, &(trace->frag_array))){
			printf("ERROR: in %s, unable to export loop to traceFragment\n", __func__);
		}
	}
	else{
		printf("ERROR: in %s, loopEngine is NULL\n", __func__);
	}
}

void trace_loop_delete(struct trace* trace){
	if (trace->loop_engine != NULL){
		loopEngine_delete(trace->loop_engine);
		trace->loop_engine = NULL;
	}
	else{
		printf("ERROR: in %s, loopEngine is NULL\n", __func__);
	}
}

/* ===================================================================== */
/* frag functions						                                 */
/* ===================================================================== */

void trace_frag_clean(struct trace* trace){
	uint32_t 				i;
	struct traceFragment* 	fragment;

	for (i = 0; i < array_get_length(&(trace->frag_array)); i++){
		fragment = (struct traceFragment*)array_get(&(trace->frag_array), i);
		traceFragment_clean(fragment);
	}
	array_empty(&(trace->frag_array));
}

void trace_frag_print_stat(struct trace* trace, char* arg){
	uint32_t 					i;
	struct simpleTraceStat 		stat;
	struct multiColumnPrinter* 	printer = NULL;
	uint32_t 					index;

	if (arg != NULL){
		index = (uint32_t)atoi(arg);
		
		if (index < array_get_length(&(trace->frag_array))){
			simpleTraceStat_init(&stat);
			simpleTraceStat_process(&stat, (struct traceFragment*)array_get(&(trace->frag_array), index));
			simpleTraceStat_print(printer, &stat);
		}
		else{
			printf("ERROR: in %s, incorrect index value %u (array size :%u)\n", __func__, index, array_get_length(&(trace->frag_array)));
		}
	}
	else{
		printer = simpleTraceStat_init_MultiColumnPrinter();
		if (printer != NULL){
			multiColumnPrinter_print_header(printer);

			for (i = 0; i < array_get_length(&(trace->frag_array)); i++){
				simpleTraceStat_init(&stat);
				simpleTraceStat_process(&stat, (struct traceFragment*)array_get(&(trace->frag_array), i));
				simpleTraceStat_print(printer, &stat);
			}

			multiColumnPrinter_delete(printer);
		}
		else{
			printf("ERROR: in %s, unable to init multiColumnPrinter\n", __func__);
		}
	}
}

/* on peut parser un index de fragment */
void trace_frag_search(struct trace* trace){
	uint32_t 				i;
	struct traceFragment* 	fragment;
	struct array* 			read_mem_arg;
	struct array* 			write_mem_arg;

	for (i = 0; i < array_get_length(&(trace->frag_array)); i++){
		fragment = (struct traceFragment*)array_get(&(trace->frag_array), i);
		if (traceFragment_create_mem_array(fragment)){
			printf("ERROR: in %s, unable to create mem array for fragment %u\n", __func__, i);
			break;
		}

		/* traceFragment_remove_read_after_write(fragment); */

		if ((fragment->nb_memory_read_access > 0) && (fragment->nb_memory_write_access > 0)){
			#ifdef VERBOSE
			printf("Searching fragment %u/%u ...\n", i, array_get_length(&(trace->frag_array)));
			#endif

			read_mem_arg = traceFragment_extract_mem_arg_adjacent(fragment->read_memory_array, fragment->nb_memory_read_access);
			write_mem_arg = traceFragment_extract_mem_arg_adjacent(fragment->write_memory_array, fragment->nb_memory_write_access);

			if (read_mem_arg != NULL && write_mem_arg != NULL){
				ioChecker_submit_arguments(trace->checker, read_mem_arg, write_mem_arg);
			}
			else{
				printf("ERROR: in %s, read_mem_arg or write_mem_arg array is NULL\n", __func__);
			}
				
			argBuffer_delete_array(read_mem_arg);
			argBuffer_delete_array(write_mem_arg);
		}
		#ifdef VERBOSE
		else{
			printf("Skipping  fragment %u/%u: no read and write mem access\n", i, array_get_length(&(trace->frag_array)));
		}
		#endif
	}
}