#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "trace.h"
#include "gephiCFGIO.h"


#define ANALYSIS_TYPE_SIMPLE_TRAVERSAL 		"simple_traveral"
#define ANALYSIS_TYPE_PRINT_INSTRUCTIONS	"print_instructions"
#define ANALYSIS_TYPE_PRINT_SIMPLETRACESTAT	"print_simpleTraceStat"
#define ANALYSIS_TYPE_PRINT_CODEMAP			"print_codeMap"
#define ANALYSIS_TYPE_BUILD_CFG				"build_cfg"
#define ANALYSIS_TYPE_BUILD_CALLTREE		"build_callTree"

static void analysis_print_command();

int main(int argc, char** argv){
	struct trace* 				ptrace	= NULL;
	
	if	(argc != 3){
		printf("ERROR: in %s, please specify the trace directory as first argument\n", __func__);
		printf("ERROR: in %s, please specify the analysis type as second argument\n", __func__);
		return 0;
	}
	
	ptrace = trace_create(argv[1]);
	if (ptrace == NULL){
		printf("ERROR: in %s, unable to create the trace\n", __func__);
	}
	
	if (!strcmp(argv[2], ANALYSIS_TYPE_SIMPLE_TRAVERSAL)){
		trace_simple_traversal(ptrace);
	}
	else if (!strcmp(argv[2], ANALYSIS_TYPE_PRINT_INSTRUCTIONS)){
		trace_print_instructions(ptrace);
	}
	else if (!strcmp(argv[2], ANALYSIS_TYPE_PRINT_SIMPLETRACESTAT)){
		trace_print_simpleTraceStat(ptrace);
	}
	else if (!strcmp(argv[2], ANALYSIS_TYPE_PRINT_CODEMAP)){
		trace_print_codeMap(ptrace);
	}
	else if (!strcmp(argv[2], ANALYSIS_TYPE_BUILD_CFG)){
		struct controlFlowGraph* cfg = trace_construct_flow_graph(ptrace);
		gephiCFGIO_print(cfg, NULL);
		controlFlowGraph_delete(cfg);
	}
	else if (!strcmp(argv[2], ANALYSIS_TYPE_BUILD_CALLTREE)){
		struct graph* callTree  =trace_construct_call_tree(ptrace);
		/* we can print the graph here */
		graph_delete(callTree);
	}
	else{
		printf("ERROR: in %s, unknown analysis type: %s\n", __func__, argv[2]);
		analysis_print_command();
	}

	trace_delete(ptrace);
	
	return 0;
}

static void analysis_print_command(){
	printf("*** Analysis Commands ***\n");
	printf("\t%s\n", ANALYSIS_TYPE_SIMPLE_TRAVERSAL);
	printf("\t%s\n", ANALYSIS_TYPE_PRINT_INSTRUCTIONS);	
	printf("\t%s\n", ANALYSIS_TYPE_PRINT_SIMPLETRACESTAT);
	printf("\t%s\n", ANALYSIS_TYPE_PRINT_CODEMAP);
	printf("\t%s\n", ANALYSIS_TYPE_BUILD_CFG);
	printf("\t%s\n", ANALYSIS_TYPE_BUILD_CALLTREE);
}