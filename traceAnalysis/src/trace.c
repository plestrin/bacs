#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "trace.h"
#include "instruction.h"
#include "argSet.h"
#include "argBuffer.h"
#include "simpleTraceStat.h"
#include "argSetGraph.h"
#include "printBuffer.h"
#include "readBuffer.h"

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

	if (array_init(&(trace->arg_array), sizeof(struct argSet))){
		printf("ERROR: in %s, unable to init argSet array\n", __func__);
		array_clean(&(trace->frag_array));
		ioChecker_delete(trace->checker);
		free(trace);
		return NULL;

	}

	/*trace->call_tree = NULL;*/
	trace->loop_engine = NULL;
	trace->arg_set_graph = NULL;

	return trace;
}

void trace_delete(struct trace* trace){
	if (trace != NULL){
		if (trace->loop_engine != NULL){
			loopEngine_delete(trace->loop_engine);
			trace->loop_engine = NULL;
		}
		/*if (trace->call_tree != NULL){
			graph_delete(trace->call_tree);
		}*/
		if (trace->arg_set_graph != NULL){
			argSetGraph_delete(trace->arg_set_graph);
			trace->arg_set_graph = NULL;
		}

		trace_arg_clean(trace);
		array_clean(&(trace->arg_array));

		trace_frag_clean(trace);
		array_clean(&(trace->frag_array));

		traceReaderJSON_clean(&(trace->ins_reader.json));
		codeMap_delete(trace->code_map);
		ioChecker_delete(trace->checker);

		free(trace);
	}
}

/* ===================================================================== */
/* Trace functions						                                 */
/* ===================================================================== */

void trace_instruction_print(struct trace* trace){
	struct instruction* instruction;

	if (!traceReaderJSON_reset(&(trace->ins_reader.json))){
		do{
			instruction = traceReaderJSON_get_next_instruction(&(trace->ins_reader.json));
			if (instruction != NULL){
				instruction_print(NULL, instruction);
			}
		} while (instruction != NULL);
	}
	else{
		printf("ERROR: in %s, unable to reset JSON trace reader\n", __func__);
	}
}

void trace_instruction_export(struct trace* trace){
	struct instruction* 	instruction;
	struct traceFragment 	fragment;

	if (traceFragment_init(&fragment)){
		printf("ERROR: in %s, unable to init traceFragment\n", __func__);
		return;
	}

	if (!traceReaderJSON_reset(&(trace->ins_reader.json))){
		do{
			instruction = traceReaderJSON_get_next_instruction(&(trace->ins_reader.json));
			if (instruction != NULL){
				if (traceFragment_add_instruction(&fragment, instruction) < 0){
					printf("ERROR: in %s, unable to add instruction to traceFragment\n", __func__);
					break;
				}
			}
		} while (instruction != NULL);

		traceFragment_set_tag(&fragment, "trace");

		if (array_add(&(trace->frag_array), &fragment) < 0){
			printf("ERROR: in %s, unable to add traceFragment to frag_array\n", __func__);
			traceFragment_clean(&fragment);
		}
	}
	else{
		printf("ERROR: in %s, unable to reset JSON trace reader\n", __func__);
	}
}

/* ===================================================================== */
/* Calltree functions						                             */
/* ===================================================================== */

#pragma GCC diagnostic ignored "-Wunused-parameter"
void trace_callTree_create(struct trace* trace){
	/*struct instruction*			ins;
	struct callTree_element		element;

	if (trace->call_tree != NULL){
		graph_delete(trace->call_tree);
		printf("WARNING: in %s, deleting the current callTree\n", __func__);
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
	}*/
}

#pragma GCC diagnostic ignored "-Wunused-parameter"
void trace_callTree_print_dot(struct trace* trace, char* file_name){
	/*if (trace->call_tree != NULL){
		if (file_name != NULL){
			if (graphPrintDot_print(trace->call_tree, file_name)){
				printf("ERROR: in %s, unable to print callTree in DOT format to file: \"%s\"\n", __func__, file_name);
			}
		}
		else{
			printf("ERROR: in %s, please specify a file name\n", __func__);
		}
	}
	else{
		printf("ERROR: in %s, callTree is NULL\n", __func__);
	}*/
}

#pragma GCC diagnostic ignored "-Wunused-parameter"
void trace_callTree_export(struct trace* trace){
	/*int32_t 				i;
	struct callTree_node* 	node;
	struct traceFragment 	fragment;

	if (trace->call_tree != NULL){
		for (i = 0; i < trace->call_tree->nb_node; i++){
			node = (struct callTree_node*)trace->call_tree->nodes[i].data;

			if (traceFragment_clone(&(node->fragment), &fragment)){
				printf("ERROR: in %s, unable to clone traceFragment\n", __func__);
				break;
			}

			traceFragment_set_tag(&fragment, node->name);

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
	}*/
}

#pragma GCC diagnostic ignored "-Wunused-parameter"
void trace_callTree_delete(struct trace* trace){
	/*if (trace->call_tree != NULL){
		graph_delete(trace->call_tree);
		trace->call_tree = NULL;
	}
	else{
		printf("ERROR: in %s, callTree is NULL\n", __func__);
	}*/
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
	struct traceFragment*		fragment;

	if (arg != NULL){
		index = (uint32_t)atoi(arg);
		
		if (index < array_get_length(&(trace->frag_array))){
			fragment = (struct traceFragment*)array_get(&(trace->frag_array), index);
			#ifdef VERBOSE
			printf("Print simpleTraceStat for fragment %u (tag: \"%s\", nb fragment: %u)\n", index, fragment->tag, array_get_length(&(trace->frag_array)));
			#endif
			simpleTraceStat_init(&stat);
			simpleTraceStat_process(&stat, fragment);
			simpleTraceStat_print(printer, 0, NULL, &stat);
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
				fragment = (struct traceFragment*)array_get(&(trace->frag_array), i);
				simpleTraceStat_init(&stat);
				simpleTraceStat_process(&stat, fragment);
				simpleTraceStat_print(printer, i, fragment->tag, &stat);
			}

			multiColumnPrinter_delete(printer);
		}
		else{
			printf("ERROR: in %s, unable to init multiColumnPrinter\n", __func__);
		}
	}
}

void trace_frag_print_ins(struct trace* trace, char* arg){
	struct multiColumnPrinter* 	printer;
	struct traceFragment* 		fragment;
	uint32_t 					i;
	uint32_t 					index;

	if (arg != NULL){
		index = (uint32_t)atoi(arg);
		
		if (index < array_get_length(&(trace->frag_array))){
			printer = instruction_init_multiColumnPrinter();
			if (printer != NULL){
				fragment = (struct traceFragment*)array_get(&(trace->frag_array), index);
				#ifdef VERBOSE
				printf("Print instructions for fragment %u (tag: \"%s\", nb fragment: %u)\n", index, fragment->tag, array_get_length(&(trace->frag_array)));
				#endif

				multiColumnPrinter_print_header(printer);

				for (i = 0; i < traceFragment_get_nb_instruction(fragment); i++){
					instruction_print(printer, traceFragment_get_instruction(fragment, i));
				}

				multiColumnPrinter_delete(printer);
			}
			else{
				printf("ERROR: in %s, init multiColumnPrinter fails\n", __func__);
			}
		}
		else{
			printf("ERROR: in %s, incorrect index value %u (array size :%u)\n", __func__, index, array_get_length(&(trace->frag_array)));
		}
	}
	else{
		printf("ERROR: in %s, an index value must be specified\n", __func__);
	}
}

void trace_frag_print_percent(struct trace* trace){
	uint32_t 					nb_opcode = 10;
	uint32_t 					opcode[10] = {XED_ICLASS_XOR, XED_ICLASS_SHL, XED_ICLASS_SHLD, XED_ICLASS_SHR, XED_ICLASS_SHRD, XED_ICLASS_NOT, XED_ICLASS_OR, XED_ICLASS_AND, XED_ICLASS_ROL, XED_ICLASS_ROR};
	uint32_t 					nb_excluded_opcode = 3;
	uint32_t 					excluded_opcode[3] = {XED_ICLASS_MOV, XED_ICLASS_PUSH, XED_ICLASS_POP};
	uint32_t 					i;
	struct multiColumnPrinter* 	printer;
	struct traceFragment* 		fragment;
	double 						percent;

	#ifdef VERBOSE
	printf("Included opcode(s): ");
	for (i = 0; i < nb_opcode; i++){
		if (i == nb_opcode - 1){
			printf("%s\n", instruction_opcode_2_string(opcode[i]));
		}
		else{
			printf("%s, ", instruction_opcode_2_string(opcode[i]));
		}
	}
	printf("Excluded opcode(s): ");
	for (i = 0; i < nb_excluded_opcode; i++){
		if (i == nb_excluded_opcode - 1){
			printf("%s\n", instruction_opcode_2_string(excluded_opcode[i]));
		}
		else{
			printf("%s, ", instruction_opcode_2_string(excluded_opcode[i]));
		}
	}
	#endif

	printer = multiColumnPrinter_create(stdout, 3, NULL, NULL, NULL);
	if (printer != NULL){
		multiColumnPrinter_set_column_size(printer, 0, 5);
		multiColumnPrinter_set_column_size(printer, 1, 24);
		multiColumnPrinter_set_column_size(printer, 2, 16);

		multiColumnPrinter_set_column_type(printer, 0, MULTICOLUMN_TYPE_INT32);
		multiColumnPrinter_set_column_type(printer, 2, MULTICOLUMN_TYPE_DOUBLE);

		multiColumnPrinter_set_title(printer, 0, (char*)"Index");
		multiColumnPrinter_set_title(printer, 1, (char*)"Tag");
		multiColumnPrinter_set_title(printer, 2, (char*)"Percent (%%)");

		multiColumnPrinter_print_header(printer);

		for (i = 0; i < array_get_length(&(trace->frag_array)); i++){
			fragment = (struct traceFragment*)array_get(&(trace->frag_array), i);
			percent = traceFragment_opcode_percent(fragment, nb_opcode, opcode, nb_excluded_opcode, excluded_opcode);
			multiColumnPrinter_print(printer, i, fragment->tag, percent*100, NULL);
		}

		multiColumnPrinter_delete(printer);
	}
	else{
		printf("ERROR: in %s, unable to create multiColumnPrinter\n", __func__);
	}
}

void trace_frag_set_tag(struct trace* trace, char* arg){
	uint32_t 				i;
	uint32_t 				index;
	struct traceFragment* 	fragment;
	uint8_t 				found_space = 0;

	if (arg != NULL){
		for (i = 0; i < strlen(arg) - 1; i++){
			if (arg[i] == ' '){
				found_space = 1;
				break;
			}
		}
		if (found_space){
			index = (uint32_t)atoi(arg);

			if (index < array_get_length(&(trace->frag_array))){
				fragment = (struct traceFragment*)array_get(&(trace->frag_array), index);

				#ifdef VERBOSE
				printf("Setting tag value for arg %u: old tag: \"%s\", new tag: \"%s\"\n", index, fragment->tag, arg + i + 1);
				#endif

				strncpy(fragment->tag, arg + i + 1, ARGSET_TAG_MAX_LENGTH);
			}
			else{
				printf("ERROR: in %s, incorrect index value %u (array size :%u)\n", __func__, index, array_get_length(&(trace->frag_array)));
			}
		}
		else{
			printf("ERROR: in %s, the index and the tag must separated by a space char\n", __func__);
		}
	}
	else{
		printf("ERROR: in %s, an index and a tag value must be specified\n", __func__);
	}
}

void trace_frag_extract_arg(struct trace* trace, char* arg){
	uint32_t 				i;
	struct traceFragment* 	fragment;
	struct argSet 			arg_set;
	uint32_t 				start;
	uint32_t				stop;
	uint32_t 				index;
	uint8_t 				found_space = 0;
	int32_t(*extract_routine_mem_read)(struct array*,struct memAccess*,int);
	int32_t(*extract_routine_mem_write)(struct array*,struct memAccess*,int);
	int32_t(*extract_routine_reg_read)(struct array*,struct regAccess*,int);
	int32_t(*extract_routine_reg_write)(struct array*,struct regAccess*,int);
	uint8_t 				remove_raw = 0;

	#define ARG_NAME_A_LP 		"A_LP"
	#define ARG_NAME_AR_LP 		"AR_LP"
	#define ARG_NAME_AS_LP 		"AS_LP"
	#define ARG_NAME_ASR_LP 	"ASR_LP"
	#define ARG_NAME_ASO_LP 	"ASO_LP"
	#define ARG_NAME_ASOR_LP 	"ASOR_LP"

	#define ARG_DESC_A 			"arguments are made of Adjacent memory access"
	#define ARG_DESC_AR 		"read after write are Removed, then same as\"A\""
	#define ARG_DESC_AS 		"same as \"A\" with additional access Size consideration (Mandatory for fragmentation)"
	#define ARG_DESC_ASR		"read after write are Removed, then same as \"AS\""
	#define ARG_DESC_ASO 		"same as \"AS\" with additional Opcode consideration"
	#define ARG_DESC_ASOR 		"read after write are Removed, then same as \"ASO\""
	#define ARG_DESC_LP 		"Large registers (>= 32bits) are combined together (Pure)"

	start = 0;
	stop = array_get_length(&(trace->frag_array));

	if (arg != NULL){
		for (i = 0; i < strlen(arg) - 1; i++){
			if (arg[i] == ' '){
				found_space = 1;
				break;
			}
		}
		if (found_space){
			index = (uint32_t)atoi(arg + i + 1);
		
			if (index < array_get_length(&(trace->frag_array))){
				start = index;
				stop = index + 1;
			}
			else{
				printf("ERROR: in %s, incorrect index value %u (array size :%u)\n", __func__, index, array_get_length(&(trace->frag_array)));
				return;
			}
		}
		else{
			i ++;
		}

		if (!strncmp(arg, ARG_NAME_A_LP, i)){
			extract_routine_mem_read 	= traceFragment_extract_mem_arg_adjacent_read;
			extract_routine_mem_write 	= traceFragment_extract_mem_arg_adjacent_write;
			extract_routine_reg_read 	= traceFragment_extract_reg_arg_large_pure;
			extract_routine_reg_write 	=  traceFragment_extract_reg_arg_large_pure;
			remove_raw 					= 0;

			#ifdef VERBOSE
			printf("Extraction routine \"%s\" : %s and %s\n", ARG_NAME_A_LP, ARG_DESC_A, ARG_DESC_LP);
			#endif
		}
		else if (!strncmp(arg, ARG_NAME_AR_LP, i)){
			extract_routine_mem_read 	= traceFragment_extract_mem_arg_adjacent_read;
			extract_routine_mem_write 	= traceFragment_extract_mem_arg_adjacent_write;
			extract_routine_reg_read 	= traceFragment_extract_reg_arg_large_pure;
			extract_routine_reg_write 	=  traceFragment_extract_reg_arg_large_pure;
			remove_raw 					= 1;

			#ifdef VERBOSE
			printf("Extraction routine \"%s\" : %s and %s\n", ARG_NAME_AR_LP, ARG_DESC_AR, ARG_DESC_LP);
			#endif
		}
		else if (!strncmp(arg, ARG_NAME_AS_LP, i)){
			extract_routine_mem_read 	= traceFragment_extract_mem_arg_adjacent_size_read;
			extract_routine_mem_write 	= traceFragment_extract_mem_arg_adjacent_write;
			extract_routine_reg_read 	= traceFragment_extract_reg_arg_large_pure;
			extract_routine_reg_write 	=  traceFragment_extract_reg_arg_large_pure;
			remove_raw 					= 0;

			#ifdef VERBOSE
			printf("Extraction routine \"%s\" : %s and %s\n", ARG_NAME_AS_LP, ARG_DESC_AS, ARG_DESC_LP);
			#endif
		}
		else if (!strncmp(arg, ARG_NAME_ASR_LP, i)){
			extract_routine_mem_read 	= traceFragment_extract_mem_arg_adjacent_size_read;
			extract_routine_mem_write 	= traceFragment_extract_mem_arg_adjacent_write;
			extract_routine_reg_read 	= traceFragment_extract_reg_arg_large_pure;
			extract_routine_reg_write 	=  traceFragment_extract_reg_arg_large_pure;
			remove_raw 					= 1;

			#ifdef VERBOSE
			printf("Extraction routine \"%s\" : %s and %s\n", ARG_NAME_ASR_LP, ARG_DESC_ASR, ARG_DESC_LP);
			#endif
		}
		else if (!strncmp(arg, ARG_NAME_ASO_LP, i)){
			extract_routine_mem_read 	= traceFragment_extract_mem_arg_adjacent_size_opcode_read;
			extract_routine_mem_write 	= traceFragment_extract_mem_arg_adjacent_write;
			extract_routine_reg_read 	= traceFragment_extract_reg_arg_large_pure;
			extract_routine_reg_write 	=  traceFragment_extract_reg_arg_large_pure;
			remove_raw 					= 0;
			
			#ifdef VERBOSE
			printf("Extraction routine \"%s\" : %s and %s\n", ARG_NAME_ASO_LP, ARG_DESC_ASO, ARG_DESC_LP);
			#endif
		}
		else if (!strncmp(arg, ARG_NAME_ASOR_LP, i)){
			extract_routine_mem_read 	= traceFragment_extract_mem_arg_adjacent_size_opcode_read;
			extract_routine_mem_write 	= traceFragment_extract_mem_arg_adjacent_write;
			extract_routine_reg_read 	= traceFragment_extract_reg_arg_large_pure;
			extract_routine_reg_write 	=  traceFragment_extract_reg_arg_large_pure;
			remove_raw 					= 1;

			#ifdef VERBOSE
			printf("Extraction routine \"%s\" : %s and %s\n", ARG_NAME_ASOR_LP, ARG_DESC_ASOR, ARG_DESC_LP);
			#endif
		}
		else{
			printf("ERROR: in %s, bad extraction routine specifier of length %u\n", __func__, i);
			goto arg_error;
		}
	}
	else{
		printf("ERROR: in %s, an argument is expected to select the extraction routine\n", __func__);
		goto arg_error;
	}

	for (i = start; i < stop; i++){
		fragment = (struct traceFragment*)array_get(&(trace->frag_array), i);
		if (traceFragment_create_mem_array(fragment)){
			printf("ERROR: in %s, unable to create mem array for fragment %u\n", __func__, i);
			break;
		}

		if (traceFragment_create_reg_array(fragment)){
			printf("ERROR: in %s, unable to create reg array for fragement %u\n", __func__, i);
			break;
		}

		if ((fragment->nb_memory_read_access > 0) && (fragment->nb_memory_write_access > 0) && remove_raw){
			traceFragment_remove_read_after_write(fragment);
		}

		if ((fragment->nb_memory_read_access > 0) && (fragment->nb_memory_write_access > 0)){
			arg_set.input = array_create(sizeof(struct argBuffer));
			if (arg_set.input == NULL){
				printf("ERROR: in %s, unable to create array\n", __func__);
				break;
			}
			arg_set.output = array_create(sizeof(struct argBuffer));
			if (arg_set.output == NULL){
				printf("ERROR: in %s, unable to create array\n", __func__);
				break;
			}

			if (extract_routine_mem_read(arg_set.input, fragment->read_memory_array, fragment->nb_memory_read_access)){
				printf("ERROR: in %s, memory read extraction routine return an error code\n", __func__);
			}
			if (extract_routine_mem_write(arg_set.output, fragment->write_memory_array, fragment->nb_memory_write_access)){
				printf("ERROR: in %s, memory write extraction routine return an error code\n", __func__);
			}
			if (extract_routine_reg_read(arg_set.input, fragment->read_register_array, fragment->nb_register_read_access)){
				printf("ERROR: in %s, register read extraction routine return an error code\n", __func__);
			}
			if (extract_routine_reg_write(arg_set.output, fragment->write_register_array, fragment->nb_register_write_access)){
				printf("ERROR: in %s, register write extraction routine return an error code\n", __func__);
			}

			strncpy(arg_set.tag, fragment->tag, ARGSET_TAG_MAX_LENGTH);
			if (strlen(arg_set.tag) == 0){
				snprintf(arg_set.tag, ARGSET_TAG_MAX_LENGTH, "Frag %u", i);
			}

			if (array_add(&(trace->arg_array), &arg_set) < 0){
				printf("ERROR: in %s, unable to add argument to arg array\n", __func__);
			}
		}
		#ifdef VERBOSE
		else{
			printf("Skipping fragment %u/%u (tag: \"%s\"): no read and write mem access\n", i, array_get_length(&(trace->frag_array)), fragment->tag);
		}
		#endif
	}

	return;

	arg_error:
	printf("Expected extraction specifier:\n");
	printf(" - \"%s\"    : %s and %s\n", 	ARG_NAME_A_LP, 		ARG_DESC_A, 	ARG_DESC_LP);
	printf(" - \"%s\"   : %s and %s\n", 	ARG_NAME_AR_LP, 	ARG_DESC_AR, 	ARG_DESC_LP);
	printf(" - \"%s\"   : %s and %s\n", 	ARG_NAME_AS_LP, 	ARG_DESC_AS, 	ARG_DESC_LP);
	printf(" - \"%s\"  : %s and %s\n", 		ARG_NAME_ASR_LP, 	ARG_DESC_ASR, 	ARG_DESC_LP);
	printf(" - \"%s\"  : %s and %s\n", 		ARG_NAME_ASO_LP, 	ARG_DESC_ASO, 	ARG_DESC_LP);
	printf(" - \"%s\" : %s and %s\n", 		ARG_NAME_ASOR_LP, 	ARG_DESC_ASOR, 	ARG_DESC_LP);
	return;

	#undef ARG_DESC_A
	#undef ARG_DESC_AR
	#undef ARG_DESC_AS
	#undef ARG_DESC_ASR
	#undef ARG_DESC_ASO
	#undef ARG_DESC_ASOR
	#undef ARG_DESC_LP

	#undef ARG_NAME_A_LP
	#undef ARG_NAME_AR_LP
	#undef ARG_NAME_AS_LP
	#undef ARG_NAME_ASR_LP
	#undef ARG_NAME_ASO_LP
	#undef ARG_NAME_ASOR_LP
}

/* ===================================================================== */
/* arg functions						                                 */
/* ===================================================================== */

void trace_arg_clean(struct trace* trace){
	uint32_t 				i;
	struct argSet* 			arg_set;

	if (trace->arg_set_graph != NULL){
		argSetGraph_delete(trace->arg_set_graph);
		trace->arg_set_graph = NULL;

		#ifdef VERBOSE
		printf("The argSetGraph attached to the argSet array has been destroyed\n");
		#endif
	}

	for (i = 0; i < array_get_length(&(trace->arg_array)); i++){
		arg_set = (struct argSet*)array_get(&(trace->arg_array), i);
		argBuffer_delete_array(arg_set->input);
		argBuffer_delete_array(arg_set->output);
	}
	array_empty(&(trace->arg_array));
}

void trace_arg_print(struct trace* trace, char* arg){
	uint32_t 					index;
	uint32_t 					i;
	struct argSet* 				arg_set;
	struct argBuffer* 			buffer;
	struct multiColumnPrinter* 	printer;

	if (arg != NULL){
		index = (uint32_t)atoi(arg);
		
		if (index < array_get_length(&(trace->arg_array))){
			arg_set = (struct argSet*)array_get(&(trace->arg_array), index);
			#ifdef VERBOSE
			printf("Print argSet %u (tag: \"%s\", nb argSet: %u)\n", index, arg_set->tag, array_get_length(&(trace->arg_array)));
			#endif

			for (i = 0; i < array_get_length(arg_set->input); i++){
				buffer = (struct argBuffer*)array_get(arg_set->input, i);
				printf("Input %u/%u ", i + 1, array_get_length(arg_set->input));
				argBuffer_print_raw(buffer);
			}

			for (i = 0; i < array_get_length(arg_set->output); i++){
				buffer = (struct argBuffer*)array_get(arg_set->output, i);
				printf("Output %u/%u ", i + 1, array_get_length(arg_set->output));
				argBuffer_print_raw(buffer);
			}
		}
		else{
			printf("ERROR: in %s, incorrect index value %u (array size :%u)\n", __func__, index, array_get_length(&(trace->arg_array)));
		}
	}
	else{
		printer = multiColumnPrinter_create(stdout, 4, NULL, NULL, NULL);
		if (printer != NULL){

			multiColumnPrinter_set_column_size(printer, 0, 5);
			multiColumnPrinter_set_column_size(printer, 1, ARGSET_TAG_MAX_LENGTH);

			multiColumnPrinter_set_title(printer, 0, "Index");
			multiColumnPrinter_set_title(printer, 1, "Tag");
			multiColumnPrinter_set_title(printer, 2, "Nb Input");
			multiColumnPrinter_set_title(printer, 3, "Nb Output");

			multiColumnPrinter_set_column_type(printer, 0, MULTICOLUMN_TYPE_UINT32);
			multiColumnPrinter_set_column_type(printer, 2, MULTICOLUMN_TYPE_UINT32);
			multiColumnPrinter_set_column_type(printer, 3, MULTICOLUMN_TYPE_UINT32);

			multiColumnPrinter_print_header(printer);

			for (i = 0; i < array_get_length(&(trace->arg_array)); i++){
				arg_set = (struct argSet*)array_get(&(trace->arg_array), i);
				multiColumnPrinter_print(printer, i, arg_set->tag, array_get_length(arg_set->input), array_get_length(arg_set->output), NULL);
			}

			multiColumnPrinter_delete(printer);
		}
		else{
			printf("ERROR: in %s, unable to init multiColumnPrinter\n", __func__);
		}
	}
}

void trace_arg_set_tag(struct trace* trace, char* arg){
	uint32_t 			i;
	uint32_t 			index;
	struct argSet* 		arg_set;
	uint8_t 			found_space = 0;

	if (arg != NULL){
		for (i = 0; i < strlen(arg) - 1; i++){
			if (arg[i] == ' '){
				found_space = 1;
				break;
			}
		}
		if (found_space){
			index = (uint32_t)atoi(arg);

			if (index < array_get_length(&(trace->arg_array))){
				arg_set = (struct argSet*)array_get(&(trace->arg_array), index);

				#ifdef VERBOSE
				printf("Setting tag value for arg %u: old tag: \"%s\", new tag: \"%s\"\n", index, arg_set->tag, arg + i + 1);
				#endif

				strncpy(arg_set->tag, arg + i + 1, ARGSET_TAG_MAX_LENGTH);
			}
			else{
				printf("ERROR: in %s, incorrect index value %u (array size :%u)\n", __func__, index, array_get_length(&(trace->arg_array)));
			}
		}
		else{
			printf("ERROR: in %s, the index and the tag must separated by a space char\n", __func__);
		}
	}
	else{
		printf("ERROR: in %s, an index and a tag value must be specified\n", __func__);
	}
}

void trace_arg_fragment(struct trace* trace, char* arg){
	uint32_t 			start;
	uint32_t			stop;
	uint32_t 			index;
	uint32_t			i;
	struct argSet* 		arg_set;
	struct array 		new_argSet_array;

	start = 0;
	stop = array_get_length(&(trace->arg_array));

	if (arg != NULL){
		index = (uint32_t)atoi(arg);
		
		if (index < array_get_length(&(trace->arg_array))){
			start = index;
			stop = index + 1;
		}
		else{
			printf("ERROR: in %s, incorrect index value %u (array size :%u)\n", __func__, index, array_get_length(&(trace->arg_array)));
			return;
		}
	}

	if (array_init(&new_argSet_array, sizeof(struct argSet))){
		printf("ERROR: in %s, unable to create array\n", __func__);
		return;
	}

	for (i = start; i < stop; i++){
		arg_set = (struct argSet*)array_get(&(trace->arg_array), i);

		#ifdef VERBOSE
		printf("Fragmenting arg %u/%u (tag: \"%s\") ...\n", i, array_get_length(&(trace->arg_array)) - 1, arg_set->tag);
		#endif

		argSet_fragment_input(arg_set, &new_argSet_array);
	}

	if (array_copy(&new_argSet_array, &(trace->arg_array), 0, array_get_length(&new_argSet_array)) != (int32_t)array_get_length(&new_argSet_array)){
		printf("ERROR: in %s, unable to copy arrays (may cause memory loss)\n", __func__);
	}

	array_clean(&new_argSet_array);
}

void trace_arg_create_argSetGraph(struct trace* trace){
	if (trace->arg_set_graph != NULL){
		argSetGraph_delete(trace->arg_set_graph);
		printf("WARNING: in %s, deleting the current argSetGraph\n", __func__);
	}
	trace->arg_set_graph = argSetGraph_create(&(trace->arg_array));
	if (trace->arg_set_graph == NULL){
		printf("ERROR: in %s, unable to create argSetGraph\n", __func__);
	}
}

void trace_arg_print_dot_argSetGraph(struct trace* trace, char* file_name){
	if (trace->arg_set_graph != NULL){
		if (file_name != NULL){
			if (argSetGraph_print_dot(trace->arg_set_graph, file_name)){
				printf("ERROR: in %s, unable to print argSetGraph in DOT format to file: \"%s\"\n", __func__, file_name);
			}
		}
		else{
			printf("ERROR: in %s, please specify a file name\n", __func__);
		}
	}
	else{
		printf("ERROR: in %s, argSetGraph is NULL\n", __func__);
	}
}

void trace_arg_pack(struct trace* trace, char* arg){
	uint32_t 				i;
	uint32_t 				start;
	uint32_t				stop;
	uint32_t 				index;
	uint8_t 				found_space = 0;
	int32_t (*pack_routine)(struct argSetGraph*,uint32_t);

	#define ARG_S_DESC 		"pack the an argSet with one of his parent. Generate a lot of argSets but smaller ones"

	start = 0;
	stop = array_get_length(&(trace->arg_array));

	if (arg != NULL){
		for (i = 0; i < strlen(arg) - 1; i++){
			if (arg[i] == ' '){
				found_space = 1;
				break;
			}
		}
		if (found_space){
			index = (uint32_t)atoi(arg + i + 1);
		
			if (index < array_get_length(&(trace->arg_array))){
				start = index;
				stop = index + 1;
			}
			else{
				printf("ERROR: in %s, incorrect index value %u (array size :%u)\n", __func__, index, array_get_length(&(trace->arg_array)));
				return;
			}
		}
		else{
			i ++;
		}

		if (!strncmp(arg, "S", i)){
			pack_routine = argSetGraph_pack_simple;
			#ifdef VERBOSE
			printf("Selecting packing routine \"S\" : %s\n", ARG_S_DESC);
			#endif
		}
		else{
			printf("ERROR: in %s, bad packing routine specifier of length %u\n", __func__, i);
			goto arg_error;
		}
		
	}
	else{
		printf("ERROR: in %s, an argument is expected to select the packing routine\n", __func__);
		goto arg_error;
	}

	if (trace->arg_set_graph != NULL){
		for (i = start; i < stop; i++){
			if (pack_routine(trace->arg_set_graph, i)){
				printf("ERROR: in %s, unable to pack argSet %u\n", __func__, i);
				break;
			}
		}
	}

	return;

	arg_error:
	printf("Expected packing specifier:\n");
	printf(" - \"S\"  : %s\n", ARG_S_DESC);
	return;

	#undef ARG_A_DESC
	#undef ARG_AS_DESC
}

void trace_arg_delete_argSetGraph(struct trace* trace){
	if (trace->arg_set_graph != NULL){
		argSetGraph_delete(trace->arg_set_graph);
		trace->arg_set_graph = NULL;
	}
	else{
		printf("ERROR: in %s, argSetGraph is NULL\n", __func__);
	}
}

void trace_arg_search(struct trace* trace, char* arg){
	uint32_t 			i;
	struct argSet* 		arg_set;
	uint32_t 			start;
	uint32_t			stop;
	uint32_t 			index;

	start = 0;
	stop = array_get_length(&(trace->arg_array));

	if (arg != NULL){
		index = (uint32_t)atoi(arg);
		
		if (index < array_get_length(&(trace->arg_array))){
			start = index;
			stop = index + 1;
		}
		else{
			printf("ERROR: in %s, incorrect index value %u (array size :%u)\n", __func__, index, array_get_length(&(trace->arg_array)));
			return;
		}
	}

	for (i = start; i < stop; i++){
		arg_set = (struct argSet*)array_get(&(trace->arg_array), i);

		#ifdef VERBOSE
		printf("Searching argSet %u/%u (tag: \"%s\") ...\n", i, array_get_length(&(trace->arg_array)) - 1, arg_set->tag);
		#endif

		ioChecker_submit_argBuffers(trace->checker, arg_set->input, arg_set->output);
	}
}

void trace_arg_seek(struct trace* trace, char* arg){
	uint32_t 			i;
	uint32_t 			j;
	uint32_t 			k;
	struct argSet* 		arg_set;
	struct argBuffer* 	arg_buffer;
	char* 				buffer;
	uint32_t 			buffer_length;
	uint32_t 			compare_length;

	#define MIN_COMPARE_SIZE 4

	buffer = readBuffer_raw(arg, strlen(arg));
	buffer_length = READBUFFER_RAW_GET_LENGTH(strlen(arg));
	if (buffer == NULL){
		printf("ERROR: in %s, readBuffer return NULL\n", __func__);
	}
	else{
		#ifdef VERBOSE
		printf("Min compare size is set to: %u\n", MIN_COMPARE_SIZE);
		#endif

		for (i = 0; i < array_get_length(&(trace->arg_array)); i++){
			arg_set = (struct argSet*)array_get(&(trace->arg_array), i);
			
			/* INPUT */
			for (j = 0; j < array_get_length(arg_set->input); j++){
				arg_buffer = (struct argBuffer*)array_get(arg_set->input, j);

				for (k = 0; k < ((arg_buffer->size > (MIN_COMPARE_SIZE - 1)) ? (arg_buffer->size - (MIN_COMPARE_SIZE - 1)) : 0); k++){
					compare_length = (buffer_length > arg_buffer->size - k) ? (arg_buffer->size - k) : buffer_length;
					if (!memcmp(buffer, arg_buffer->data + k, compare_length)){
						printf("Found correspondence in argset %u, (tag: \"%s\"), input %u:\n", i, arg_set->tag, j);
						printBuffer_raw_color(arg_buffer->data, arg_buffer->size, k, compare_length);
						printf("\n");
					}
				}
			}

			/* OUTPUT */
			for (j = 0; j < array_get_length(arg_set->output); j++){
				arg_buffer = (struct argBuffer*)array_get(arg_set->output, j);

				for (k = 0; k < ((arg_buffer->size > (MIN_COMPARE_SIZE - 1)) ? (arg_buffer->size - (MIN_COMPARE_SIZE - 1)) : 0); k++){
					compare_length = (buffer_length > arg_buffer->size - k) ? (arg_buffer->size - k) : buffer_length;
					if (!memcmp(buffer, arg_buffer->data + k, compare_length)){
						printf("Found correspondence in argset %u, (tag: \"%s\"), output %u:\n", i, arg_set->tag, j);
						printBuffer_raw_color(arg_buffer->data, arg_buffer->size, k, compare_length);
						printf("\n");
					}
				}
			}
		}
		free(buffer);
	}

	#undef MIN_COMPARE_SIZE
}