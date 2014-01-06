#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>

#include "traceReaderJSON.h"

#define JSON_MAP_KEY_NAME_TRACE			"trace"
#define JSON_MAP_KEY_NAME_PC			"pc"
#define JSON_MAP_KEY_NAME_INS			"ins"
#define JSON_MAP_KEY_NAME_DATA_READ 	"read"
#define JSON_MAP_KEY_NAME_DATA_WRITE 	"write"
#define JSON_MAP_KEY_NAME_DATA_MEM 		"mem"
#define JSON_MAP_KEY_NAME_DATA_SIZE 	"size"
#define JSON_MAP_KEY_NAME_DATA_VAL		"val"

#define INSTRUCTION_MAP_LEVEL			2
#define INSTRUCTION_DATA_MAP_LEVEL 		3

static int traceReaderJSON_number(void* ctx, const char* s, size_t l);
static int traceReaderJSON_string(void* ctx, const unsigned char* stringVal, size_t stringLen);
static int traceReaderJSON_map_key(void* ctx, const unsigned char* stringVal, size_t stringLen);
static int traceReaderJSON_start_map(void* ctx);
static int traceReaderJSON_end_map(void* ctx);

static yajl_callbacks json_parser_callback = {
	NULL,							/* null */	
	NULL,							/* boolean */
	NULL,							/* integer */
	NULL,							/* double */
	traceReaderJSON_number,			/* number */
	traceReaderJSON_string,			/* string */
	traceReaderJSON_start_map,		/* start map */
	traceReaderJSON_map_key,		/* map key */
	traceReaderJSON_end_map,		/* end map */
	NULL,							/* start array */
	NULL							/* end array */
};


int traceReaderJSON_init(struct traceReaderJSON* trace_reader, const char* file_name){
	struct stat 		sb;

	trace_reader->read_offset = 0;
	trace_reader->buffer = NULL;
	trace_reader->buffer_length = 0;
	trace_reader->json_parser_handle = NULL;

	trace_reader->file = open(file_name, O_RDONLY);
	if (trace_reader->file == -1){
		printf("ERROR: in %s; unable to open file %s read only\n", __func__, file_name);
		return -1;
	}

	if (fstat(trace_reader->file, &sb) < 0){
		printf("ERROR: in %s, unable to read file size\n", __func__);
		close(trace_reader->file);
		trace_reader->file = -1;
		return -1;
	}

	trace_reader->file_length 						= sb.st_size;

	trace_reader->actual_key 						= TRACE_JSON_MAP_KEY_IDLE;
	trace_reader->actual_map_level 					= 0;
	trace_reader->actual_instruction_data_offset 	= 0;

	trace_reader->instruction_cursor 				= 0;
	trace_reader->cache_bot_offset	 				= 0;
	trace_reader->cache_top_offset 					= 0;
	trace_reader->cache_bot_cursor 					= 0;
	trace_reader->cache_top_cursor 					= 0;
	
	return 0;
}

int traceReaderJSON_reset(struct traceReaderJSON* trace_reader){
	int result = -1;

	if (trace_reader->buffer != NULL && trace_reader->buffer_length > 0){
		if (munmap(trace_reader->buffer, trace_reader->buffer_length)){
			printf("ERROR: in %s, munmap failed\n", __func__);
		}
		trace_reader->buffer = NULL;
	}

	if (trace_reader->json_parser_handle != NULL){
		yajl_free(trace_reader->json_parser_handle);
		trace_reader->json_parser_handle = NULL;
	}

	trace_reader->json_parser_handle = yajl_alloc(&json_parser_callback, NULL, (void*)trace_reader);
	if (trace_reader->json_parser_handle == NULL){
		printf("ERROR: in %s, unable to allocate YAJL parser\n", __func__);
		close(trace_reader->file);
		trace_reader->file = -1;
		return result;
	}

	trace_reader->read_offset 						= 0;
	trace_reader->buffer_length 					= 0;

	trace_reader->actual_key 						= TRACE_JSON_MAP_KEY_IDLE;
	trace_reader->actual_map_level 					= 0;
	trace_reader->actual_instruction_data_offset 	= 0;

	trace_reader->instruction_cursor 				= 0;
	trace_reader->cache_bot_offset	 				= 0;
	trace_reader->cache_top_offset 					= 0;
	trace_reader->cache_bot_cursor 					= 0;
	trace_reader->cache_top_cursor 					= 0;

	result = 0;

	return result;
}

struct instruction* traceReaderJSON_get_next_instruction(struct traceReaderJSON* trace_reader){
	struct instruction* result = NULL;
	long				mapping_size;
	yajl_status 		status;

	if (trace_reader->instruction_cursor >= trace_reader->cache_bot_cursor && trace_reader->instruction_cursor < trace_reader->cache_top_cursor){
		result = trace_reader->instruction_cache + (trace_reader->cache_bot_offset + (trace_reader->instruction_cursor - trace_reader->cache_bot_cursor))%TRACEREADERJSON_NB_INSTRUCTION_IN_CACHE;
		trace_reader->instruction_cursor ++;
	}
	else{
		if (trace_reader->buffer != NULL && trace_reader->buffer_length > 0){
			if (munmap(trace_reader->buffer, trace_reader->buffer_length)){
				printf("ERROR: in %s, munmap failed\n", __func__);
			}
			trace_reader->buffer = NULL;
		}
		mapping_size = (trace_reader->read_offset + TRACEREADERJSON_MMAP_MEMORY_SIZE > trace_reader->file_length)?(trace_reader->file_length - trace_reader->read_offset):TRACEREADERJSON_MMAP_MEMORY_SIZE;
		if (mapping_size != 0){
			trace_reader->buffer = mmap(NULL, mapping_size, PROT_READ, MAP_PRIVATE, trace_reader->file, trace_reader->read_offset);
			if (trace_reader->buffer == NULL){
				printf("ERROR: in %s, unable to map trace file @ offset %ld\n", __func__, trace_reader->read_offset);
			}
			else{
				trace_reader->read_offset += mapping_size;
				trace_reader->buffer_length = mapping_size;

				status = yajl_parse(trace_reader->json_parser_handle, trace_reader->buffer, trace_reader->buffer_length);
				if (status != yajl_status_ok){
					printf("ERROR: in %s, YAJL parser return an error status\n", __func__);
				}
				else{
					if (trace_reader->read_offset == trace_reader->file_length){
						status = yajl_complete_parse(trace_reader->json_parser_handle);
						if (status != yajl_status_ok){
							printf("ERROR: in %s, YAJL parser return an error status (complete)\n", __func__);
						}
						else{
							result = traceReaderJSON_get_next_instruction(trace_reader);
						}
					}
					else{
						result = traceReaderJSON_get_next_instruction(trace_reader);
					}
				}
			}
		}
	}

	return result;
}

void traceReaderJSON_clean(struct traceReaderJSON* trace_reader){
	if (trace_reader != NULL){
		if (trace_reader->file != -1){
			if (close(trace_reader->file)){
				printf("ERROR: in %s, file close failed\n", __func__);
			}
			trace_reader->file = -1;
		}
		if (trace_reader->buffer != NULL && trace_reader->buffer_length > 0){
			if (munmap(trace_reader->buffer, trace_reader->buffer_length)){
				printf("ERROR: in %s, munmap failed\n", __func__);
			}
			trace_reader->buffer = NULL;
			trace_reader->buffer_length = 0;
		}
		if (trace_reader->json_parser_handle != NULL){
			yajl_free(trace_reader->json_parser_handle);
			trace_reader->json_parser_handle = NULL;
		}
	}
}


/* ===================================================================== */
/* YAJL callbacks                                                   	 */
/* ===================================================================== */

#pragma GCC diagnostic ignored "-Wunused-parameter"
static int traceReaderJSON_number(void* ctx, const char* s, size_t l){
	struct traceReaderJSON* trace_reader = (struct traceReaderJSON*)ctx;

	switch (trace_reader->actual_key){
	case TRACE_JSON_MAP_KEY_INS : {
		trace_reader->instruction_cache[trace_reader->cache_top_offset].opcode = (uint32_t)atoi(s);
		break;
	}
	case TRACE_JSON_MAP_KEY_DATA_SIZE : {
		trace_reader->instruction_cache[trace_reader->cache_top_offset].data[trace_reader->actual_instruction_data_offset].size = (uint8_t)atoi(s);
		break;
	}
	default : {
		printf("ERROR: in %s, wrong data type for the current key\n", __func__);
		break;
	}
	}

	return 1;
}

#pragma GCC diagnostic ignored "-Wunused-parameter"
static int traceReaderJSON_string(void* ctx, const unsigned char* stringVal, size_t stringLen){
	struct traceReaderJSON* trace_reader = (struct traceReaderJSON*)ctx;

	switch (trace_reader->actual_key){
	case TRACE_JSON_MAP_KEY_PC : {
		#if defined ARCH_32
		trace_reader->instruction_cache[trace_reader->cache_top_offset].pc = strtoul((const char*)stringVal, NULL, 16);
		#elif defined ARCH_64
		trace_reader->instruction_cache[trace_reader->cache_top_offset].pc = strtoull((const char*)stringVal, NULL, 16);
		#else
		#error Please specify an architecture {ARCH_32 or ARCH_64}
		#endif
		break;
	}
	case TRACE_JSON_MAP_KEY_DATA_VAL : {
		trace_reader->instruction_cache[trace_reader->cache_top_offset].data[trace_reader->actual_instruction_data_offset].value = (uint32_t)strtoul((const char*)stringVal, NULL, 16);
		break;
	}
	case TRACE_JSON_MAP_KEY_DATA_MEM : {
		INSTRUCTION_DATA_TYPE_SET_MEM(trace_reader->instruction_cache[trace_reader->cache_top_offset].data[trace_reader->actual_instruction_data_offset].type);
		#if defined ARCH_32
		trace_reader->instruction_cache[trace_reader->cache_top_offset].data[trace_reader->actual_instruction_data_offset].location.address = strtoul((const char*)stringVal, NULL, 16);
		#elif defined ARCH_64
		trace_reader->instruction_cache[trace_reader->cache_top_offset].data[trace_reader->actual_instruction_data_offset].location.address = strtoull((const char*)stringVal, NULL, 16);
		#else
		#error Please specify an architecture {ARCH_32 or ARCH_64}
		#endif
		break;
	}
	default : {
		printf("ERROR: in %s, wrong data type for the current key\n", __func__);
		break;
	}
	}

	return 1;
}

static int traceReaderJSON_map_key(void* ctx, const unsigned char* stringVal, size_t stringLen){
	struct traceReaderJSON* trace_reader = (struct traceReaderJSON*)ctx;

	if (!strncmp((const char*)stringVal, JSON_MAP_KEY_NAME_TRACE, stringLen)){
		trace_reader->actual_key = TRACE_JSON_MAP_KEY_TRACE;
	}
	else if (!strncmp((const char*)stringVal, JSON_MAP_KEY_NAME_PC, stringLen)){
		trace_reader->actual_key = TRACE_JSON_MAP_KEY_PC;
	}
	else if (!strncmp((const char*)stringVal, JSON_MAP_KEY_NAME_INS, stringLen)){
		trace_reader->actual_key = TRACE_JSON_MAP_KEY_INS;
	}
	else if (!strncmp((const char*)stringVal, JSON_MAP_KEY_NAME_DATA_READ, stringLen)){
		trace_reader->actual_key = TRACE_JSON_MAP_KEY_DATA_READ;
	}
	else if (!strncmp((const char*)stringVal, JSON_MAP_KEY_NAME_DATA_WRITE, stringLen)){
		trace_reader->actual_key = TRACE_JSON_MAP_KEY_DATA_WRITE;
	}
	else if (!strncmp((const char*)stringVal, JSON_MAP_KEY_NAME_DATA_MEM, stringLen)){
		trace_reader->actual_key = TRACE_JSON_MAP_KEY_DATA_MEM;
	}
	else if (!strncmp((const char*)stringVal, JSON_MAP_KEY_NAME_DATA_VAL, stringLen)){
		trace_reader->actual_key = TRACE_JSON_MAP_KEY_DATA_VAL;
	}
	else if (!strncmp((const char*)stringVal, JSON_MAP_KEY_NAME_DATA_SIZE, stringLen)){
		trace_reader->actual_key = TRACE_JSON_MAP_KEY_DATA_SIZE;
	}
	else{
		printf("ERROR: in %s, unknown map key\n", __func__);
		trace_reader->actual_key = TRACE_JSON_MAP_KEY_UNKNOWN;
	}
	return 1;
}

static int traceReaderJSON_start_map(void* ctx){
	struct traceReaderJSON* 	trace_reader = (struct traceReaderJSON*)ctx;
	int 						i;

	trace_reader->actual_map_level ++;

	if (trace_reader->actual_map_level == INSTRUCTION_MAP_LEVEL){
		trace_reader->actual_instruction_data_offset = 0;
		for (i = 0; i < INSTRUCTION_MAX_NB_DATA; i++){
			trace_reader->instruction_cache[trace_reader->cache_top_offset].data[i].type = INSDATA_INVALID;
		}
	}
	else if (trace_reader->actual_map_level == INSTRUCTION_DATA_MAP_LEVEL){
		if (trace_reader->actual_instruction_data_offset >= INSTRUCTION_MAX_NB_DATA){
			printf("WARNING: in %s, data offset in current instruction is larger than allocated array. Overwritting last data\n", __func__);
			trace_reader->actual_instruction_data_offset --;
		}

		switch(trace_reader->actual_key){
		case TRACE_JSON_MAP_KEY_DATA_READ : {
			INSTRUCTION_DATA_TYPE_SET_VALID(trace_reader->instruction_cache[trace_reader->cache_top_offset].data[trace_reader->actual_instruction_data_offset].type);
			INSTRUCTION_DATA_TYPE_SET_READ(trace_reader->instruction_cache[trace_reader->cache_top_offset].data[trace_reader->actual_instruction_data_offset].type);
			break;
		}
		case TRACE_JSON_MAP_KEY_DATA_WRITE : {
			INSTRUCTION_DATA_TYPE_SET_VALID(trace_reader->instruction_cache[trace_reader->cache_top_offset].data[trace_reader->actual_instruction_data_offset].type);
			INSTRUCTION_DATA_TYPE_SET_WRITE(trace_reader->instruction_cache[trace_reader->cache_top_offset].data[trace_reader->actual_instruction_data_offset].type);
			break;
		}
		default : {
			printf("ERROR: in %s, incorrect key value to start map level %u\n", __func__, trace_reader->actual_map_level);
			break;
		}
		}
	}

	return 1;
}

static int traceReaderJSON_end_map(void* ctx){
	struct traceReaderJSON* trace_reader = (struct traceReaderJSON*)ctx;

	if (trace_reader->actual_map_level == INSTRUCTION_MAP_LEVEL){
		
		if ((trace_reader->cache_top_offset + 1)%TRACEREADERJSON_NB_INSTRUCTION_IN_CACHE == trace_reader->cache_bot_offset){
			trace_reader->cache_bot_offset = (trace_reader->cache_bot_offset + 1)%TRACEREADERJSON_NB_INSTRUCTION_IN_CACHE;
			trace_reader->cache_bot_cursor ++;
		}
		trace_reader->cache_top_offset = (trace_reader->cache_top_offset + 1)%TRACEREADERJSON_NB_INSTRUCTION_IN_CACHE;
		trace_reader->cache_top_cursor ++;
	}
	else if (trace_reader->actual_map_level == INSTRUCTION_DATA_MAP_LEVEL){
		trace_reader->actual_instruction_data_offset ++;
	}

	trace_reader->actual_map_level --;

	return 1;
}