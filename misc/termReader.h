#ifndef TERMREADER_H
#define TERMREADER_H

#include <stdint.h>

int32_t termReader_set_raw_mode();

int32_t termReader_get_line(char* buffer, uint32_t buffer_length);

#endif