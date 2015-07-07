#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <string.h>

#include "refReaderJSON.h"
#include "primitiveReference.h"
#include "mapFile.h"

#define JSON_MAP_KEY_NAME_REF	"ref"
#define JSON_MAP_KEY_NAME_NAME	"name"
#define JSON_MAP_KEY_NAME_LIB	"lib"
#define JSON_MAP_KEY_NAME_FUNC	"func"
#define JSON_MAP_KEY_NAME_ARG	"arg"
#define JSON_MAP_KEY_NAME_TYPE	"type"
#define JSON_MAP_KEY_NAME_EXACT	"exact_size"
#define JSON_MAP_KEY_NAME_MULTI	"multiple_size"
#define JSON_MAP_KEY_NAME_IMPL	"implicit_size"
#define JSON_MAP_KEY_NAME_INPUT	"input_size"
#define JSON_MAP_KEY_NAME_MAX	"max_size"
#define JSON_MAP_KEY_NAME_UNDEF	"undefined_size"

#define IDLE_MAP_LEVEL 			1
#define REF_MAP_LEVEL			2
#define ARG_MAP_LEVEL			3

#define REFREADERJSON_LIB_NAME_LENGTH 	256
#define REFREADERJSON_FUNC_NAME_LENGTH 	256
#define REFREADERJSON_MAX_NB_INPUT 		10
#define REFREADERJSON_MAX_NB_OUTPUT 	5

enum ref_json_map_key{
	REF_JSON_MAP_KEY_REF,
	REF_JSON_MAP_KEY_NAME,
	REF_JSON_MAP_KEY_LIB,
	REF_JSON_MAP_KEY_FUNC,
	REF_JSON_MAP_KEY_ARG,
	REF_JSON_MAP_KEY_TYPE,
	REF_JSON_MAP_KEY_EXACT,
	REF_JSON_MAP_KEY_MULTI,
	REF_JSON_MAP_KEY_IMPL,
	REF_JSON_MAP_KEY_INPUT,
	REF_JSON_MAP_KEY_MAX,
	REF_JSON_MAP_KEY_UNDEF,
	REF_JSON_MAP_KEY_UNKNOWN
};

enum ref_arg_type{
	REF_ARG_TYPE_UNKNOWN,
	REF_ARG_TYPE_INPUT,
	REF_ARG_TYPE_OUTPUT
};

struct refReaderJSON{
	struct array* 			array;
	enum ref_json_map_key 	actual_key;
	int8_t 					actual_map_level;
	enum ref_arg_type 		actual_arg_type;

	char 					prim_name[PRIMITIVEREFERENCE_MAX_NAME_SIZE];
	char					prim_lib[REFREADERJSON_LIB_NAME_LENGTH];
	char					prim_func[REFREADERJSON_FUNC_NAME_LENGTH];
	uint8_t 				prim_nb_input;
	uint8_t 				prim_nb_output;
	uint32_t 				prim_input[REFREADERJSON_MAX_NB_INPUT];
	uint32_t 				prim_output[REFREADERJSON_MAX_NB_OUTPUT];

	uint8_t 				set_name;
	uint8_t 				set_lib;
	uint8_t 				set_func;
};

static int32_t refReaderJSON_init(struct refReaderJSON* ref_reader, struct array* array);

static int refReaderJSON_number(void* ctx, const char* s, size_t l);
static int refReaderJSON_string(void* ctx, const unsigned char* stringVal, size_t stringLen);
static int refReaderJSON_map_key(void* ctx, const unsigned char* stringVal, size_t stringLen);
static int refReaderJSON_start_map(void* ctx);
static int refReaderJSON_end_map(void* ctx);


static int32_t refReaderJSON_init(struct refReaderJSON* ref_reader, struct array* array){
	ref_reader->array = array;
	ref_reader->actual_map_level = 0;

	return 0;
}


static yajl_callbacks json_parser_callback = {
	NULL,							/* null */	
	NULL,							/* boolean */
	NULL,							/* integer */
	NULL,							/* double */
	refReaderJSON_number,			/* number */
	refReaderJSON_string,			/* string */
	refReaderJSON_start_map,		/* start map */
	refReaderJSON_map_key,			/* map key */
	refReaderJSON_end_map,			/* end map */
	NULL,							/* start array */
	NULL							/* end array */
};


int32_t refReaderJSON_parse(const char* file_name, struct array* array){
	void*					buffer;
	size_t 					size;
	yajl_handle 			json_parser_handle;
	yajl_status 			status;
	struct refReaderJSON 	ref_reader;

	if (refReaderJSON_init(&ref_reader, array)){
		printf("ERROR: in %s, unable to init ref_reader\n", __func__);
		return -1;
	}

	buffer = mapFile_map(file_name, &size);
	if (buffer == NULL){
		printf("ERROR: in %s, unable to map file\n", __func__);
		return -1;
	}

	json_parser_handle = yajl_alloc(&json_parser_callback, NULL, (void*)&ref_reader);
	if (json_parser_handle == NULL){
		printf("ERROR: in %s, unable to allocate YAJL parser\n", __func__);
		munmap(buffer, size);
		return -1;
	}

	status = yajl_parse(json_parser_handle, buffer, size);
	if (status != yajl_status_ok){
		printf("ERROR: in %s, YAJL parser return an error status\n", __func__);
	}

	yajl_free(json_parser_handle);
	munmap(buffer, size);

	return 0;
}

/* ===================================================================== */
/* YAJL callbacks                                                   	 */
/* ===================================================================== */

#pragma GCC diagnostic ignored "-Wunused-parameter"
static int refReaderJSON_number(void* ctx, const char* s, size_t l){
	struct refReaderJSON* 	ref_reader = (struct refReaderJSON*)ctx;
	uint32_t 				value;

	value = (unsigned int)atoi(s);

	if (ref_reader->actual_key == REF_JSON_MAP_KEY_EXACT){
		switch(ref_reader->actual_arg_type){
			case REF_ARG_TYPE_UNKNOWN 	: {printf("ERROR: in %s, arg type type has not been specified\n", __func__); break;}
			case REF_ARG_TYPE_INPUT 	: {PRIMITIVEREFERENCE_ARG_SPECIFIER_SET_SIZE_EXACT_VALUE(ref_reader->prim_input[ref_reader->prim_nb_input], value); break;}
			case REF_ARG_TYPE_OUTPUT 	: {PRIMITIVEREFERENCE_ARG_SPECIFIER_SET_SIZE_EXACT_VALUE(ref_reader->prim_output[ref_reader->prim_nb_output], value); break;}
		}
	}
	else if (ref_reader->actual_key == REF_JSON_MAP_KEY_MULTI){
		switch(ref_reader->actual_arg_type){
			case REF_ARG_TYPE_UNKNOWN 	: {printf("ERROR: in %s, arg type type has not been specified\n", __func__); break;}
			case REF_ARG_TYPE_INPUT 	: {PRIMITIVEREFERENCE_ARG_SPECIFIER_SET_SIZE_MULTIPLE(ref_reader->prim_input[ref_reader->prim_nb_input], value); break;}
			case REF_ARG_TYPE_OUTPUT 	: {PRIMITIVEREFERENCE_ARG_SPECIFIER_SET_SIZE_MULTIPLE(ref_reader->prim_output[ref_reader->prim_nb_output], value); break;}
		}
	}
	else if (ref_reader->actual_key == REF_JSON_MAP_KEY_IMPL){
		switch(ref_reader->actual_arg_type){
			case REF_ARG_TYPE_UNKNOWN 	: {printf("ERROR: in %s, arg type type has not been specified\n", __func__); break;}
			case REF_ARG_TYPE_INPUT 	: {PRIMITIVEREFERENCE_ARG_SPECIFIER_SET_IMPLICIT_SIZE(ref_reader->prim_input[ref_reader->prim_nb_input], value); break;}
			case REF_ARG_TYPE_OUTPUT 	: {PRIMITIVEREFERENCE_ARG_SPECIFIER_SET_IMPLICIT_SIZE(ref_reader->prim_output[ref_reader->prim_nb_output], value); break;}
		}
	}
	else if (ref_reader->actual_key == REF_JSON_MAP_KEY_INPUT){
		switch(ref_reader->actual_arg_type){
			case REF_ARG_TYPE_UNKNOWN 	: {printf("ERROR: in %s, arg type type has not been specified\n", __func__); break;}
			case REF_ARG_TYPE_INPUT 	: {PRIMITIVEREFERENCE_ARG_SPECIFIER_SET_SIZE_OF_INPUT_ARG(ref_reader->prim_input[ref_reader->prim_nb_input], value); break;}
			case REF_ARG_TYPE_OUTPUT 	: {PRIMITIVEREFERENCE_ARG_SPECIFIER_SET_SIZE_OF_INPUT_ARG(ref_reader->prim_output[ref_reader->prim_nb_output], value); break;}
		}
	}
	else if (ref_reader->actual_key == REF_JSON_MAP_KEY_MAX){
		switch(ref_reader->actual_arg_type){
			case REF_ARG_TYPE_UNKNOWN 	: {printf("ERROR: in %s, arg type type has not been specified\n", __func__); break;}
			case REF_ARG_TYPE_INPUT 	: {PRIMITIVEREFERENCE_ARG_SPECIFIER_SET_SIZE_MAX(ref_reader->prim_input[ref_reader->prim_nb_input], value); break;}
			case REF_ARG_TYPE_OUTPUT 	: {PRIMITIVEREFERENCE_ARG_SPECIFIER_SET_SIZE_MAX(ref_reader->prim_output[ref_reader->prim_nb_output], value); break;}
		}
	}
	else if (ref_reader->actual_key == REF_JSON_MAP_KEY_UNDEF){
		switch(ref_reader->actual_arg_type){
			case REF_ARG_TYPE_UNKNOWN 	: {printf("ERROR: in %s, arg type type has not been specified\n", __func__); break;}
			case REF_ARG_TYPE_INPUT 	: {PRIMITIVEREFERENCE_ARG_SPECIFIER_SET_SIZE_UNDEFINED(ref_reader->prim_input[ref_reader->prim_nb_input]); break;}
			case REF_ARG_TYPE_OUTPUT 	: {PRIMITIVEREFERENCE_ARG_SPECIFIER_SET_SIZE_UNDEFINED(ref_reader->prim_output[ref_reader->prim_nb_output]); break;}
		}
	}
	else{
		printf("ERROR: in %s, wrong data type (number) for the current key\n", __func__);
	}

	return 1;
}

static int refReaderJSON_string(void* ctx, const unsigned char* stringVal, size_t stringLen){
	struct refReaderJSON* ref_reader = (struct refReaderJSON*)ctx;

	if (ref_reader->actual_key == REF_JSON_MAP_KEY_NAME){
		memcpy(ref_reader->prim_name, stringVal, ((PRIMITIVEREFERENCE_MAX_NAME_SIZE > stringLen) ? stringLen : PRIMITIVEREFERENCE_MAX_NAME_SIZE));
		ref_reader->set_name = 1;
	}
	else if (ref_reader->actual_key == REF_JSON_MAP_KEY_FUNC){
		memcpy(ref_reader->prim_func, stringVal, ((REFREADERJSON_FUNC_NAME_LENGTH > stringLen) ? stringLen : REFREADERJSON_FUNC_NAME_LENGTH));
		ref_reader->set_func = 1;
	}
	else if (ref_reader->actual_key == REF_JSON_MAP_KEY_LIB){
		memcpy(ref_reader->prim_lib, stringVal, ((REFREADERJSON_LIB_NAME_LENGTH > stringLen) ? stringLen : REFREADERJSON_LIB_NAME_LENGTH));
		ref_reader->set_lib = 1;
		
	}
	else if (ref_reader->actual_key == REF_JSON_MAP_KEY_TYPE){
		if (!strncmp((char*)stringVal, "input", stringLen)){
			ref_reader->actual_arg_type = REF_ARG_TYPE_INPUT;
		}
		else if (!strncmp((char*)stringVal, "output", stringLen)){
			ref_reader->actual_arg_type = REF_ARG_TYPE_OUTPUT;
		}
		else{
			printf("ERROR: in %s, incorrect value for key: \"type\"\n", __func__);
		}
	}
	else{
		printf("ERROR: in %s, wrong data type (string) for the current key\n", __func__);
	}

	return 1;
}

static int refReaderJSON_map_key(void* ctx, const unsigned char* stringVal, size_t stringLen){
	struct refReaderJSON* ref_reader = (struct refReaderJSON*)ctx;

	if (!strncmp((const char*)stringVal, JSON_MAP_KEY_NAME_REF, stringLen)){
		ref_reader->actual_key = REF_JSON_MAP_KEY_REF;
	}
	else if (!strncmp((const char*)stringVal, JSON_MAP_KEY_NAME_NAME, stringLen)){
		ref_reader->actual_key = REF_JSON_MAP_KEY_NAME;
	}
	else if (!strncmp((const char*)stringVal, JSON_MAP_KEY_NAME_LIB, stringLen)){
		ref_reader->actual_key = REF_JSON_MAP_KEY_LIB;
	}
	else if (!strncmp((const char*)stringVal, JSON_MAP_KEY_NAME_FUNC, stringLen)){
		ref_reader->actual_key = REF_JSON_MAP_KEY_FUNC;
	}
	else if (!strncmp((const char*)stringVal, JSON_MAP_KEY_NAME_ARG, stringLen)){
		ref_reader->actual_key = REF_JSON_MAP_KEY_ARG;
	}
	else if (!strncmp((const char*)stringVal, JSON_MAP_KEY_NAME_TYPE, stringLen)){
		ref_reader->actual_key = REF_JSON_MAP_KEY_TYPE;
	}
	else if (!strncmp((const char*)stringVal, JSON_MAP_KEY_NAME_EXACT, stringLen)){
		ref_reader->actual_key = REF_JSON_MAP_KEY_EXACT;
	}
	else if (!strncmp((const char*)stringVal, JSON_MAP_KEY_NAME_MULTI, stringLen)){
		ref_reader->actual_key = REF_JSON_MAP_KEY_MULTI;
	}
	else if (!strncmp((const char*)stringVal, JSON_MAP_KEY_NAME_IMPL, stringLen)){
		ref_reader->actual_key = REF_JSON_MAP_KEY_IMPL;
	}
	else if (!strncmp((const char*)stringVal, JSON_MAP_KEY_NAME_INPUT, stringLen)){
		ref_reader->actual_key = REF_JSON_MAP_KEY_INPUT;
	}
	else if (!strncmp((const char*)stringVal, JSON_MAP_KEY_NAME_MAX, stringLen)){
		ref_reader->actual_key = REF_JSON_MAP_KEY_MAX;
	}
	else if (!strncmp((const char*)stringVal, JSON_MAP_KEY_NAME_UNDEF, stringLen)){
		ref_reader->actual_key = REF_JSON_MAP_KEY_UNDEF;
	}
	else{
		printf("ERROR: in %s, unknown map key\n", __func__);
		ref_reader->actual_key = REF_JSON_MAP_KEY_UNKNOWN;
	}
	return 1;
}

static int refReaderJSON_start_map(void* ctx){
	struct refReaderJSON* ref_reader = (struct refReaderJSON*)ctx;

	ref_reader->actual_map_level ++;

	switch (ref_reader->actual_map_level){
		case IDLE_MAP_LEVEL :{
			break;
		}
		case REF_MAP_LEVEL : {
			ref_reader->set_name = 0;
			ref_reader->set_lib = 0;
			ref_reader->set_func = 0;

			memset(ref_reader->prim_name, '\0', PRIMITIVEREFERENCE_MAX_NAME_SIZE);
			memset(ref_reader->prim_lib, '\0', REFREADERJSON_LIB_NAME_LENGTH);
			memset(ref_reader->prim_func, '\0', REFREADERJSON_FUNC_NAME_LENGTH);

			ref_reader->prim_nb_input = 0;
			ref_reader->prim_nb_output = 0;
			break;
		}
		case ARG_MAP_LEVEL : {
			if (ref_reader->prim_nb_input >= REFREADERJSON_MAX_NB_INPUT){
				ref_reader->prim_nb_input --;
				printf("ERROR: in %s, max nb output (%u) has been reached\n", __func__, REFREADERJSON_MAX_NB_INPUT);
			}
			if (ref_reader->prim_nb_output >= REFREADERJSON_MAX_NB_OUTPUT){
				ref_reader->prim_nb_output --;
				printf("ERROR: in %s, max nb output (%u) has been reached\n", __func__, REFREADERJSON_MAX_NB_OUTPUT);
			}
			ref_reader->actual_arg_type = REF_ARG_TYPE_UNKNOWN;
			break;
		}
		default : {
			printf("ERROR: in %s, wrong map level (%d), this case is not meant to happen\n", __func__, ref_reader->actual_map_level);
			break;
		}
	}

	return 1;
}

static int refReaderJSON_end_map(void* ctx){
	struct refReaderJSON* 		ref_reader = (struct refReaderJSON*)ctx;
	struct primitiveReference 	primitive;

	switch (ref_reader->actual_map_level){
		case IDLE_MAP_LEVEL :{
			break;
		}
		case REF_MAP_LEVEL : {
			if (ref_reader->set_name && ref_reader->set_lib && ref_reader->set_func){
				if (primitiveReference_init(&primitive, ref_reader->prim_name, ref_reader->prim_nb_input, ref_reader->prim_nb_output, ref_reader->prim_input, ref_reader->prim_output, ref_reader->prim_lib, ref_reader->prim_func)){
					printf("ERROR: in %s, unable to init primitive reference structure\n", __func__);
				}
				else{
					if (array_add(ref_reader->array, &primitive) < 0){
						printf("ERROR: in %s, unable to add element to array\n", __func__);
					}
				}
			}
			else{
				printf("ERROR: in %s, incomplete primitive reference entry - skip\n", __func__);
			}
			break;
		}
		case ARG_MAP_LEVEL : {
			switch(ref_reader->actual_arg_type){
				case REF_ARG_TYPE_UNKNOWN 	: {printf("ERROR: in %s, arg type type has not been specified\n", __func__); break;}
				case REF_ARG_TYPE_INPUT 	: {ref_reader->prim_nb_input ++; break;}
				case REF_ARG_TYPE_OUTPUT 	: {ref_reader->prim_nb_output ++; break;}
			}
			break;
		}
		default : {
			printf("ERROR: in %s, wrong map level (%d), this case is not meant to happen\n", __func__, ref_reader->actual_map_level);
			break;
		}
	}

	ref_reader->actual_map_level --;

	return 1;
}