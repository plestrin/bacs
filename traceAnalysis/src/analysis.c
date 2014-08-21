#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include "analysis.h"
#include "inputParser.h"
#include "instruction.h"
#include "argument.h"
#include "argSet.h"
#include "simpleTraceStat.h"
#include "printBuffer.h"
#include "readBuffer.h"
#include "ir.h"

#define ADD_CMD_TO_INPUT_PARSER(parser, cmd, cmd_desc, arg_desc, type, arg, func)									\
	{																									\
		if (inputParser_add_cmd((parser), (cmd), (cmd_desc), (arg_desc), (type), (arg), (void(*)(void))(func))){		\
			printf("ERROR: in %s, unable to add cmd: \"%s\" to inputParser\n", __func__, (cmd));		\
		}																								\
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

	/* ioChecker specific commands*/
	ADD_CMD_TO_INPUT_PARSER(parser, "load ioChecker", 			"Load primitive reference from a file", 		"File path", 					INPUTPARSER_CMD_TYPE_ARG, 		&(analysis->io_checker), 				ioChecker_load)
	ADD_CMD_TO_INPUT_PARSER(parser, "print ioChecker", 			"Display the ioChecker structure", 				NULL, 							INPUTPARSER_CMD_TYPE_NO_ARG, 	&(analysis->io_checker), 				ioChecker_print)
	ADD_CMD_TO_INPUT_PARSER(parser, "clean ioChecker", 			"Remove every primitive reference", 			NULL, 							INPUTPARSER_CMD_TYPE_NO_ARG, 	&(analysis->io_checker), 				ioChecker_empty)

	/* cstChecker specific commands*/
	ADD_CMD_TO_INPUT_PARSER(parser, "load cstChecker", 			"Load constant from a file", 					"File path", 					INPUTPARSER_CMD_TYPE_ARG, 		&(analysis->cst_checker), 				cstChecker_load)
	ADD_CMD_TO_INPUT_PARSER(parser, "print cstChecker", 		"Display the cstChecker structure", 			NULL, 							INPUTPARSER_CMD_TYPE_NO_ARG, 	&(analysis->cst_checker), 				cstChecker_print)
	ADD_CMD_TO_INPUT_PARSER(parser, "clean cstChecker", 		"Remove every constant", 						NULL, 							INPUTPARSER_CMD_TYPE_NO_ARG, 	&(analysis->cst_checker), 				cstChecker_empty)

	/* trace specific commands */
	ADD_CMD_TO_INPUT_PARSER(parser, "load trace", 				"Load a trace in the analysis engine", 			"Trace directory", 				INPUTPARSER_CMD_TYPE_ARG, 		analysis, 								analysis_trace_load)
	ADD_CMD_TO_INPUT_PARSER(parser, "print trace", 				"Print trace's instructions (trace format)", 	"Index or range", 				INPUTPARSER_CMD_TYPE_OPT_ARG, 	analysis, 								analysis_trace_print)
	ADD_CMD_TO_INPUT_PARSER(parser, "print asm", 				"Print trace's instructions (assembly code)", 	"Index or range", 				INPUTPARSER_CMD_TYPE_OPT_ARG, 	analysis, 								analysis_trace_print_asm)
	ADD_CMD_TO_INPUT_PARSER(parser, "check trace", 				"Check the current trace for format errors", 	NULL, 							INPUTPARSER_CMD_TYPE_NO_ARG, 	analysis, 								analysis_trace_check)
	ADD_CMD_TO_INPUT_PARSER(parser, "check codeMap", 			"Perform basic checks on the codeMap address", 	NULL, 							INPUTPARSER_CMD_TYPE_NO_ARG, 	analysis, 								analysis_trace_check_codeMap)
	ADD_CMD_TO_INPUT_PARSER(parser, "print codeMap", 			"Print the codeMap", 							"Specific filter", 				INPUTPARSER_CMD_TYPE_OPT_ARG, 	analysis, 								analysis_trace_print_codeMap)
	ADD_CMD_TO_INPUT_PARSER(parser, "search constant", 			"Search constant from the cstChecker in the trace", NULL, 						INPUTPARSER_CMD_TYPE_NO_ARG, 	analysis, 								analysis_trace_search_constant)
	ADD_CMD_TO_INPUT_PARSER(parser, "export trace", 			"Export a trace segment as a traceFragment", 	"Range", 						INPUTPARSER_CMD_TYPE_ARG, 		analysis, 								analysis_trace_export)
	ADD_CMD_TO_INPUT_PARSER(parser, "locate pc", 				"Return trace offset that match a given pc", 	"PC (hexa)", 					INPUTPARSER_CMD_TYPE_ARG, 		analysis, 								analysis_trace_locate_pc)
	ADD_CMD_TO_INPUT_PARSER(parser, "clean trace", 				"Delete the current trace", 					NULL, 							INPUTPARSER_CMD_TYPE_NO_ARG, 	analysis, 								analysis_trace_delete)

	/* loop specific commands */
	ADD_CMD_TO_INPUT_PARSER(parser, "create loop", 				"Create a loopEngine and parse the trace", 		"Creation method", 				INPUTPARSER_CMD_TYPE_ARG, 		analysis, 								analysis_loop_create)
	ADD_CMD_TO_INPUT_PARSER(parser, "remove redundant loop", 	"Remove the redundant loops", 					"Removing method", 				INPUTPARSER_CMD_TYPE_ARG, 		analysis, 								analysis_loop_remove_redundant)
	ADD_CMD_TO_INPUT_PARSER(parser, "print loop", 				"Print the loops contained in the loopEngine", 	NULL, 							INPUTPARSER_CMD_TYPE_NO_ARG, 	analysis, 								analysis_loop_print)
	ADD_CMD_TO_INPUT_PARSER(parser, "export loop", 				"Export loop(s) to traceFragment array", 		"Export method & loop index", 	INPUTPARSER_CMD_TYPE_ARG, 		analysis, 								analysis_loop_export)
	ADD_CMD_TO_INPUT_PARSER(parser, "delete loop", 				"Delete the loopEngine", 						NULL, 							INPUTPARSER_CMD_TYPE_NO_ARG, 	analysis, 								analysis_loop_delete)

	/* traceFragement specific commands */
	ADD_CMD_TO_INPUT_PARSER(parser, "print frag stat", 			"Print stats about the traceFragments", 		"Frag index", 					INPUTPARSER_CMD_TYPE_OPT_ARG, 	analysis, 								analysis_frag_print_stat)
	ADD_CMD_TO_INPUT_PARSER(parser, "print frag ins", 			"Print instructions of a given traceFragment",  "Frag index", 					INPUTPARSER_CMD_TYPE_ARG, 		analysis, 								analysis_frag_print_ins)
	ADD_CMD_TO_INPUT_PARSER(parser, "print frag asm", 			"Print assembly code of a given traceFragment", "Frag index", 					INPUTPARSER_CMD_TYPE_ARG, 		analysis, 								analysis_frag_print_asm)
	ADD_CMD_TO_INPUT_PARSER(parser, "print frag percent", 		"Print some stat about instructions frequency", NULL, 							INPUTPARSER_CMD_TYPE_NO_ARG, 	analysis, 								analysis_frag_print_percent)
	ADD_CMD_TO_INPUT_PARSER(parser, "print frag register", 		"Print register access", 						"Frag index", 					INPUTPARSER_CMD_TYPE_ARG, 		analysis, 								analysis_frag_print_register)
	ADD_CMD_TO_INPUT_PARSER(parser, "print frag memory", 		"Print memory access", 							"Frag index", 					INPUTPARSER_CMD_TYPE_ARG, 		analysis, 								analysis_frag_print_memory)
	ADD_CMD_TO_INPUT_PARSER(parser, "set frag tag", 			"Set tag value for a given traceFragment", 		"Frag index and tag value", 	INPUTPARSER_CMD_TYPE_ARG, 		analysis, 								analysis_frag_set_tag)
	ADD_CMD_TO_INPUT_PARSER(parser, "locate frag", 				"Locate traceFragement in the codeMap", 		"Frag index", 					INPUTPARSER_CMD_TYPE_OPT_ARG, 	analysis, 								analysis_frag_locate)
	ADD_CMD_TO_INPUT_PARSER(parser, "extract frag arg", 		"Extract input and output argument(s)", 		"Extraction method & frag index", INPUTPARSER_CMD_TYPE_ARG, 	analysis, 								analysis_frag_extract_arg)
	ADD_CMD_TO_INPUT_PARSER(parser, "clean frag", 				"Clean the traceFragment array", 				NULL, 							INPUTPARSER_CMD_TYPE_NO_ARG, 	analysis, 								analysis_frag_clean)

	/* ir specific commands */
	ADD_CMD_TO_INPUT_PARSER(parser, "create ir", 				"Create an IR directly from a traceFragment", 	"Frag index", 					INPUTPARSER_CMD_TYPE_ARG, 		analysis, 								analysis_frag_create_ir)
	ADD_CMD_TO_INPUT_PARSER(parser, "printDot ir", 				"Write the IR to a file in the dot format", 	"Frag index", 					INPUTPARSER_CMD_TYPE_ARG, 		analysis, 								analysis_frag_printDot_ir)
	ADD_CMD_TO_INPUT_PARSER(parser, "print frag io", 			"Print IR input and output", 					"Frag index", 					INPUTPARSER_CMD_TYPE_OPT_ARG, 	analysis, 								analysis_frag_print_io)
	ADD_CMD_TO_INPUT_PARSER(parser, "extract arg ir", 			"Extract argument from the IR representation", 	"Frag index", 					INPUTPARSER_CMD_TYPE_OPT_ARG, 	analysis, 								analysis_frag_extract_arg_ir)

	/* argument specific commands */
	ADD_CMD_TO_INPUT_PARSER(parser, "print arg", 				"Print arguments from the argSet array", 		"ArgSet index", 				INPUTPARSER_CMD_TYPE_OPT_ARG, 	analysis, 								analysis_arg_print)
	ADD_CMD_TO_INPUT_PARSER(parser, "set arg tag", 				"Set tag value for a given argSet",				"ArgSet index & tag value", 	INPUTPARSER_CMD_TYPE_ARG, 		analysis, 								analysis_arg_set_tag)
	ADD_CMD_TO_INPUT_PARSER(parser, "search arg", 				"Search every elements in the argSet array", 	"ArgSet index", 				INPUTPARSER_CMD_TYPE_OPT_ARG, 	analysis, 								analysis_arg_search)
	ADD_CMD_TO_INPUT_PARSER(parser, "seek arg", 				"Seek for a argBuffer in the argSet array", 	"Binary buffer (raw format)", 	INPUTPARSER_CMD_TYPE_ARG, 		analysis, 								analysis_arg_seek)
	ADD_CMD_TO_INPUT_PARSER(parser, "clean arg", 				"Clean the argSet array", 						NULL, 							INPUTPARSER_CMD_TYPE_NO_ARG, 	analysis, 								analysis_arg_clean)
	
	inputParser_exe(parser, argc - 1, argv + 1);

	exit:

	analysis_delete(analysis);
	inputParser_delete(parser);

	pthread_exit(NULL); /* due to dlopen */

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

	
	if (ioChecker_init(&(analysis->io_checker))){
		printf("ERROR: in %s, unable to init ioChecker\n", __func__);
		free(analysis);
		return NULL;
	}

	if (cstChecker_init(&(analysis->cst_checker))){
		printf("ERROR: in %s, unable to init cstChecker\n", __func__);
		ioChecker_clean(&(analysis->io_checker));
		free(analysis);
		return NULL;
	}

	if (array_init(&(analysis->frag_array), sizeof(struct traceFragment))){
		printf("ERROR: in %s, unable to init traceFragment array\n", __func__);
		cstChecker_clean(&(analysis->cst_checker));
		ioChecker_clean(&(analysis->io_checker));
		free(analysis);
		return NULL;
	}

	if (array_init(&(analysis->arg_array), sizeof(struct argSet))){
		printf("ERROR: in %s, unable to init argSet array\n", __func__);
		array_clean(&(analysis->frag_array));
		cstChecker_clean(&(analysis->cst_checker));
		ioChecker_clean(&(analysis->io_checker));
		free(analysis);
		return NULL;

	}

	analysis->trace 		= NULL;
	analysis->code_map 		= NULL;
	analysis->loop_engine 	= NULL;

	return analysis;
}

void analysis_delete(struct analysis* analysis){
	if (analysis->loop_engine != NULL){
		loopEngine_delete(analysis->loop_engine);
		analysis->loop_engine = NULL;
	}

	analysis_arg_clean(analysis);
	array_clean(&(analysis->arg_array));

	analysis_frag_clean(analysis);
	array_clean(&(analysis->frag_array));

	ioChecker_clean(&(analysis->io_checker));
	cstChecker_clean(&(analysis->cst_checker));

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
	}

	if (analysis->code_map != NULL){
		printf("WARNING: in %s, deleting previous codeMap\n", __func__);
		codeMap_delete(analysis->code_map);
	}

	analysis->trace = trace_create(arg);
	if (analysis->trace == NULL){
		printf("ERROR: in %s, unable to create trace\n", __func__);
	}

	analysis->code_map = cmReaderJSON_parse(arg);
	if (analysis->code_map == NULL){
		printf("ERROR: in %s, unable to create codeMap\n", __func__);
	}
}

void analysis_trace_print(struct analysis* analysis, char* arg){
	uint32_t start = 0;
	uint32_t stop = 0;

	if (analysis->trace != NULL){
		inputParser_extract_index(arg, &start, &stop);
		trace_print(analysis->trace, start, stop, NULL);
	}
	else{
		printf("ERROR: in %s, trace is NULL\n", __func__);
	}
}

void analysis_trace_print_asm(struct analysis* analysis, char* arg){
	uint32_t start = 0;
	uint32_t stop = 0;

	if (analysis->trace != NULL){
		inputParser_extract_index(arg, &start, &stop);
		trace_print_asm(analysis->trace, start, stop);
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

void analysis_trace_search_constant(struct analysis* analysis){
	if (analysis->trace != NULL){
		if (cstChecker_check(&(analysis->cst_checker), analysis->trace)){
			printf("ERROR: in %s, unable to check constant(s)\n", __func__);
		}
	}
	else{
		printf("ERROR: in %s, trace is NULL\n", __func__);
	}
}

void analysis_trace_export(struct analysis* analysis, char* arg){
	uint32_t 				start = 0;
	uint32_t 				stop = 0;
	struct traceFragment 	fragment;

	if (analysis->trace != NULL){
		if (traceFragment_init(&fragment, TRACEFRAGMENT_TYPE_NONE, NULL, NULL)){
			printf("ERROR: in %s, unable to init traceFragment\n", __func__);
		}
		else{
			inputParser_extract_index(arg, &start, &stop);
			if (trace_extract_segment(analysis->trace, &(fragment.trace), start, stop - start)){
				printf("ERROR: in %s, unable to extract traceFragment\n", __func__);
			}
			else{
				snprintf(fragment.tag, TRACEFRAGMENT_TAG_LENGTH, "trace [%u:%u]", start, stop);

				if (array_add(&(analysis->frag_array), &fragment) < 0){
					printf("ERROR: in %s, unable to add traceFragment to array\n", __func__);
					traceFragment_clean(&fragment);
				}
			}
		}
	}
	else{
		printf("ERROR: in %s, trace is NULL\n", __func__);
	}
}

void analysis_trace_locate_pc(struct analysis* analysis, char* arg){
	ADDRESS 	pc;
	uint32_t 	i;

	pc = strtoul((const char*)arg, NULL, 16);

	#if defined ARCH_32
	printf("Instance of EIP: 0x%08x:\n", pc);
	#elif defined ARCH_64
	printf("Instance of EIP: 0x%llx:\n", pc);
	#else
	#error Please specify an architecture {ARCH_32 or ARCH_64}
	#endif
	
	for (i = 0; i < analysis->trace->nb_instruction; i++){
		if (analysis->trace->instructions[i].pc == pc){
			printf("\t- Found EIP in trace at offset: %u\n", i);
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
/* Loop functions						                                 */
/* ===================================================================== */

void analysis_loop_create(struct analysis* analysis, char* arg){
	if (analysis->loop_engine != NULL){
		printf("WARNING: in %s, deleting previous loopEngine\n", __func__);
		loopEngine_delete(analysis->loop_engine);
	}

	if (analysis->trace == NULL){
		printf("ERROR: %s, trace is NULL\n", __func__);
	}
	else{
		analysis->loop_engine = loopEngine_create(analysis->trace);
		if (analysis->loop_engine == NULL){
			printf("ERROR: in %s, unable to init loopEngine\n", __func__);
			return;
		}

		if (!strcmp(arg, "STRICT")){
			if (loopEngine_process_strict(analysis->loop_engine)){
				printf("ERROR: in %s, unable to process strict loopEngine\n", __func__);
			}
		}
		else if (!strcmp(arg, "NORDER")){
			if (loopEngine_process_norder(analysis->loop_engine)){
				printf("ERROR: in %s, unable to process norder loopEngine\n", __func__);
			}
		}
		else{
			printf("Expected loop creation specifier:\n");
			printf(" - \"STRICT\" : compare exact instruction sequence\n");
			printf(" - \"NORDER\" : does not take into account instructions order\n");
		}
	}
}

void analysis_loop_remove_redundant(struct analysis* analysis, char* arg){
	if (analysis->loop_engine != NULL){
		if (!strcmp(arg, "STRICT")){
			if (loopEngine_remove_redundant_loop_strict(analysis->loop_engine)){
				printf("ERROR: in %s, unable to remove redundant loop\n", __func__);
			}
		}
		else if (!strcmp(arg, "PACKED")){
			if (loopEngine_remove_redundant_loop_packed(analysis->loop_engine)){
				printf("ERROR: in %s, unable to remove redundant loop\n", __func__);
			}
		}
		else if (!strcmp(arg, "NESTED")){
			if (loopEngine_remove_redundant_loop_nested(analysis->loop_engine)){
				printf("ERROR: in %s, unable to remove redundant loop\n", __func__);
			}
		}
		else{
			printf("Expected remove redundant loop specifier:\n");
			printf(" - \"STRICT\" : nested loop(s) are deleted. No epilogue packing\n");
			printf(" - \"PACKED\" : nested loop(s) are deleted. Epilogue packing\n");
			printf(" - \"NESTED\" : nested loop(s) are not deleted. Epilogue packing\n");
		}
	}
	else{
		printf("ERROR: in %s, loopEngine is NULL\n", __func__);
	}
}

void analysis_loop_print(struct analysis* analysis){
	if (analysis->loop_engine != NULL){
		loopEngine_print_loop(analysis->loop_engine);
	}
	else{
		printf("ERROR: in %s, loopEngine is NULL\n", __func__);
	}
}

void analysis_loop_export(struct analysis* analysis, char* arg){
	int32_t 	loop_index = -1;
	int32_t 	iteration_index = -1;
	uint32_t 	i;
	uint8_t 	found_space = 0;


	if (analysis->loop_engine != NULL){
		for (i = 0; i < strlen(arg) - 1; i++){
			if (arg[i] == ' '){
				found_space = 1;
				break;
			}
		}
		if (found_space){
			loop_index = atoi(arg + i + 1);
		}
		else{
			i ++;
		}

		if (!strncmp(arg, "ALL", i)){
			if (loopEngine_export_all(analysis->loop_engine, &(analysis->frag_array), loop_index)){
				printf("ERROR: in %s, unable to export loop to traceFragment\n", __func__);
			}
		}
		else if (i > 3 && !strncmp(arg, "IT=", 3)){
			iteration_index = atoi(arg + 3);
			if (loopEngine_export_it(analysis->loop_engine, &(analysis->frag_array), loop_index, iteration_index)){
				printf("ERROR: in %s, unable to export loop to traceFragment\n", __func__);
			}
		}
		else if (!strncmp(arg, "NOEP", i)){
			if (loopEngine_export_noEp(analysis->loop_engine, &(analysis->frag_array), loop_index)){
				printf("ERROR: in %s, unable to export loop to traceFragment\n", __func__);
			}
		}
		else{
			printf("Expected export specifier:\n");
			printf(" - \"ALL\"   : export every loop's iteration and the epilogue if it exists\n");
			printf(" - \"NOEP\"  : export every loop's iteration\n");
			printf(" - \"IT=xx\" : export the xx iteration of the loop\n");
		}
	}
	else{
		printf("ERROR: in %s, loopEngine is NULL\n", __func__);
	}

	return;
}

void analysis_loop_delete(struct analysis* analysis){
	if (analysis->loop_engine != NULL){
		loopEngine_delete(analysis->loop_engine);
		analysis->loop_engine = NULL;
	}
	else{
		printf("ERROR: in %s, loopEngine is NULL\n", __func__);
	}
}

/* ===================================================================== */
/* frag functions						                                 */
/* ===================================================================== */

void analysis_frag_print_stat(struct analysis* analysis, char* arg){
	uint32_t 					i;
	struct simpleTraceStat 		stat;
	struct multiColumnPrinter* 	printer = NULL;
	uint32_t 					index;
	struct traceFragment*		fragment;

	if (arg != NULL){
		index = (uint32_t)atoi(arg);
		
		if (index < array_get_length(&(analysis->frag_array))){
			fragment = (struct traceFragment*)array_get(&(analysis->frag_array), index);
			#ifdef VERBOSE
			printf("Print simpleTraceStat for fragment %u (tag: \"%s\", nb fragment: %u)\n", index, fragment->tag, array_get_length(&(analysis->frag_array)));
			#endif
			simpleTraceStat_init(&stat);
			simpleTraceStat_process(&stat, fragment);
			simpleTraceStat_print(printer, 0, NULL, &stat);
		}
		else{
			printf("ERROR: in %s, incorrect index value %u (array size :%u)\n", __func__, index, array_get_length(&(analysis->frag_array)));
		}
	}
	else{
		printer = simpleTraceStat_init_MultiColumnPrinter();
		if (printer != NULL){
			multiColumnPrinter_print_header(printer);

			for (i = 0; i < array_get_length(&(analysis->frag_array)); i++){
				fragment = (struct traceFragment*)array_get(&(analysis->frag_array), i);
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

void analysis_frag_print_ins(struct analysis* analysis, char* arg){
	uint32_t index;

	index = (uint32_t)atoi(arg);	
	if (index < array_get_length(&(analysis->frag_array))){
		traceFragment_print_instruction((struct traceFragment*)array_get(&(analysis->frag_array), index));
	}
	else{
		printf("ERROR: in %s, incorrect index value %u (array size :%u)\n", __func__, index, array_get_length(&(analysis->frag_array)));
	}
}

void analysis_frag_print_asm(struct analysis* analysis, char* arg){
	uint32_t index;

	index = (uint32_t)atoi(arg);	
	if (index < array_get_length(&(analysis->frag_array))){
		traceFragment_print_assembly((struct traceFragment*)array_get(&(analysis->frag_array), index));
	}
	else{
		printf("ERROR: in %s, incorrect index value %u (array size :%u)\n", __func__, index, array_get_length(&(analysis->frag_array)));
	}
}

void analysis_frag_print_percent(struct analysis* analysis){
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

		for (i = 0; i < array_get_length(&(analysis->frag_array)); i++){
			fragment = (struct traceFragment*)array_get(&(analysis->frag_array), i);
			percent = traceFragment_opcode_percent(fragment, nb_opcode, opcode, nb_excluded_opcode, excluded_opcode);
			multiColumnPrinter_print(printer, i, fragment->tag, percent*100, NULL);
		}

		multiColumnPrinter_delete(printer);
	}
	else{
		printf("ERROR: in %s, unable to create multiColumnPrinter\n", __func__);
	}
}

void analysis_frag_print_register(struct analysis* analysis, char* arg){
	struct traceFragment* 		fragment;
	uint32_t 					index;

	index = (uint32_t)atoi(arg);	
	if (index < array_get_length(&(analysis->frag_array))){
		fragment = (struct traceFragment*)array_get(&(analysis->frag_array), index);
		#ifdef VERBOSE
		printf("Print registers for fragment %u (tag: \"%s\", nb fragment: %u)\n", index, fragment->tag, array_get_length(&(analysis->frag_array)));
		#endif

		if (fragment->read_register_array == NULL || fragment->write_register_array == NULL){
			#ifdef VERBOSE
			printf("WARNING: register arrays have not been built for the current fragment. Building them now.\n");
			#endif

			if (traceFragment_create_reg_array(fragment)){
				printf("ERROR: in %s, unable to create reg array for the fragment\n", __func__);
			}
		}

		printf("*** Input Register(s) ***\n");
		regAccess_print(fragment->read_register_array, fragment->nb_register_read_access);
		printf("\n*** Output Register(s) ***\n");
		regAccess_print(fragment->write_register_array, fragment->nb_register_write_access);
	}
	else{
		printf("ERROR: in %s, incorrect index value %u (array size :%u)\n", __func__, index, array_get_length(&(analysis->frag_array)));
	}
}

void analysis_frag_print_memory(struct analysis* analysis, char* arg){
	struct traceFragment* 		fragment;
	uint32_t 					index;

	index = (uint32_t)atoi(arg);	
	if (index < array_get_length(&(analysis->frag_array))){
		fragment = (struct traceFragment*)array_get(&(analysis->frag_array), index);
		#ifdef VERBOSE
		printf("Print memory for fragment %u (tag: \"%s\", nb fragment: %u)\n", index, fragment->tag, array_get_length(&(analysis->frag_array)));
		#endif

		if (fragment->read_memory_array == NULL || fragment->write_memory_array == NULL){
			#ifdef VERBOSE
			printf("WARNING: memory arrays have not been built for the current fragment. Building them now.\n");
			#endif

			if (traceFragment_create_mem_array(fragment)){
				printf("ERROR: in %s, unable to create mem array for the fragment\n", __func__);
			}
		}

		printf("*** Read Memory ***\n");
		memAccess_print(fragment->read_memory_array, fragment->nb_memory_read_access);
		printf("\n*** Write Memory ***\n");
		memAccess_print(fragment->write_memory_array, fragment->nb_memory_write_access);
	}
	else{
		printf("ERROR: in %s, incorrect index value %u (array size :%u)\n", __func__, index, array_get_length(&(analysis->frag_array)));
	}
}

void analysis_frag_set_tag(struct analysis* analysis, char* arg){
	uint32_t 				i;
	uint32_t 				index;
	struct traceFragment* 	fragment;
	uint8_t 				found_space = 0;

	for (i = 0; i < strlen(arg) - 1; i++){
		if (arg[i] == ' '){
			found_space = 1;
			break;
		}
	}
	if (found_space){
		index = (uint32_t)atoi(arg);

		if (index < array_get_length(&(analysis->frag_array))){
			fragment = (struct traceFragment*)array_get(&(analysis->frag_array), index);

			#ifdef VERBOSE
			printf("Setting tag value for frag %u: old tag: \"%s\", new tag: \"%s\"\n", index, fragment->tag, arg + i + 1);
			#endif

			strncpy(fragment->tag, arg + i + 1, ARGSET_TAG_MAX_LENGTH);
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
	uint32_t 				i;
	uint32_t 				index;
	uint32_t 				start;
	uint32_t 				stop;
	struct traceFragment* 	fragment;

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
			fragment = (struct traceFragment*)array_get(&(analysis->frag_array), i);

			#ifdef VERBOSE
			printf("Locating frag %u (tag: \"%s\")\n", i, fragment->tag);
			#endif
			
			traceFragment_print_location(fragment, analysis->code_map);
		}
	}
	else{
		printf("ERROR: in %s, codeMap is NULL, unable to locate\n", __func__);
	}
}

void analysis_frag_extract_arg(struct analysis* analysis, char* arg){
	uint32_t 				i;
	uint32_t 				j;
	struct traceFragment* 	fragment;
	struct argSet 			arg_set;
	uint32_t 				start;
	uint32_t				stop;
	uint32_t 				index;
	uint8_t 				found_space = 0;
	int32_t(*extract_routine_mem_read)(struct argSet*,struct memAccess*,int,void*);
	int32_t(*extract_routine_mem_write)(struct argSet*,struct memAccess*,int,void*);
	int32_t(*extract_routine_reg_read)(struct argSet*,struct regAccess*,int);
	int32_t(*extract_routine_reg_write)(struct argSet*,struct regAccess*,int);

	#define ARG_NAME_ASR_LP 	"ASR_LP"
	#define ARG_NAME_ASOR_LP 	"ASOR_LP"
	#define ARG_NAME_ASR_LM 	"ASR_LM"
	#define ARG_NAME_ASOR_LM 	"ASOR_LM"
	#define ARG_NAME_LASOR_LM 	"LASOR_LM"

	#define ARG_DESC_ASR		"arguments are made of Adjacent memory access of the same Size"
	#define ARG_DESC_ASOR 		"same as \"ASR\" with additional Opcode consideration"
	#define ARG_DESC_LASOR 		"[Specific LOOP] memory access at the same index in different iterations belong to the same argument, then same as \"ASOR\""
	#define ARG_DESC_LP 		"Large registers (>= 32bits) are combined together (Pure)"
	#define ARG_DESC_LM 		"Large registers (>= 32bits) are combined together and with the memory arguments (Mix)"

	start = 0;
	stop = array_get_length(&(analysis->frag_array));

	for (i = 0; i < strlen(arg) - 1; i++){
		if (arg[i] == ' '){
			found_space = 1;
			break;
		}
	}
	if (found_space){
		index = (uint32_t)atoi(arg + i + 1);
	
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
		i ++;
	}

	if (!strncmp(arg, ARG_NAME_ASR_LP, i)){
		extract_routine_mem_read 	= memAccess_extract_arg_adjacent_size_read;
		extract_routine_mem_write 	= memAccess_extract_arg_large_write;
		extract_routine_reg_read 	= regAccess_extract_arg_large_pure_read;
		extract_routine_reg_write 	= regAccess_extract_arg_large_write;

		#ifdef VERBOSE
		printf("Extraction routine \"%s\" : %s and %s\n", ARG_NAME_ASR_LP, ARG_DESC_ASR, ARG_DESC_LP);
	#endif
	}
	else if (!strncmp(arg, ARG_NAME_ASOR_LP, i)){
		extract_routine_mem_read 	= memAccess_extract_arg_adjacent_size_opcode_read;
		extract_routine_mem_write 	= memAccess_extract_arg_large_write;
		extract_routine_reg_read 	= regAccess_extract_arg_large_pure_read;
		extract_routine_reg_write 	= regAccess_extract_arg_large_write;

		#ifdef VERBOSE
		printf("Extraction routine \"%s\" : %s and %s\n", ARG_NAME_ASOR_LP, ARG_DESC_ASOR, ARG_DESC_LP);
		#endif
	}
	else if (!strncmp(arg, ARG_NAME_ASR_LM, i)){
		extract_routine_mem_read 	= memAccess_extract_arg_adjacent_size_read;
		extract_routine_mem_write 	= memAccess_extract_arg_large_write;
		extract_routine_reg_read 	= regAccess_extract_arg_large_mix_read;
		extract_routine_reg_write 	= regAccess_extract_arg_large_write;

		#ifdef VERBOSE
		printf("Extraction routine \"%s\" : %s and %s\n", ARG_NAME_ASR_LM, ARG_DESC_ASR, ARG_DESC_LM);
		#endif
	}
	else if (!strncmp(arg, ARG_NAME_ASOR_LM, i)){
		extract_routine_mem_read 	= memAccess_extract_arg_adjacent_size_opcode_read;
		extract_routine_mem_write 	= memAccess_extract_arg_large_write;
		extract_routine_reg_read 	= regAccess_extract_arg_large_mix_read;
		extract_routine_reg_write 	= regAccess_extract_arg_large_write;

		#ifdef VERBOSE
		printf("Extraction routine \"%s\" : %s and %s\n", ARG_NAME_ASOR_LM, ARG_DESC_ASOR, ARG_DESC_LM);
		#endif
	}
	else if (!strncmp(arg, ARG_NAME_LASOR_LM, i)){
		extract_routine_mem_read 	= memAccess_extract_arg_loop_adjacent_size_opcode_read;
		extract_routine_mem_write 	= memAccess_extract_arg_large_write;
		extract_routine_reg_read 	= regAccess_extract_arg_large_mix_read;
		extract_routine_reg_write 	= regAccess_extract_arg_large_write;

		#ifdef VERBOSE
		printf("Extraction routine \"%s\" : %s and %s\n", ARG_NAME_LASOR_LM, ARG_DESC_LASOR, ARG_DESC_LM);
		#endif
	}
	else{
		printf("ERROR: in %s, bad extraction routine specifier of length %u\n", __func__, i);
		printf("Expected extraction specifier:\n");
		printf(" - \"%s\"   : %s and %s\n", 	ARG_NAME_ASR_LP, 	ARG_DESC_ASR, 		ARG_DESC_LP);
		printf(" - \"%s\"  : %s and %s\n", 		ARG_NAME_ASOR_LP, 	ARG_DESC_ASOR, 		ARG_DESC_LP);
		printf(" - \"%s\"   : %s and %s\n", 	ARG_NAME_ASR_LM, 	ARG_DESC_ASR, 		ARG_DESC_LM);
		printf(" - \"%s\"  : %s and %s\n", 		ARG_NAME_ASOR_LM, 	ARG_DESC_ASOR, 		ARG_DESC_LM);
		printf(" - \"%s\" : %s and %s\n", 		ARG_NAME_LASOR_LM, 	ARG_DESC_LASOR, 	ARG_DESC_LM);
		return;
	}

	for (j = start; j < stop; j++){
		fragment = (struct traceFragment*)array_get(&(analysis->frag_array), j);
		if (traceFragment_create_mem_array(fragment)){
			printf("ERROR: in %s, unable to create mem array for fragment %u\n", __func__, j);
			break;
		}

		if (traceFragment_create_reg_array(fragment)){
			printf("ERROR: in %s, unable to create reg array for fragment %u\n", __func__, j);
			break;
		}

		regAccess_propagate_read(fragment->read_register_array, fragment->nb_register_read_access);
		regAccess_propagate_write(fragment->write_register_array, fragment->nb_register_write_access);

		if ((fragment->nb_memory_read_access > 0) && (fragment->nb_memory_write_access > 0)){
			traceFragment_remove_read_after_write(fragment);
		}

		if ((fragment->nb_memory_read_access > 0 || fragment->nb_register_read_access > 0) && (fragment->nb_memory_write_access > 0 || fragment->nb_register_write_access > 0)){
			if (argSet_init(&arg_set, fragment->tag)){
				printf("ERROR: in %s, unable to init argSet\n", __func__);
				break;
			}

			if (extract_routine_mem_read(&arg_set, fragment->read_memory_array, fragment->nb_memory_read_access, fragment)){
				printf("ERROR: in %s, memory read extraction routine return an error code\n", __func__);
			}
			if (extract_routine_mem_write(&arg_set, fragment->write_memory_array, fragment->nb_memory_write_access, fragment)){
				printf("ERROR: in %s, memory write extraction routine return an error code\n", __func__);
			}
			if (extract_routine_reg_read(&arg_set, fragment->read_register_array, fragment->nb_register_read_access)){
				printf("ERROR: in %s, register read extraction routine return an error code\n", __func__);
			}
			if (extract_routine_reg_write(&arg_set, fragment->write_register_array, fragment->nb_register_write_access)){
				printf("ERROR: in %s, register write extraction routine return an error code\n", __func__);
			}

			if (strlen(arg_set.tag) == 0){
				snprintf(arg_set.tag, ARGSET_TAG_MAX_LENGTH, "Frag %u", j);
			}

			if (array_add(&(analysis->arg_array), &arg_set) < 0){
				printf("ERROR: in %s, unable to add argument to arg array\n", __func__);
			}
		}
	}

	return;

	#undef ARG_DESC_ASR
	#undef ARG_DESC_ASOR
	#undef ARG_DESC_LASOR
	#undef ARG_DESC_LP
	#undef ARG_DESC_LM

	#undef ARG_NAME_ASR_LP
	#undef ARG_NAME_ASOR_LP
	#undef ARG_NAME_ASR_LM
	#undef ARG_NAME_ASOR_LM
	#undef ARG_NAME_LASOR_LM
}

void analysis_frag_clean(struct analysis* analysis){
	uint32_t i;

	for (i = 0; i < array_get_length(&(analysis->frag_array)); i++){
		traceFragment_clean((struct traceFragment*)array_get(&(analysis->frag_array), i));
	}
	array_empty(&(analysis->frag_array));
}

/* ===================================================================== */
/* ir functions						                                	 */
/* ===================================================================== */

void analysis_frag_create_ir(struct analysis* analysis, char* arg){
	int32_t 	index = -1;
	uint8_t 	found_space = 0;
	uint32_t 	start;
	uint32_t 	stop;
	uint32_t 	i;

	for (i = 0; i < strlen(arg) - 1; i++){
		if (arg[i] == ' '){
			found_space = 1;
			break;
		}
	}
	if (found_space){
		index = atoi(arg + i + 1);
		if (index < (int32_t)array_get_length(&(analysis->frag_array))){
			start = index;
			stop = index + 1;
		}
		else{
			printf("ERROR: in %s, incorrect index value %u (array size :%u)\n", __func__, index, array_get_length(&(analysis->frag_array)));
			return;
		}
	}
	else{
		i ++;
		start = 0;
		stop = array_get_length(&(analysis->frag_array));
	}

	if (!strncmp(arg, "ASM", i)){
		for (i = start; i < stop; i++){	
			traceFragment_create_ir((struct traceFragment*)array_get(&(analysis->frag_array), i), IR_CREATE_ASM);
		}
	}
	else if (!strncmp(arg, "TRACE", i)){
		for (i = start; i < stop; i++){	
			traceFragment_create_ir((struct traceFragment*)array_get(&(analysis->frag_array), i), IR_CREATE_TRACE);
		}
	}
	else{
		printf("Expected creation specifier:\n");
		printf(" - \"ASM\"   : create the IR from ASM code\n");
		printf(" - \"TRACE\" : create the IR from the execution trace\n");
	}

	return;
}

void analysis_frag_printDot_ir(struct analysis* analysis, char* arg){
	uint32_t 				index;

	index = (uint32_t)atoi(arg);	
	if (index < array_get_length(&(analysis->frag_array))){
		traceFragment_printDot_ir((struct traceFragment*)array_get(&(analysis->frag_array), index));
	}
	else{
		printf("ERROR: in %s, incorrect index value %u (array size :%u)\n", __func__, index, array_get_length(&(analysis->frag_array)));
	}
}

void analysis_frag_print_io(struct analysis* analysis, char* arg){
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
			printf("ERROR: in %s, incorrect index value %u (array size :%u)\n", __func__, index, array_get_length(&(analysis->frag_array)));
			return;
		}
	}
	else{
		start = 0;
		stop = array_get_length(&(analysis->frag_array));
	}

	for (i = start; i < stop; i++){	
		traceFragment_print_io((struct traceFragment*)array_get(&(analysis->frag_array), i));
	}
}

void analysis_frag_extract_arg_ir(struct analysis* analysis, char* arg){
	uint32_t 				index;
	uint32_t 				start;
	uint32_t 				stop;
	uint32_t 				i;
	struct argSet 			arg_set;
	struct traceFragment* 	fragment;


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
		fragment = (struct traceFragment*)array_get(&(analysis->frag_array), i);
		if (argSet_init(&arg_set, fragment->tag)){
			printf("ERROR: in %s, unable to init argSet\n", __func__);
		}
		else{
			traceFragment_extract_arg_ir(fragment, &arg_set);

			if (argSet_get_nb_input(&arg_set) > 0 && argSet_get_nb_output(&arg_set) > 0){
				if (strlen(arg_set.tag) == 0){
					snprintf(arg_set.tag, ARGSET_TAG_MAX_LENGTH, "Frag %u", i);
				}

				if (array_add(&(analysis->arg_array), &arg_set) < 0){
					printf("ERROR: in %s, unable to add argument to arg array\n", __func__);
				}
			}
			else{
				argSet_clean(&arg_set);
			}
		}
	}
}

/* ===================================================================== */
/* arg functions						                                 */
/* ===================================================================== */

void analysis_arg_print(struct analysis* analysis, char* arg){
	uint32_t 					index;
	uint32_t 					i;
	struct argSet* 				arg_set;
	struct multiColumnPrinter* 	printer;
	enum argFragType 			filter_type;
	enum argFragType* 			filter_type_pointer;
	uint8_t 					found_space = 0;

	if (arg != NULL){
		for (i = 0; i < strlen(arg) - 1; i++){
			if (arg[i] == ' '){
				found_space = 1;
				break;
			}
		}
		if (found_space){
			index = (uint32_t)atoi(arg + i + 1);

			if (!strncmp(arg, "mem", i)){
				#ifdef VERBOSE
				printf("Filter MEMORY argBuffer(s) only\n");
				#endif
				filter_type = ARGFRAG_MEM;
				filter_type_pointer = &filter_type;
			}
			else if (!strncmp(arg, "reg", i)){
				#ifdef VERBOSE
				printf("Filter REGISTER argBuffer(s) only\n");
				#endif
				filter_type = ARGFRAG_REG;
				filter_type_pointer = &filter_type;
			}
			else{
				printf("ERROR: in %s, unkown argument: \"%s\"\n", __func__, arg);
				filter_type_pointer = NULL;
			}
		}
		else{
			index = (uint32_t)atoi(arg);
			filter_type_pointer = NULL;
		}
		
		if (index < array_get_length(&(analysis->arg_array))){
			arg_set = (struct argSet*)array_get(&(analysis->arg_array), index);
			#ifdef VERBOSE
			printf("Print argSet %u (tag: \"%s\", nb argSet: %u)\n", index, arg_set->tag, array_get_length(&(analysis->arg_array)));
			#endif

			argSet_print(arg_set, filter_type_pointer);
		}
		else{
			printf("ERROR: in %s, incorrect index value %u (array size :%u)\n", __func__, index, array_get_length(&(analysis->arg_array)));
		}
	}
	else{
		printer = multiColumnPrinter_create(stdout, 4, NULL, NULL, NULL);
		if (printer != NULL){
			multiColumnPrinter_set_column_size(printer, 0, 5);
			multiColumnPrinter_set_column_size(printer, 1, ARGSET_TAG_MAX_LENGTH);
			multiColumnPrinter_set_column_size(printer, 2, 9);
			multiColumnPrinter_set_column_size(printer, 3, 9);

			multiColumnPrinter_set_title(printer, 0, "Index");
			multiColumnPrinter_set_title(printer, 1, "Tag");
			multiColumnPrinter_set_title(printer, 2, "Nb Input");
			multiColumnPrinter_set_title(printer, 3, "Nb Output");

			multiColumnPrinter_set_column_type(printer, 0, MULTICOLUMN_TYPE_UINT32);
			multiColumnPrinter_set_column_type(printer, 2, MULTICOLUMN_TYPE_UINT32);
			multiColumnPrinter_set_column_type(printer, 3, MULTICOLUMN_TYPE_UINT32);

			multiColumnPrinter_print_header(printer);

			for (i = 0; i < array_get_length(&(analysis->arg_array)); i++){
				arg_set = (struct argSet*)array_get(&(analysis->arg_array), i);
				multiColumnPrinter_print(printer, i, arg_set->tag, argSet_get_nb_input(arg_set), argSet_get_nb_output(arg_set), NULL);
			}

			multiColumnPrinter_delete(printer);
		}
		else{
			printf("ERROR: in %s, unable to init multiColumnPrinter\n", __func__);
		}
	}
}

void analysis_arg_set_tag(struct analysis* analysis, char* arg){
	uint32_t 			i;
	uint32_t 			index;
	struct argSet* 		arg_set;
	uint8_t 			found_space = 0;

	for (i = 0; i < strlen(arg) - 1; i++){
		if (arg[i] == ' '){
			found_space = 1;
			break;
		}
	}
	if (found_space){
		index = (uint32_t)atoi(arg);

		if (index < array_get_length(&(analysis->arg_array))){
			arg_set = (struct argSet*)array_get(&(analysis->arg_array), index);

			#ifdef VERBOSE
			printf("Setting tag value for arg %u: old tag: \"%s\", new tag: \"%s\"\n", index, arg_set->tag, arg + i + 1);
			#endif

			strncpy(arg_set->tag, arg + i + 1, ARGSET_TAG_MAX_LENGTH);
		}
		else{
			printf("ERROR: in %s, incorrect index value %u (array size :%u)\n", __func__, index, array_get_length(&(analysis->arg_array)));
		}
	}
	else{
		printf("ERROR: in %s, the index and the tag must separated by a space char\n", __func__);
	}
}

void analysis_arg_search(struct analysis* analysis, char* arg){
	uint32_t 			i;
	uint32_t 			start;
	uint32_t			stop;
	uint32_t 			index;

	start = 0;
	stop = array_get_length(&(analysis->arg_array));

	if (arg != NULL){
		index = (uint32_t)atoi(arg);
		
		if (index < array_get_length(&(analysis->arg_array))){
			start = index;
			stop = index + 1;
		}
		else{
			printf("ERROR: in %s, incorrect index value %u (array size :%u)\n", __func__, index, array_get_length(&(analysis->arg_array)));
			return;
		}
	}

	#ifndef VERBOSE
	ioChecker_start(&(analysis->io_checker));
	#endif

	for (i = start; i < stop; i++){
		if (ioChecker_submit_argSet(&(analysis->io_checker), (struct argSet*)array_get(&(analysis->arg_array), i))){
			printf("ERROR: in %s, unable to submit argSet to the ioChecker\n", __func__);
		}
	}
	#ifdef VERBOSE
	ioChecker_check(&(analysis->io_checker));
	#else
	ioChecker_wait(&(analysis->io_checker));
	#endif
}

void analysis_arg_seek(struct analysis* analysis, char* arg){
	uint32_t 			i;
	struct argSet* 		arg_set;
	char* 				buffer;
	uint32_t 			buffer_length;

	buffer = readBuffer_raw(arg, strlen(arg), NULL);
	buffer_length = READBUFFER_RAW_GET_LENGTH(strlen(arg));
	if (buffer == NULL){
		printf("ERROR: in %s, readBuffer return NULL\n", __func__);
	}
	else{
		for (i = 0; i < array_get_length(&(analysis->arg_array)); i++){
			arg_set = (struct argSet*)array_get(&(analysis->arg_array), i);
				
			argSet_search_input(arg_set, buffer, buffer_length);
			argSet_search_output(arg_set, buffer, buffer_length);
		}
		free(buffer);
	}
}

void analysis_arg_clean(struct analysis* analysis){
	uint32_t 				i;

	for (i = 0; i < array_get_length(&(analysis->arg_array)); i++){
		argSet_clean((struct argSet*)array_get(&(analysis->arg_array), i));
	}
	array_empty(&(analysis->arg_array));
}