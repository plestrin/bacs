#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "trace.h"
#include "ioChecker.h"
#include "gephiCFGIO.h"			/* gephi is a crap and must be destroy with fire */
#include "graphPrintDot.h"		/* maybe delete later */
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
	struct trace* 				trace	= NULL;
	struct ioChecker*			checker = NULL;
	
	if	(argc != 3){
		printf("ERROR: in %s, please specify the trace directory as first argument\n", __func__);
		printf("ERROR: in %s, please specify the analysis type as second argument\n", __func__);
		return 0;
	}

	checker = ioChecker_create();
	if (checker == NULL){
		printf("ERROR: in %s, unable to create ioChecker\n", __func__);
	}
	
	trace = trace_create(argv[1]);
	if (trace == NULL){
		printf("ERROR: in %s, unable to create the trace\n", __func__);
	}


	if (trace != NULL && checker != NULL){
		if (!strcmp(argv[2], ANALYSIS_TYPE_SIMPLE_TRAVERSAL)){
			trace_simple_traversal(trace);
		}
		else if (!strcmp(argv[2], ANALYSIS_TYPE_PRINT_INSTRUCTIONS)){
			trace_print_instructions(trace);
		}
		else if (!strcmp(argv[2], ANALYSIS_TYPE_PRINT_SIMPLETRACESTAT)){
			trace_print_simpleTraceStat(trace);
		}
		else if (!strcmp(argv[2], ANALYSIS_TYPE_PRINT_CODEMAP)){
			trace_print_codeMap(trace);
		}
		else if (!strcmp(argv[2], ANALYSIS_TYPE_BUILD_CFG)){
			struct controlFlowGraph* cfg = trace_construct_flow_graph(trace);
			gephiCFGIO_print(cfg, NULL);
			controlFlowGraph_delete(cfg);
		}
		else if (!strcmp(argv[2], ANALYSIS_TYPE_BUILD_CALLTREE)){
			struct graph* callTree = trace_construct_callTree(trace);
			if (graphPrintDot_print(callTree, "callTree.dot")){
				printf("ERROR: in %s, unable to print call tree to dot format\n", __func__);
			}
			graph_delete(callTree);
		}
		else if(!strcmp(argv[2], ANALYSIS_TYPE_PRINT_RTN_OPCODE_PERCENT)){
			struct graph* callTree = trace_construct_callTree(trace);
			callTree_print_opcode_percent(callTree);
			graph_delete(callTree);
		}
		/* Warning this section is tmp - only for dev purpose - do not run on general input */
		else if (!strcmp(argv[2], ANALYSIS_TYPE_DEV)){
			struct graph* 			callTree;
			struct callTree_node* 	node;
			struct array* 			read_mem_arg;
			struct array* 			write_mem_arg;
			uint32_t 				i;
			struct argBuffer* 		arg;

			callTree = trace_construct_callTree(trace);

			node = (struct callTree_node*)callTree->nodes[20].data;

			/* Make sur to select the crypto  node */
			printf("Node %d routine name: %s\n", 20, node->name);

			traceFragment_create_mem_array(&(node->fragment));

			traceFragment_print_mem_array(node->fragment.write_memory_array, node->fragment.nb_memory_write_access);

			printf("Nb mem read access:  %d\n", node->fragment.nb_memory_read_access);
			printf("Nb mem write access: %d\n", node->fragment.nb_memory_write_access);

			read_mem_arg = traceFragment_extract_mem_arg_adjacent(node->fragment.read_memory_array, node->fragment.nb_memory_read_access);
			write_mem_arg = traceFragment_extract_mem_arg_adjacent(node->fragment.write_memory_array, node->fragment.nb_memory_write_access);

			printf("Nb read mem arg: %u\n", array_get_length(read_mem_arg));

			for (i = 0; i < array_get_length(read_mem_arg); i++){
				arg = (struct argBuffer*)array_get(read_mem_arg, i);
				argBuffer_print_raw(arg);
				free(arg->data);
			}

			printf("Nb write mem arg: %u\n", array_get_length(write_mem_arg));

			for (i = 0; i < array_get_length(write_mem_arg); i++){
				arg = (struct argBuffer*)array_get(write_mem_arg, i);
				argBuffer_print_raw(arg);
				free(arg->data);
			}

			array_delete(read_mem_arg);
			array_delete(write_mem_arg);

			graph_delete(callTree);
		}
		/* Warning this section is tmp - only for dev purpose - do not run on general input */
		else if (!strcmp(argv[2], ANALYSIS_TYPE_IO)){
			struct argBuffer txt;
			struct argBuffer hash;
			unsigned char hash_val[16] = {0x3e, 0x25, 0x96, 0x0a, 0x79, 0xdb, 0xc6, 0x9b, 0x67, 0x4c, 0xd4, 0xec, 0x67, 0xa7, 0x2c, 0x62};
			unsigned char txt_val[64] = {0x48, 0x65, 0x6c, 0x6c, 0x6f, 0x20, 0x77, 0x6f, 0x72, 0x6c, 0x64, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x58, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
			struct primitiveReference* prim;

			txt.location_type = ARG_LOCATION_MEMORY;
			txt.location.address = 1;
			txt.size = 64;
			txt.data = (char*)malloc(64); /* srry but we don't check allocation */
			memcpy(txt.data, txt_val, 64);

			hash.location_type = ARG_LOCATION_MEMORY;
			hash.location.address = 2;
			hash.size = 16;
			hash.data  =(char*)malloc(16); /* srry but we don't check allocation */
			memcpy(hash.data, hash_val, 16); 

			argBuffer_print_raw(&txt);
			argBuffer_print_raw(&hash);

			prim = (struct primitiveReference*)array_get(&(checker->reference_array), 0);

			primitiveReference_print(prim);

			if (primitiveReference_test(prim, 1, 1, &(txt), &(hash)) == 0){
				printf("SUCCESS!\n");
			}
			else{
				printf("FAILURE!\n");
			}

			free(txt.data);
			free(hash.data);
		}
		else{
			printf("ERROR: in %s, unknown analysis type: %s\n", __func__, argv[2]);
			analysis_print_command();
		}
	}

	trace_delete(trace);
	ioChecker_delete(checker);
	
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