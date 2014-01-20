#ifndef PRINTBUFFER_H
#define PRINTBUFFER_H

#include <stdint.h>
#include <stdio.h>

void printBuffer_raw(FILE* file, char* buffer, uint64_t buffer_length);
void printBuffer_raw_color(char* buffer, uint64_t buffer_length, uint64_t color_start, uint64_t color_length);

#endif