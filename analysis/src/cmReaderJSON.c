#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/mman.h>
#include <string.h>

#include "cmReaderJSON.h"
#include "mapFile.h"
#include "base.h"

#define JSON_MAP_KEY_NAME_IMAGE			"image"
#define JSON_MAP_KEY_NAME_SECTION		"section"
#define JSON_MAP_KEY_NAME_ROUTINE		"routine"
#define JSON_MAP_KEY_NAME_NAME			"name"
#define JSON_MAP_KEY_NAME_START			"start"
#define JSON_MAP_KEY_NAME_STOP			"stop"
#define JSON_MAP_KEY_NAME_WHITELIST		"whl"
#define JSON_MAP_KEY_NAME_EXE			"exe"

#define IDLE_MAP_LEVEL 					1
#define IMAGE_MAP_LEVEL					2
#define SECTION_MAP_LEVEL				3
#define ROUTINE_MAP_LEVEL				4

#define CMREADERJSON_PATH_MAX_LENGTH 	256

enum cm_json_map_key{
	CM_JSON_MAP_KEY_IDLE,
	CM_JSON_MAP_KEY_IMAGE,
	CM_JSON_MAP_KEY_SECTION,
	CM_JSON_MAP_KEY_ROUTINE,
	CM_JSON_MAP_KEY_NAME,
	CM_JSON_MAP_KEY_START,
	CM_JSON_MAP_KEY_STOP,
	CM_JSON_MAP_KEY_WHITELIST,
	CM_JSON_MAP_KEY_EXE,
	CM_JSON_MAP_KEY_UNKNOWN
};

struct cmReaderJSON{
	struct codeMap* 		cm;
	struct cm_image 		current_image;
	struct cm_section 		current_section;
	struct cm_routine		current_routine;
	enum cm_json_map_key 	actual_key;
	int 					actual_map_level;
};

static int cmReaderJSON_boolean(void* ctx, int boolean);
static int cmReaderJSON_number(void* ctx, const char* s, size_t l);
static int cmReaderJSON_string(void* ctx, const unsigned char* stringVal, size_t stringLen);
static int cmReaderJSON_map_key(void* ctx, const unsigned char* stringVal, size_t stringLen);
static int cmReaderJSON_start_map(void* ctx);
static int cmReaderJSON_end_map(void* ctx);

static yajl_callbacks json_parser_callback = {
	NULL,							/* null */
	cmReaderJSON_boolean,			/* boolean */
	NULL,							/* integer */
	NULL,							/* double */
	cmReaderJSON_number,			/* number */
	cmReaderJSON_string,			/* string */
	cmReaderJSON_start_map,			/* start map */
	cmReaderJSON_map_key,			/* map key */
	cmReaderJSON_end_map,			/* end map */
	NULL,							/* start array */
	NULL							/* end array */
};


struct codeMap* cmReaderJSON_parse(const char* directory_path, uint32_t pid){
	void*				buffer;
	size_t 				size;
	yajl_handle 		json_parser_handle;
	unsigned char* 		json_parser_error;
	yajl_status 		status;
	struct cmReaderJSON cm_reader;
	char 				file_name[CMREADERJSON_PATH_MAX_LENGTH];

	cm_reader.cm = codeMap_create();
	if (cm_reader.cm == NULL){
		log_err("unable to create code map");
		return NULL;
	}

	cm_reader.actual_key 		= CM_JSON_MAP_KEY_IDLE;
	cm_reader.actual_map_level 	= 0;

	snprintf(file_name, CMREADERJSON_PATH_MAX_LENGTH, "%s/cm%u.json", directory_path, pid);
	buffer = mapFile_map(file_name, &size);
	if (buffer == NULL){
		log_err("unable to map file");
		codeMap_delete(cm_reader.cm);
		return NULL;
	}

	json_parser_handle = yajl_alloc(&json_parser_callback, NULL, (void*)&cm_reader);
	if (json_parser_handle == NULL){
		log_err("unable to allocate YAJL parser");
		munmap(buffer, size);
		codeMap_delete(cm_reader.cm);
		return NULL;
	}

	status = yajl_parse(json_parser_handle, buffer, size);
	if (status != yajl_status_ok){
		json_parser_error = yajl_get_error(json_parser_handle, 1, buffer, size);
		log_err_m("YAJL parser return an error status: %s", (char*)json_parser_error);
		yajl_free_error(json_parser_handle, json_parser_error);
	}

	yajl_free(json_parser_handle);
	munmap(buffer, size);

	return cm_reader.cm;
}


/* ===================================================================== */
/* YAJL callbacks                                                   	 */
/* ===================================================================== */

static int cmReaderJSON_boolean(void* ctx, int boolean){
	struct cmReaderJSON* cm_reader = (struct cmReaderJSON*)ctx;

	if (cm_reader->actual_key == CM_JSON_MAP_KEY_WHITELIST){
		switch (cm_reader->actual_map_level){
			case IMAGE_MAP_LEVEL	: {cm_reader->current_image.white_listed = (boolean == 0)?CODEMAP_NOT_WHITELISTED:CODEMAP_WHITELISTED; break;}
			case ROUTINE_MAP_LEVEL	: {cm_reader->current_routine.white_listed = (boolean == 0)?CODEMAP_NOT_WHITELISTED:CODEMAP_WHITELISTED; break;}
			default : {log_err("boolean attribute is not expected at this level"); break;}
		}
	}
	else{
		log_err("wrong data type (boolean) for the current key");
	}

	return 1;
}

#pragma GCC diagnostic ignored "-Wunused-parameter"
static int cmReaderJSON_number(void* ctx, const char* s, size_t l){
	struct cmReaderJSON* cm_reader = (struct cmReaderJSON*)ctx;

	if (cm_reader->actual_key == CM_JSON_MAP_KEY_EXE){
		if (cm_reader->actual_map_level == ROUTINE_MAP_LEVEL){
			cm_reader->current_routine.nb_execution = (unsigned int)atoi(s);
		}
		else{
			log_err("number attribute is not expected at this level");
		}
	}
	else{
		log_err("wrong data type (number) for the current key");
	}

	return 1;
}

static int cmReaderJSON_string(void* ctx, const unsigned char* stringVal, size_t stringLen){
	struct cmReaderJSON* cm_reader = (struct cmReaderJSON*)ctx;

	if (cm_reader->actual_key == CM_JSON_MAP_KEY_NAME){
		switch (cm_reader->actual_map_level){
			case IMAGE_MAP_LEVEL	: {memcpy(cm_reader->current_image.name, stringVal, (stringLen > CODEMAP_DEFAULT_NAME_SIZE)?CODEMAP_DEFAULT_NAME_SIZE:stringLen); break;}
			case SECTION_MAP_LEVEL	: {memcpy(cm_reader->current_section.name, stringVal, (stringLen > CODEMAP_DEFAULT_NAME_SIZE)?CODEMAP_DEFAULT_NAME_SIZE:stringLen); break;}
			case ROUTINE_MAP_LEVEL	: {memcpy(cm_reader->current_routine.name, stringVal, (stringLen > CODEMAP_DEFAULT_NAME_SIZE)?CODEMAP_DEFAULT_NAME_SIZE:stringLen); break;}
			default : {log_err("string attribute is not expected at this level"); break;}
		}
	}
	else if (cm_reader->actual_key == CM_JSON_MAP_KEY_START){
		switch (cm_reader->actual_map_level){
			#if defined ARCH_32
			case IMAGE_MAP_LEVEL	: {cm_reader->current_image.address_start = strtoul((const char*)stringVal, NULL, 16); break;}
			case SECTION_MAP_LEVEL	: {cm_reader->current_section.address_start = strtoul((const char*)stringVal, NULL, 16); break;}
			case ROUTINE_MAP_LEVEL	: {cm_reader->current_routine.address_start = strtoul((const char*)stringVal, NULL, 16); break;}
			#elif defined ARCH_64
			case IMAGE_MAP_LEVEL	: {cm_reader->current_image.address_start = strtoull((const char*)stringVal, NULL, 16); break;}
			case SECTION_MAP_LEVEL	: {cm_reader->current_section.address_start = strtoull((const char*)stringVal, NULL, 16); break;}
			case ROUTINE_MAP_LEVEL	: {cm_reader->current_routine.address_start = strtoull((const char*)stringVal, NULL, 16); break;}
			#else
			#error Please specify an architecture {ARCH_32 or ARCH_64}
			#endif
			default : {log_err("string attribute is not expected at this level"); break;}
		}
	}
	else if (cm_reader->actual_key == CM_JSON_MAP_KEY_STOP){
		switch (cm_reader->actual_map_level){
			#if defined ARCH_32
			case IMAGE_MAP_LEVEL	: {cm_reader->current_image.address_stop = strtoul((const char*)stringVal, NULL, 16); break;}
			case SECTION_MAP_LEVEL	: {cm_reader->current_section.address_stop = strtoul((const char*)stringVal, NULL, 16); break;}
			case ROUTINE_MAP_LEVEL	: {cm_reader->current_routine.address_stop = strtoul((const char*)stringVal, NULL, 16); break;}
			#elif defined ARCH_64
			case IMAGE_MAP_LEVEL	: {cm_reader->current_image.address_stop = strtoull((const char*)stringVal, NULL, 16); break;}
			case SECTION_MAP_LEVEL	: {cm_reader->current_section.address_stop = strtoull((const char*)stringVal, NULL, 16); break;}
			case ROUTINE_MAP_LEVEL	: {cm_reader->current_routine.address_stop = strtoull((const char*)stringVal, NULL, 16); break;}
			#else
			#error Please specify an architecture {ARCH_32 or ARCH_64}
			#endif
			default : {log_err("string attribute is not expected at this level"); break;}
		}
	}
	else{
		log_err("wrong data type (string) for the current key");
	}

	return 1;
}

static int cmReaderJSON_map_key(void* ctx, const unsigned char* stringVal, size_t stringLen){
	struct cmReaderJSON* cm_reader = (struct cmReaderJSON*)ctx;

	if (!strncmp((const char*)stringVal, JSON_MAP_KEY_NAME_IMAGE, stringLen)){
		cm_reader->actual_key = CM_JSON_MAP_KEY_IMAGE;
	}
	else if (!strncmp((const char*)stringVal, JSON_MAP_KEY_NAME_SECTION, stringLen)){
		cm_reader->actual_key = CM_JSON_MAP_KEY_SECTION;
		if (codeMap_add_static_image(cm_reader->cm, &(cm_reader->current_image))){
			log_err("unable to add image to code map");
		}
	}
	else if (!strncmp((const char*)stringVal, JSON_MAP_KEY_NAME_ROUTINE, stringLen)){
		cm_reader->actual_key = CM_JSON_MAP_KEY_ROUTINE;
		if (codeMap_add_static_section(cm_reader->cm, &(cm_reader->current_section))){
			log_err("unable to add section to code map");
		}
	}
	else if (!strncmp((const char*)stringVal, JSON_MAP_KEY_NAME_NAME, stringLen)){
		cm_reader->actual_key = CM_JSON_MAP_KEY_NAME;
	}
	else if (!strncmp((const char*)stringVal, JSON_MAP_KEY_NAME_START, stringLen)){
		cm_reader->actual_key = CM_JSON_MAP_KEY_START;
	}
	else if (!strncmp((const char*)stringVal, JSON_MAP_KEY_NAME_STOP, stringLen)){
		cm_reader->actual_key = CM_JSON_MAP_KEY_STOP;
	}
	else if (!strncmp((const char*)stringVal, JSON_MAP_KEY_NAME_WHITELIST, stringLen)){
		cm_reader->actual_key = CM_JSON_MAP_KEY_WHITELIST;
	}
	else if (!strncmp((const char*)stringVal, JSON_MAP_KEY_NAME_EXE, stringLen)){
		cm_reader->actual_key = CM_JSON_MAP_KEY_EXE;
	}
	else{
		log_err("unknown map key");
		cm_reader->actual_key = CM_JSON_MAP_KEY_UNKNOWN;
	}
	return 1;
}

static int cmReaderJSON_start_map(void* ctx){
	struct cmReaderJSON* cm_reader = (struct cmReaderJSON*)ctx;

	cm_reader->actual_map_level ++;

	switch (cm_reader->actual_map_level){
		case ROUTINE_MAP_LEVEL : {
			codeMap_clean_routine(&(cm_reader->current_routine));
			break;
		}
		case SECTION_MAP_LEVEL : {
			codeMap_clean_section(&(cm_reader->current_section));
			break;
		}
		case IMAGE_MAP_LEVEL : {
			codeMap_clean_image(&(cm_reader->current_image));
			break;
		}
		case IDLE_MAP_LEVEL : {
			break;
		}
		default : {
			log_err_m("wrong map level (%d), this case is not meant to happen", cm_reader->actual_map_level);
			break;
		}
	}

	return 1;
}

static int cmReaderJSON_end_map(void* ctx){
	struct cmReaderJSON* cm_reader = (struct cmReaderJSON*)ctx;

	if (cm_reader->actual_map_level == ROUTINE_MAP_LEVEL){
		if (codeMap_add_static_routine(cm_reader->cm, &(cm_reader->current_routine))){
			log_err("unable to add routine to code map");
		}
	}
	else if ((cm_reader->actual_map_level != SECTION_MAP_LEVEL) && (cm_reader->actual_map_level != IMAGE_MAP_LEVEL) && (cm_reader->actual_map_level != IDLE_MAP_LEVEL)){
		log_err_m("wrong map level (%d), this case is not meant to happen", cm_reader->actual_map_level);
	}

	cm_reader->actual_map_level --;

	return 1;
}
