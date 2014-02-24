#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "termReader.h"

#define TERMREADER_SPECIAL_CHAR_DEL 127

static void termReader_remove_last_blanck(char* buffer, uint32_t length);


int32_t termReader_set_raw_mode(struct termReader* term){
	struct termios settings;

	if (isatty(STDIN_FILENO)){
		term->is_tty = 1;
		if (tcgetattr(STDIN_FILENO, &settings)){
			printf("ERROR: in %s, tcgetattr fails\n", __func__);
			return -1;
		}
		memcpy(&(term->saved_settings), &settings, sizeof(struct termios));

		settings.c_lflag &= ~(ICANON | ECHO);

		if (tcsetattr(STDIN_FILENO, TCSANOW, &settings)){
			printf("ERROR: in %s, tcsetattr fails\n", __func__);
			return -1;
		}
	}
	else{
		term->is_tty = 0;
		#ifdef VERBOSE
		printf("WARNING: in %s, stdin is not a terminal - skipping\n", __func__);
		#endif
	}

	return 0;
}

int32_t termReader_get_line(struct termReader* term, char* buffer, uint32_t buffer_length){
	uint32_t 	i = 0;
	char 		character;

	do{
		character = fgetc(stdin);
		switch(character){
			case '\n' 	: {
				if (term->is_tty){
					write(STDIN_FILENO, &character, 1);
				}
				break;
			}
			/*case '\t' 	: {break;}*/
			case TERMREADER_SPECIAL_CHAR_DEL : {
				if (i > 0){
					i --;
					if (term->is_tty){
						character = '\b';
						write(STDIN_FILENO, &character, 1);
					}
				}
				break;
			}
			case EOF	: {
				printf("ERROR: in %s, fgetc returns EOF on stdin\n", __func__);
				return - 1;
			}
			default 	: {
				if (term->is_tty){
					write(STDIN_FILENO, &character, 1);
				}
				buffer[i++] = character;
				break;
			}
		}
	} while(character != '\n' && i < buffer_length - 1);
	buffer[i] = '\0';

	termReader_remove_last_blanck(buffer, i);

	return 0;
}

int32_t termReader_reset_mode(struct termReader* term){
	if (term->is_tty){
		if (tcsetattr(STDIN_FILENO, TCSANOW, &(term->saved_settings))){
			printf("ERROR: in %s, tcsetattr fails\n", __func__);
			return -1;
		}
	}

	return 0;
}

static void termReader_remove_last_blanck(char* buffer, uint32_t length){
	while(length > 0 && buffer[length - 1] == ' '){
		buffer[length - 1] = '\0';
		length--;
	}
}