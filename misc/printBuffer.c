#include <stdlib.h>
#include <stdio.h>

#include "printBuffer.h"

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

void printBuffer_raw(FILE* file, char* buffer, uint64_t buffer_length){
	uint64_t 	i;
	char 		hexa[16] = {'0', '1', '2', '3', '4' , '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

	for (i = 0; i < buffer_length; i ++){
		fprintf(file, "%c%c", hexa[(buffer[i] >> 4) & 0x0f], hexa[buffer[i] & 0x0f]);
	}
}

void printBuffer_raw_color(char* buffer, uint64_t buffer_length, uint64_t color_start, uint64_t color_length){
	uint64_t 	i;
	char 		hexa[16] = {'0', '1', '2', '3', '4' , '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

	for (i = 0; i < buffer_length; i ++){
		if (i == color_start){
			printf(ANSI_COLOR_MAGENTA);
		}
		if (i == color_start + color_length){
			printf(ANSI_COLOR_RESET);
		}
		printf("%c%c", hexa[(buffer[i] >> 4) & 0x0f], hexa[buffer[i] & 0x0f]);
	}

	if (color_start + color_length >= buffer_length){
		printf(ANSI_COLOR_RESET);
	}
}