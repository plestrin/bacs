#ifndef PRINTBUFFER_H
#define PRINTBUFFER_H

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

void fprintBuffer_raw(FILE* file, const char* buffer, size_t buffer_length);

#ifdef COLOR
void printBuffer_raw_color(const char* buffer, size_t buffer_length, size_t color_start, size_t color_length);
#else
#define printBuffer_raw_color(buffer, buffer_length, color_start, color_length) printBuffer_raw(stdout, buffer, buffer_length)
#endif

#define PRINTBUFFER_GET_STRING_SIZE(buffer_length) ((buffer_length) * 2 + 1)

void sprintBuffer_raw(char* string, size_t string_length, const char* buffer, size_t buffer_length);

#endif
