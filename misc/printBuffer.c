#include <stdlib.h>
#include <stdio.h>

#include "printBuffer.h"
#include "base.h"

#ifdef _WIN32
#include "windowsComp.h"
#endif

static const char __hexa__[16] = {'0', '1', '2', '3', '4' , '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

void fprintBuffer_raw(FILE* file, const char* buffer, size_t buffer_length){
	size_t i;

	for (i = 0; i < buffer_length; i ++){
		fprintf(file, "%c%c", __hexa__[(buffer[i] >> 4) & 0x0f], __hexa__[buffer[i] & 0x0f]);
	}
}

#ifdef COLOR
void printBuffer_raw_color(const char* buffer, size_t buffer_length, size_t color_start, size_t color_length){
	size_t i;

	for (i = 0; i < buffer_length; i ++){
		if (i == color_start){
			printf(ANSI_COLOR_MAGENTA);
		}
		if (i == color_start + color_length){
			printf(ANSI_COLOR_RESET);
		}
		printf("%c%c", __hexa__[(buffer[i] >> 4) & 0x0f], __hexa__[buffer[i] & 0x0f]);
	}

	if (color_start + color_length >= buffer_length){
		printf(ANSI_COLOR_RESET);
	}
}
#endif

void sprintBuffer_raw(char* string, size_t string_length, const char* buffer, size_t buffer_length){
	size_t i;

	for (i = 0; i < buffer_length && 2 * i < string_length; i ++){
		snprintf(string + 2 * i, string_length - 2 * i, "%c%c", __hexa__[(buffer[i] >> 4) & 0x0f], __hexa__[buffer[i] & 0x0f]);
	}
}
