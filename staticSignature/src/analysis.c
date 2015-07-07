#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "analysis.h"
#include "inputParser.h"
#include "printBuffer.h"
#include "readBuffer.h"
#include "result.h"
#include "traceMine.h"
#include "codeSignatureReader.h"
#include "cmReaderJSON.h"
#include "multiColumn.h"
#include "base.h"

#define ADD_CMD_TO_INPUT_PARSER(parser, cmd, cmd_desc, arg_desc, type, arg, func)										\
	{																													\
		if (inputParser_add_cmd((parser), (cmd), (cmd_desc), (arg_desc), (type), (arg), (void(*)(void))(func))){		\
			log_err_m("unable to add cmd: \"%s\" to inputParser", (cmd));												\
		}																												\
	}

int main(int argc, char** argv){
	struct analysis* 			analysis = NULL;
	struct inputParser* 		parser = NULL;

	parser = inputParser_create();
	if (parser == NULL){
		log_err("unable to create inputParser");
		goto exit;
	}
	
	analysis = analysis_create();
	if (analysis == NULL){
		log_err("unable to create the analysis structure");
		goto exit;
	}

	/* trace specific commands */
	ADD_CMD_TO_INPUT_PARSER(parser, "load trace", 				"Load a trace in the analysis engine", 			"Trace directory", 			INPUTPARSER_CMD_TYPE_ARG, 		analysis, 								analysis_trace_load)
	ADD_CMD_TO_INPUT_PARSER(parser, "change thread", 			"Switch the current thread", 					"Thread Index", 			INPUTPARSER_CMD_TYPE_ARG, 		analysis, 								analysis_trace_change_thread)
	ADD_CMD_TO_INPUT_PARSER(parser, "load elf", 				"Load an ELF file in the analysis engine", 		"ELF file", 				INPUTPARSER_CMD_TYPE_ARG, 		analysis, 								analysis_trace_load_elf)
	ADD_CMD_TO_INPUT_PARSER(parser, "print trace", 				"Print trace's instructions (assembly code)", 	"Index or range", 			INPUTPARSER_CMD_TYPE_OPT_ARG, 	analysis, 								analysis_trace_print)
	ADD_CMD_TO_INPUT_PARSER(parser, "check trace", 				"Check the current trace for format errors", 	NULL, 						INPUTPARSER_CMD_TYPE_NO_ARG, 	analysis, 								analysis_trace_check)
	ADD_CMD_TO_INPUT_PARSER(parser, "check codeMap", 			"Perform basic checks on the codeMap address", 	NULL, 						INPUTPARSER_CMD_TYPE_NO_ARG, 	analysis, 								analysis_trace_check_codeMap)
	ADD_CMD_TO_INPUT_PARSER(parser, "print codeMap", 			"Print the codeMap", 							"Specific filter", 			INPUTPARSER_CMD_TYPE_OPT_ARG, 	analysis, 								analysis_trace_print_codeMap)
	ADD_CMD_TO_INPUT_PARSER(parser, "export trace", 			"Export a trace segment as a traceFragment", 	"Range", 					INPUTPARSER_CMD_TYPE_ARG, 		analysis, 								analysis_trace_export)
	ADD_CMD_TO_INPUT_PARSER(parser, "locate pc", 				"Return trace offset that match a given pc", 	"PC (hexa)", 				INPUTPARSER_CMD_TYPE_ARG, 		analysis, 								analysis_trace_locate_pc)
	ADD_CMD_TO_INPUT_PARSER(parser, "clean trace", 				"Delete the current trace", 					NULL, 						INPUTPARSER_CMD_TYPE_NO_ARG, 	analysis, 								analysis_trace_delete)

	/* traceFragment specific commands */
	ADD_CMD_TO_INPUT_PARSER(parser, "print frag", 				"Print traceFragment (assembly or list)", 		"Frag index", 				INPUTPARSER_CMD_TYPE_OPT_ARG, 	analysis, 								analysis_frag_print)
	ADD_CMD_TO_INPUT_PARSER(parser, "set frag tag", 			"Set tag value for a given traceFragment", 		"Frag index and tag value", INPUTPARSER_CMD_TYPE_ARG, 		analysis, 								analysis_frag_set_tag)
	ADD_CMD_TO_INPUT_PARSER(parser, "locate frag", 				"Locate traceFragment in the codeMap", 			"Frag index", 				INPUTPARSER_CMD_TYPE_OPT_ARG, 	analysis, 								analysis_frag_locate)
	ADD_CMD_TO_INPUT_PARSER(parser, "concat frag", 				"Concat two or more traceFragments", 			"Frag indexes", 			INPUTPARSER_CMD_TYPE_ARG, 		analysis, 								analysis_frag_concat)
	ADD_CMD_TO_INPUT_PARSER(parser, "print result", 			"Print code signature result in details", 		"Frag index", 				INPUTPARSER_CMD_TYPE_OPT_ARG, 	analysis, 								analysis_frag_print_result)
	ADD_CMD_TO_INPUT_PARSER(parser, "export result", 			"Appends selected results to the IR", 			"Frag index & signatures", 	INPUTPARSER_CMD_TYPE_OPT_ARG, 	analysis, 								analysis_frag_export_result)
	ADD_CMD_TO_INPUT_PARSER(parser, "mine frag", 				"Search for relation between results in IR", 	"Frag index", 				INPUTPARSER_CMD_TYPE_OPT_ARG, 	analysis, 								analysis_frag_mine)
	ADD_CMD_TO_INPUT_PARSER(parser, "clean frag", 				"Clean the traceFragment array", 				NULL, 						INPUTPARSER_CMD_TYPE_NO_ARG, 	analysis, 								analysis_frag_clean)

	/* ir specific commands */
	ADD_CMD_TO_INPUT_PARSER(parser, "create ir", 				"Create an IR directly from a traceFragment", 	"Frag index", 				INPUTPARSER_CMD_TYPE_OPT_ARG, 	analysis, 								analysis_frag_create_ir)
	ADD_CMD_TO_INPUT_PARSER(parser, "printDot ir", 				"Write the IR to a file in the dot format", 	"Frag index", 				INPUTPARSER_CMD_TYPE_ARG, 		analysis, 								analysis_frag_printDot_ir)
	ADD_CMD_TO_INPUT_PARSER(parser, "normalize ir", 			"Normalize the IR (useful for signature)", 		"Frag index", 				INPUTPARSER_CMD_TYPE_OPT_ARG, 	analysis, 								analysis_frag_normalize_ir)
	ADD_CMD_TO_INPUT_PARSER(parser, "check ir", 				"Perform a set of tests on the IR", 			"Frag index", 				INPUTPARSER_CMD_TYPE_OPT_ARG, 	analysis, 								analysis_frag_check_ir)
	ADD_CMD_TO_INPUT_PARSER(parser, "print aliasing ir", 		"Print remaining aliasing conflict in IR", 		"Frag index", 				INPUTPARSER_CMD_TYPE_OPT_ARG, 	analysis, 								analysis_frag_print_aliasing_ir)

	/* code signature specific commands */
	ADD_CMD_TO_INPUT_PARSER(parser, "load code signature", 		"Load code signature from a file", 				"File path", 				INPUTPARSER_CMD_TYPE_ARG, 		&(analysis->code_signature_collection), codeSignatureReader_parse)
	ADD_CMD_TO_INPUT_PARSER(parser, "search code signature", 	"Search code signature for a given IR", 		"Frag index", 				INPUTPARSER_CMD_TYPE_OPT_ARG, 	analysis, 								analysis_code_signature_search)
	ADD_CMD_TO_INPUT_PARSER(parser, "printDot code signature", 	"Print every code signature in dot format", 	NULL, 						INPUTPARSER_CMD_TYPE_NO_ARG, 	&(analysis->code_signature_collection), codeSignatureCollection_printDot)
	ADD_CMD_TO_INPUT_PARSER(parser, "clean code signature", 	"Remove every code signature", 					NULL, 						INPUTPARSER_CMD_TYPE_NO_ARG, 	analysis, 								analysis_code_signature_clean)

	/* callGraph specific commands */
	ADD_CMD_TO_INPUT_PARSER(parser, "create callGraph", 		"Create a call graph", 							"OS & range [opt]", 		INPUTPARSER_CMD_TYPE_ARG, 		analysis, 								analysis_call_create)
	ADD_CMD_TO_INPUT_PARSER(parser, "printDot callGraph", 		"Write the call graph in the dot format", 		NULL, 						INPUTPARSER_CMD_TYPE_NO_ARG, 	analysis, 								analysis_call_printDot)
	ADD_CMD_TO_INPUT_PARSER(parser, "check callGraph", 			"Perform some check on the callGraph", 			NULL, 						INPUTPARSER_CMD_TYPE_NO_ARG, 	analysis, 								analysis_call_check)
	ADD_CMD_TO_INPUT_PARSER(parser, "export callGraph", 		"Export callGraph's routine as traceFragments", "Routine name", 			INPUTPARSER_CMD_TYPE_OPT_ARG, 	analysis, 								analysis_call_export)
	ADD_CMD_TO_INPUT_PARSER(parser, "print callGraph stack", 	"Print the call stack for a given instruction", "Index", 					INPUTPARSER_CMD_TYPE_ARG, 		analysis, 								analysis_call_print_stack)

	inputParser_exe(parser, argc - 1, argv + 1);

	exit:

	analysis_delete(analysis);
	inputParser_delete(parser);

	return 0;
}

/* ===================================================================== */
/* Analysis functions						                             */
/* ===================================================================== */

struct analysis* analysis_create(){
	struct analysis* 	analysis;

	analysis = (struct analysis*)malloc(sizeof(struct analysis));
	if (analysis == NULL){
		log_err("unable to allocate memory");
		return NULL;
	}

	codeSignatureCollection_init(&(analysis->code_signature_collection));

	if (array_init(&(analysis->frag_array), sizeof(struct trace))){
		log_err("unable to init traceFragment array");
		codeSignatureCollection_clean(&(analysis->code_signature_collection));
		free(analysis);
		return NULL;
	}

	analysis->trace 		= NULL;
	analysis->code_map 		= NULL;
	analysis->call_graph 	= NULL;

	return analysis;
}

void analysis_delete(struct analysis* analysis){
	if (analysis->call_graph != NULL){
		callGraph_delete(analysis->call_graph);
		analysis->call_graph = NULL;
	}

	analysis_frag_clean(analysis);
	array_clean(&(analysis->frag_array));

	codeSignatureCollection_clean(&(analysis->code_signature_collection));

	if (analysis->trace != NULL){
		trace_delete(analysis->trace);
		analysis->trace = NULL;
	}

	if (analysis->code_map != NULL){
		codeMap_delete(analysis->code_map);
		analysis->code_map = NULL;
	}

	free(analysis);
}

/* ===================================================================== */
/* Trace functions						                                 */
/* ===================================================================== */

void analysis_trace_load(struct analysis* analysis, char* arg){
	if (analysis->trace != NULL){
		log_warn("deleting previous trace");
		trace_delete(analysis->trace);

		if (analysis->call_graph != NULL){
			callGraph_delete(analysis->call_graph);
			analysis->call_graph = NULL;
		}
	}

	if (analysis->code_map != NULL){
		log_warn("deleting previous codeMap");
		codeMap_delete(analysis->code_map);
	}

	analysis->trace = trace_load(arg);
	if (analysis->trace == NULL){
		log_err("unable to create trace");
	}

	analysis->code_map = cmReaderJSON_parse(arg);
	if (analysis->code_map == NULL){
		log_err("unable to create codeMap");
	}
}

void analysis_trace_change_thread(struct analysis* analysis, char* arg){
	if (analysis->trace != NULL){
		trace_change_thread(analysis->trace, atoi(arg));
	}
	else{
		log_err("trace is NULL");
	}
}

void analysis_trace_load_elf(struct analysis* analysis, char* arg){
	if (analysis->trace != NULL){
		log_warn("deleting previous trace");
		trace_delete(analysis->trace);

		if (analysis->call_graph != NULL){
			callGraph_delete(analysis->call_graph);
			analysis->call_graph = NULL;
		}
	}

	if (analysis->code_map != NULL){
		log_warn("deleting previous codeMap");
		codeMap_delete(analysis->code_map);
		analysis->code_map = NULL;
	}

	analysis->trace = trace_load_elf(arg);
	if (analysis->trace == NULL){
		log_err("unable to create trace");
	}
}

void analysis_trace_print(struct analysis* analysis, char* arg){
	uint32_t start = 0;
	uint32_t stop = 0;

	if (analysis->trace != NULL){
		if (arg == NULL){
			stop = trace_get_nb_instruction(analysis->trace);
		}
		else{
			inputParser_extract_index(arg, &start, &stop);
		}
		trace_print(analysis->trace, start, stop);
	}
	else{
		log_err("trace is NULL");
	}
}

void analysis_trace_check(struct analysis* analysis){
	if (analysis->trace != NULL){
		trace_check(analysis->trace);
	}
	else{
		log_err("trace is NULL");
	}
}

void analysis_trace_check_codeMap(struct analysis* analysis){
	if (analysis->code_map != NULL){
		codeMap_check_address(analysis->code_map);
	}
	else{
		log_err("codeMap is NULL");
	}
}

void analysis_trace_print_codeMap(struct analysis* analysis, char* arg){
	if (analysis->code_map != NULL){
		codeMap_print(analysis->code_map, arg);
	}
	else{
		log_err("codeMap is NULL");
	}
}

void analysis_trace_export(struct analysis* analysis, char* arg){
	uint32_t 		start = 0;
	uint32_t 		stop = 0;
	struct trace 	fragment;

	if (analysis->trace != NULL){
		log_err("trace is NULL");
		return;
	}

	inputParser_extract_index(arg, &start, &stop);
	if (trace_extract_segment(analysis->trace, &fragment, start, stop - start)){
		log_err("unable to extract traceFragment");
		return;
	}

	snprintf(fragment.tag, TRACE_TAG_LENGTH, "trace [%u:%u]", start, stop);

	if (array_add(&(analysis->frag_array), &fragment) < 0){
		log_err("unable to add traceFragment to array");
		trace_clean(&fragment);
	}
}

void analysis_trace_locate_pc(struct analysis* analysis, char* arg){
	ADDRESS 					pc;
	struct instructionIterator 	it;
	uint32_t 					i;

	pc = strtoul((const char*)arg, NULL, 16);

	#if defined ARCH_32
	printf("Instance of EIP: 0x%08x:\n", pc);
	#elif defined ARCH_64
	printf("Instance of EIP: 0x%llx:\n", pc);
	#else
	#error Please specify an architecture {ARCH_32 or ARCH_64}
	#endif

	codeMap_print_address_info(analysis->code_map, pc, stdout);
	printf("\n");

	for (i = 0; i < analysis->trace->assembly.nb_dyn_block; i++){
		if (dynBlock_is_valid(analysis->trace->assembly.dyn_blocks + i)){
			if (pc >= analysis->trace->assembly.dyn_blocks[i].block->header.address && analysis->trace->assembly.dyn_blocks[i].block->header.address + analysis->trace->assembly.dyn_blocks[i].block->header.size > pc){
				if (assembly_get_instruction(&(analysis->trace->assembly), &it, analysis->trace->assembly.dyn_blocks[i].instruction_count)){
					log_err("unable to fetch first instruction from the assembly");
					continue;
				}

				for (;;){
					if (it.instruction_address == pc){
						printf("\t- Found EIP in trace at offset: %u\n", it.instruction_index);
						break;
					}

					if (it.instruction_index == analysis->trace->assembly.dyn_blocks[i].instruction_count + analysis->trace->assembly.dyn_blocks[i].block->header.nb_ins - 1){
						break;
					}

					if (assembly_get_next_instruction(&(analysis->trace->assembly), &it)){
						log_err("unable to fetch next instruction from the assembly");
						break;
					}
				}
			}
		}
	}
}

void analysis_trace_delete(struct analysis* analysis){
	if (analysis->trace != NULL){
		trace_delete(analysis->trace);
		analysis->trace = NULL;
	}
	else{
		log_err("trace is NULL");
	}

	if (analysis->code_map != NULL){
		codeMap_delete(analysis->code_map);
		analysis->code_map = NULL;
	}
	else{
		log_err("codeMap is NULL");
	}
}

/* ===================================================================== */
/* frag functions						                                 */
/* ===================================================================== */

void analysis_frag_print(struct analysis* analysis, char* arg){
	uint32_t 					i;
	struct multiColumnPrinter* 	printer;
	uint32_t 					index;
	struct trace*				fragment;
	double 						percent;
	uint32_t 					nb_opcode = 10;
	uint32_t 					opcode[10] = {XED_ICLASS_XOR, XED_ICLASS_SHL, XED_ICLASS_SHLD, XED_ICLASS_SHR, XED_ICLASS_SHRD, XED_ICLASS_NOT, XED_ICLASS_OR, XED_ICLASS_AND, XED_ICLASS_ROL, XED_ICLASS_ROR};
	uint32_t 					nb_excluded_opcode = 3;
	uint32_t 					excluded_opcode[3] = {XED_ICLASS_MOV, XED_ICLASS_PUSH, XED_ICLASS_POP};
	#define IRDESCRIPTOR_MAX_LENGTH 32
	char 						ir_descriptor[IRDESCRIPTOR_MAX_LENGTH];

	if (arg != NULL){
		index = (uint32_t)atoi(arg);
		
		if (index < array_get_length(&(analysis->frag_array))){
			fragment = (struct trace*)array_get(&(analysis->frag_array), index);
			trace_print(fragment, 0, trace_get_nb_instruction(fragment));
			return;
		}
		else{
			log_err_m("incorrect index value %u (array size :%u)", index, array_get_length(&(analysis->frag_array)));
		}
	}

	#ifdef VERBOSE
	printf("Included opcode(s): ");
	for (i = 0; i < nb_opcode; i++){
		if (i == nb_opcode - 1){
			printf("%s\n", xed_iclass_enum_t2str(opcode[i]));
		}
		else{
			printf("%s, ", xed_iclass_enum_t2str(opcode[i]));
		}
	}
	printf("Excluded opcode(s): ");
	for (i = 0; i < nb_excluded_opcode; i++){
		if (i == nb_excluded_opcode - 1){
			printf("%s\n", xed_iclass_enum_t2str(excluded_opcode[i]));
		}
		else{
			printf("%s, ", xed_iclass_enum_t2str(excluded_opcode[i]));
		}
	}
	#endif

	printer = multiColumnPrinter_create(stdout, 5, NULL, NULL, NULL);
	if (printer != NULL){
		multiColumnPrinter_set_column_size(printer, 0, 5);
		multiColumnPrinter_set_column_size(printer, 1, TRACE_TAG_LENGTH);
		multiColumnPrinter_set_column_size(printer, 2, 8);
		multiColumnPrinter_set_column_size(printer, 3, 16);
		multiColumnPrinter_set_column_size(printer, 4, IRDESCRIPTOR_MAX_LENGTH);

		multiColumnPrinter_set_column_type(printer, 0, MULTICOLUMN_TYPE_INT32);
		multiColumnPrinter_set_column_type(printer, 2, MULTICOLUMN_TYPE_INT32);
		multiColumnPrinter_set_column_type(printer, 3, MULTICOLUMN_TYPE_DOUBLE);

		multiColumnPrinter_set_title(printer, 0, "Index");
		multiColumnPrinter_set_title(printer, 1, "Tag");
		multiColumnPrinter_set_title(printer, 2, "Ins");
		multiColumnPrinter_set_title(printer, 3, "Percent (%)");
		multiColumnPrinter_set_title(printer, 4, "IR");

		multiColumnPrinter_print_header(printer);

		for (i = 0; i < array_get_length(&(analysis->frag_array)); i++){
			fragment = (struct trace*)array_get(&(analysis->frag_array), i);
			percent = trace_opcode_percent(fragment, nb_opcode, opcode, nb_excluded_opcode, excluded_opcode);
			if (fragment->ir == NULL){
				snprintf(ir_descriptor, IRDESCRIPTOR_MAX_LENGTH, "NULL");
			}
			else{
				snprintf(ir_descriptor, IRDESCRIPTOR_MAX_LENGTH, "%u node(s), %u edge(s)", fragment->ir->graph.nb_node, fragment->ir->graph.nb_edge);
			}
			multiColumnPrinter_print(printer, i, fragment->tag, trace_get_nb_instruction(fragment), percent*100, ir_descriptor, NULL);
		}

		multiColumnPrinter_delete(printer);
	}
	else{
		log_err("unable to create multiColumnPrinter");
	}
	#undef IRDESCRIPTOR_MAX_LENGTH
}

void analysis_frag_set_tag(struct analysis* analysis, char* arg){
	uint32_t 		i;
	uint32_t 		index;
	struct trace* 	fragment;
	uint8_t 		found_space = 0;

	for (i = 0; i < strlen(arg) - 1; i++){
		if (arg[i] == ' '){
			found_space = 1;
			break;
		}
	}
	if (found_space){
		index = (uint32_t)atoi(arg);

		if (index < array_get_length(&(analysis->frag_array))){
			fragment = (struct trace*)array_get(&(analysis->frag_array), index);

			#ifdef VERBOSE
			log_info_m("setting tag value for frag %u: old tag: \"%s\", new tag: \"%s\"", index, fragment->tag, arg + i + 1);
			#endif

			strncpy(fragment->tag, arg + i + 1, TRACE_TAG_LENGTH);
		}
		else{
			log_err_m("incorrect index value %u (array size :%u)", index, array_get_length(&(analysis->frag_array)));
		}
	}
	else{
		log_err("the index and the tag must separated by a space char");
	}
}

void analysis_frag_locate(struct analysis* analysis, char* arg){
	uint32_t 		i;
	uint32_t 		index;
	uint32_t 		start;
	uint32_t 		stop;
	struct trace* 	fragment;

	if (arg != NULL){
		index = (uint32_t)atoi(arg);
		if (index < array_get_length(&(analysis->frag_array))){
			start = index;
			stop = index + 1;
		}
		else{
			log_err_m("incorrect index value %u (array size :%u)", index, array_get_length(&(analysis->frag_array)));
			return;
		}
	}
	else{
		start = 0;
		stop = array_get_length(&(analysis->frag_array));
	}

	if (analysis->code_map != NULL){
		for (i = start; i < stop; i++){
			fragment = (struct trace*)array_get(&(analysis->frag_array), i);

			#ifdef VERBOSE
			printf("Locating frag %u (tag: \"%s\")\n", i, fragment->tag);
			#endif
			
			trace_print_location(fragment, analysis->code_map);
		}
	}
	else{
		log_err("codeMap is NULL, unable to locate");
	}
}

void analysis_frag_concat(struct analysis* analysis, char* arg){
	uint32_t 		i;
	uint32_t 		nb_index;
	uint8_t 		start_index;
	uint32_t 		index;
	struct trace** 	trace_src_buffer;
	struct trace 	new_fragment;

	for (i = 0, nb_index = 0, start_index = 0; i < strlen(arg); i++){
		if (arg[i] >= 48 && arg[i] <= 57){
			if (start_index == 0){
				start_index = 1;
				nb_index ++;
			}
		}
		else{
			start_index = 0;
		}
	}

	if (nb_index < 2){
		log_err_m("at last two indexes must be specified (but get %u)", nb_index);
		return;
	}

	trace_src_buffer = (struct trace**)alloca(sizeof(struct trace*) * nb_index);
	for (i = 0, nb_index = 0, start_index = 0; i < strlen(arg); i++){
		if (arg[i] >= 48 && arg[i] <= 57){
			if (start_index == 0){
				index = atoi(arg + i);
				if (index < array_get_length(&(analysis->frag_array))){
					trace_src_buffer[nb_index ++] = (struct trace*)array_get(&(analysis->frag_array), index);
				}
				else{
					log_err_m("the index specified @ %u is incorrect (array size: %u)", nb_index, array_get_length(&(analysis->frag_array)));
				}

				start_index = 1;
			}
		}
		else{
			start_index = 0;
		}
	}

	if (nb_index < 2){
		log_err_m("at last two valid indexes must be specified (but get %u)", nb_index);
		return;
	}

	if (trace_concat(trace_src_buffer, nb_index, &new_fragment)){
		log_err("unable to concat the given traceFragment(s)");
		return;
	}

	snprintf(new_fragment.tag, TRACE_TAG_LENGTH, "concat %s", arg);

	if (array_add(&(analysis->frag_array), &new_fragment) < 0){
		log_err("unable to add traceFragment to array");
		trace_clean(&new_fragment);
	}
}

void analysis_frag_print_result(struct analysis* analysis, char* arg){
	uint32_t 		index;
	uint32_t 		start;
	uint32_t 		stop;
	uint32_t 		i;
	uint32_t 		j;
	struct trace* 	fragment;

	if (arg != NULL){
		index = (uint32_t)atoi(arg);
		if (index < array_get_length(&(analysis->frag_array))){
			start = index;
			stop = index + 1;
		}
		else{
			log_err_m("incorrect index value %u (array size :%u)", index, array_get_length(&(analysis->frag_array)));
			return;
		}
	}
	else{
		start = 0;
		stop = array_get_length(&(analysis->frag_array));
	}

	for (i = start; i < stop; i++){
		fragment = (struct trace*)array_get(&(analysis->frag_array), i);
		for (j = 0; j < array_get_length(&(fragment->result_array)); j++){
			result_print((struct result*)array_get(&(fragment->result_array), j));
		}
	}
}

void analysis_frag_export_result(struct analysis* analysis, char* arg){
	uint32_t 				index;
	uint32_t 				start;
	uint32_t 				stop;
	uint32_t 				i;
	void** 					signature_buffer;
	uint32_t 				nb_signature;
	struct node* 			node_cursor;
	struct codeSignature* 	signature_cursor;
	char* 					ptr;

	signature_buffer = (void**)malloc(sizeof(void*) * codeSignaturecollection_get_nb_signature(&(analysis->code_signature_collection)));
	if (signature_buffer == NULL){
		log_err("unable to allocate memory");
		return;
	}

	if (arg != NULL){
		index = (uint32_t)atoi(arg);
		if (index < array_get_length(&(analysis->frag_array))){
			start = index;
			stop = index + 1;
		}
		else{
			log_err_m("incorrect index value %u (array size :%u)", index, array_get_length(&(analysis->frag_array)));
			goto exit;
		}
	}
	else{
		start = 0;
		stop = array_get_length(&(analysis->frag_array));
	}

	for (node_cursor = graph_get_head_node(&(analysis->code_signature_collection.syntax_graph)), nb_signature = 0; node_cursor != NULL; node_cursor = node_get_next(node_cursor)){
		signature_cursor = syntax_node_get_codeSignature(node_cursor);
		if (arg == NULL){
			signature_buffer[nb_signature ++] = signature_cursor;
		}
		else{
			ptr = strstr(arg, signature_cursor->name);
			if (ptr != NULL){
				if (strlen(ptr) == strlen(signature_cursor->name) || ptr[strlen(signature_cursor->name)] == ' '){
					if (ptr == arg || ptr[-1] == ' '){
						signature_buffer[nb_signature ++] = signature_cursor;
					}
				}
			}
		}
	}

	for (i = start; i < stop; i++){
		trace_export_result((struct trace*)array_get(&(analysis->frag_array), i), signature_buffer, nb_signature);
	}

	exit:
	free(signature_buffer);
}

void analysis_frag_mine(struct analysis* analysis, char* arg){
	uint32_t index;
	uint32_t start;
	uint32_t stop;
	uint32_t i;

	if (arg != NULL){
		index = (uint32_t)atoi(arg);
		if (index < array_get_length(&(analysis->frag_array))){
			start = index;
			stop = index + 1;
		}
		else{
			log_err_m("incorrect index value %u (array size :%u)", index, array_get_length(&(analysis->frag_array)));
			return;
		}
	}
	else{
		start = 0;
		stop = array_get_length(&(analysis->frag_array));
	}

	for (i = start; i < stop; i++){
		traceMine_mine((struct trace*)array_get(&(analysis->frag_array), i));
	}
}

void analysis_frag_clean(struct analysis* analysis){
	uint32_t i;

	for (i = 0; i < array_get_length(&(analysis->frag_array)); i++){
		trace_clean((struct trace*)array_get(&(analysis->frag_array), i));
	}
	array_empty(&(analysis->frag_array));
}

/* ===================================================================== */
/* ir functions						                                	 */
/* ===================================================================== */

void analysis_frag_create_ir(struct analysis* analysis, char* arg){
	uint32_t index;
	uint32_t start;
	uint32_t stop;
	uint32_t i;

	if (arg != NULL){
		index = (uint32_t)atoi(arg);
		if (index < array_get_length(&(analysis->frag_array))){
			start = index;
			stop = index + 1;
		}
		else{
			log_err_m("incorrect index value %u (array size :%u)", index, array_get_length(&(analysis->frag_array)));
			return;
		}
	}
	else{
		start = 0;
		stop = array_get_length(&(analysis->frag_array));
	}

	for (i = start; i < stop; i++){	
		trace_create_ir((struct trace*)array_get(&(analysis->frag_array), i));
	}

	return;
}

void analysis_frag_printDot_ir(struct analysis* analysis, char* arg){
	uint32_t index;

	index = (uint32_t)atoi(arg);
	if (index < array_get_length(&(analysis->frag_array))){
		trace_printDot_ir((struct trace*)array_get(&(analysis->frag_array), index));
	}
	else{
		log_err_m("incorrect index value %u (array size :%u)", index, array_get_length(&(analysis->frag_array)));
	}
}

void analysis_frag_normalize_ir(struct analysis* analysis, char* arg){
	uint32_t index;
	uint32_t start;
	uint32_t stop;
	uint32_t i;

	if (arg != NULL){
		index = (uint32_t)atoi(arg);
		if (index < array_get_length(&(analysis->frag_array))){
			start = index;
			stop = index + 1;
		}
		else{
			log_err_m("incorrect index value %u (array size :%u)", index, array_get_length(&(analysis->frag_array)));
			return;
		}
	}
	else{
		start = 0;
		stop = array_get_length(&(analysis->frag_array));
	}

	for (i = start; i < stop; i++){
		trace_normalize_ir((struct trace*)array_get(&(analysis->frag_array), i));
	}
}

void analysis_frag_check_ir(struct analysis* analysis, char* arg){
	uint32_t 		index;
	uint32_t 		start;
	uint32_t 		stop;
	uint32_t 		i;
	struct trace* 	fragment;

	if (arg != NULL){
		index = (uint32_t)atoi(arg);
		if (index < array_get_length(&(analysis->frag_array))){
			start = index;
			stop = index + 1;
		}
		else{
			log_err_m("incorrect index value %u (array size :%u)", index, array_get_length(&(analysis->frag_array)));
			return;
		}
	}
	else{
		start = 0;
		stop = array_get_length(&(analysis->frag_array));
	}

	for (i = start; i < stop; i++){
		fragment = (struct trace*)array_get(&(analysis->frag_array), i);
		if (fragment->ir != NULL){
			ir_check(fragment->ir);
		}
		else{
			log_err("the IR is NULL for the current fragment");
		}
	}
}

void analysis_frag_print_aliasing_ir(struct analysis* analysis, char* arg){
	uint32_t 		index;
	uint32_t 		start;
	uint32_t 		stop;
	uint32_t 		i;
	struct trace* 	fragment;

	if (arg != NULL){
		index = (uint32_t)atoi(arg);
		if (index < array_get_length(&(analysis->frag_array))){
			start = index;
			stop = index + 1;
		}
		else{
			log_err_m("incorrect index value %u (array size :%u)", index, array_get_length(&(analysis->frag_array)));
			return;
		}
	}
	else{
		start = 0;
		stop = array_get_length(&(analysis->frag_array));
	}

	for (i = start; i < stop; i++){
		fragment = (struct trace*)array_get(&(analysis->frag_array), i);
		if (fragment->ir != NULL){
			ir_print_aliasing(fragment->ir);
		}
		else{
			log_err("the IR is NULL for the current fragment");
		}
	}
}

/* ===================================================================== */
/* code signature functions						                         */
/* ===================================================================== */

void analysis_code_signature_search(struct analysis* analysis, char* arg){
	uint32_t 		index;
	uint32_t 		start;
	uint32_t 		stop;
	uint32_t 		i;
	struct trace* 	fragment;
	struct trace**	trace_buffer;
	uint32_t 		nb_trace;

	if (arg != NULL){
		index = (uint32_t)atoi(arg);
		if (index < array_get_length(&(analysis->frag_array))){
			start = index;
			stop = index + 1;
		}
		else{
			log_err_m("incorrect index value %u (array size :%u)", index, array_get_length(&(analysis->frag_array)));
			return;
		}
	}
	else{
		start = 0;
		stop = array_get_length(&(analysis->frag_array));
	}

	trace_buffer = (struct trace**)malloc(sizeof(struct trace*) * (stop - start));
	if (trace_buffer == NULL){
		log_err("unable to allocate memory");
		return;
	}

	for (i = start, nb_trace = 0; i < stop; i++){
		fragment = (struct trace*)array_get(&(analysis->frag_array), i);
		if (fragment->ir != NULL){
			trace_buffer[nb_trace ++] = fragment;
		}
		else{
			log_err_m("the IR is NULL for fragment %u", i);
		}
	}

	codeSignatureCollection_search(&(analysis->code_signature_collection), trace_buffer, nb_trace);
	free(trace_buffer);
}

void analysis_code_signature_clean(struct analysis* analysis){
	struct trace* 	fragment;
	uint32_t 		i;
	uint32_t 		j;
	struct result* 	result;

	for (i = 0; i < array_get_length(&(analysis->frag_array)); i++){
		fragment = (struct trace*)array_get(&(analysis->frag_array), i);

		if (array_get_length(&(fragment->result_array))){
			for (j = 0; j < array_get_length(&(fragment->result_array)); j++){
				result = (struct result*)array_get(&(fragment->result_array), j);
				if (result->state == RESULTSTATE_PUSH && fragment->ir != NULL){
					ir_delete(fragment->ir)
					fragment->ir = NULL;
				}
				result_clean(result)
			}
			array_empty(&(fragment->result_array));
		}
	}

	codeSignatureCollection_clean(&(analysis->code_signature_collection));
}

/* ===================================================================== */
/* call graph functions						    	                     */
/* ===================================================================== */

void analysis_call_create(struct analysis* analysis, char* arg){
	uint32_t start = 0;
	uint32_t stop;

	if (analysis->call_graph != NULL){
		log_warn("deleting previous callGraph");
		callGraph_delete(analysis->call_graph);
		analysis->call_graph = NULL;
	}

	if (analysis->trace == NULL){
		log_err("trace is NULL");
	}
	else{
		stop = trace_get_nb_instruction(analysis->trace);
		inputParser_extract_index(arg, &start, &stop);

		analysis->call_graph = callGraph_create(analysis->trace, start, stop);
		if (analysis->call_graph == NULL){
			log_err("unable to create callGraph");
		}
		else if (analysis->code_map != NULL){
			if (strstr(arg, "LINUX")){
				callGraph_locate_in_codeMap_linux(analysis->call_graph, analysis->trace, analysis->code_map);
			}
			else if (strstr(arg, "WINDOWS")){
				callGraph_locate_in_codeMap_windows(analysis->call_graph, analysis->trace, analysis->code_map);
			}
			else{
				log_err("expected os specifier: {LINUX, WINDOWS}");
			}
		}
	}
}

void analysis_call_printDot(struct analysis* analysis){
	if (analysis->call_graph == NULL){
		log_err("callGraph is NULL cannot print");
	}
	else{
		callGraph_printDot(analysis->call_graph);
	}
}

void analysis_call_check(struct analysis* analysis){
	if (analysis->call_graph == NULL){
		log_err("callGraph is NULL cannot check");
	}
	else{
		callGraph_check(analysis->call_graph, analysis->code_map);
	}
}

void analysis_call_export(struct analysis* analysis, char* arg){
	if (analysis->call_graph == NULL){
		log_err("callGraph is NULL cannot export");
	}
	else{
		if (callGraph_export_inclusive(analysis->call_graph, analysis->trace, &(analysis->frag_array), arg)){
			log_err("unable to export callGraph");
		}
	}
}

void analysis_call_print_stack(struct analysis* analysis, char* arg){
	if (analysis->call_graph == NULL){
		log_err("callGraph is NULL cannot print stack");
	}
	else{
		callGraph_print_stack(analysis->call_graph, atoi(arg));
	}
}