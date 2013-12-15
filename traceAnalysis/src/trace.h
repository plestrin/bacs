#ifndef TRACE_H
#define TRACE_H

#include "codeMap.h"
#include "traceReaderJSON.h"
#include "cmReaderJSON.h"
#include "callTree.h"
#include "graph.h"
#include "simpleTraceStat.h"
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
	struct simpleTraceStat* 	simple_trace_stat;
	struct graph* 				call_tree;
	struct loopEngine*			loop_engine;
};

struct trace* trace_create(const char* dir_name);

void trace_instruction_print(struct trace* trace);

void trace_simpleTraceStat_create(struct trace* trace);
void trace_simpleTraceStat_print(struct trace* trace);
void trace_simpleTraceStat_delete(struct trace* trace);

/* concernant le codefragment il faudrait faire un object général indépendant de la méthode de génération */
/* ce qui permettrait de resortir le io checker de la trace ce qui n'a pas de raison d'être d'un point de vue structurel */

void trace_callTree_create(struct trace* trace);
void trace_callTree_print_dot(struct trace* trace, char* file_name);
void trace_callTree_print_opcode_percent(struct trace* trace); 	/* il faudrait faire un iterateur sur les tracefragments contenus dans le callTree */
void trace_callTree_bruteForce(struct trace* trace); 			/* idem */
void trace_callTree_handmade_test(struct trace* trace); 		/* This is a debuging routine */
void trace_callTree_delete(struct trace* trace);

void trace_loop_create(struct trace* trace);
void trace_loop_remove_redundant(struct trace* trace);
void trace_loop_pack_epilogue(struct trace* trace);
void trace_loop_print(struct trace* trace);
void trace_loop_delete(struct trace* trace);

void trace_delete(struct trace* trace);

#endif