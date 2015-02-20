#ifndef MULTICOLUMN_H
#define MULTICOLUMN_H

#include <stdint.h>
#include <stdio.h>

#define MULTICOLUMN_DEFAULT_SIZE 		12
#define MULTICOLUMN_DEFAULT_SEPARATOR 	" | "
#define MULTICOLUMN_DEFAULT_TYPE		1
#define MULTICOLUMN_STRING_MAX_SIZE		256

#define MULTICOLUMN_TYPE_STRING			1
#define MULTICOLUMN_TYPE_INT8 			2
#define MULTICOLUMN_TYPE_INT32 			3
#define MULTICOLUMN_TYPE_UINT32 		4
#define MULTICOLUMN_TYPE_DOUBLE 		5
#define MULTICOLUMN_TYPE_HEX_32			6
#define MULTICOLUMN_TYPE_HEX_64 		7
#define MULTICOLUMN_TYPE_UNBOUND_STRING 8

#define MULTICOLUMN_TYPE_IS_VALID(type) 		\
	((type) == MULTICOLUMN_TYPE_STRING 	||		\
	(type)  == MULTICOLUMN_TYPE_INT8  	||		\
	(type)  == MULTICOLUMN_TYPE_INT32  	||		\
	(type)  == MULTICOLUMN_TYPE_UINT32 	||		\
	(type)  == MULTICOLUMN_TYPE_DOUBLE 	||		\
	(type) 	== MULTICOLUMN_TYPE_HEX_32 	|| 		\
	(type)  == MULTICOLUMN_TYPE_HEX_64)

struct multiColumnColumn{
	uint32_t 					size;
	char 						title[MULTICOLUMN_STRING_MAX_SIZE];
	char 						type;
};

struct multiColumnPrinter{
	FILE* 						file;
	uint32_t 					nb_column;
	char 						separator[MULTICOLUMN_STRING_MAX_SIZE];
	struct multiColumnColumn 	columns[1];
};

struct multiColumnPrinter* multiColumnPrinter_create(FILE* file, uint32_t nb_column, uint32_t* sizes, uint8_t* types, char* separator);
void multiColumnPrinter_set_column_size(struct multiColumnPrinter* printer, uint32_t column, uint32_t size);
void multiColumnPrinter_set_column_type(struct multiColumnPrinter* printer, uint32_t column, uint8_t type);
void multiColumnPrinter_set_title(struct multiColumnPrinter* printer, uint32_t column, const char* title);
void multiColumnPrinter_print_header(struct multiColumnPrinter* printer);
void multiColumnPrinter_print_horizontal_separator(struct multiColumnPrinter* printer);
void multiColumnPrinter_print(struct multiColumnPrinter* printer, ...);
void multiColumnPrinter_print_string_line(struct multiColumnPrinter* printer, char* string, uint32_t string_size);
void multiColumnPrinter_delete(struct multiColumnPrinter* printer);

#endif