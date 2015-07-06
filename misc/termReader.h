#ifndef TERMREADER_H
#define TERMREADER_H

#include <stdint.h>
#include <termios.h>

struct termReader{
	uint32_t 		is_tty;
	struct termios 	saved_settings;
	uint32_t(*tab_handler)(char* buffer, uint32_t buffer_length, uint32_t offset, void* arg);
	void* 			arg;
};

#define termReader_set_tab_handler(term, handler, arg_) 												\
	((term)->tab_handler = (uint32_t(*)(char*,uint32_t,uint32_t,void*))(handler)); 					\
	((term)->arg = (arg_));

int32_t termReader_set_raw_mode(struct termReader* term);

int32_t termReader_get_line(struct termReader* term, char* buffer, uint32_t buffer_length);

int32_t termReader_reset_mode(struct termReader* term);

#endif