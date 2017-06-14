#ifndef TERMREADER_H
#define TERMREADER_H

#include <stdint.h>
#include <termios.h>

struct termReader{
	uint32_t 		is_tty;
	struct termios 	saved_settings;
	uint32_t(*tab_handler)(char* buffer, uint32_t buffer_length, uint32_t offset, void* arg);
	int32_t(*history_up_handler)(char* buffer, void* arg);
	int32_t(*history_down_handler)(char* buffer, void* arg);
	void* 			arg;
};

#define termReader_init(term) 														\
	(term)->tab_handler = NULL; 													\
	(term)->history_up_handler = NULL; 												\
	(term)->history_down_handler = NULL;

#define termReader_set_tab_handler(term, handler, arg_) 							\
	((term)->tab_handler = (uint32_t(*)(char*,uint32_t,uint32_t,void*))(handler)); 	\
	((term)->arg = (arg_));

#define termReader_set_history_handler(term, up_handler, down_handler, arg_) 		\
	((term)->history_up_handler = (int32_t(*)(char*,void*))(up_handler)); 			\
	((term)->history_down_handler = (int32_t(*)(char*,void*))(down_handler)); 		\
	((term)->arg = (arg_));

int32_t termReader_set_raw_mode(struct termReader* term);

int32_t termReader_get_line(struct termReader* term, char* buffer, uint32_t buffer_length);

int32_t termReader_reset_mode(struct termReader* term);

#endif
