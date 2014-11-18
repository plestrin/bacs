#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "analysis.h"
#include "inputParser.h"
#include "printBuffer.h"
#include "readBuffer.h"
#include "ir.h"
#include "signatureReader.h"
#include "cmReaderJSON.h"
#include "multiColumn.h"


#define ADD_CMD_TO_INPUT_PARSER(parser, cmd, cmd_desc, arg_desc, type, arg, func)										\
	{																													\
		if (inputParser_add_cmd((parser), (cmd), (cmd_desc), (arg_desc), (type), (arg), (void(*)(void))(func))){		\
			printf("ERROR: in %s, unable to add cmd: \"%s\" to inputParser\n", __func__, (cmd));						\
		}																												\
	}

int main(int argc, char** argv){
	struct analysis* 			analysis = NULL;
	struct inputParser* 		parser = NULL;

	parser = inputParser_create();
	if (parser == NULL){
		printf("ERROR: in %s, unable to create inputParser\n", __func__);
		goto exit;
	}
	
	analysis = analysis_create();
	if (analysis == NULL){
		printf("ERROR: in %s, unable to create the analysis structure\n", __func__);
		goto exit;
	}

	/* trace specific commands */
	ADD_CMD_TO_INPUT_PARSER(parser, "load trace", 				"Load a trace in the analysis engine", 			"Trace directory", 				INPUTPARSER_CMD_TYPE_ARG, 		analysis, 								analysis_trace_load)
	ADD_CMD_TO_INPUT_PARSER(parser, "load elf", 				"Load an ELF file in the analysis engine", 		"ELF file", 					INPUTPARSER_CMD_TYPE_ARG, 		analysis, 								analysis_trace_load_elf)
	ADD_CMD_TO_INPUT_PARSER(parser, "print trace", 				"Print trace's instructions (assembly code)", 	"Index or range", 				INPUTPARSER_CMD_TYPE_OPT_ARG, 	analysis, 								analysis_trace_print)
	ADD_CMD_TO_INPUT_PARSER(parser, "check trace", 				"Check the current trace for format errors", 	NULL, 							INPUTPARSER_CMD_TYPE_NO_ARG, 	analysis, 								analysis_trace_check)
	ADD_CMD_TO_INPUT_PARSER(parser, "check codeMap", 			"Perform basic checks on the codeMap address", 	NULL, 							INPUTPARSER_CMD_TYPE_NO_ARG, 	analysis, 								analysis_trace_check_codeMap)
	ADD_CMD_TO_INPUT_PARSER(parser, "print codeMap", 			"Print the codeMap", 							"Specific filter", 				INPUTPARSER_CMD_TYPE_OPT_ARG, 	analysis, 								analysis_trace_print_codeMap)
	ADD_CMD_TO_INPUT_PARSER(parser, "export trace", 			"Export a trace segment as a traceFragment", 	"Range", 						INPUTPARSER_CMD_TYPE_ARG, 		analysis, 								analysis_trace_export)
	ADD_CMD_TO_INPUT_PARSER(parser, "locate pc", 				"Return trace offset that match a given pc", 	"PC (hexa)", 					INPUTPARSER_CMD_TYPE_ARG, 		analysis, 								analysis_trace_locate_pc)
	ADD_CMD_TO_INPUT_PARSER(parser, "clean trace", 				"Delete the current trace", 					NULL, 							INPUTPARSER_CMD_TYPE_NO_ARG, 	analysis, 								analysis_trace_delete)

	/* traceFragement specific commands */
	ADD_CMD_TO_INPUT_PARSER(parser, "print frag", 				"Print traceFragment (assembly or list)", 		"Frag index", 					INPUTPARSER_CMD_TYPE_OPT_ARG, 	analysis, 								analysis_frag_print)
	ADD_CMD_TO_INPUT_PARSER(parser, "set frag tag", 			"Set tag value for a given traceFragment", 		"Frag index and tag value", 	INPUTPARSER_CMD_TYPE_ARG, 		analysis, 								analysis_frag_set_tag)
	ADD_CMD_TO_INPUT_PARSER(parser, "locate frag", 				"Locate traceFragement in the codeMap", 		"Frag index", 					INPUTPARSER_CMD_TYPE_OPT_ARG, 	analysis, 								analysis_frag_locate)
	ADD_CMD_TO_INPUT_PARSER(parser, "concat frag", 				"Concat two or more traceFragments", 			"Frag indexes", 				INPUTPARSER_CMD_TYPE_ARG, 		analysis, 								analysis_frag_concat)
	ADD_CMD_TO_INPUT_PARSER(parser, "clean frag", 				"Clean the traceFragment array", 				NULL, 							INPUTPARSER_CMD_TYPE_NO_ARG, 	analysis, 								analysis_frag_clean)

	/* ir specific commands */
	ADD_CMD_TO_INPUT_PARSER(parser, "create ir", 				"Create an IR directly from a traceFragment", 	"Frag index", 					INPUTPARSER_CMD_TYPE_OPT_ARG, 	analysis, 								analysis_frag_create_ir)
	ADD_CMD_TO_INPUT_PARSER(parser, "printDot ir", 				"Write the IR to a file in the dot format", 	"Frag index", 					INPUTPARSER_CMD_TYPE_ARG, 		analysis, 								analysis_frag_printDot_ir)
	ADD_CMD_TO_INPUT_PARSER(parser, "normalize ir", 			"Normalize the IR (usefull for signature)", 	"Frag index", 					INPUTPARSER_CMD_TYPE_OPT_ARG, 	analysis, 								analysis_frag_normalize_ir)

	/* code signature specific commands */
	ADD_CMD_TO_INPUT_PARSER(parser, "load code signature", 		"Load code signature from a file", 				"File path", 					INPUTPARSER_CMD_TYPE_ARG, 		&(analysis->code_signature_collection), codeSignatureReader_parse)
	ADD_CMD_TO_INPUT_PARSER(parser, "search code signature", 	"Search code signature for a given IR", 		"Frag index", 					INPUTPARSER_CMD_TYPE_OPT_ARG, 	analysis, 								analysis_code_signature_search)
	ADD_CMD_TO_INPUT_PARSER(parser, "printDot code signature", 	"Print every code signature in dot format", 	NULL, 							INPUTPARSER_CMD_TYPE_NO_ARG, 	&(analysis->code_signature_collection), codeSignature_printDot_collection)
	ADD_CMD_TO_INPUT_PARSER(parser, "clean code signature", 	"Remove every code signature", 					NULL, 							INPUTPARSER_CMD_TYPE_NO_ARG, 	&(analysis->code_signature_collection), codeSignature_clean_collection)

	/* callGraph specific commands */
	ADD_CMD_TO_INPUT_PARSER(parser, "create callGraph", 		"Create a call graph", 							"Specify OS", 					INPUTPARSER_CMD_TYPE_ARG, 		analysis, 								analysis_call_create)
	ADD_CMD_TO_INPUT_PARSER(parser, "printDot callGraph", 		"Write the call graph in the dot format", 		NULL, 							INPUTPARSER_CMD_TYPE_NO_ARG, 	analysis, 								analysis_call_printDot)
	ADD_CMD_TO_INPUT_PARSER(parser, "export callGraph", 		"Export callGraph's routine as traceFragments", "Routine name", 				INPUTPARSER_CMD_TYPE_OPT_ARG, 	analysis, 								analysis_call_export)

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
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
		return NULL;
	}

	codeSignature_init_collection(&(analysis->code_signature_collection));

	if (array_init(&(analysis->frag_array), sizeof(struct trace))){
		printf("ERROR: in %s, unable to init traceFragment array\n", __func__);
		codeSignature_clean_collection(&(analysis->code_signature_collection));
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

	codeSignature_clean_collection(&(analysis->code_signature_collection));

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
		printf("WARNING: in %s, deleting previous trace\n", __func__);
		trace_delete(analysis->trace);

		if (analysis->call_graph != NULL){
			callGraph_delete(analysis->call_graph);
			analysis->call_graph = NULL;
		}
	}

	if (analysis->code_map != NULL){
		printf("WARNING: in %s, deleting previous codeMap\n", __func__);
		codeMap_delete(analysis->code_map);
	}

	analysis->trace = trace_load(arg);
	if (analysis->trace == NULL){
		printf("ERROR: in %s, unable to create trace\n", __func__);
	}

	analysis->code_map = cmReaderJSON_parse(arg);
	if (analysis->code_map == NULL){
		printf("ERROR: in %s, unable to create codeMap\n", __func__);
	}
}

void analysis_trace_load_elf(struct analysis* analysis, char* arg){
	if (analysis->trace != NULL){
		printf("WARNING: in %s, deleting previous trace\n", __func__);
		trace_delete(analysis->trace);

		if (analysis->call_graph != NULL){
			callGraph_delete(analysis->call_graph);
			analysis->call_graph = NULL;
		}
	}

	if (analysis->code_map != NULL){
		printf("WARNING: in %s, deleting previous codeMap\n", __func__);
		codeMap_delete(analysis->code_map);
		analysis->code_map = NULL;
	}

	analysis->trace = trace_load_elf(arg);
	if (analysis->trace == NULL){
		printf("ERROR: in %s, unable to create trace\n", __func__);
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
		printf("ERROR: in %s, trace is NULL\n", __func__);
	}
}

void analysis_trace_check(struct analysis* analysis){
	if (analysis->trace != NULL){
		trace_check(analysis->trace);
	}
	else{
		printf("ERROR: in %s, trace is NULL\n", __func__);
	}
}

void analysis_trace_check_codeMap(struct analysis* analysis){
	if (analysis->code_map != NULL){
		codeMap_check_address(analysis->code_map);
	}
	else{
		printf("ERROR: in %s, codeMap is NULL\n", __func__);
	}
}

void analysis_trace_print_codeMap(struct analysis* analysis, char* arg){
	if (analysis->code_map != NULL){
		codeMap_print(analysis->code_map, arg);
	}
	else{
		printf("ERROR: in %s, codeMap is NULL\n", __func__);
	}
}

void analysis_trace_export(struct analysis* analysis, char* arg){
	uint32_t 		start = 0;
	uint32_t 		stop = 0;
	struct trace 	fragment;

	if (analysis->trace != NULL){
		trace_init(&fragment);
		inputParser_extract_index(arg, &start, &stop);
		if (trace_extract_segment(analysis->trace, &fragment, start, stop - start)){
			printf("ERROR: in %s, unable to extract traceFragment\n", __func__);
		}
		else{
			snprintf(fragment.tag, TRACE_TAG_LENGTH, "trace [%u:%u]", start, stop);

			if (array_add(&(analysis->frag_array), &fragment) < 0){
				printf("ERROR: in %s, unable to add traceFragment to array\n", __func__);
				trace_clean(&fragment);
			}
		}
	}
	else{
		printf("ERROR: in %s, trace is NULL\n", __func__);
	}
}

void analysis_trace_locate_pc(struct analysis* analysis, char* arg){
	ADDRESS 					pc;
	struct instructionIterator 	it;

	pc = strtoul((const char*)arg, NULL, 16);

	#if defined ARCH_32
	printf("Instance of EIP: 0x%08x:\n", pc);
	#elif defined ARCH_64
	printf("Instance of EIP: 0x%llx:\n", pc);
	#else
	#error Please specify an architecture {ARCH_32 or ARCH_64}
	#endif

	if (assembly_get_instruction(&(analysis->trace->assembly), &it, 0)){
		printf("ERROR: in %s, unable to fetch first instruction from the assembly\n", __func__);
		return;
	}

	for (;;){
		if (it.instruction_address == pc){
			printf("\t- Found EIP in trace at offset: %u\n", it.instruction_index);
		}

		if (instructionIterator_get_instruction_index(&it) ==  assembly_get_nb_instruction(&(analysis->trace->assembly)) - 1){
			break;
		}
		else{
			if (assembly_get_next_instruction(&(analysis->trace->assembly), &it)){
				printf("ERROR: in %s, unable to fetch next instruction from the assembly\n", __func__);
				return;
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
		printf("ERROR: in %s, trace is NULL\n", __func__);
	}

	if (analysis->code_map != NULL){
		codeMap_delete(analysis->code_map);
		analysis->code_map = NULL;
	}
	else{
		printf("ERROR: in %s, codeMap is NULL\n", __func__);
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

	if (arg != NULL){
		index = (uint32_t)atoi(arg);
		
		if (index < array_get_length(&(analysis->frag_array))){
			fragment = (struct trace*)array_get(&(analysis->frag_array), index);
			trace_print(fragment, 0, trace_get_nb_instruction(fragment));
			return;
		}
		else{
			printf("ERROR: in %s, incorrect index value %u (array size :%u)\n", __func__, index, array_get_length(&(analysis->frag_array)));
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

	printer = multiColumnPrinter_create(stdout, 4, NULL, NULL, NULL);
	if (printer != NULL){
		multiColumnPrinter_set_column_size(printer, 0, 5);
		multiColumnPrinter_set_column_size(printer, 1, TRACE_TAG_LENGTH);
		multiColumnPrinter_set_column_size(printer, 2, 8);
		multiColumnPrinter_set_column_size(printer, 3, 16);

		multiColumnPrinter_set_column_type(printer, 0, MULTICOLUMN_TYPE_INT32);
		multiColumnPrinter_set_column_type(printer, 2, MULTICOLUMN_TYPE_INT32);
		multiColumnPrinter_set_column_type(printer, 3, MULTICOLUMN_TYPE_DOUBLE);

		multiColumnPrinter_set_title(printer, 0, (char*)"Index");
		multiColumnPrinter_set_title(printer, 1, (char*)"Tag");
		multiColumnPrinter_set_title(printer, 2, (char*)"# Ins");
		multiColumnPrinter_set_title(printer, 3, (char*)"Percent (%%)");

		multiColumnPrinter_print_header(printer);

		for (i = 0; i < array_get_length(&(analysis->frag_array)); i++){
			fragment = (struct trace*)array_get(&(analysis->frag_array), i);
			percent = trace_opcode_percent(fragment, nb_opcode, opcode, nb_excluded_opcode, excluded_opcode);
			multiColumnPrinter_print(printer, i, fragment->tag, trace_get_nb_instruction(fragment), percent*100, NULL);
		}

		multiColumnPrinter_delete(printer);
	}
	else{
		printf("ERROR: in %s, unable to create multiColumnPrinter\n", __func__);
	}
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
			printf("Setting tag value for frag %u: old tag: \"%s\", new tag: \"%s\"\n", index, fragment->tag, arg + i + 1);
			#endif

			strncpy(fragment->tag, arg + i + 1, TRACE_TAG_LENGTH);
		}
		else{
			printf("ERROR: in %s, incorrect index value %u (array size :%u)\n", __func__, index, array_get_length(&(analysis->frag_array)));
		}
	}
	else{
		printf("ERROR: in %s, the index and the tag must separated by a space char\n", __func__);
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
			printf("ERROR: in %s, incorrect index value %u (array size :%u)\n", __func__, index, array_get_length(&(analysis->frag_array)));
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
		printf("ERROR: in %s, codeMap is NULL, unable to locate\n", __func__);
	}
}

void analysis_frag_concat(struct analysis* analysis, char* arg){
	uint32_t 		i;
	uint32_t 		nb_index;
	uint8_t 		start_index;
	uint32_t 		index;
	struct trace** 	trace_src_buffer;
	struct trace 	new_fragment;
	int32_t 		tag_offset;

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
		printf("ERROR: in %s, at last two indexes must be specicied (but get %u)\n", __func__, nb_index);
		return;
	}

	trace_src_buffer = (struct trace**)alloca(sizeof(struct trace*) * nb_index);
	trace_init(&new_fragment);
	tag_offset = snprintf(new_fragment.tag, TRACE_TAG_LENGTH, "concat ");

	for (i = 0, nb_index = 0, start_index = 0; i < strlen(arg); i++){
		if (arg[i] >= 48 && arg[i] <= 57){
			if (start_index == 0){
				index = atoi(arg + i);
				if (index < array_get_length(&(analysis->frag_array))){
					trace_src_buffer[nb_index ++] = (struct trace*)array_get(&(analysis->frag_array), index);
					if (tag_offset < TRACE_TAG_LENGTH){
						tag_offset += snprintf(new_fragment.tag + tag_offset, TRACE_TAG_LENGTH - tag_offset, "%u ", index);
					}
				}
				else{
					printf("ERROR: in %s, the index specified @ %u is incorrect (array size: %u)\n", __func__, nb_index, array_get_length(&(analysis->frag_array)));
				}

				start_index = 1;
			}
		}
		else{
			start_index = 0;
		}
	}

	if (nb_index < 2){
		printf("ERROR: in %s, at last two valid indexes must be specicied (but get %u)\n", __func__, nb_index);
		return;
	}

	if (trace_concat(trace_src_buffer, nb_index, &new_fragment)){
		printf("ERROR: in %s, unable to concat the given frags\n", __func__);
	}
	else{
		if (array_add(&(analysis->frag_array), &new_fragment) < 0){
			printf("ERROR: in %s, unable to add traceFragment to array\n", __func__);
			trace_clean(&new_fragment);
		}
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
	uint32_t 	index;
	uint32_t 	start;
	uint32_t 	stop;
	uint32_t 	i;

	if (arg != NULL){
		index = (uint32_t)atoi(arg);
		if (index < array_get_length(&(analysis->frag_array))){
			start = index;
			stop = index + 1;
		}
		else{
			printf("ERROR: in %s, incorrect index value %u (array size :%u)\n", __func__, index, array_get_length(&(analysis->frag_array)));
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
		printf("ERROR: in %s, incorrect index value %u (array size :%u)\n", __func__, index, array_get_length(&(analysis->frag_array)));
	}
}

void analysis_frag_normalize_ir(struct analysis* analysis, char* arg){
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
			printf("ERROR: in %s, incorrect index value %u (array size :%u)\n", __func__, index, array_get_length(&(analysis->frag_array)));
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
			ir_normalize(fragment->ir);
		}
		else{
			printf("ERROR: in %s, the IR is NULL for the current fragment\n", __func__);
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
	struct ir**		ir_buffer;
	uint32_t 		nb_ir;

	if (arg != NULL){
		index = (uint32_t)atoi(arg);
		if (index < array_get_length(&(analysis->frag_array))){
			start = index;
			stop = index + 1;
		}
		else{
			printf("ERROR: in %s, incorrect index value %u (array size :%u)\n", __func__, index, array_get_length(&(analysis->frag_array)));
			return;
		}
	}
	else{
		start = 0;
		stop = array_get_length(&(analysis->frag_array));
	}

	ir_buffer = (struct ir**)malloc(sizeof(struct ir*) * (stop - start));
	if (ir_buffer == NULL){
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
		return;
	}

	for (i = start, nb_ir = 0; i < stop; i++){
		fragment = (struct trace*)array_get(&(analysis->frag_array), i);
		if (fragment->ir != NULL){
			ir_buffer[nb_ir ++] = fragment->ir;
		}
		else{
			printf("ERROR: in %s, the IR is NULL for fragment %u\n", __func__, i);
		}
	}

	codeSignature_search_collection(&(analysis->code_signature_collection), ir_buffer, nb_ir);
	free(ir_buffer);
}


/* ===================================================================== */
/* call graph functions						    	                     */
/* ===================================================================== */

void analysis_call_create(struct analysis* analysis, char* arg){
	if (analysis->call_graph != NULL){
		printf("WARNING: in %s, deleting previous callGraph\n", __func__);
		callGraph_delete(analysis->call_graph);
		analysis->call_graph = NULL;
	}

	if (analysis->trace == NULL){
		printf("ERROR: %s, trace is NULL\n", __func__);
	}
	else{
		analysis->call_graph = callGraph_create(analysis->trace);
		if (analysis->call_graph == NULL){
			printf("ERROR: in %s, unable to create callGraph\n", __func__);
		}
		else if (analysis->code_map != NULL){
			if (!strcmp(arg, "LINUX")){
				callGraph_locate_in_codeMap_linux(analysis->call_graph, analysis->trace, analysis->code_map);
			}
			else if (!strcmp(arg, "WINDOWS")){
				callGraph_locate_in_codeMap_windows(analysis->call_graph, analysis->trace, analysis->code_map);
			}
			else{
				printf("Expected os specifier: {LINUX, WINDOWS}\n");
			}
		}
	}
}

void analysis_call_printDot(struct analysis* analysis){
	if (analysis->call_graph == NULL){
		printf("ERROR: in %s, callGraph is NULL cannot print\n", __func__);
	}
	else{
		callGraph_printDot(analysis->call_graph);
	}
}

void analysis_call_export(struct analysis* analysis, char* arg){
	if (analysis->call_graph == NULL){
		printf("ERROR: in %s, callGraph is NULL cannot export\n", __func__);
	}
	else{
		if (callGraph_export_inclusive(analysis->call_graph, analysis->trace, &(analysis->frag_array), arg)){
			printf("ERROR: in %s, unable to export callGraph\n", __func__);
		}
	}
}