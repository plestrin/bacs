#ifndef TERMREADER_H
#define TERMREADER_H

#include <stdint.h>
#include <termios.h>

struct termReader{
	uint8_t 		is_tty;
	struct termios 	saved_settings;
};

int32_t termReader_set_raw_mode(struct termReader* term);

int32_t termReader_get_line(struct termReader* term, char* buffer, uint32_t buffer_length);

int32_t termReader_reset_mode(struct termReader* term);

#endif