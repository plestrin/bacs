#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "trace.h"
#include "inputParser.h"

#define ADD_CMD_TO_INPUT_PARSER(parser, cmd, desc, interac, arg, func)									\
	{																									\
		if (inputParser_add_cmd((parser), (cmd), (desc), (interac), (arg), (void(*)(void))(func))){		\
			printf("ERROR: in %s, unable to add cmd: \"%s\" to inputParser\n", __func__, (cmd));		\
		}																								\
	}

int main(int argc, char** argv){
	struct trace* 				trace	= NULL;
	struct inputParser* 		parser 	= NULL;
	
	if	(argc < 2){
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
	ADD_CMD_TO_INPUT_PARSER(parser, "print ioChecker", "Display the ioChecker structure", INPUTPARSER_CMD_NOT_INTERACTIVE, trace->checker, ioChecker_print)

	/* codeMap specific commands */
	ADD_CMD_TO_INPUT_PARSER(parser, "check codeMap", "Perform basic checks on the codeMap address", INPUTPARSER_CMD_NOT_INTERACTIVE, trace->code_map, codeMap_check_address)
	ADD_CMD_TO_INPUT_PARSER(parser, "print codeMap", "Print the codeMap (informations about routine address). Specify a filter as second arg", INPUTPARSER_CMD_INTERACTIVE, trace->code_map, codeMap_print)

	/* trace specific commands */
	ADD_CMD_TO_INPUT_PARSER(parser, "print trace", "Print all the instructions of the trace", INPUTPARSER_CMD_NOT_INTERACTIVE, trace, trace_instruction_print)
	ADD_CMD_TO_INPUT_PARSER(parser, "export trace", "Export the whole trace as traceFragment", INPUTPARSER_CMD_NOT_INTERACTIVE, trace, trace_instruction_export)

	/* callTree specific commands */
	ADD_CMD_TO_INPUT_PARSER(parser, "create callTree", "Create the routine callTree", INPUTPARSER_CMD_NOT_INTERACTIVE, trace, trace_callTree_create)
	ADD_CMD_TO_INPUT_PARSER(parser, "print dot callTree", "Print the callTree in the DOT format to a file. Specify file name as second arg", INPUTPARSER_CMD_INTERACTIVE, trace, trace_callTree_print_dot)
	ADD_CMD_TO_INPUT_PARSER(parser, "export callTree", "Export the traceFragment(s) of the callTree to the traceFragement array", INPUTPARSER_CMD_NOT_INTERACTIVE, trace, trace_callTree_export)
	ADD_CMD_TO_INPUT_PARSER(parser, "delete callTree", "Delete a previously create callTree", INPUTPARSER_CMD_NOT_INTERACTIVE, trace, trace_callTree_delete)

	/* loop specific commands */
	ADD_CMD_TO_INPUT_PARSER(parser, "create loop", "Create a loopEngine and parse the trace looking for A^n pattern", INPUTPARSER_CMD_NOT_INTERACTIVE, trace, trace_loop_create)
	ADD_CMD_TO_INPUT_PARSER(parser, "remove redundant loop", "Remove the redundant loops from the loops list", INPUTPARSER_CMD_NOT_INTERACTIVE, trace, trace_loop_remove_redundant)
	ADD_CMD_TO_INPUT_PARSER(parser, "pack epilogue", "Pack loop epilogue to reduce the number of loops (must be run after \"remove redundant loop\")", INPUTPARSER_CMD_NOT_INTERACTIVE, trace, trace_loop_pack_epilogue)
	ADD_CMD_TO_INPUT_PARSER(parser, "print loop", "Print every loops contained in the loopEngine", INPUTPARSER_CMD_NOT_INTERACTIVE, trace, trace_loop_print)
	ADD_CMD_TO_INPUT_PARSER(parser, "export loop", "Export loop(s) to traceFragment array", INPUTPARSER_CMD_NOT_INTERACTIVE, trace, trace_loop_export)
	ADD_CMD_TO_INPUT_PARSER(parser, "delete loop", "Delete a previously created loopEngine (loop)", INPUTPARSER_CMD_NOT_INTERACTIVE, trace, trace_loop_delete)

	/* traceFragement specific commands */
	ADD_CMD_TO_INPUT_PARSER(parser, "clean frag", "Clean the traceFragment array", INPUTPARSER_CMD_NOT_INTERACTIVE, trace, trace_frag_clean)
	ADD_CMD_TO_INPUT_PARSER(parser, "print frag stat", "Print simpleTraceStat about the traceFragment. Specify an index as second arg", INPUTPARSER_CMD_INTERACTIVE, trace, trace_frag_print_stat)
	ADD_CMD_TO_INPUT_PARSER(parser, "print frag ins", "Print instructions of the traceFragment. Specify an index as second arg [mandatory]", INPUTPARSER_CMD_INTERACTIVE, trace, trace_frag_print_ins)
	ADD_CMD_TO_INPUT_PARSER(parser, "print frag percent", "Print for every traceFragments some stat about instructions frequency", INPUTPARSER_CMD_NOT_INTERACTIVE, trace, trace_frag_print_percent)
	ADD_CMD_TO_INPUT_PARSER(parser, "print frag register", "Print for every traceFragments input and output registers. Specify an index as second arg [mandatory]", INPUTPARSER_CMD_INTERACTIVE, trace, trace_frag_print_register)
	ADD_CMD_TO_INPUT_PARSER(parser, "set frag tag", "Set tag value for specific fragment. Specify frag index and tag value [respect order]", INPUTPARSER_CMD_INTERACTIVE, trace, trace_frag_set_tag)
	ADD_CMD_TO_INPUT_PARSER(parser, "extract frag arg", "Extract input and output argument(s) from the traceFragment. Specify extraction routine [mandatory] and index", INPUTPARSER_CMD_INTERACTIVE, trace, trace_frag_extract_arg)

	/* argument specific commands */
	ADD_CMD_TO_INPUT_PARSER(parser, "clean arg", "Clean the argSet array", INPUTPARSER_CMD_NOT_INTERACTIVE, trace, trace_arg_clean)
	ADD_CMD_TO_INPUT_PARSER(parser, "print arg", "Print arguments from the argSet array. Specify an index as second arg", INPUTPARSER_CMD_INTERACTIVE, trace, trace_arg_print)
	ADD_CMD_TO_INPUT_PARSER(parser, "set arg tag", "Set tag value for specific argSet. Specify arg index and tag value [respect order]", INPUTPARSER_CMD_INTERACTIVE, trace, trace_arg_set_tag)
	ADD_CMD_TO_INPUT_PARSER(parser, "fragment arg", "Fragment argument. Specify arg index and tag value", INPUTPARSER_CMD_INTERACTIVE, trace, trace_arg_fragment)
	ADD_CMD_TO_INPUT_PARSER(parser, "create argSetGraph", "Create a dependency graph between argSets", INPUTPARSER_CMD_NOT_INTERACTIVE, trace, trace_arg_create_argSetGraph)
	ADD_CMD_TO_INPUT_PARSER(parser, "print dot argSetGraph", "Print the argSetGraph in the DOT format. Specify file name as second arg", INPUTPARSER_CMD_INTERACTIVE, trace, trace_arg_print_dot_argSetGraph)
	ADD_CMD_TO_INPUT_PARSER(parser, "pack arg", "Pack arguments that are dependent according to the argSetGraph. Specify packing method [mandatory] and index", INPUTPARSER_CMD_INTERACTIVE, trace, trace_arg_pack)
	ADD_CMD_TO_INPUT_PARSER(parser, "delete argSetGraph", "Delete a previously created argSetGraph", INPUTPARSER_CMD_NOT_INTERACTIVE, trace, trace_arg_delete_argSetGraph)
	ADD_CMD_TO_INPUT_PARSER(parser, "search arg", "Search every element in the argSet array. Specify an index as second arg", INPUTPARSER_CMD_INTERACTIVE, trace, trace_arg_search)
	ADD_CMD_TO_INPUT_PARSER(parser, "seek arg", "Seek for a user defined argBuffer in the argSet array. Specify the buffer as second arg (raw format)", INPUTPARSER_CMD_INTERACTIVE, trace, trace_arg_seek)


	inputParser_exe(parser, argc - 2, argv + 2);

	exit:

	trace_delete(trace);
	inputParser_delete(parser);

	return 0;
}