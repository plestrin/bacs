#ifndef READBUFFER_H
#define READBUFFER_H

#include <stdint.h>

#define READBUFFER_RAW_GET_LENGTH(a) (((a) >> 1) + ((a) & 0x00000001))

char* readBuffer_raw(const char* txt, size_t txt_length, char* buffer);

void readBuffer_reverse_endianness(char* buffer, size_t buffer_length);

#endif