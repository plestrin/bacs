#ifndef TRACE_H
#define TRACE_H

#include "traceReaderJSON.h"
#include "cmReaderJSON.h"
#include "controlFlowGraph.h"

#define TRACE_DIRECTORY_NAME_MAX_LENGTH 256
#define TRACE_INS_FILE_NAME "ins.json"
#define TRACE_CM_FILE_NAME "cm.json"

struct trace{
	char directory_name[TRACE_DIRECTORY_NAME_MAX_LENGTH];

	union {
		struct traceReaderJSON json;
	} ins_reader;
};

struct trace* trace_create(const char* dir_name);

void trace_simple_traversal(struct trace* ptrace);
void trace_print_instructions(struct trace* ptrace);
void trace_print_simpleTraceStat(struct trace* ptrace);
struct controlFlowGraph* trace_construct_flow_graph(struct trace* ptrace);

void trace_delete(struct trace* ptrace);

#endif