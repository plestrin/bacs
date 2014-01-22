#ifndef TRACEREADERJSON_H
#define TRACEREADERJSON_H

#include <yajl/yajl_parse.h>

#include "instruction.h"

#define	TRACEREADERJSON_NB_INSTRUCTION_IN_CACHE	1024
#define TRACEREADERJSON_MMAP_MEMORY_SIZE		4096

enum trace_json_map_key{
	TRACE_JSON_MAP_KEY_IDLE,
	TRACE_JSON_MAP_KEY_TRACE,
	TRACE_JSON_MAP_KEY_PC,
	TRACE_JSON_MAP_KEY_INS,
	TRACE_JSON_MAP_KEY_DATA_READ,
	TRACE_JSON_MAP_KEY_DATA_WRITE,
	TRACE_JSON_MAP_KEY_DATA_MEM,
	TRACE_JSON_MAP_KEY_DATA_REG,
	TRACE_JSON_MAP_KEY_DATA_SIZE,
	TRACE_JSON_MAP_KEY_DATA_VAL,
	TRACE_JSON_MAP_KEY_UNKNOWN
};

struct traceReaderJSON{
	int 					file;
	long 					file_length;
	long 					read_offset;
	void*					buffer;
	int 					buffer_length;

	yajl_handle 			json_parser_handle;
	enum trace_json_map_key actual_key;
	int 					actual_map_level;
	int 					actual_instruction_data_offset;
	enum trace_json_map_key actual_data_specifier;

	long 					instruction_cursor;
	struct instruction 		instruction_cache[TRACEREADERJSON_NB_INSTRUCTION_IN_CACHE];
	unsigned int 			cache_bot_offset;
	unsigned int 			cache_top_offset;
	long 					cache_bot_cursor;
	long					cache_top_cursor;
};

int traceReaderJSON_init(struct traceReaderJSON* trace_reader, const char* file_name);
int traceReaderJSON_reset(struct traceReaderJSON* trace_reader);
struct instruction* traceReaderJSON_get_next_instruction(struct traceReaderJSON* trace_reader);
void traceReaderJSON_clean(struct traceReaderJSON* trace_reader);

#endif