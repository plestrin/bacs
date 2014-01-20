#ifndef READBUFFER_H
#define READBUFFER_H

#include <stdint.h>

#define READBUFFER_RAW_GET_LENGTH(a) (((a) >> 1) + ((a) & 0x00000001))

char* readBuffer_raw(char* txt, uint64_t txt_length);

#endif