#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "trace.h"
#include "inputParser.h"

#define ADD_CMD_TO_INPUT_PARSER(parser, cmd, desc, arg, func)										\
	{																								\
		if (inputParser_add_cmd((parser), (cmd), (desc), (arg), (void(*)(void*))(func))){			\
			printf("ERROR: in %s unable to add cmd: \"%s\" to inputParser\n", __func__, (cmd));		\
		}																							\
	}

int main(int argc, char** argv){
	struct trace* 				trace	= NULL;
	struct inputParser* 		parser 	= NULL;
	
	if	(argc != 2){
		printf("ERROR: in %s, please specify the trace directory as first argument\n", __func__);
		goto exit;
	}

	parser = inputParser_create();
	if (parser == NULL){
		printf("ERROR: in %s, unable to create inputParser\n", __func__);
		goto exit;
	}
	
	trace = trace_create(argv[1]);
	if (trace == NULL){
		printf("ERROR: in %s, unable to create the trace\n", __func__);
		goto exit;
	}

	/* ioChecker specific commands*/
	ADD_CMD_TO_INPUT_PARSER(parser, "print ioChecker", "Display the ioChecker structure", trace->checker, ioChecker_print)
	ADD_CMD_TO_INPUT_PARSER(parser, "test ioChecker", "User define test on the ioChecker (CAUTION - debug only)", trace->checker, ioChecker_handmade_test) /* DEV */

	/* codeMap specific commands */
	ADD_CMD_TO_INPUT_PARSER(parser, "check codeMap", "Perform basic checks on the codeMap address", trace->code_map, codeMap_check_address)
	ADD_CMD_TO_INPUT_PARSER(parser, "print codeMap", "Print the codeMap (informations about routine address)", trace, trace_codeMap_print) /* it would be great to select a filter from the command line - no need to pass through trace */

	/* trace specific commands */
	ADD_CMD_TO_INPUT_PARSER(parser, "print trace", "Print all the instructions of the trace", trace, trace_instruction_print)

	/* simpleTraceStat specific commands */
	ADD_CMD_TO_INPUT_PARSER(parser, "create simpleTraceStat", "Create a simpleTraceStat (holds synthetic informations about instructions in the trace)", trace, trace_simpleTraceStat_create)
	ADD_CMD_TO_INPUT_PARSER(parser, "print simpleTraceStat", "Print a previously created simpleTraceStat", trace, trace_simpleTraceStat_print)
	ADD_CMD_TO_INPUT_PARSER(parser, "delete simpleTraceStat", "Delete a previously created simpleTraceStat", trace, trace_simpleTraceStat_delete)

	/* callTree specific commands */
	ADD_CMD_TO_INPUT_PARSER(parser, "create callTree", "Create the routine callTree", trace, trace_callTree_create)
	ADD_CMD_TO_INPUT_PARSER(parser, "print dot callTree", "Print the callTree in the DOT format to specified file", trace, trace_callTree_print_dot) /*specified file*/
	ADD_CMD_TO_INPUT_PARSER(parser, "print callTree opcode percent", "For each traceFragment in the callTree, print its special instruction percent", trace, trace_callTree_print_opcode_percent) /* modify later */
	ADD_CMD_TO_INPUT_PARSER(parser, "search callTree", "Search crypto primitives in every routine of the callTree", trace, trace_callTree_bruteForce) /* modify later */
	ADD_CMD_TO_INPUT_PARSER(parser, "test callTree", "User define test on the callTree (CAUTION - debug only)", trace, trace_callTree_handmade_test) /* DEV */
	ADD_CMD_TO_INPUT_PARSER(parser, "delete callTree", "Delete a previously create callTree", trace, trace_callTree_delete)

	/* loop specific commands */
	ADD_CMD_TO_INPUT_PARSER(parser, "create loop", "Create a loopEngine and parse the trace looking for A^n pattern", trace, trace_loop_create)
	ADD_CMD_TO_INPUT_PARSER(parser, "print loop", "Print every loops contained in the loopEngine", trace, trace_loop_print)
	ADD_CMD_TO_INPUT_PARSER(parser, "delete loop", "Delete a previously created loopEngine (loop)", trace, trace_loop_delete)

	inputParser_exe(parser);

	exit:

	trace_delete(trace);
	inputParser_delete(parser);

	return 0;
}