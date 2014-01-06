#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>

#include "cmReaderJSON.h"


#define JSON_MAP_KEY_NAME_IMAGE		"image"
#define JSON_MAP_KEY_NAME_SECTION	"section"
#define JSON_MAP_KEY_NAME_ROUTINE	"routine"
#define JSON_MAP_KEY_NAME_NAME		"name"
#define JSON_MAP_KEY_NAME_START		"start"
#define JSON_MAP_KEY_NAME_STOP		"stop"
#define JSON_MAP_KEY_NAME_WHITELIST	"whl"
#define JSON_MAP_KEY_NAME_EXE		"exe"

#define IDLE_MAP_LEVEL 				1
#define IMAGE_MAP_LEVEL				2
#define SECTION_MAP_LEVEL			3
#define ROUTINE_MAP_LEVEL			4

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


struct codeMap* cmReaderJSON_parse_trace(const char* file_name){
	int 				file;
	struct stat 		sb;
	void*				buffer;
	yajl_handle 		json_parser_handle;
	yajl_status 		status;
	struct cmReaderJSON cm_reader;

	cm_reader.cm = codeMap_create();
	if (cm_reader.cm == NULL){
		printf("ERROR: in %s, unable to create code map\n", __func__);
		return NULL;
	}

	cm_reader.actual_key 		= CM_JSON_MAP_KEY_IDLE;
	cm_reader.actual_map_level 	= 0;

	file = open(file_name, O_RDONLY);
	if (file == -1){
		printf("ERROR: in %s; unable to open file %s read only\n", __func__, file_name);
		codeMap_delete(cm_reader.cm);
		return NULL;
	}

	if (fstat(file, &sb) < 0){
		printf("ERROR: in %s, unable to read file size\n", __func__);
		close(file);
		codeMap_delete(cm_reader.cm);
		return NULL;
	}

	buffer = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, file, 0);
	close(file);
	if (buffer == NULL){
		printf("ERROR: in %s, unable to map file\n", __func__);
		codeMap_delete(cm_reader.cm);
		return NULL;
	}

	json_parser_handle = yajl_alloc(&json_parser_callback, NULL, (void*)&cm_reader);
	if (json_parser_handle == NULL){
		printf("ERROR: in %s, unable to allocate YAJL parser\n", __func__);
		munmap(buffer, sb.st_size);
		codeMap_delete(cm_reader.cm);
		return NULL;
	}

	status = yajl_parse(json_parser_handle, buffer, sb.st_size);
	if (status != yajl_status_ok){
		printf("ERROR: in %s, YAJL parser return an error status\n", __func__);
	}

	yajl_free(json_parser_handle);
	munmap(buffer, sb.st_size);

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
			default : {printf("ERROR: in %s, boolean attribute is not expected at this level\n", __func__); break;}
		}
	}
	else{
		printf("ERROR: in %s, wrong data type (boolean) for the current key\n", __func__);
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
			printf("ERROR: in %s, number attribute is not expected at this level\n", __func__);
		}
	}
	else{
		printf("ERROR: in %s, wrong data type (number) for the current key\n", __func__);
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
			default : {printf("ERROR: in %s, string attribute is not expected at this level\n", __func__); break;}
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
			default : {printf("ERROR: in %s, string attribute is not expected at this level\n", __func__); break;}
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
			default : {printf("ERROR: in %s, string attribute is not expected at this level\n", __func__); break;}
		}
	}
	else{
		printf("ERROR: in %s, wrong data type (string) for the current key\n", __func__);
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
			printf("ERROR: in %s, unable to add image to code map\n", __func__);
		}
	}
	else if (!strncmp((const char*)stringVal, JSON_MAP_KEY_NAME_ROUTINE, stringLen)){
		cm_reader->actual_key = CM_JSON_MAP_KEY_ROUTINE;
		if (codeMap_add_static_section(cm_reader->cm, &(cm_reader->current_section))){
			printf("ERROR: in %s, unable to add section to code map\n", __func__);
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
		printf("ERROR: in %s, unknown map key\n", __func__);
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
			printf("ERROR: in %s, wrong map level (%d), this case is not meant to happen\n", __func__, cm_reader->actual_map_level);
			break;
		}
	}

	return 1;
}

static int cmReaderJSON_end_map(void* ctx){
	struct cmReaderJSON* cm_reader = (struct cmReaderJSON*)ctx;

	if (cm_reader->actual_map_level == ROUTINE_MAP_LEVEL){
		if (codeMAp_add_static_routine(cm_reader->cm, &(cm_reader->current_routine))){
			printf("ERROR: in %s, unable to add routine to code map\n", __func__);
		}
	}
	else if ((cm_reader->actual_map_level != SECTION_MAP_LEVEL) && (cm_reader->actual_map_level != IMAGE_MAP_LEVEL) && (cm_reader->actual_map_level != IDLE_MAP_LEVEL)){
		printf("ERROR: in %s, wrong map level (%d), this case is not meant to happen\n", __func__, cm_reader->actual_map_level);
	}

	cm_reader->actual_map_level --;

	return 1;
}