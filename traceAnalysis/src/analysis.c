#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include "analysis.h"
#include "inputParser.h"
#include "instruction.h"
#include "argSet.h"
#include "argBuffer.h"
#include "simpleTraceStat.h"
#include "printBuffer.h"
#include "readBuffer.h"

#define ADD_CMD_TO_INPUT_PARSER(parser, cmd, desc, interac, arg, func)									\
	{																									\
		if (inputParser_add_cmd((parser), (cmd), (desc), (interac), (arg), (void(*)(void))(func))){		\
			printf("ERROR: in %s, unable to add cmd: \"%s\" to inputParser\n", __func__, (cmd));		\
		}																								\
	}

int main(int argc, char** argv){
	struct analysis* 			analysis = NULL;
	struct inputParser* 		parser = NULL;
	
	if	(argc < 2){
		printf("ERROR: in %s, please specify the trace directory as first argument\n", __func__);
		goto exit;
	}

	parser = inputParser_create();
	if (parser == NULL){
		printf("ERROR: in %s, unable to create inputParser\n", __func__);
		goto exit;
	}
	
	analysis = analysis_create(argv[1]);
	if (analysis == NULL){
		printf("ERROR: in %s, unable to create the analysis structure\n", __func__);
		goto exit;
	}

	/* ioChecker specific commands*/
	ADD_CMD_TO_INPUT_PARSER(parser, "load ioChecker", "Load primitive reference from a file. Specify file name as second arg", INPUTPARSER_CMD_INTERACTIVE, analysis->checker, ioChecker_load)
	ADD_CMD_TO_INPUT_PARSER(parser, "print ioChecker", "Display the ioChecker structure", INPUTPARSER_CMD_NOT_INTERACTIVE, analysis->checker, ioChecker_print)
	ADD_CMD_TO_INPUT_PARSER(parser, "clean ioChecker", "Remove every primitive reference from the ioChecker", INPUTPARSER_CMD_NOT_INTERACTIVE, analysis->checker, ioChecker_empty)

	/* codeMap specific commands */
	ADD_CMD_TO_INPUT_PARSER(parser, "check codeMap", "Perform basic checks on the codeMap address", INPUTPARSER_CMD_NOT_INTERACTIVE, analysis->code_map, codeMap_check_address)
	ADD_CMD_TO_INPUT_PARSER(parser, "print codeMap", "Print the codeMap (informations about routine address). Specify a filter as second arg", INPUTPARSER_CMD_INTERACTIVE, analysis->code_map, codeMap_print)

	/* trace specific commands */
	ADD_CMD_TO_INPUT_PARSER(parser, "print trace", "Print all the instructions of the trace. Specify an index / range as a second arg", INPUTPARSER_CMD_INTERACTIVE, analysis, analysis_instruction_print)
	ADD_CMD_TO_INPUT_PARSER(parser, "export trace", "Export the whole trace as traceFragment", INPUTPARSER_CMD_NOT_INTERACTIVE, analysis, analysis_instruction_export)

	/* loop specific commands */
	ADD_CMD_TO_INPUT_PARSER(parser, "create loop", "Create a loopEngine and parse the trace looking for A^n pattern", INPUTPARSER_CMD_NOT_INTERACTIVE, analysis, analysis_loop_create)
	ADD_CMD_TO_INPUT_PARSER(parser, "remove redundant loop", "Remove the redundant loops from the loops list", INPUTPARSER_CMD_NOT_INTERACTIVE, analysis, analysis_loop_remove_redundant)
	ADD_CMD_TO_INPUT_PARSER(parser, "pack epilogue", "Pack loop epilogue to reduce the number of loops (must be run after \"remove redundant loop\")", INPUTPARSER_CMD_NOT_INTERACTIVE, analysis, analysis_loop_pack_epilogue)
	ADD_CMD_TO_INPUT_PARSER(parser, "print loop", "Print every loops contained in the loopEngine", INPUTPARSER_CMD_NOT_INTERACTIVE, analysis, analysis_loop_print)
	ADD_CMD_TO_INPUT_PARSER(parser, "export loop", "Export loop(s) to traceFragment array. Specify export method [mandatory] and loop index", INPUTPARSER_CMD_INTERACTIVE, analysis, analysis_loop_export)
	ADD_CMD_TO_INPUT_PARSER(parser, "delete loop", "Delete a previously created loopEngine (loop)", INPUTPARSER_CMD_NOT_INTERACTIVE, analysis, analysis_loop_delete)

	/* traceFragement specific commands */
	ADD_CMD_TO_INPUT_PARSER(parser, "clean frag", "Clean the traceFragment array", INPUTPARSER_CMD_NOT_INTERACTIVE, analysis, analysis_frag_clean)
	ADD_CMD_TO_INPUT_PARSER(parser, "print frag stat", "Print simpleTraceStat about the traceFragment. Specify an index as second arg", INPUTPARSER_CMD_INTERACTIVE, analysis, analysis_frag_print_stat)
	ADD_CMD_TO_INPUT_PARSER(parser, "print frag ins", "Print instructions of the traceFragment. Specify an index as second arg [mandatory]", INPUTPARSER_CMD_INTERACTIVE, analysis, analysis_frag_print_ins)
	ADD_CMD_TO_INPUT_PARSER(parser, "print frag percent", "Print for every traceFragments some stat about instructions frequency", INPUTPARSER_CMD_NOT_INTERACTIVE, analysis, analysis_frag_print_percent)
	ADD_CMD_TO_INPUT_PARSER(parser, "print frag register", "Print for every traceFragments input and output registers. Specify an index as second arg [mandatory]", INPUTPARSER_CMD_INTERACTIVE, analysis, analysis_frag_print_register)
	ADD_CMD_TO_INPUT_PARSER(parser, "print frag memory", "Print for every traceFragments input and output registers. Specify an index as second arg [mandatory]", INPUTPARSER_CMD_INTERACTIVE, analysis, analysis_frag_print_memory)
	ADD_CMD_TO_INPUT_PARSER(parser, "set frag tag", "Set tag value for specific fragment. Specify frag index and tag value [respect order]", INPUTPARSER_CMD_INTERACTIVE, analysis, analysis_frag_set_tag)
	ADD_CMD_TO_INPUT_PARSER(parser, "locate frag", "Locate traceFragement (an index may be specified as second arg) in the codeMap", INPUTPARSER_CMD_INTERACTIVE, analysis, analysis_frag_locate)
	ADD_CMD_TO_INPUT_PARSER(parser, "extract frag arg", "Extract input and output argument(s) from the traceFragment. Specify extraction routine [mandatory] and index", INPUTPARSER_CMD_INTERACTIVE, analysis, analysis_frag_extract_arg)

	/* argument specific commands */
	ADD_CMD_TO_INPUT_PARSER(parser, "clean arg", "Clean the argSet array", INPUTPARSER_CMD_NOT_INTERACTIVE, analysis, analysis_arg_clean)
	ADD_CMD_TO_INPUT_PARSER(parser, "print arg", "Print arguments from the argSet array. Specify an index as second arg", INPUTPARSER_CMD_INTERACTIVE, analysis, analysis_arg_print)
	ADD_CMD_TO_INPUT_PARSER(parser, "set arg tag", "Set tag value for specific argSet. Specify arg index and tag value [respect order]", INPUTPARSER_CMD_INTERACTIVE, analysis, analysis_arg_set_tag)
	ADD_CMD_TO_INPUT_PARSER(parser, "search arg", "Search every element in the argSet array. Specify an index as second arg", INPUTPARSER_CMD_INTERACTIVE, analysis, analysis_arg_search)
	ADD_CMD_TO_INPUT_PARSER(parser, "seek arg", "Seek for a user defined argBuffer in the argSet array. Specify the buffer as second arg (raw format)", INPUTPARSER_CMD_INTERACTIVE, analysis, analysis_arg_seek)

	inputParser_exe(parser, argc - 2, argv + 2);

	exit:

	analysis_delete(analysis);
	inputParser_delete(parser);

	pthread_exit(NULL); /* due to dlopen */
	return 0;
}


/* ===================================================================== */
/* Analysis functions						                             */
/* ===================================================================== */

struct analysis* analysis_create(const char* dir_name){
	struct analysis* 	analysis;
	char				file_name[ANALYSIS_DIRECTORY_NAME_MAX_LENGTH];

	analysis = (struct analysis*)malloc(sizeof(struct analysis));
	if (analysis == NULL){
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
		return NULL;
	}

	analysis->checker = ioChecker_create();
	if (analysis->checker == NULL){
		printf("ERROR: in %s, unable to create ioChecker\n", __func__);
		free(analysis);
		return NULL;
	}

	snprintf(file_name, ANALYSIS_DIRECTORY_NAME_MAX_LENGTH, "%s/%s", dir_name, ANALYSIS_INS_FILE_NAME);
	if (traceReaderJSON_init(&(analysis->ins_reader.json), file_name)){
		printf("ERROR: in %s, unable to init trace JSON reader\n", __func__);
		ioChecker_delete(analysis->checker);
		free(analysis);
		return NULL;
	}

	snprintf(file_name, ANALYSIS_DIRECTORY_NAME_MAX_LENGTH, "%s/%s", dir_name, ANALYSIS_CM_FILE_NAME);
	analysis->code_map = cmReaderJSON_parse(file_name);
	if (analysis->code_map == NULL){
		printf("WARNING: in %s, continue without code map information\n", __func__);
	}

	if (array_init(&(analysis->frag_array), sizeof(struct traceFragment))){
		printf("ERROR: in %s, unable to init traceFragment array\n", __func__);
		ioChecker_delete(analysis->checker);
		free(analysis);
		return NULL;
	}

	if (array_init(&(analysis->arg_array), sizeof(struct argSet))){
		printf("ERROR: in %s, unable to init argSet array\n", __func__);
		array_clean(&(analysis->frag_array));
		ioChecker_delete(analysis->checker);
		free(analysis);
		return NULL;

	}

	analysis->trace = NULL;
	analysis->loop_engine = NULL;

	return analysis;
}

void analysis_delete(struct analysis* analysis){
	if (analysis != NULL){
		if (analysis->loop_engine != NULL){
			loopEngine_delete(analysis->loop_engine);
			analysis->loop_engine = NULL;
		}

		analysis_arg_clean(analysis);
		array_clean(&(analysis->arg_array));

		analysis_frag_clean(analysis);
		array_clean(&(analysis->frag_array));

		traceReaderJSON_clean(&(analysis->ins_reader.json));
		codeMap_delete(analysis->code_map);

		if (analysis->trace != NULL){
			trace_delete(analysis->trace);
		}

		ioChecker_delete(analysis->checker);

		free(analysis);
	}
}

/* ===================================================================== */
/* Trace functions						                                 */
/* ===================================================================== */

/*reprendre cette méthode est faire un premier commit si tout fonctionne correcetement */
void analysis_instruction_print(struct analysis* analysis, char* arg){
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

void analysis_instruction_export(struct analysis* analysis){
	struct instruction* 	instruction;
	struct traceFragment 	fragment;

	if (traceFragment_init(&fragment, TRACEFRAGMENT_TYPE_NONE, NULL, NULL)){
		printf("ERROR: in %s, unable to init traceFragment\n", __func__);
		return;
	}

	if (!traceReaderJSON_reset(&(analysis->ins_reader.json))){
		do{
			instruction = traceReaderJSON_get_next_instruction(&(analysis->ins_reader.json));
			if (instruction != NULL){
				if (traceFragment_add_instruction(&fragment, instruction) < 0){
					printf("ERROR: in %s, unable to add instruction to traceFragment\n", __func__);
					break;
				}
			}
		} while (instruction != NULL);

		traceFragment_set_tag(&fragment, "trace");

		if (array_add(&(analysis->frag_array), &fragment) < 0){
			printf("ERROR: in %s, unable to add traceFragment to frag_array\n", __func__);
			traceFragment_clean(&fragment);
		}

		/* new earth army */
		if (analysis->trace != NULL){
			printf("WARNING: in %s, deleting previous trace\n", __func__);
			trace_delete(analysis->trace);
		}
		analysis->trace = trace_create(&(fragment.instruction_array));
		if (analysis->trace == NULL){
			printf("ERROR: in %s, unable to create trace\n", __func__);
		}

	}
	else{
		printf("ERROR: in %s, unable to reset JSON trace reader\n", __func__);
	}
}

/* ===================================================================== */
/* Loop functions						                                 */
/* ===================================================================== */

void analysis_loop_create(struct analysis* analysis){
	if (analysis->loop_engine != NULL){
		printf("WARNING: in %s, deleting previous loopEngine\n", __func__);
		loopEngine_delete(analysis->loop_engine);
	}

	if (analysis->trace == NULL){
		printf("ERROR: %s, trace is NULL\n", __func__);
		return;
	}

	analysis->loop_engine = loopEngine_create(analysis->trace);
	if (analysis->loop_engine == NULL){
		printf("ERROR: in %s, unable to init loopEngine\n", __func__);
		return;
	}

	if (loopEngine_process(analysis->loop_engine)){
		printf("ERROR: in %s, unable to process loopEngine\n", __func__);
	}
}

void analysis_loop_remove_redundant(struct analysis* analysis){
	if (analysis->loop_engine != NULL){
		if (loopEngine_remove_redundant_loop(analysis->loop_engine)){
			printf("ERROR: in %s, unable to remove redundant loop\n", __func__);
		}
	}
	else{
		printf("ERROR: in %s, loopEngine is NULL\n", __func__);
	}
}

void analysis_loop_pack_epilogue(struct analysis* analysis){
	if (analysis->loop_engine != NULL){
		if (loopEngine_pack_epilogue(analysis->loop_engine)){
			printf("ERROR: in %s, unable to pack epilogue\n", __func__);
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
		if (arg != NULL){
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
				goto arg_error;
			}
		}
		else{
			goto arg_error;
		}
	}
	else{
		printf("ERROR: in %s, loopEngine is NULL\n", __func__);
	}

	return;

	arg_error:
	printf("Expected export specifier:\n");
	printf(" - \"ALL\"   : export every loop's iteration and the epilogue if it exists\n");
	printf(" - \"NOEP\"  : export every loop's iteration\n");
	printf(" - \"IT=xx\" : export the xx iteration of the loop\n");

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

void analysis_frag_clean(struct analysis* analysis){
	uint32_t 				i;
	struct traceFragment* 	fragment;

	for (i = 0; i < array_get_length(&(analysis->frag_array)); i++){
		fragment = (struct traceFragment*)array_get(&(analysis->frag_array), i);
		traceFragment_clean(fragment);
	}
	array_empty(&(analysis->frag_array));
}

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
	struct multiColumnPrinter* 	printer;
	struct traceFragment* 		fragment;
	uint32_t 					i;
	uint32_t 					index;

	if (arg != NULL){
		index = (uint32_t)atoi(arg);
		
		if (index < array_get_length(&(analysis->frag_array))){
			printer = instruction_init_multiColumnPrinter();
			if (printer != NULL){
				fragment = (struct traceFragment*)array_get(&(analysis->frag_array), index);
				#ifdef VERBOSE
				printf("Print instructions for fragment %u (tag: \"%s\", nb fragment: %u)\n", index, fragment->tag, array_get_length(&(analysis->frag_array)));
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
			printf("ERROR: in %s, incorrect index value %u (array size :%u)\n", __func__, index, array_get_length(&(analysis->frag_array)));
		}
	}
	else{
		printf("ERROR: in %s, an index value must be specified\n", __func__);
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

	if (arg != NULL){
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
					printf("ERROR: in %s, unable to create reg array for the fragement\n", __func__);
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
	else{
		printf("ERROR: in %s, an index value must be specified\n", __func__);
	}
}

void analysis_frag_print_memory(struct analysis* analysis, char* arg){
	struct traceFragment* 		fragment;
	uint32_t 					index;

	if (arg != NULL){
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
					printf("ERROR: in %s, unable to create mem array for the fragement\n", __func__);
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
	else{
		printf("ERROR: in %s, an index value must be specified\n", __func__);
	}
}

void analysis_frag_set_tag(struct analysis* analysis, char* arg){
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
	else{
		printf("ERROR: in %s, an index and a tag value must be specified\n", __func__);
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
	int32_t(*extract_routine_mem_read)(struct array*,struct memAccess*,int,void*);
	int32_t(*extract_routine_mem_write)(struct array*,struct memAccess*,int,void*);
	int32_t(*extract_routine_reg_read)(struct array*,struct regAccess*,int);
	int32_t(*extract_routine_reg_write)(struct array*,struct regAccess*,int);
	uint8_t 				remove_raw = 0;

	#define ARG_NAME_A_LP 		"A_LP"
	#define ARG_NAME_AR_LP 		"AR_LP"
	#define ARG_NAME_AS_LP 		"AS_LP"
	#define ARG_NAME_ASR_LP 	"ASR_LP"
	#define ARG_NAME_ASO_LP 	"ASO_LP"
	#define ARG_NAME_ASOR_LP 	"ASOR_LP"
	#define ARG_NAME_ASR_LM 	"ASR_LM"
	#define ARG_NAME_ASOR_LM 	"ASOR_LM"
	#define ARG_NAME_LASOR_LM 	"LASOR_LM"

	#define ARG_DESC_A 			"arguments are made of Adjacent memory access"
	#define ARG_DESC_AR 		"read after write are Removed, then same as\"A\""
	#define ARG_DESC_AS 		"same as \"A\" with additional access Size consideration (Mandatory for fragmentation)"
	#define ARG_DESC_ASR		"read after write are Removed, then same as \"AS\""
	#define ARG_DESC_ASO 		"same as \"AS\" with additional Opcode consideration"
	#define ARG_DESC_ASOR 		"read after write are Removed, then same as \"ASO\""
	#define ARG_DESC_LASOR 		"[Specific LOOP] memory access at the same index in different iterations belong to the same argument, then same as \"ASOR\""
	#define ARG_DESC_LP 		"Large registers (>= 32bits) are combined together (Pure)"
	#define ARG_DESC_LM 		"Large registers (>= 32bits) are combined together and with the memory arguments (Mix)"

	start = 0;
	stop = array_get_length(&(analysis->frag_array));

	if (arg != NULL){
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

		if (!strncmp(arg, ARG_NAME_A_LP, i)){
			extract_routine_mem_read 	= memAccess_extract_arg_adjacent_read;
			extract_routine_mem_write 	= memAccess_extract_arg_large_write;
			extract_routine_reg_read 	= regAccess_extract_arg_large_pure_read;
			extract_routine_reg_write 	= regAccess_extract_arg_large_write;
			remove_raw 					= 0;

			#ifdef VERBOSE
			printf("Extraction routine \"%s\" : %s and %s\n", ARG_NAME_A_LP, ARG_DESC_A, ARG_DESC_LP);
			#endif
		}
		else if (!strncmp(arg, ARG_NAME_AR_LP, i)){
			extract_routine_mem_read 	= memAccess_extract_arg_adjacent_read;
			extract_routine_mem_write 	= memAccess_extract_arg_large_write;
			extract_routine_reg_read 	= regAccess_extract_arg_large_pure_read;
			extract_routine_reg_write 	= regAccess_extract_arg_large_write;
			remove_raw 					= 1;

			#ifdef VERBOSE
			printf("Extraction routine \"%s\" : %s and %s\n", ARG_NAME_AR_LP, ARG_DESC_AR, ARG_DESC_LP);
			#endif
		}
		else if (!strncmp(arg, ARG_NAME_AS_LP, i)){
			extract_routine_mem_read 	= memAccess_extract_arg_adjacent_size_read;
			extract_routine_mem_write 	= memAccess_extract_arg_large_write;
			extract_routine_reg_read 	= regAccess_extract_arg_large_pure_read;
			extract_routine_reg_write 	= regAccess_extract_arg_large_write;
			remove_raw 					= 0;

			#ifdef VERBOSE
			printf("Extraction routine \"%s\" : %s and %s\n", ARG_NAME_AS_LP, ARG_DESC_AS, ARG_DESC_LP);
			#endif
		}
		else if (!strncmp(arg, ARG_NAME_ASR_LP, i)){
			extract_routine_mem_read 	= memAccess_extract_arg_adjacent_size_read;
			extract_routine_mem_write 	= memAccess_extract_arg_large_write;
			extract_routine_reg_read 	= regAccess_extract_arg_large_pure_read;
			extract_routine_reg_write 	= regAccess_extract_arg_large_write;
			remove_raw 					= 1;

			#ifdef VERBOSE
			printf("Extraction routine \"%s\" : %s and %s\n", ARG_NAME_ASR_LP, ARG_DESC_ASR, ARG_DESC_LP);
			#endif
		}
		else if (!strncmp(arg, ARG_NAME_ASO_LP, i)){
			extract_routine_mem_read 	= memAccess_extract_arg_adjacent_size_opcode_read;
			extract_routine_mem_write 	= memAccess_extract_arg_large_write;
			extract_routine_reg_read 	= regAccess_extract_arg_large_pure_read;
			extract_routine_reg_write 	= regAccess_extract_arg_large_write;
			remove_raw 					= 0;
			
			#ifdef VERBOSE
			printf("Extraction routine \"%s\" : %s and %s\n", ARG_NAME_ASO_LP, ARG_DESC_ASO, ARG_DESC_LP);
			#endif
		}
		else if (!strncmp(arg, ARG_NAME_ASOR_LP, i)){
			extract_routine_mem_read 	= memAccess_extract_arg_adjacent_size_opcode_read;
			extract_routine_mem_write 	= memAccess_extract_arg_large_write;
			extract_routine_reg_read 	= regAccess_extract_arg_large_pure_read;
			extract_routine_reg_write 	= regAccess_extract_arg_large_write;
			remove_raw 					= 1;

			#ifdef VERBOSE
			printf("Extraction routine \"%s\" : %s and %s\n", ARG_NAME_ASOR_LP, ARG_DESC_ASOR, ARG_DESC_LP);
			#endif
		}
		else if (!strncmp(arg, ARG_NAME_ASR_LM, i)){
			extract_routine_mem_read 	= memAccess_extract_arg_adjacent_size_read;
			extract_routine_mem_write 	= memAccess_extract_arg_large_write;
			extract_routine_reg_read 	= regAccess_extract_arg_large_mix_read;
			extract_routine_reg_write 	= regAccess_extract_arg_large_write;
			remove_raw 					= 1;

			#ifdef VERBOSE
			printf("Extraction routine \"%s\" : %s and %s\n", ARG_NAME_ASR_LM, ARG_DESC_ASR, ARG_DESC_LM);
			#endif
		}
		else if (!strncmp(arg, ARG_NAME_ASOR_LM, i)){
			extract_routine_mem_read 	= memAccess_extract_arg_adjacent_size_opcode_read;
			extract_routine_mem_write 	= memAccess_extract_arg_large_write;
			extract_routine_reg_read 	= regAccess_extract_arg_large_mix_read;
			extract_routine_reg_write 	= regAccess_extract_arg_large_write;
			remove_raw 					= 1;

			#ifdef VERBOSE
			printf("Extraction routine \"%s\" : %s and %s\n", ARG_NAME_ASOR_LM, ARG_DESC_ASOR, ARG_DESC_LM);
			#endif
		}
		else if (!strncmp(arg, ARG_NAME_LASOR_LM, i)){
			extract_routine_mem_read 	= memAccess_extract_arg_loop_adjacent_size_opcode_read;
			extract_routine_mem_write 	= memAccess_extract_arg_large_write;
			extract_routine_reg_read 	= regAccess_extract_arg_large_mix_read;
			extract_routine_reg_write 	= regAccess_extract_arg_large_write;
			remove_raw 					= 1;

			#ifdef VERBOSE
			printf("Extraction routine \"%s\" : %s and %s\n", ARG_NAME_LASOR_LM, ARG_DESC_LASOR, ARG_DESC_LM);
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

	for (j = start; j < stop; j++){
		fragment = (struct traceFragment*)array_get(&(analysis->frag_array), j);
		if (traceFragment_create_mem_array(fragment)){
			printf("ERROR: in %s, unable to create mem array for fragment %u\n", __func__, j);
			break;
		}

		if (traceFragment_create_reg_array(fragment)){
			printf("ERROR: in %s, unable to create reg array for fragement %u\n", __func__, j);
			break;
		}

		regAccess_propagate_read(fragment->read_register_array, fragment->nb_register_read_access);
		regAccess_propagate_write(fragment->write_register_array, fragment->nb_register_write_access);

		if ((fragment->nb_memory_read_access > 0) && (fragment->nb_memory_write_access > 0) && remove_raw){
			traceFragment_remove_read_after_write(fragment);
		}

		if ((fragment->nb_memory_read_access > 0 || fragment->nb_register_read_access > 0) && (fragment->nb_memory_write_access > 0 || fragment->nb_register_write_access > 0)){
			if (argSet_init(&arg_set, fragment->tag)){
				printf("ERROR: in %s, unable to init argSet\n", __func__);
				break;
			}

			if (extract_routine_mem_read(arg_set.input, fragment->read_memory_array, fragment->nb_memory_read_access, fragment)){
				printf("ERROR: in %s, memory read extraction routine return an error code\n", __func__);
			}
			if (extract_routine_mem_write(arg_set.output, fragment->write_memory_array, fragment->nb_memory_write_access, fragment)){
				printf("ERROR: in %s, memory write extraction routine return an error code\n", __func__);
			}
			if (extract_routine_reg_read(arg_set.input, fragment->read_register_array, fragment->nb_register_read_access)){
				printf("ERROR: in %s, register read extraction routine return an error code\n", __func__);
			}
			if (extract_routine_reg_write(arg_set.output, fragment->write_register_array, fragment->nb_register_write_access)){
				printf("ERROR: in %s, register write extraction routine return an error code\n", __func__);
			}

			if (strlen(arg_set.tag) == 0){
				snprintf(arg_set.tag, ARGSET_TAG_MAX_LENGTH, "Frag %u", j);
			}

			if (argSet_sort_output(&arg_set)){
				printf("ERROR: in %s, unable to sort argSet output\n", __func__);
			}

			if (array_add(&(analysis->arg_array), &arg_set) < 0){
				printf("ERROR: in %s, unable to add argument to arg array\n", __func__);
			}
		}
		#ifdef VERBOSE
		else{
			printf("Skipping fragment %u/%u (tag: \"%s\"): no read or write argument\n", j, array_get_length(&(analysis->frag_array)), fragment->tag);
		}
		#endif
	}

	return;

	arg_error:
	printf("Expected extraction specifier:\n");
	printf(" - \"%s\"     : %s and %s\n", 	ARG_NAME_A_LP, 		ARG_DESC_A, 		ARG_DESC_LP);
	printf(" - \"%s\"    : %s and %s\n", 	ARG_NAME_AR_LP, 	ARG_DESC_AR, 		ARG_DESC_LP);
	printf(" - \"%s\"    : %s and %s\n", 	ARG_NAME_AS_LP, 	ARG_DESC_AS, 		ARG_DESC_LP);
	printf(" - \"%s\"   : %s and %s\n", 	ARG_NAME_ASR_LP, 	ARG_DESC_ASR, 		ARG_DESC_LP);
	printf(" - \"%s\"   : %s and %s\n", 	ARG_NAME_ASO_LP, 	ARG_DESC_ASO, 		ARG_DESC_LP);
	printf(" - \"%s\"  : %s and %s\n", 		ARG_NAME_ASOR_LP, 	ARG_DESC_ASOR, 		ARG_DESC_LP);
	printf(" - \"%s\"   : %s and %s\n", 	ARG_NAME_ASR_LM, 	ARG_DESC_ASR, 		ARG_DESC_LM);
	printf(" - \"%s\"  : %s and %s\n", 		ARG_NAME_ASOR_LM, 	ARG_DESC_ASOR, 		ARG_DESC_LM);
	printf(" - \"%s\" : %s and %s\n", 		ARG_NAME_LASOR_LM, 	ARG_DESC_LASOR, 	ARG_DESC_LM);
	return;

	#undef ARG_DESC_A
	#undef ARG_DESC_AR
	#undef ARG_DESC_AS
	#undef ARG_DESC_ASR
	#undef ARG_DESC_ASO
	#undef ARG_DESC_ASOR
	#undef ARG_DESC_LASOR
	#undef ARG_DESC_LP
	#undef ARG_DESC_LM

	#undef ARG_NAME_A_LP
	#undef ARG_NAME_AR_LP
	#undef ARG_NAME_AS_LP
	#undef ARG_NAME_ASR_LP
	#undef ARG_NAME_ASO_LP
	#undef ARG_NAME_ASOR_LP
	#undef ARG_NAME_ASR_LM
	#undef ARG_NAME_ASOR_LM
	#undef ARG_NAME_LASOR_LM
}

/* ===================================================================== */
/* arg functions						                                 */
/* ===================================================================== */

void analysis_arg_clean(struct analysis* analysis){
	uint32_t 				i;

	for (i = 0; i < array_get_length(&(analysis->arg_array)); i++){
		argSet_clean((struct argSet*)array_get(&(analysis->arg_array), i));
	}
	array_empty(&(analysis->arg_array));
}

void analysis_arg_print(struct analysis* analysis, char* arg){
	uint32_t 					index;
	uint32_t 					i;
	struct argSet* 				arg_set;
	struct multiColumnPrinter* 	printer;
	enum argLocationType 		filter_type;
	enum argLocationType* 		filter_type_pointer;
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
				filter_type = ARG_LOCATION_MEMORY;
				filter_type_pointer = &filter_type;
			}
			else if (!strncmp(arg, "reg", i)){
				#ifdef VERBOSE
				printf("Filter REGISTER argBuffer(s) only\n");
				#endif
				filter_type = ARG_LOCATION_REGISTER;
				filter_type_pointer = &filter_type;
			}
			else if (!strncmp(arg, "mix", i)){
				#ifdef VERBOSE
				printf("Filter MIX argBuffer(s) only\n");
				#endif
				filter_type = ARG_LOCATION_MEMORY;
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
		printer = multiColumnPrinter_create(stdout, 10, NULL, NULL, NULL);
		if (printer != NULL){

			multiColumnPrinter_set_column_size(printer, 0, 5);
			multiColumnPrinter_set_column_size(printer, 1, ARGSET_TAG_MAX_LENGTH);
			multiColumnPrinter_set_column_size(printer, 2, 9);
			multiColumnPrinter_set_column_size(printer, 3, 7);
			multiColumnPrinter_set_column_size(printer, 4, 7);
			multiColumnPrinter_set_column_size(printer, 5, 7);
			multiColumnPrinter_set_column_size(printer, 6, 9);
			multiColumnPrinter_set_column_size(printer, 7, 7);
			multiColumnPrinter_set_column_size(printer, 8, 7);
			multiColumnPrinter_set_column_size(printer, 9, 7);

			multiColumnPrinter_set_title(printer, 0, "Index");
			multiColumnPrinter_set_title(printer, 1, "Tag");
			multiColumnPrinter_set_title(printer, 2, "Nb Input");
			multiColumnPrinter_set_title(printer, 3, "I Mem");
			multiColumnPrinter_set_title(printer, 4, "I Reg");
			multiColumnPrinter_set_title(printer, 5, "I Mix");
			multiColumnPrinter_set_title(printer, 6, "Nb Output");
			multiColumnPrinter_set_title(printer, 7, "O Mem");
			multiColumnPrinter_set_title(printer, 8, "O Reg");
			multiColumnPrinter_set_title(printer, 9, "O Mix");

			multiColumnPrinter_set_column_type(printer, 0, MULTICOLUMN_TYPE_UINT32);
			multiColumnPrinter_set_column_type(printer, 2, MULTICOLUMN_TYPE_UINT32);
			multiColumnPrinter_set_column_type(printer, 3, MULTICOLUMN_TYPE_UINT32);
			multiColumnPrinter_set_column_type(printer, 4, MULTICOLUMN_TYPE_UINT32);
			multiColumnPrinter_set_column_type(printer, 5, MULTICOLUMN_TYPE_UINT32);
			multiColumnPrinter_set_column_type(printer, 6, MULTICOLUMN_TYPE_UINT32);
			multiColumnPrinter_set_column_type(printer, 7, MULTICOLUMN_TYPE_UINT32);
			multiColumnPrinter_set_column_type(printer, 8, MULTICOLUMN_TYPE_UINT32);
			multiColumnPrinter_set_column_type(printer, 9, MULTICOLUMN_TYPE_UINT32);

			multiColumnPrinter_print_header(printer);

			for (i = 0; i < array_get_length(&(analysis->arg_array)); i++){
				uint32_t nb_i_mem;
				uint32_t nb_i_reg;
				uint32_t nb_i_mix;
				uint32_t nb_o_mem;
				uint32_t nb_o_reg;
				uint32_t nb_o_mix;

				arg_set = (struct argSet*)array_get(&(analysis->arg_array), i);

				argSet_get_nb_mem(arg_set, &nb_i_mem, &nb_o_mem);
				argSet_get_nb_reg(arg_set, &nb_i_reg, &nb_o_reg);
				argSet_get_nb_mix(arg_set, &nb_i_mix, &nb_o_mix);

				multiColumnPrinter_print(printer, i, arg_set->tag, array_get_length(arg_set->input), nb_i_mem, nb_i_reg, nb_i_mix, array_get_length(arg_set->output), nb_o_mem, nb_o_reg, nb_o_mix, NULL);
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

	if (arg != NULL){
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
	else{
		printf("ERROR: in %s, an index and a tag value must be specified\n", __func__);
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
	ioChecker_start(analysis->checker);
	#endif

	for (i = start; i < stop; i++){
		if (ioChecker_submit_argSet(analysis->checker, (struct argSet*)array_get(&(analysis->arg_array), i))){
			printf("ERROR: in %s, unable to submit argSet to the ioChecker\n", __func__);
		}
	}
	#ifdef VERBOSE
	ioChecker_check(analysis->checker);
	#else
	ioChecker_wait(analysis->checker);
	#endif
}

void analysis_arg_seek(struct analysis* analysis, char* arg){
	uint32_t 			i;
	struct argSet* 		arg_set;
	char* 				buffer;
	uint32_t 			buffer_length;

	if (arg != NULL){
		buffer = readBuffer_raw(arg, strlen(arg));
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
	else{
		printf("ERROR: in %s, please specify an argument\n", __func__);
	}
}