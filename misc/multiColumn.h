#ifndef MULTICOLUMN_H
#define MULTICOLUMN_H

#include <stdint.h>
#include <stdio.h>

#define MULTICOLUMN_DEFAULT_SIZE 		12
#define MULTICOLUMN_DEFAULT_SEPARATOR 	" | "
#define MULTICOLUMN_STRING_MAX_SIZE		256

struct multiColumnColumn{
	uint32_t 					size;
	char 						title[MULTICOLUMN_STRING_MAX_SIZE];
};

struct multiColumnPrinter{
	FILE* 						file;
	uint32_t 					nb_column;
	char 						separator[MULTICOLUMN_STRING_MAX_SIZE];
	struct multiColumnColumn 	columns[1]; 
};

struct multiColumnPrinter* multiColumnPrinter_create(FILE* file, uint32_t nb_column, uint32_t* sizes,  char* separator);
void multiColumnPrinter_set_column_size(struct multiColumnPrinter* printer, uint32_t column, uint32_t size);
void multiColumnPrinter_set_title(struct multiColumnPrinter* printer, uint32_t column, char* title);
void multiColumnPrinter_print_header(struct multiColumnPrinter* printer);
void multiColumnPrinter_print_horizontal_separator(struct multiColumnPrinter* printer);
void multiColumnPrinter_print(struct multiColumnPrinter* printer, ...);
void multiColumnPrinter_delete(struct multiColumnPrinter* printer);



#endif