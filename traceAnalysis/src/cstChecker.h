#ifndef CSTCHECKER_H
#define CSTCHECKER_H

#include <stdint.h>

#include "trace.h"
#include "array.h"
#include "pageTable.h"
#include "address.h"

#define CONSTANT_NAME_LENGTH 128

#define CONSTANT_THRESHOLD_TAB	4

/* Constant type value:
 * - bit 0 		: set to 0 -> INVALID 	set to 1 -> VALID
 * - bit [4:5]	: set to 0 -> CST 		set to 1 -> TAB 		set to 2 -> LST
 * - bit [8:15] : size (example: 1=8bits, 2=16bits, 4=32bits)
 */

enum constantType{
	CST_TYPE_INVALID 	= 0x00000000,
	CST_TYPE_CST_8 		= 0x00000101,
	CST_TYPE_CST_16 	= 0x00000201,
	CST_TYPE_CST_32 	= 0x00000401,
	CST_TYPE_TAB_8 		= 0x00000111,
	CST_TYPE_TAB_16 	= 0x00000211,
	CST_TYPE_TAB_32 	= 0x00000411,
	CST_TYPE_LST_8 		= 0x00000121,
	CST_TYPE_LST_16 	= 0x00000221,
	CST_TYPE_LST_32 	= 0x00000421
};

#define CONSTANT_IS_CST(type) 				(((type) & 0x00000031) == 0x00000001)
#define CONSTANT_IS_TAB(type) 				(((type) & 0x00000031) == 0x00000011)
#define CONSTANT_IS_LST(type) 				(((type) & 0x00000031) == 0x00000021)
#define CONSTANT_GET_ELEMENT_SIZE(type) 	(((type) >> 8) & 0x000000ff)

char* constantType_to_string(enum constantType type);
enum constantType constantType_from_string(const char* arg, uint32_t length);

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

struct cstTableAccess{
	uint32_t 				table_hit;
	ADDRESS 				table_address;
	struct pageTable* 		pt;
};

union cstResult{
	uint32_t 				cst_hit_counter;
	uint32_t*  				lst_hit_counter;
	struct cstTableAccess 	tab_hit_counter;
};

#endif