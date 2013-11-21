#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "multiColumn.h"

static void multiColumnPrinter_constrain_string(char* src, char* dst, uint32_t size);

struct multiColumnPrinter* multiColumnPrinter_create(FILE* file, uint32_t nb_column, uint32_t* sizes,  char* separator){
	struct multiColumnPrinter* 	printer;
	uint32_t 					i;
	
	if (nb_column == 0){
		printf("WARNING: in %s, at least one column must be created\n", __func__);
		nb_column = 1;
	}
	
	printer = (struct multiColumnPrinter*)malloc(sizeof(struct multiColumnPrinter) + (nb_column - 1) * sizeof(struct multiColumnColumn));
	if (printer != NULL){
		printer->file = file;
		printer->nb_column = nb_column;
		
		if (separator != NULL){
			strncpy(printer->separator, separator, MULTICOLUMN_STRING_MAX_SIZE);
		}
		else{
			snprintf(printer->separator, MULTICOLUMN_STRING_MAX_SIZE, "%s", MULTICOLUMN_DEFAULT_SEPARATOR);
		}
		
		for (i = 0; i < nb_column; i++){
			if (sizes != NULL){
				if (sizes[i] >= MULTICOLUMN_STRING_MAX_SIZE){
					printf("WARNING: in %s, required size for column %u exceeds MULTICOLUMN_STRING_MAX_SIZE, please increment\n", __func__, i);
					printer->columns[i].size = MULTICOLUMN_STRING_MAX_SIZE - 1;
				}
				else{
					printer->columns[i].size = sizes[i];
				}
			}
			else{
				printer->columns[i].size = MULTICOLUMN_DEFAULT_SIZE;
			}
			
			memset(printer->columns[i].title, '\0', MULTICOLUMN_STRING_MAX_SIZE);
			snprintf(printer->columns[i].title, MULTICOLUMN_STRING_MAX_SIZE, "TITLE %u", i);
		}
	}
	else{
		printf("ERROR: in %s, unable to allocate memory\n", __func__);
	}
	
	return printer;
}

void multiColumnPrinter_set_column_size(struct multiColumnPrinter* printer, uint32_t column, uint32_t size){
	if (printer != NULL){
		if (printer->nb_column > column){
			if (size >= MULTICOLUMN_STRING_MAX_SIZE){
				printf("WARNING: in %s, required size for column %u exceeds MULTICOLUMN_STRING_MAX_SIZE, please increment\n", __func__, column);
				printer->columns[column].size = MULTICOLUMN_STRING_MAX_SIZE - 1;
			}
			else{
				printer->columns[column].size = size;
			}
		}
		else{
			printf("ERROR: in %s, column argument exceeds printer size\n", __func__);
		}
	}
}

void multiColumnPrinter_print_header(struct multiColumnPrinter* printer){
	char 		print_value[MULTICOLUMN_STRING_MAX_SIZE];
	uint32_t	i;
	
	if (printer != NULL){
		for (i = 0; i < printer->nb_column; i++){	
			multiColumnPrinter_constrain_string(printer->columns[i].title, print_value, printer->columns[i].size);
			
			if (i == printer->nb_column - 1){
				fprintf(printer->file, "%s\n", print_value);
			}
			else{
				fprintf(printer->file, "%s%s", print_value, printer->separator);
			}
		}

		multiColumnPrinter_print_horizontal_separator(printer);
	}
}

void multiColumnPrinter_print_horizontal_separator(struct multiColumnPrinter* printer){
	char 		print_value[MULTICOLUMN_STRING_MAX_SIZE];
	uint32_t	i;

	if (printer != NULL){
		for (i = 0; i < printer->nb_column; i++){
			memset(print_value, '-', printer->columns[i].size);
			print_value[printer->columns[i].size] = '\0';
			
			if (i == printer->nb_column - 1){
				fprintf(printer->file, "%s\n", print_value);
			}
			else{
				fprintf(printer->file, "%s%s", print_value, printer->separator);
			}
		}
	}
}

void multiColumnPrinter_print(struct multiColumnPrinter* printer, ...){
	char* 		value;
	char 		print_value[MULTICOLUMN_STRING_MAX_SIZE];
	va_list 	vl;
	uint32_t	i;

	if (printer != NULL){
		va_start(vl, printer);
		
		for (i = 0; i < printer->nb_column; i++){
			value = va_arg(vl, char*);
			if (value == NULL){
				printf("ERROR: in %s, missing argument(s) in function call\n", __func__);
				break;
			}
			else{		
				multiColumnPrinter_constrain_string(value, print_value, printer->columns[i].size);
			
				if (i == printer->nb_column - 1){
					fprintf(printer->file, "%s\n", print_value);
				}
				else{
					fprintf(printer->file, "%s%s", print_value, printer->separator);
				}
			}
		}

		va_end(vl);
	}
}

void multiColumnPrinter_set_title(struct multiColumnPrinter* printer, uint32_t column, char* title){
	if (printer != NULL){
		if (printer->nb_column > column){
			strncpy(printer->columns[column].title, title, MULTICOLUMN_STRING_MAX_SIZE);
		}
		else{
			printf("ERROR: in %s, column argument exceeds printer size\n", __func__);
		}
	}
}

void multiColumnPrinter_delete(struct multiColumnPrinter* printer){
	if (printer != NULL){
		free(printer);
	}
}

static void multiColumnPrinter_constrain_string(char* src, char* dst, uint32_t size){
	uint32_t length;
	
	strncpy(dst, src, MULTICOLUMN_STRING_MAX_SIZE);
	length = strlen(src);
	dst[size] = '\0';
	if (length > size){
		if (size >= 3){
			dst[size - 3] = '.';
		}
		if (size >= 2){
			dst[size - 2] = '.';
		}
		if (size >= 1){
			dst[size - 1] = '.';
		}
	}
	else{
		memset(dst + length, ' ', (size - length));
	}
}
