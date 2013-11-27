#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "trace.h"
#include "gephiCFGIO.h"
#include "graphPrintDot.h"
#include "argBuffer.h" 			/* delete later*/
#include "primitiveReference.h" /* maybe delete later */


#define ANALYSIS_TYPE_SIMPLE_TRAVERSAL 			"simple_traveral"
#define ANALYSIS_TYPE_PRINT_INSTRUCTIONS		"print_instructions"
#define ANALYSIS_TYPE_PRINT_SIMPLETRACESTAT		"print_simpleTraceStat"
#define ANALYSIS_TYPE_PRINT_CODEMAP				"print_codeMap"
#define ANALYSIS_TYPE_BUILD_CFG					"build_cfg"
#define ANALYSIS_TYPE_BUILD_CALLTREE			"build_callTree"
#define ANALYSIS_TYPE_PRINT_RTN_OPCODE_PERCENT	"print_rtn_opcode_percent"
#define ANALYSIS_TYPE_DEV 						"dev" 	/* this is garbage just for the dev */
#define ANALYSIS_TYPE_IO 						"io" 	/* this is garbage just for the dev */

static void analysis_print_command();

/* Ca serait bien de faire quelque chose d'interactif - pour pouvoir construire et printer dans une même session */

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
		struct graph* callTree = trace_construct_callTree(ptrace);
		if (graphPrintDot_print(callTree, "callTree.dot")){
			printf("ERROR: in %s, unable to print call tree to dot format\n", __func__);
		}
		graph_delete(callTree);
	}
	else if(!strcmp(argv[2], ANALYSIS_TYPE_PRINT_RTN_OPCODE_PERCENT)){
		struct graph* callTree = trace_construct_callTree(ptrace);
		callTree_print_opcode_percent(callTree);
		graph_delete(callTree);
	}
	/* Warning this section is tmp - only for dev purpose - do not run on general input */
	else if (!strcmp(argv[2], ANALYSIS_TYPE_DEV)){
		struct graph* 			callTree;
		struct callTree_node* 	node;
		struct array* 			read_mem_arg;
		uint32_t 				i;
		struct argBuffer* 		arg;

		callTree = trace_construct_callTree(ptrace);

		node = (struct callTree_node*)callTree->nodes[20].data;

		/* Make sur to select the crypto  node */
		printf("Node %d routine name: %s\n", 20, node->name);

		traceFragment_create_mem_array(&(node->fragment));

		printf("Nb mem read access: %d\n", node->fragment.nb_memory_read_access);

		read_mem_arg = traceFragement_extract_mem_arg_adjacent(node->fragment.read_memory_array, node->fragment.nb_memory_read_access);

		printf("Nb read mem arg: %u\n", array_get_length(read_mem_arg));

		for (i = 0; i < array_get_length(read_mem_arg); i++){
			arg = (struct argBuffer*)array_get(read_mem_arg, i);
			argBuffer_print_raw(arg);
			free(arg->data);
		}

		array_delete(read_mem_arg);

		graph_delete(callTree);
	}
	/* Warning this section is tmp - only for dev purpose - do not run on general input */
	else if (!strcmp(argv[2], ANALYSIS_TYPE_IO)){
		struct array* 				reference_array;
		struct primitiveReference	primitive;
		struct primitiveReference* 	pointer;
		uint32_t 					i;

		reference_array = array_create(sizeof(struct primitiveReference));
		if (reference_array == NULL){
			printf("ERROR: in %s, unable to create array\n", __func__);
		}
		else{
			uint32_t input_specifier[2];
			uint32_t output_specifier[1];

			PRIMITIVEREFERENCE_ARG_SPECIFIER_SET_SIZE_MULTIPLE(input_specifier[0], 64); /* a remplacer avec les définition du md5 */
			PRIMITIVEREFERENCE_ARG_SPECIFIER_SET_IMPLICIT_SIZE(input_specifier[1], 0);
			PRIMITIVEREFERENCE_ARG_SPECIFIER_SET_SIZE_EXACT_VALUE(output_specifier[0], 16); /* a remplacer avec les définitions du md5 */


			if (primitiveReference_init(&primitive, "md5", 2, 1, input_specifier, output_specifier, NULL)){
				printf("ERROR: in %s, unable to init primitive reference structure\n", __func__);
			}
			else{
				if (array_add(reference_array, &primitive)){
					printf("ERROR: in %s, unable to add element to array\n", __func__);
				}
			}

			for (i = 0; i < array_get_length(reference_array); i++){
				pointer = (struct primitiveReference*)array_get(reference_array, i);
				primitiveReference_print(pointer);
				primitiveReference_clean(pointer);
			}

			array_delete(reference_array);
		}

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
	printf("\t%s\n", ANALYSIS_TYPE_PRINT_RTN_OPCODE_PERCENT);
	printf("\t%s * WARNING *\n", ANALYSIS_TYPE_DEV);
	printf("\t%s * WARNING *\n", ANALYSIS_TYPE_IO);
}