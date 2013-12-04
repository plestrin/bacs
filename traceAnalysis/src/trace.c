#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "trace.h"
#include "instruction.h"
#include "graphPrintDot.h"

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

	trace->simple_trace_stat = NULL;
	trace->call_tree = NULL;

	return trace;
}

void trace_instructions_print(struct trace* trace){
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

void trace_simpleTraceStat_create(struct trace* trace){
	struct instruction* 	ins;
	
	if (trace->simple_trace_stat != NULL){
		simpleTraceStat_delete(trace->simple_trace_stat);
		#ifdef VERBOSE
		printf("Deleting previous simpleTraceStat\n");
		#endif
	}
	trace->simple_trace_stat = simpleTraceStat_create();
	if (trace->simple_trace_stat != NULL){
		if (!traceReaderJSON_reset(&(trace->ins_reader.json))){
			do{
				ins = traceReaderJSON_get_next_instruction(&(trace->ins_reader.json));
				if (ins != NULL){
					if (simpleTraceStat_add_instruction(trace->simple_trace_stat, ins)){
						printf("ERROR: in %s, unable to add instruction to simpleTraceStat\n", __func__);
					}
				}
			} while (ins != NULL);
		}
		else{
			printf("ERROR: in %s, unable to reset JSON trace reader\n", __func__);
			simpleTraceStat_delete(trace->simple_trace_stat);
			trace->simple_trace_stat = NULL;
		}
	}
	else{
		printf("ERROR: in %s, unable to create simpleTraceStat\n", __func__);
	}
}

void trace_simpleTraceStat_print(struct trace* trace){
	if (trace->simple_trace_stat != NULL){
		simpleTraceStat_print(trace->simple_trace_stat);
	}
	else{
		printf("ERROR: in %s, simpleTaceStat is not created\n", __func__);
	}
}

void trace_codeMap_print(struct trace* trace){
	if (trace->code_map != NULL){
		codeMap_print(trace->code_map, CODEMAP_FILTER_EXECUTED);
	}
	else{
		printf("ERROR: in %s, codeMap is NULL\n", __func__);
	}
}

void trace_simpleTraceStat_delete(struct trace* trace){
	if (trace->simple_trace_stat != NULL){
		simpleTraceStat_delete(trace->simple_trace_stat);
		trace->simple_trace_stat = NULL;
	}
	else{
		printf("ERROR: in %s, codeMap is NULL\n", __func__);
	}
}

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

void trace_callTree_print_dot(struct trace* trace){
	if (trace->call_tree != NULL){
		if (graphPrintDot_print(trace->call_tree, "callTree.dot")){
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

void trace_callTree_delete(struct trace* trace){
	if (trace->call_tree != NULL){
		graph_delete(trace->call_tree);
		trace->call_tree = NULL;
	}
	else{
		printf("ERROR: in %s, callTree is NULL\n", __func__);
	}
}

void trace_delete(struct trace* trace){
	if (trace != NULL){
		if (trace->simple_trace_stat != NULL){
			simpleTraceStat_delete(trace->simple_trace_stat);
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

/* ===================================================================== */
/* Special debug stuff - don't touch	                                 */
/* ===================================================================== */

#include "argBuffer.h"


void trace_callTree_handmade_test(struct trace* trace){
	struct callTree_node* 	node;
	struct array* 			read_mem_arg;
	struct array* 			write_mem_arg;
	uint32_t 				i;
	struct argBuffer* 		arg;
		
	if (trace->call_tree != NULL){
		node = (struct callTree_node*)trace->call_tree->nodes[31].data;

		/* Make sur to select the crypto  node */
		printf("Node %d routine name: %s\n", 21, node->name);

		traceFragment_create_mem_array(&(node->fragment));
		/* traceFragment_remove_read_after_write(&(node->fragment)); */

		/* traceFragment_print_mem_array(node->fragment.write_memory_array, node->fragment.nb_memory_write_access); */

		printf("Nb mem read access:  %d\n", node->fragment.nb_memory_read_access);
		printf("Nb mem write access: %d\n", node->fragment.nb_memory_write_access);
		
		read_mem_arg = traceFragment_extract_mem_arg_adjacent(node->fragment.read_memory_array, node->fragment.nb_memory_read_access);
		write_mem_arg = traceFragment_extract_mem_arg_adjacent(node->fragment.write_memory_array, node->fragment.nb_memory_write_access);

		printf("Nb read mem arg: %u\n", array_get_length(read_mem_arg));

		for (i = 0; i < array_get_length(read_mem_arg); i++){
			arg = (struct argBuffer*)array_get(read_mem_arg, i);
			argBuffer_print_raw(arg);
		}

		printf("Nb write mem arg: %u\n", array_get_length(write_mem_arg));

		for (i = 0; i < array_get_length(write_mem_arg); i++){
			arg = (struct argBuffer*)array_get(write_mem_arg, i);
			argBuffer_print_raw(arg);
		}

		/* Check submit */
		ioChecker_submit_arguments(trace->checker, read_mem_arg, write_mem_arg);

		for (i = 0; i < array_get_length(write_mem_arg); i++){
			arg = (struct argBuffer*)array_get(write_mem_arg, i);
			free(arg->data);
		}
		for (i = 0; i < array_get_length(read_mem_arg); i++){
			arg = (struct argBuffer*)array_get(read_mem_arg, i);
			free(arg->data);
		}

		array_delete(read_mem_arg);
		array_delete(write_mem_arg);
	}
	else{
		printf("ERROR: in %s, callTree is NULL\n", __func__);
	}
}