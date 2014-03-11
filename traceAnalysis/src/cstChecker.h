#ifndef CSTCHECKER_H
#define CSTCHECKER_H

#include <stdint.h>

#include "trace.h"
#include "array.h"
#include "address.h"

#define CONSTANT_NAME_LENGTH 128

#define CONSTANT_THRESHOLD_TAB	3

enum constantType{
	CST_TYPE_INVALID,
	CST_TYPE_CST_8,
	CST_TYPE_CST_16,
	CST_TYPE_CST_32,
	CST_TYPE_TAB_8,
	CST_TYPE_TAB_16,
	CST_TYPE_TAB_32,
	CST_TYPE_LST_8,
	CST_TYPE_LST_16,
	CST_TYPE_LST_32
};

#define CONSTANT_IS_CST(type) 	((type) == CST_TYPE_CST_8 || (type) == CST_TYPE_CST_16 || (type) == CST_TYPE_CST_32)
#define CONSTANT_IS_TAB(type) 	((type) == CST_TYPE_TAB_8 || (type) == CST_TYPE_TAB_16 || (type) == CST_TYPE_TAB_32)
#define CONSTANT_IS_LST(type) 	((type) == CST_TYPE_LST_8 || (type) == CST_TYPE_LST_16 || (type) == CST_TYPE_LST_32)

#define CONSTANT_IS_8(type)		((type) == CST_TYPE_CST_8  || (type) == CST_TYPE_TAB_8  || (type) == CST_TYPE_LST_8 )
#define CONSTANT_IS_16(type)	((type) == CST_TYPE_CST_16 || (type) == CST_TYPE_TAB_16 || (type) == CST_TYPE_LST_16)
#define CONSTANT_IS_32(type)	((type) == CST_TYPE_CST_32 || (type) == CST_TYPE_TAB_32 || (type) == CST_TYPE_LST_32)


char* constantType_to_string(enum constantType type);
enum constantType constantType_from_string(const char* arg, uint32_t length);

static uint8_t constantType_element_size(enum constantType type){
	if (CONSTANT_IS_8(type)){
		return 1;
	}
	else if (CONSTANT_IS_16(type)){
		return 2;
	}
	else if (CONSTANT_IS_32(type)){
		return 4;
	}

	return 1;
}

struct constant{
	char 				name[CONSTANT_NAME_LENGTH];
	enum constantType 	type;
	union{
		struct{
			void* 		buffer;
			uint32_t 	nb_element;
		}				tab;
		struct{
			void* 		buffer;
			uint32_t 	nb_element;
		}				list;
		uint32_t 		value;
	} 					content;
};

void constant_clean(struct constant* cst);

struct cstChecker{
	struct array 		cst_array;
};

struct cstChecker* cstChecker_create();
int32_t cstChecker_init(struct cstChecker* checker);

void cstChecker_load(struct cstChecker* checker, char* arg);
void cstChecker_print(struct cstChecker* checker);
void cstChecker_empty(struct cstChecker* checker);

int32_t cstChecker_check(struct cstChecker* checker, struct trace* trace);

void cstChecker_clean(struct cstChecker* checker);
void cstChecker_delete(struct cstChecker* checker);

union cstResult{
	uint32_t 		cst_hit_counter;
	uint32_t*  		lst_hit_counter;
	struct array 	tab_hit_counter;
};

struct cstTableAccess{
	uint32_t 	offset;
	ADDRESS 	address;
};

#endif