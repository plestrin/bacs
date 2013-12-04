#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "trace.h"
#include "inputParser.h"

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
	if(inputParser_add_cmd(parser, "print ioChecker", "Display the ioChecker structure", trace->checker, (void(*)(void*))ioChecker_print)){
		printf("ERROR: in %s unable to add cmd to inputParser\n", __func__);
	}
	if(inputParser_add_cmd(parser, "test ioChecker", "User define test on the ioChecker (CAUTION - debug only)", trace->checker, (void(*)(void*))ioChecker_handmade_test)){ /* DEV */
		printf("ERROR: in %s unable to add cmd to inputParser\n", __func__);
	}

	/* codeMap specific commands */
	if(inputParser_add_cmd(parser, "check codeMap", "Perform basic checks on the codeMap address", trace->code_map, (void(*)(void*))codeMap_check_address)){
		printf("ERROR: in %s unable to add cmd to inputParser\n", __func__);
	}
	if(inputParser_add_cmd(parser, "print codeMap", "Print the codeMap (informations about routine address)", trace, (void(*)(void*))trace_codeMap_print)){ /* it would be great to select a filter from the command line */
		printf("ERROR: in %s unable to add cmd to inputParser\n", __func__);
	}

	/* trace specific commands */
	if(inputParser_add_cmd(parser, "print trace", "Print all the instructions of the trace", trace, (void(*)(void*))trace_instructions_print)){
		printf("ERROR: in %s unable to add cmd to inputParser\n", __func__);
	}

	/* simpleTraceStat specific commands */
	if(inputParser_add_cmd(parser, "create simpleTraceStat", "Create a simpleTraceStat (holds synthetic informations about instructions in the trace)", trace, (void(*)(void*))trace_simpleTraceStat_create)){
		printf("ERROR: in %s unable to add cmd to inputParser\n", __func__);
	}
	if(inputParser_add_cmd(parser, "print simpleTraceStat", "Print a previously created simpleTraceStat", trace, (void(*)(void*))trace_simpleTraceStat_print)){
		printf("ERROR: in %s unable to add cmd to inputParser\n", __func__);
	}	
	if(inputParser_add_cmd(parser, "delete simpleTraceStat", "Delete a previously created simpleTraceStat", trace, (void(*)(void*))trace_simpleTraceStat_delete)){
		printf("ERROR: in %s unable to add cmd to inputParser\n", __func__);
	}

	/* callTree specific commands */
	if(inputParser_add_cmd(parser, "create callTree", "Create the routine call tree", trace, (void(*)(void*))trace_callTree_create)){
		printf("ERROR: in %s unable to add cmd to inputParser\n", __func__);
	}
	if(inputParser_add_cmd(parser, "print dot callTree", "Print the callTree in the DOT format to specified file", trace, (void(*)(void*))trace_simpleTraceStat_delete)){ /*specified file*/
		printf("ERROR: in %s unable to add cmd to inputParser\n", __func__);
	}
	if(inputParser_add_cmd(parser, "print callTree opcode percent", "For each traceFragment in the callTree, print its special instruction percent", trace, (void(*)(void*))trace_callTree_print_opcode_percent)){ /* modify later */
		printf("ERROR: in %s unable to add cmd to inputParser\n", __func__);
	}
	if(inputParser_add_cmd(parser, "test callTree", "User define test on the ioChecker (CAUTION - debug only)", trace, (void(*)(void*))trace_callTree_handmade_test)){ /* DEV */
		printf("ERROR: in %s unable to add cmd to inputParser\n", __func__);
	}
	if(inputParser_add_cmd(parser, "delete callTree", "Print the codeMap (informations about routine address)", trace, (void(*)(void*))trace_codeMap_print)){
		printf("ERROR: in %s unable to add cmd to inputParser\n", __func__);
	}

	inputParser_exe(parser);

	exit:

	trace_delete(trace);
	inputParser_delete(parser);

	return 0;
}