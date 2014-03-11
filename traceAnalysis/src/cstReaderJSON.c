#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <string.h>

#include "cstReaderJSON.h"
#include "cstChecker.h"
#include "mapFile.h"
#include "readBuffer.h"

#define JSON_MAP_KEY_NAME_REF	"cst"
#define JSON_MAP_KEY_NAME_NAME	"name"
#define JSON_MAP_KEY_NAME_TYPE	"type"
#define JSON_MAP_KEY_NAME_VALUE	"value"

#define IDLE_MAP_LEVEL 			1
#define CST_MAP_LEVEL			2

enum cst_json_map_key{
	CST_JSON_MAP_KEY_CST,
	CST_JSON_MAP_KEY_NAME,
	CST_JSON_MAP_KEY_TYPE,
	CST_JSON_MAP_KEY_VALUE,
	CST_JSON_MAP_KEY_UNKNOWN
};

struct cstReaderJSON{
	struct array* 			array;
	enum cst_json_map_key 	actual_key;
	int8_t 					actual_map_level;
	struct constant 		actual_cst;

	uint8_t 				set_name;
	uint8_t 				set_type;
	uint8_t 				set_content;
};

static int32_t cstReaderJSON_init(struct cstReaderJSON* cst_reader, struct array* array);

static int cstReaderJSON_string(void* ctx, const unsigned char* stringVal, size_t stringLen);
static int cstReaderJSON_map_key(void* ctx, const unsigned char* stringVal, size_t stringLen);
static int cstReaderJSON_start_map(void* ctx);
static int cstReaderJSON_end_map(void* ctx);


static int32_t cstReaderJSON_init(struct cstReaderJSON* cst_reader, struct array* array){
	cst_reader->array = array;
	cst_reader->actual_map_level = 0;

	return 0;
}


static yajl_callbacks json_parser_callback = {
	NULL,							/* null */	
	NULL,							/* boolean */
	NULL,							/* integer */
	NULL,							/* double */
	NULL,							/* number */
	cstReaderJSON_string,			/* string */
	cstReaderJSON_start_map,		/* start map */
	cstReaderJSON_map_key,			/* map key */
	cstReaderJSON_end_map,			/* end map */
	NULL,							/* start array */
	NULL							/* end array */
};


int32_t cstReaderJSON_parse(const char* file_name, struct array* array){
	void*					buffer;
	uint64_t 				size;
	yajl_handle 			json_parser_handle;
	yajl_status 			status;
	struct cstReaderJSON 	cst_reader;

	if (cstReaderJSON_init(&cst_reader, array)){
		printf("ERROR: in %s, unable to init cst_reader\n", __func__);
		return -1;
	}

	buffer = mapFile_map(file_name, &size);
	if (buffer == NULL){
		printf("ERROR: in %s, unable to map file\n", __func__);
		return -1;
	}

	json_parser_handle = yajl_alloc(&json_parser_callback, NULL, (void*)&cst_reader);
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

static int cstReaderJSON_string(void* ctx, const unsigned char* stringVal, size_t stringLen){
	struct cstReaderJSON* cst_reader = (struct cstReaderJSON*)ctx;

	if (cst_reader->actual_key == CST_JSON_MAP_KEY_NAME){
		strncpy(cst_reader->actual_cst.name, (char*)stringVal, ((CONSTANT_NAME_LENGTH - 1 > stringLen) ? stringLen : CONSTANT_NAME_LENGTH - 1));
		cst_reader->set_name = 1;
	}
	else if (cst_reader->actual_key == CST_JSON_MAP_KEY_TYPE){
		cst_reader->actual_cst.type = constantType_from_string((char*)stringVal, stringLen);
		cst_reader->set_type = 1;
	}
	else if (cst_reader->actual_key == CST_JSON_MAP_KEY_VALUE){
		switch (cst_reader->actual_cst.type){
			case CST_TYPE_INVALID 	: {
				printf("ERROR: in %s, constant type is invalid: unable to set the content correctly\n", __func__);
				break;
			}
			case CST_TYPE_CST_8 		: {
				if (READBUFFER_RAW_GET_LENGTH(stringLen) == 1){
					cst_reader->actual_cst.content.value = 0;
					readBuffer_raw((char*)stringVal, stringLen, (char*)&(cst_reader->actual_cst.content.value));
					cst_reader->set_content = 1;
				}
				else{
					printf("ERROR: in %s, value size is incorrect for this constant type: %s\n", __func__, constantType_to_string(cst_reader->actual_cst.type));
				}
				break;
			}
			case CST_TYPE_CST_16 		: {
				if (READBUFFER_RAW_GET_LENGTH(stringLen) == 2){
					cst_reader->actual_cst.content.value = 0;
					readBuffer_raw((char*)stringVal, stringLen, (char*)&(cst_reader->actual_cst.content.value));
					cst_reader->set_content = 1;
				}
				else{
					printf("ERROR: in %s, value size is incorrect for this constant type: %s\n", __func__, constantType_to_string(cst_reader->actual_cst.type));
				}
				break;
			}
			case CST_TYPE_CST_32 		: {
				if (READBUFFER_RAW_GET_LENGTH(stringLen) == 4){
					cst_reader->actual_cst.content.value = 0;
					readBuffer_raw((char*)stringVal, stringLen, (char*)&(cst_reader->actual_cst.content.value));
					cst_reader->set_content = 1;
				}
				else{
					printf("ERROR: in %s, value size is incorrect for this constant type: %s\n", __func__, constantType_to_string(cst_reader->actual_cst.type));
				}
				break;
			}
			case CST_TYPE_TAB_8 		: {
				cst_reader->actual_cst.content.tab.nb_element = READBUFFER_RAW_GET_LENGTH(stringLen);
				cst_reader->actual_cst.content.tab.buffer = calloc(cst_reader->actual_cst.content.tab.nb_element, 1);
				if (cst_reader->actual_cst.content.tab.buffer == NULL){
					printf("ERROR: in %s, unable to allocate memory\n", __func__);
				}
				else{
					readBuffer_raw((char*)stringVal, stringLen, (char*)cst_reader->actual_cst.content.tab.buffer);
					cst_reader->set_content = 1;
				}
				break;
			}
			case CST_TYPE_TAB_16 	: {
				if (READBUFFER_RAW_GET_LENGTH(stringLen) % 2 == 0){
					cst_reader->actual_cst.content.tab.nb_element = READBUFFER_RAW_GET_LENGTH(stringLen) / 2;
					cst_reader->actual_cst.content.tab.buffer = calloc(cst_reader->actual_cst.content.tab.nb_element, 2);
					if (cst_reader->actual_cst.content.tab.buffer == NULL){
						printf("ERROR: in %s, unable to allocate memory\n", __func__);
					}
					else{
						readBuffer_raw((char*)stringVal, stringLen, (char*)cst_reader->actual_cst.content.tab.buffer);
						cst_reader->set_content = 1;
					}
				}
				else{
					printf("ERROR: in %s, value size is incorrect for this constant type: %s\n", __func__, constantType_to_string(cst_reader->actual_cst.type));
				}
				break;
			}
			case CST_TYPE_TAB_32 	: {
				if (READBUFFER_RAW_GET_LENGTH(stringLen) % 4 == 0){
					cst_reader->actual_cst.content.tab.nb_element = READBUFFER_RAW_GET_LENGTH(stringLen) / 4;
					cst_reader->actual_cst.content.tab.buffer = calloc(cst_reader->actual_cst.content.tab.nb_element, 4);
					if (cst_reader->actual_cst.content.tab.buffer == NULL){
						printf("ERROR: in %s, unable to allocate memory\n", __func__);
					}
					else{
						readBuffer_raw((char*)stringVal, stringLen, (char*)cst_reader->actual_cst.content.tab.buffer);
						readBuffer_reverse_endianness((char*)cst_reader->actual_cst.content.tab.buffer, 4 *cst_reader->actual_cst.content.tab.nb_element);
						cst_reader->set_content = 1;
					}
				}
				else{
					printf("ERROR: in %s, value size is incorrect for this constant type: %s\n", __func__, constantType_to_string(cst_reader->actual_cst.type));
				}
				break;
			}
			case CST_TYPE_LST_8 		: {
				cst_reader->actual_cst.content.list.nb_element = READBUFFER_RAW_GET_LENGTH(stringLen);
				cst_reader->actual_cst.content.list.buffer = calloc(cst_reader->actual_cst.content.list.nb_element, 1);
				if (cst_reader->actual_cst.content.list.buffer == NULL){
					printf("ERROR: in %s, unable to allocate memory\n", __func__);
				}
				else{
					readBuffer_raw((char*)stringVal, stringLen, (char*)cst_reader->actual_cst.content.list.buffer);
					cst_reader->set_content = 1;
				}
				break;
			}
			case CST_TYPE_LST_16 	: {
				if (READBUFFER_RAW_GET_LENGTH(stringLen) % 2 == 0){
					cst_reader->actual_cst.content.list.nb_element = READBUFFER_RAW_GET_LENGTH(stringLen) / 2;
					cst_reader->actual_cst.content.list.buffer = calloc(cst_reader->actual_cst.content.list.nb_element, 2);
					if (cst_reader->actual_cst.content.list.buffer == NULL){
						printf("ERROR: in %s, unable to allocate memory\n", __func__);
					}
					else{
						readBuffer_raw((char*)stringVal, stringLen, (char*)cst_reader->actual_cst.content.list.buffer);
						cst_reader->set_content = 1;
					}
				}
				else{
					printf("ERROR: in %s, value size is incorrect for this constant type: %s\n", __func__, constantType_to_string(cst_reader->actual_cst.type));
				}
				break;
			}
			case CST_TYPE_LST_32 	: {
				if (READBUFFER_RAW_GET_LENGTH(stringLen) % 4 == 0){
					cst_reader->actual_cst.content.list.nb_element = READBUFFER_RAW_GET_LENGTH(stringLen) / 4;
					cst_reader->actual_cst.content.list.buffer = calloc(cst_reader->actual_cst.content.list.nb_element, 4);
					if (cst_reader->actual_cst.content.list.buffer == NULL){
						printf("ERROR: in %s, unable to allocate memory\n", __func__);
					}
					else{
						readBuffer_raw((char*)stringVal, stringLen, (char*)cst_reader->actual_cst.content.list.buffer);
						cst_reader->set_content = 1;
					}
				}
				else{
					printf("ERROR: in %s, value size is incorrect for this constant type: %s\n", __func__, constantType_to_string(cst_reader->actual_cst.type));
				}
				break;
			}
		}
		
	}
	else{
		printf("ERROR: in %s, wrong data type (string) for the current key\n", __func__);
	}

	return 1;
}

static int cstReaderJSON_map_key(void* ctx, const unsigned char* stringVal, size_t stringLen){
	struct cstReaderJSON* cst_reader = (struct cstReaderJSON*)ctx;

	if (!strncmp((const char*)stringVal, JSON_MAP_KEY_NAME_REF, stringLen)){
		cst_reader->actual_key = CST_JSON_MAP_KEY_CST;
	}
	else if (!strncmp((const char*)stringVal, JSON_MAP_KEY_NAME_NAME, stringLen)){
		cst_reader->actual_key = CST_JSON_MAP_KEY_NAME;
	}
	else if (!strncmp((const char*)stringVal, JSON_MAP_KEY_NAME_TYPE, stringLen)){
		cst_reader->actual_key = CST_JSON_MAP_KEY_TYPE;
	}
	else if (!strncmp((const char*)stringVal, JSON_MAP_KEY_NAME_VALUE, stringLen)){
		cst_reader->actual_key = CST_JSON_MAP_KEY_VALUE;
	}
	else{
		printf("ERROR: in %s, unknown map key\n", __func__);
		cst_reader->actual_key = CST_JSON_MAP_KEY_UNKNOWN;
	}

	return 1;
}

static int cstReaderJSON_start_map(void* ctx){
	struct cstReaderJSON* cst_reader = (struct cstReaderJSON*)ctx;

	cst_reader->actual_map_level ++;

	switch (cst_reader->actual_map_level){
		case IDLE_MAP_LEVEL :{
			break;
		}
		case CST_MAP_LEVEL : {
			memset(cst_reader->actual_cst.name, '\0', CONSTANT_NAME_LENGTH);
			cst_reader->actual_cst.type = CST_TYPE_INVALID;

			cst_reader->set_name = 0;
			cst_reader->set_type = 0;
			cst_reader->set_content = 0;
			break;
		}
		default : {
			printf("ERROR: in %s, wrong map level (%d), this case is not meant to happen\n", __func__, cst_reader->actual_map_level);
			break;
		}
	}

	return 1;
}

static int cstReaderJSON_end_map(void* ctx){
	struct cstReaderJSON* cst_reader = (struct cstReaderJSON*)ctx;

	switch (cst_reader->actual_map_level){
		case IDLE_MAP_LEVEL :{
			break;
		}
		case CST_MAP_LEVEL : {
			if (cst_reader->set_name && cst_reader->set_type && cst_reader->set_content){
				if (array_add(cst_reader->array, &(cst_reader->actual_cst)) < 0){
					printf("ERROR: in %s, unable to add element to array\n", __func__);
				}
			}
			else{
				printf("ERROR: in %s, incomplete constant entry - skip\n", __func__);
			}
			break;
		}
		default : {
			printf("ERROR: in %s, wrong map level (%d), this case is not meant to happen\n", __func__, cst_reader->actual_map_level);
			break;
		}
	}

	cst_reader->actual_map_level --;

	return 1;
}