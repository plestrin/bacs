#ifndef TRACE_H
#define TRACE_H

#include "codeMap.h"
#include "traceReaderJSON.h"
#include "cmReaderJSON.h"
#include "callTree.h"
#include "graph.h"
#include "simpleTraceStat.h"
#include "ioChecker.h"

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
};

struct trace* trace_create(const char* dir_name);

void trace_instructions_print(struct trace* trace);

void trace_simpleTraceStat_create(struct trace* trace);
void trace_simpleTraceStat_print(struct trace* trace);
void trace_simpleTraceStat_delete(struct trace* trace);

void trace_codeMap_print(struct trace* trace);

void trace_callTree_create(struct trace* trace);
void trace_callTree_print_dot(struct trace* trace);
void trace_callTree_print_opcode_percent(struct trace* trace); 	/* il faudrait faire un iterateur sur les tracefragments contenus dans le callTree */
void trace_callTree_bruteForce(struct trace* trace); 			/* idem */
void trace_callTree_handmade_test(struct trace* trace); 		/* This is a debuging routine */
void trace_callTree_delete(struct trace* trace);


void trace_delete(struct trace* trace);

#endif