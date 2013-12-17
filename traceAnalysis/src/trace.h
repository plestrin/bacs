#ifndef TRACE_H
#define TRACE_H

#include "array.h"
#include "codeMap.h"
#include "traceReaderJSON.h"
#include "cmReaderJSON.h"
#include "callTree.h"
#include "graph.h"
#include "ioChecker.h"
#include "loop.h"

#define TRACE_DIRECTORY_NAME_MAX_LENGTH 256
#define TRACE_INS_FILE_NAME "ins.json"
#define TRACE_CM_FILE_NAME "cm.json"

struct trace{
	char directory_name[TRACE_DIRECTORY_NAME_MAX_LENGTH];

	union {
		struct traceReaderJSON 	json;
	} 							ins_reader;

	struct ioChecker*			checker;
	struct codeMap* 			code_map;
	struct graph* 				call_tree;
	struct loopEngine*			loop_engine;
	struct array				frag_array;
	struct array 				arg_array;
};

struct trace* trace_create(const char* dir_name);

void trace_instruction_print(struct trace* trace);

/* faire un object indépendant pour stocker les arguments candidats- à la manière de frag le but test double
 * - d'une part permettre plusieurs méthode de génération d'arguments
 * - d'autre part vérifier les résultats issus de cette phase à l'aide de méthodes print dédiées */

void trace_callTree_create(struct trace* trace);
void trace_callTree_print_dot(struct trace* trace, char* file_name);
void trace_callTree_print_opcode_percent(struct trace* trace); /* déplacer cette méthode sur les traceFragments */
void trace_callTree_export(struct trace* trace);
void trace_callTree_delete(struct trace* trace);

void trace_loop_create(struct trace* trace);
void trace_loop_remove_redundant(struct trace* trace);
void trace_loop_pack_epilogue(struct trace* trace);
void trace_loop_print(struct trace* trace);
void trace_loop_export(struct trace* trace);
void trace_loop_delete(struct trace* trace);

void trace_frag_clean(struct trace* trace);
void trace_frag_print_stat(struct trace* trace, char* arg);
void trace_frag_extract_arg(struct trace* trace, char* arg);

void trace_arg_clean(struct trace* trace);
void trace_arg_print(struct trace* trace);
void trace_arg_search(struct trace* trace, char* arg);

void trace_delete(struct trace* trace);

#endif