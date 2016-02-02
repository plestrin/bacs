#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "analysis.h"
#include "inputParser.h"
#include "readBuffer.h"
#include "result.h"
#include "codeSignature.h"
#include "codeSignatureReader.h"
#include "modeSignature.h"
#include "modeSignatureReader.h"
#include "cmReaderJSON.h"
#include "multiColumn.h"
#include "base.h"

#define add_cmd_to_input_parser(parser, cmd, cmd_desc, arg_desc, type, arg, func)														\
	{																																	\
		if (inputParser_add_cmd((parser), (cmd), (cmd_desc), (arg_desc), (type), (arg), (void(*)(void))(func))){						\
			log_err_m("unable to add cmd: \"%s\" to inputParser", (cmd));																\
		}																																\
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
	add_cmd_to_input_parser(parser, "load trace", 				"Load a trace in the analysis engine", 			"Trace directory", 			INPUTPARSER_CMD_TYPE_ARG, 		analysis, 								analysis_trace_load)
	add_cmd_to_input_parser(parser, "change trace", 			"Switch the current process/thread", 			"Proc and thread index", 	INPUTPARSER_CMD_TYPE_ARG, 		analysis, 								analysis_trace_change)
	add_cmd_to_input_parser(parser, "load elf", 				"Load an ELF file in the analysis engine", 		"ELF file", 				INPUTPARSER_CMD_TYPE_ARG, 		analysis, 								analysis_trace_load_elf)
	add_cmd_to_input_parser(parser, "print trace", 				"Print trace's instructions (assembly code)", 	"Index or range", 			INPUTPARSER_CMD_TYPE_OPT_ARG, 	analysis, 								analysis_trace_print)
	add_cmd_to_input_parser(parser, "check trace", 				"Check the current trace for format errors", 	NULL, 						INPUTPARSER_CMD_TYPE_NO_ARG, 	analysis, 								analysis_trace_check)
	add_cmd_to_input_parser(parser, "check codeMap", 			"Perform basic checks on the codeMap address", 	NULL, 						INPUTPARSER_CMD_TYPE_NO_ARG, 	analysis, 								analysis_trace_check_codeMap)
	add_cmd_to_input_parser(parser, "print codeMap", 			"Print the codeMap", 							"Specific filter", 			INPUTPARSER_CMD_TYPE_OPT_ARG, 	analysis, 								analysis_trace_print_codeMap)
	add_cmd_to_input_parser(parser, "export trace", 			"Export a trace segment as a traceFragment", 	"Range", 					INPUTPARSER_CMD_TYPE_ARG, 		analysis, 								analysis_trace_export)
	add_cmd_to_input_parser(parser, "locate pc", 				"Return trace offset that match a given pc", 	"PC (hexa)", 				INPUTPARSER_CMD_TYPE_ARG, 		analysis, 								analysis_trace_locate_pc)
	add_cmd_to_input_parser(parser, "locate opcode", 			"Search given hexa string in the trace", 		"Hexa string", 				INPUTPARSER_CMD_TYPE_ARG, 		analysis, 								analysis_trace_locate_opcode)
	add_cmd_to_input_parser(parser, "scan trace", 				"Scan trace and report interesting fragments", 	"Filters", 					INPUTPARSER_CMD_TYPE_OPT_ARG, 	analysis, 								analysis_trace_scan)
	add_cmd_to_input_parser(parser, "clean trace", 				"Delete the current trace", 					NULL, 						INPUTPARSER_CMD_TYPE_NO_ARG, 	analysis, 								analysis_trace_delete)

	/* traceFragment specific commands */
	add_cmd_to_input_parser(parser, "print frag", 				"Print traceFragment (assembly or list)", 		"Frag index", 				INPUTPARSER_CMD_TYPE_OPT_ARG, 	analysis, 								analysis_frag_print)
	add_cmd_to_input_parser(parser, "set frag tag", 			"Set tag value for a given traceFragment", 		"Frag index and tag value", INPUTPARSER_CMD_TYPE_ARG, 		analysis, 								analysis_frag_set_tag)
	add_cmd_to_input_parser(parser, "locate frag", 				"Locate traceFragment in the codeMap", 			"Frag index", 				INPUTPARSER_CMD_TYPE_OPT_ARG, 	analysis, 								analysis_frag_locate)
	add_cmd_to_input_parser(parser, "concat frag", 				"Concat two or more traceFragments", 			"Frag indexes", 			INPUTPARSER_CMD_TYPE_ARG, 		analysis, 								analysis_frag_concat)
	add_cmd_to_input_parser(parser, "check frag", 				"Check traceFragment: assembly and IR", 		"Frag index or Frag range", INPUTPARSER_CMD_TYPE_OPT_ARG, 	analysis, 								analysis_frag_check)
	add_cmd_to_input_parser(parser, "print result", 			"Print code signature result in details", 		"Frag index", 				INPUTPARSER_CMD_TYPE_OPT_ARG, 	analysis, 								analysis_frag_print_result)
	add_cmd_to_input_parser(parser, "export result", 			"Appends selected results to the IR", 			"Frag index & signatures", 	INPUTPARSER_CMD_TYPE_OPT_ARG, 	analysis, 								analysis_frag_export_result)
	add_cmd_to_input_parser(parser, "clean frag", 				"Clean the traceFragment array", 				NULL, 						INPUTPARSER_CMD_TYPE_NO_ARG, 	analysis, 								analysis_frag_clean)

	/* ir specific commands */
	add_cmd_to_input_parser(parser, "create ir", 				"Create an IR directly from a traceFragment", 	"Frag index or Frag range", INPUTPARSER_CMD_TYPE_OPT_ARG, 	analysis, 								analysis_frag_create_ir)
	add_cmd_to_input_parser(parser, "create compound ir", 		"Create an IR, using previously created IR(s)", "Frag index", 				INPUTPARSER_CMD_TYPE_OPT_ARG, 	analysis, 								analysis_frag_create_compound_ir)
	add_cmd_to_input_parser(parser, "printDot ir", 				"Write the IR to a file in the dot format", 	"Frag index", 				INPUTPARSER_CMD_TYPE_OPT_ARG, 	analysis, 								analysis_frag_printDot_ir)
	add_cmd_to_input_parser(parser, "normalize ir", 			"Normalize the IR (useful for signature)", 		"Frag index or Frag range", INPUTPARSER_CMD_TYPE_OPT_ARG, 	analysis, 								analysis_frag_normalize_ir)
	add_cmd_to_input_parser(parser, "print aliasing ir", 		"Print remaining aliasing conflict in IR", 		"Frag index or Frag range", INPUTPARSER_CMD_TYPE_OPT_ARG, 	analysis, 								analysis_frag_print_aliasing_ir)
	add_cmd_to_input_parser(parser, "simplify concrete ir", 	"Simplify memory accesses using concrete addr", "Frag index or Frag range", INPUTPARSER_CMD_TYPE_OPT_ARG, 	analysis, 								analysis_frag_simplify_concrete_ir)

	/* signature specific commands */
	add_cmd_to_input_parser(parser, "load code signature", 		"Load code signature from a file", 				"File path", 				INPUTPARSER_CMD_TYPE_ARG, 		&(analysis->code_signature_collection), codeSignatureReader_parse)
	add_cmd_to_input_parser(parser, "search code signature", 	"Search code signature for a given IR", 		"Frag index", 				INPUTPARSER_CMD_TYPE_OPT_ARG, 	analysis, 								analysis_code_signature_search)
	add_cmd_to_input_parser(parser, "printDot code signature", 	"Print every code signature in dot format", 	NULL, 						INPUTPARSER_CMD_TYPE_NO_ARG, 	&(analysis->code_signature_collection), signatureCollection_printDot)
	add_cmd_to_input_parser(parser, "clean code signature", 	"Remove every code signature", 					NULL, 						INPUTPARSER_CMD_TYPE_NO_ARG, 	analysis, 								analysis_code_signature_clean)
	add_cmd_to_input_parser(parser, "load mode signature", 		"Load mode signature from a file", 				"File path", 				INPUTPARSER_CMD_TYPE_ARG, 		&(analysis->mode_signature_collection), modeSignatureReader_parse)
	add_cmd_to_input_parser(parser, "search mode signature", 	"Search mode signature for a given synthesis", 	"Frag index", 				INPUTPARSER_CMD_TYPE_OPT_ARG, 	analysis, 								analysis_mode_signature_search)
	add_cmd_to_input_parser(parser, "printDot mode signature", 	"Print every mode signature in dot format", 	NULL, 						INPUTPARSER_CMD_TYPE_NO_ARG, 	&(analysis->mode_signature_collection), signatureCollection_printDot)
	add_cmd_to_input_parser(parser, "clean mode signature", 	"Remove every mode signature", 					NULL, 						INPUTPARSER_CMD_TYPE_NO_ARG, 	&(analysis->mode_signature_collection), signatureCollection_clean)

	/* callGraph specific commands */
	add_cmd_to_input_parser(parser, "create callGraph", 		"Create a call graph", 							"OS & range [opt]", 		INPUTPARSER_CMD_TYPE_ARG, 		analysis, 								analysis_call_create)
	add_cmd_to_input_parser(parser, "printDot callGraph", 		"Write the call graph in the dot format", 		NULL, 						INPUTPARSER_CMD_TYPE_NO_ARG, 	analysis, 								analysis_call_printDot)
	add_cmd_to_input_parser(parser, "check callGraph", 			"Perform some check on the callGraph", 			NULL, 						INPUTPARSER_CMD_TYPE_NO_ARG, 	analysis, 								analysis_call_check)
	add_cmd_to_input_parser(parser, "export callGraph", 		"Export callGraph's routine as traceFragments", "Routine name or Index", 	INPUTPARSER_CMD_TYPE_ARG, 		analysis, 								analysis_call_export)
	add_cmd_to_input_parser(parser, "print callGraph stack", 	"Print the call stack for a given instruction", "Index", 					INPUTPARSER_CMD_TYPE_ARG, 		analysis, 								analysis_call_print_stack)

	/* synthesisGraph specific commands */
	add_cmd_to_input_parser(parser, "create synthesis", 		"Search for relation between results in IR", 	"Frag index or Frag range", INPUTPARSER_CMD_TYPE_OPT_ARG, 	analysis, 								analysis_synthesis_create)
	add_cmd_to_input_parser(parser, "printDot synthesis", 		"Print the synthesis graph in dot format", 		"Frag index", 				INPUTPARSER_CMD_TYPE_OPT_ARG, 	analysis, 								analysis_synthesis_printDot)

	inputParser_exe(parser, argc - 1, argv + 1);

	exit:

	analysis_delete(analysis);
	inputParser_delete(parser);

	return 0;
}

#define apply_to_one_frag(analysis, func, arg) 																							\
	{ 																																	\
		uint32_t index; 																												\
																																		\
		if ((arg) == NULL){ 																											\
			if (array_get_length(&((analysis)->frag_array)) < 2){  																		\
				index = 0; 																												\
			} 																															\
			else{ 																														\
				log_err_m("%u fragments available, please specify fragment index", array_get_length(&((analysis)->frag_array))); 		\
				return; 																												\
			} 																															\
		} 																																\
		else{ 																															\
			index = (uint32_t)atoi((arg)); 																								\
		} 																																\
																																		\
		if (index < array_get_length(&((analysis)->frag_array))){ 																		\
			func((struct trace*)array_get(&((analysis)->frag_array), index)); 															\
		} 																																\
		else{ 																															\
			log_err_m("incorrect fragment index %u (array size: %u)", index, array_get_length(&((analysis)->frag_array))); 				\
		} 																																\
	}

#define apply_to_multiple_frags(analysis, func, arg) 																					\
	{ 																																	\
		uint32_t start; 																												\
		uint32_t stop; 																													\
		uint32_t i; 																													\
																																		\
		if ((arg) != NULL){ 																											\
			inputParser_extract_index(arg, &start, &stop); 																				\
			if (start >= array_get_length(&((analysis)->frag_array))){ 																	\
				log_err_m("incorrect fragment index %u (array size: %u)", start, array_get_length(&((analysis)->frag_array))); 			\
				return; 																												\
			} 																															\
			if (stop > array_get_length(&((analysis)->frag_array))){ 																	\
				log_warn_m("fragment range exceeds array size, cropping to %u", array_get_length(&((analysis)->frag_array))); 			\
				stop = array_get_length(&((analysis)->frag_array)); 																	\
			} 																															\
		} 																																\
		else{ 																															\
			start = 0; 																													\
			stop = array_get_length(&((analysis)->frag_array)); 																		\
		} 																																\
																																		\
		for (i = start; i < stop; i++){ 																								\
			func((struct trace*)array_get(&((analysis)->frag_array), i)); 																\
		} 																																\
	}


/* ===================================================================== */
/* Analysis functions						                             */
/* ===================================================================== */

struct analysis* analysis_create(){
	struct analysis* 			analysis;
	struct signatureCallback 	callback;

	analysis = (struct analysis*)malloc(sizeof(struct analysis));
	if (analysis == NULL){
		log_err("unable to allocate memory");
		return NULL;
	}

	if (array_init(&(analysis->frag_array), sizeof(struct trace))){
		log_err("unable to init traceFragment array");
		free(analysis);
		return NULL;
	}

	callback.signatureNode_get_label = codeSignatureNode_get_label;
	callback.signatureEdge_get_label = codeSignatureEdge_get_label;

	signatureCollection_init(&(analysis->code_signature_collection), sizeof(struct codeSignature), &callback);

	callback.signatureNode_get_label = modeSignatureNode_get_label;
	callback.signatureEdge_get_label = modeSignatureEdge_get_label;

	signatureCollection_init(&(analysis->mode_signature_collection), sizeof(struct modeSignature), &callback);
	graph_register_node_clean_call_back(&(analysis->mode_signature_collection.syntax_graph), modeSignature_clean);

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

	signatureCollection_clean(&(analysis->code_signature_collection));
	signatureCollection_clean(&(analysis->mode_signature_collection));

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
		analysis->code_map = NULL;
	}

	analysis->trace = trace_load_exe(arg);
	if (analysis->trace == NULL){
		log_err("unable to create trace");
		return;
	}

	analysis->code_map = cmReaderJSON_parse(arg, analysis->trace->trace_type.exe.identifier.current_pid);
	if (analysis->code_map == NULL){
		log_err("unable to create codeMap");
	}
}

void analysis_trace_change(struct analysis* analysis, char* arg){
	char* 		t_index_ptr;
	uint32_t 	prev_pid;

	if (analysis->trace != NULL){
		prev_pid = analysis->trace->trace_type.exe.identifier.current_pid;
		t_index_ptr = strchr(arg, ' ');
		if (trace_change(analysis->trace, atoi(arg), (t_index_ptr != NULL) ? atoi(t_index_ptr + 1) : 0)){
			log_err_m("unable to load trace process:%u, thread:%u", (uint32_t)atoi(arg), (uint32_t)((t_index_ptr != NULL) ? atoi(t_index_ptr + 1) : 0));
			trace_delete(analysis->trace);
			analysis->trace = NULL;
			return;
		}

		if (prev_pid != analysis->trace->trace_type.exe.identifier.current_pid){
			if (analysis->code_map != NULL){
				log_warn("deleting previous codeMap");
				codeMap_delete(analysis->code_map);
				analysis->code_map = NULL;
			}

			analysis->code_map = cmReaderJSON_parse(analysis->trace->trace_type.exe.directory_path, analysis->trace->trace_type.exe.identifier.current_pid);
			if (analysis->code_map == NULL){
				log_err("unable to create codeMap");
			}
		}
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
	uint32_t 		i;

	if (analysis->trace == NULL){
		log_err("trace is NULL");
		return;
	}

	inputParser_extract_index(arg, &start, &stop);
	if (trace_extract_segment(analysis->trace, &fragment, start, stop - start)){
		log_err("unable to extract traceFragment");
		return;
	}

	for (i = 0; i < array_get_length(&(analysis->frag_array)); i++){
		if (trace_compare(&fragment, (struct trace*)array_get(&(analysis->frag_array), i)) == 0){
			log_info_m("an equivalent fragment (%u) has already been exported", i);
			trace_clean(&fragment);
			return;
		}
	}

	if (array_add(&(analysis->frag_array), &fragment) < 0){
		log_err("unable to add traceFragment to array");
		trace_clean(&fragment);
	}
}

void analysis_trace_locate_pc(struct analysis* analysis, char* arg){
	ADDRESS 					pc;
	struct instructionIterator 	it;
	int32_t 					return_code;
	uint32_t 					last_index = 0;

	if (analysis->trace == NULL){
		log_err("trace is NULL");
		return;
	}

	pc = strtoul((const char*)arg, NULL, 16);

	#if defined ARCH_32
	printf("Instance of EIP: 0x%08x:\n", pc);
	#elif defined ARCH_64
	printf("Instance of EIP: 0x%llx:\n", pc);
	#else
	#error Please specify an architecture {ARCH_32 or ARCH_64}
	#endif

	codeMap_fprint_address_info(analysis->code_map, pc, stdout);
	putchar('\n');

	for (return_code = assembly_get_first_pc(&(analysis->trace->assembly), &it, pc); return_code == 0; return_code = assembly_get_next_pc(&(analysis->trace->assembly), &it)){
		printf("\t- Found EIP in trace at offset: %u (last occurence is %u instruction(s) back)\n", it.instruction_index, it.instruction_index - last_index);
		last_index = it.instruction_index;
	}
	if (return_code < 0){
		log_err_m("assembly PC iterator returned code: %d", return_code);
	}
}

void analysis_trace_locate_opcode(struct analysis* analysis, char* arg){
	uint8_t* 			opcode;
	size_t 				opcode_length = 0;

	if (analysis->trace == NULL){
		log_err("trace is NULL");
		return;
	}

	opcode = (uint8_t*)readBuffer_raw(arg, strlen(arg), NULL, &opcode_length);
	if (opcode == NULL){
		log_err("unable to parse input hexa string");
		return;
	}

	assembly_locate_opcode(&(analysis->trace->assembly), opcode, opcode_length);

	free(opcode);
}

void analysis_trace_scan(struct analysis* analysis, char* arg){
	uint32_t 	filters = 0;
	size_t 		i;

	if (analysis->trace == NULL){
		log_err("trace is NULL");
		return;
	}

	if (arg != NULL){
		for (i = 0; i < strlen(arg); i++){
			switch(arg[i]){
				case 'E' : {filters |= ASSEMBLYSCAN_FILTER_BBL_EXEC; 	break;}
				case 'L' : {filters |= ASSEMBLYSCAN_FILTER_FUNC_LEAF; 	break;}
				case 'R' : {filters |= ASSEMBLYSCAN_FILTER_BBL_RATIO; 	break;}
				case 'S' : {filters |= ASSEMBLYSCAN_FILTER_BBL_SIZE; 	break;}
				default  : {log_err_m("incorrect filter defintion: %c. Correct filters are: {E, L, R, S}", arg[i]); break;}
			}
		}
	}

	assemblyScan_scan(&(analysis->trace->assembly), analysis->call_graph, filters);
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

	printer = multiColumnPrinter_create(stdout, 7, NULL, NULL, NULL);
	if (printer != NULL){
		multiColumnPrinter_set_column_size(printer, 0, 5);
		multiColumnPrinter_set_column_size(printer, 1, TRACE_TAG_LENGTH);
		multiColumnPrinter_set_column_size(printer, 2, 8);
		multiColumnPrinter_set_column_size(printer, 3, 16);
		multiColumnPrinter_set_column_size(printer, 4, IRDESCRIPTOR_MAX_LENGTH);
		multiColumnPrinter_set_column_size(printer, 5, 9);
		multiColumnPrinter_set_column_size(printer, 6, 9);

		multiColumnPrinter_set_column_type(printer, 0, MULTICOLUMN_TYPE_INT32);
		multiColumnPrinter_set_column_type(printer, 2, MULTICOLUMN_TYPE_INT32);
		multiColumnPrinter_set_column_type(printer, 3, MULTICOLUMN_TYPE_DOUBLE);

		multiColumnPrinter_set_title(printer, 0, "Index");
		multiColumnPrinter_set_title(printer, 1, "Tag");
		multiColumnPrinter_set_title(printer, 2, "Ins");
		multiColumnPrinter_set_title(printer, 3, "Percent (%)");
		multiColumnPrinter_set_title(printer, 4, "IR");
		multiColumnPrinter_set_title(printer, 5, "Mem Trace");
		multiColumnPrinter_set_title(printer, 6, "Synthesis");

		multiColumnPrinter_print_header(printer);

		for (i = 0; i < array_get_length(&(analysis->frag_array)); i++){
			fragment = (struct trace*)array_get(&(analysis->frag_array), i);
			percent = trace_opcode_percent(fragment, nb_opcode, opcode, nb_excluded_opcode, excluded_opcode);
			if (fragment->type != FRAGMENT_TRACE || fragment->trace_type.frag.ir == NULL){
				snprintf(ir_descriptor, IRDESCRIPTOR_MAX_LENGTH, "NULL");
			}
			else{
				snprintf(ir_descriptor, IRDESCRIPTOR_MAX_LENGTH, "%u node(s), %u edge(s)", fragment->trace_type.frag.ir->graph.nb_node, fragment->trace_type.frag.ir->graph.nb_edge);
			}

			multiColumnPrinter_print(printer, i, (fragment->type == FRAGMENT_TRACE) ? fragment->trace_type.frag.tag : "", trace_get_nb_instruction(fragment), percent*100, ir_descriptor, (fragment->mem_trace != NULL) ? "yes" : "no", (fragment->type == FRAGMENT_TRACE && fragment->trace_type.frag.synthesis_graph != NULL) ? "yes" : "no", NULL);
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
			if (fragment->type == FRAGMENT_TRACE){
				#ifdef VERBOSE
				log_info_m("setting tag value for frag %u: old tag: \"%s\", new tag: \"%s\"", index, fragment->trace_type.frag.tag, arg + i + 1);
				#endif

				strncpy(fragment->trace_type.frag.tag, arg + i + 1, TRACE_TAG_LENGTH);
			}
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
			if (fragment->type == FRAGMENT_TRACE){
				printf("Locating frag %u (tag: \"%s\")\n", i, fragment->trace_type.frag.tag);
			}
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

	snprintf(new_fragment.trace_type.frag.tag, TRACE_TAG_LENGTH, "concat %s", arg);

	if (array_add(&(analysis->frag_array), &new_fragment) < 0){
		log_err("unable to add traceFragment to array");
		trace_clean(&new_fragment);
	}
}

void analysis_frag_check(struct analysis* analysis, char* arg){
	apply_to_multiple_frags(analysis, trace_check, arg)
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
		if (fragment->type != FRAGMENT_TRACE){
			continue;
		}

		for (j = 0; j < array_get_length(&(fragment->trace_type.frag.result_array)); j++){
			result_print((struct result*)array_get(&(fragment->trace_type.frag.result_array), j));
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

	signature_buffer = (void**)malloc(sizeof(void*) * signatureCollection_get_nb_signature(&(analysis->code_signature_collection)));
	if (signature_buffer == NULL){
		log_err("unable to allocate memory");
		return;
	}

	if (arg != NULL && arg[0] >= 48 && arg[0] <= 57){
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
		signature_cursor = signatureCollection_node_get_codeSignature(node_cursor);
		if (arg == NULL){
			signature_buffer[nb_signature ++] = signature_cursor;
		}
		else{
			ptr = strstr(arg, signature_cursor->signature.name);
			if (ptr != NULL){
				if (strlen(ptr) == strlen(signature_cursor->signature.name) || ptr[strlen(signature_cursor->signature.name)] == ' '){
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
	apply_to_multiple_frags(analysis, trace_create_ir, arg)
}

void analysis_frag_create_compound_ir(struct analysis* analysis, char* arg){
	uint32_t 		index;
	uint32_t 		start;
	uint32_t 		stop;
	uint32_t 		i;
	uint32_t 		j;
	struct trace* 	selected_trace;
	struct array 	ir_component_array;

	if (arg != NULL){
		index = (uint32_t)atoi(arg);
		if (index < array_get_length(&(analysis->frag_array))){
			start = index;
			stop = index + 1;
		}
		else{
			log_err_m("incorrect fragment index %u (array size: %u)", index, array_get_length(&(analysis->frag_array)));
			return;
		}
	}
	else{
		start = 0;
		stop = array_get_length(&(analysis->frag_array));
	}

	for (i = start; i < stop; i++){
		selected_trace = (struct trace*)array_get(&(analysis->frag_array), i);

		if (array_init(&ir_component_array, sizeof(struct irComponent))){
			log_err("unable to init componentFrag array");
			break;
		}

		for (j = 0; j < array_get_length(&(analysis->frag_array)); j++){
			if (j == i){
				continue;
			}
			trace_search_irComponent(selected_trace, (struct trace*)array_get(&(analysis->frag_array), j), &ir_component_array);
		}

		trace_create_compound_ir(selected_trace, &ir_component_array);
		componentFrag_clean_array(&ir_component_array);
	}
}

void analysis_frag_printDot_ir(struct analysis* analysis, char* arg){
	apply_to_one_frag(analysis, trace_printDot_ir, arg)
}

void analysis_frag_normalize_ir(struct analysis* analysis, char* arg){
	apply_to_multiple_frags(analysis, trace_normalize_ir, arg)
}

void analysis_frag_print_aliasing_ir(struct analysis* analysis, char* arg){
	apply_to_multiple_frags(analysis, trace_print_aliasing_ir, arg)
}

void analysis_frag_simplify_concrete_ir(struct analysis* analysis, char* arg){
	apply_to_multiple_frags(analysis, trace_normalize_concrete_ir, arg)
}

/* ===================================================================== */
/* signature functions										             */
/* ===================================================================== */

void analysis_code_signature_search(struct analysis* analysis, char* arg){
	uint32_t 				index;
	uint32_t 				start;
	uint32_t 				stop;
	uint32_t 				i;
	struct trace* 			fragment;
	struct graphSearcher*	graph_searcher_buffer;
	uint32_t 				nb_graph_searcher;

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

	graph_searcher_buffer = (struct graphSearcher*)malloc(sizeof(struct graphSearcher) * (stop - start));
	if (graph_searcher_buffer == NULL){
		log_err("unable to allocate memory");
		return;
	}

	for (i = start, nb_graph_searcher = 0; i < stop; i++){
		fragment = (struct trace*)array_get(&(analysis->frag_array), i);
		if (fragment->type != FRAGMENT_TRACE){
			continue;
		}

		if (fragment->trace_type.frag.ir != NULL){
			graph_searcher_buffer[nb_graph_searcher].graph 				= &(fragment->trace_type.frag.ir->graph);
			graph_searcher_buffer[nb_graph_searcher].result_register 	= trace_register_code_signature_result;
			graph_searcher_buffer[nb_graph_searcher].result_push 		= trace_push_code_signature_result;
			graph_searcher_buffer[nb_graph_searcher].result_pop 		= trace_pop_code_signature_result;
			graph_searcher_buffer[nb_graph_searcher].arg 				= fragment;

			nb_graph_searcher ++;
		}
		else{
			log_err_m("the IR is NULL for fragment %u", i);
		}
	}

	signatureCollection_search(&(analysis->code_signature_collection), graph_searcher_buffer, nb_graph_searcher, irNode_get_label, irEdge_get_label);
	free(graph_searcher_buffer);
}

void analysis_code_signature_clean(struct analysis* analysis){
	struct trace* 	fragment;
	uint32_t 		i;
	uint32_t 		j;
	struct result* 	result;

	for (i = 0; i < array_get_length(&(analysis->frag_array)); i++){
		fragment = (struct trace*)array_get(&(analysis->frag_array), i);
		if (fragment->type != FRAGMENT_TRACE){
			continue;
		}

		if (array_get_length(&(fragment->trace_type.frag.result_array))){
			for (j = 0; j < array_get_length(&(fragment->trace_type.frag.result_array)); j++){
				result = (struct result*)array_get(&(fragment->trace_type.frag.result_array), j);
				if (result->state == RESULTSTATE_PUSH && fragment->trace_type.frag.ir != NULL){
					log_warn_m("deleting IR of fragment \"%s\" because it depends on signature \"%s\"", fragment->trace_type.frag.tag, result->code_signature->signature.name);
					ir_delete(fragment->trace_type.frag.ir)
					fragment->trace_type.frag.ir = NULL;
				}
				result_clean(result)
			}
			array_empty(&(fragment->trace_type.frag.result_array));
		}
	}

	signatureCollection_clean(&(analysis->code_signature_collection));
}

void analysis_mode_signature_search(struct analysis* analysis, char* arg){
	uint32_t 				index;
	uint32_t 				start;
	uint32_t 				stop;
	uint32_t 				i;
	struct trace* 			fragment;
	struct graphSearcher*	graph_searcher_buffer;
	uint32_t 				nb_graph_searcher;

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

	graph_searcher_buffer = (struct graphSearcher*)malloc(sizeof(struct graphSearcher) * (stop - start));
	if (graph_searcher_buffer == NULL){
		log_err("unable to allocate memory");
		return;
	}

	for (i = start, nb_graph_searcher = 0; i < stop; i++){
		fragment = (struct trace*)array_get(&(analysis->frag_array), i);
		if (fragment->type != FRAGMENT_TRACE){
			continue;
		}

		if (fragment->trace_type.frag.synthesis_graph != NULL){
			graph_searcher_buffer[nb_graph_searcher].graph 				= &(fragment->trace_type.frag.synthesis_graph->graph);
			graph_searcher_buffer[nb_graph_searcher].result_register 	= NULL;
			graph_searcher_buffer[nb_graph_searcher].result_push 		= NULL;
			graph_searcher_buffer[nb_graph_searcher].result_pop 		= NULL;
			graph_searcher_buffer[nb_graph_searcher].arg 				= NULL;

			nb_graph_searcher ++;
		}
		else{
			log_err_m("the synthesis graph is NULL for fragment %u", i);
		}
	}

	signatureCollection_search(&(analysis->mode_signature_collection), graph_searcher_buffer, nb_graph_searcher, synthesisGraphNode_get_label, synthesisGraphEdge_get_label);
	free(graph_searcher_buffer);
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
		callGraph_check(analysis->call_graph, &(analysis->trace->assembly), analysis->code_map);
	}
}

void analysis_call_export(struct analysis* analysis, char* arg){
	struct cm_routine* 			rtn;
	struct instructionIterator 	it;
	int32_t 					return_code;

	if (analysis->call_graph == NULL){
		log_err("callGraph is NULL cannot export");
		return;
	}

	if (arg[0] >= 0x30 && arg[0] <= 0x39){
		if (callGraph_export_node_inclusive(analysis->call_graph, callGraph_get_index(analysis->call_graph, atoi(arg)), analysis->trace, &(analysis->frag_array))){
			log_err_m("unable to export callGraph @ %u", atoi(arg));
		}
		return;
	}

	if (analysis->code_map == NULL){
		log_err("codeMap is NULL, unable to convert symbol to address");
		return;
	}

	for (rtn = codeMap_search_symbol(analysis->code_map, NULL, arg); rtn; rtn = codeMap_search_symbol(analysis->code_map, rtn, arg)){
		for (return_code = assembly_get_first_pc(&(analysis->trace->assembly), &it, rtn->address_start); return_code == 0; return_code = assembly_get_next_pc(&(analysis->trace->assembly), &it)){
			if (callGraph_export_node_inclusive(analysis->call_graph, callGraph_get_index(analysis->call_graph, it.instruction_index), analysis->trace, &(analysis->frag_array))){
				log_err_m("unable to export callGraph @ %u", it.instruction_index);
			}
		}
		if (return_code < 0){
			log_err_m("assembly PC iterator returned code: %d", return_code);
		}
	}
}

void analysis_call_print_stack(struct analysis* analysis, char* arg){
	if (analysis->call_graph == NULL){
		log_err("callGraph is NULL cannot print stack");
	}
	else{
		callGraph_fprint_stack(analysis->call_graph, callGraph_get_index(analysis->call_graph, atoi(arg)), stdout);
	}
}

/* ===================================================================== */
/* synthesis graph functions								             */
/* ===================================================================== */

void analysis_synthesis_create(struct analysis* analysis, char* arg){
	apply_to_multiple_frags(analysis, trace_create_synthesis, arg)
}

void analysis_synthesis_printDot(struct analysis* analysis, char* arg){
	uint32_t 	index;
	char* 		name = NULL;
	size_t 		offset;

	if (arg == NULL){
		if (array_get_length(&(analysis->frag_array)) < 2){
			index = 0;
		}
		else{
			log_err_m("%u fragments available, please specify fragment index", array_get_length(&(analysis->frag_array)));
			return;
		}

	}
	else{
		index = (uint32_t)atoi(arg);
		offset = strspn(arg, " 0123456789");
		if (arg[offset] != '\0'){
			name = arg + offset;
		}
	}

	if (index < array_get_length(&(analysis->frag_array))){
		trace_printDot_synthesis((struct trace*)array_get(&(analysis->frag_array), index), name);
	}
	else{
		log_err_m("incorrect fragment index %u (array size: %u)", index, array_get_length(&(analysis->frag_array)));
	}
}