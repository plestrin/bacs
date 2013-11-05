#ifndef CMREADERJSON_H
#define CMREADERJSON_H

#include <yajl/yajl_parse.h>

enum cm_json_map_key{
	CM_JSON_MAP_KEY_IDLE,
	CM_JSON_MAP_KEY_UNKNOWN
};

struct traceReaderJSON{
	int 				file;
	long 				file_length;
	long 				read_offset;
	void*				buffer;
	int 				buffer_length;

	yajl_handle 		json_parser_handle;
	enum json_map_key 	actual_key;
	int 				actual_map_level;

	long 				instruction_cursor;
	struct instruction 	instruction_cache[TRACEREADERJSON_NB_INSTRUCTION_IN_CACHE];
	unsigned int 		cache_bot_offset;
	unsigned int 		cache_top_offset;
	long 				cache_bot_cursor;
	long				cache_top_cursor;
};

int traceReaderJSON_init(struct traceReaderJSON* trace_reader, const char* file_name);
struct instruction* traceReaderJSON_get_next_instruction(struct traceReaderJSON* trace_reader);
void traceReaderJSON_clean(struct traceReaderJSON* trace_reader);

#endif